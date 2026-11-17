#include "packert_hardware/hardware_model.hpp"
#include <fmt/format.h>
#include <cstddef>
#include <hardware_interface/hardware_info.hpp>
#include <hardware_interface/lexical_casts.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "packert_hardware/layout.hpp"
#include "packert_hardware/packert.hpp"

namespace packert_hardware {

constexpr std::string_view component_type_as_string(
    ComponentType component_type) {
  switch (component_type) {
    case ComponentType::JOINT:
      return "joint";
    case ComponentType::SENSOR:
      return "sensor";
    case ComponentType::GPIO:
      return "gpio";
  }
  return {};
}

HardwareModel::HardwareModel(std::string_view source,
                             const hardware_interface::HardwareInfo &info)
    : name_(info.name), source_(source) {
  load_parameters(info);
  build_interfaces(info);
  canonical_ = build_canonical();
  hash_ = fmt::format("0x{:x}", packert_hardware::hash(canonical_));
}

hardware_interface::HandleDataType HardwareModel::map_data_type(
    std::string_view data_type) {
  hardware_interface::HandleDataType type{std::string{data_type}};

  if (type == hardware_interface::HandleDataType::DOUBLE && !use_double_) {
    type = hardware_interface::HandleDataType::FLOAT32;
  }

  return type;
}

void HardwareModel::load_parameters(
    const hardware_interface::HardwareInfo &info) {
  if (info.hardware_parameters.find("use_double") !=
      info.hardware_parameters.end()) {
    use_double_ = hardware_interface::parse_bool(
        info.hardware_parameters.at("use_double"));
  }

  if (info.hardware_parameters.find("strip_prefix") !=
      info.hardware_parameters.end()) {
    strip_prefix_ = info.hardware_parameters.at("strip_prefix");
  }
}

void HardwareModel::push_interfaces(
    ComponentType component_type, const std::string &component_name,
    const std::string &stripped_name,
    std::vector<hardware_interface::InterfaceInfo> &from,
    std::vector<std::shared_ptr<Interface>> &to,
    std::map<std::pair<std::string, std::string>,
             std::weak_ptr<NestedInterface>> &nested_map) {
  size_t position;
  for (auto &interface : from) {
    if ((position = interface.name.find('.')) != interface.name.size()) {
      auto interface_name = interface.name.substr(0, position);
      auto field_name = interface.name.substr(position + 1);

      auto nested_interface = nested_map.find({component_name, interface_name});

      if (nested_interface == nested_map.end()) {
        auto nested_interface_ptr =
            std::make_shared<NestedInterface>(NestedInterface{
                component_type,
                stripped_name,
                interface_name,
                {{field_name, map_data_type(interface.data_type)}}});
        to.emplace_back(nested_interface_ptr);
        nested_map[{component_name, interface_name}] = nested_interface_ptr;
      } else {
        nested_interface->second.lock()->fields.push_back(
            {field_name, map_data_type(interface.data_type)});
      }
    } else {
      to.emplace_back(component_type, stripped_name, interface.name,
                      map_data_type(interface.data_type));
    }
  }
}

void HardwareModel::build_interfaces(
    const hardware_interface::HardwareInfo &info) {
  std::vector<
      std::pair<ComponentType, std::vector<hardware_interface::ComponentInfo>>>
      component_map{{{ComponentType::JOINT, info.joints},
                     {ComponentType::SENSOR, info.sensors},
                     {ComponentType::GPIO, info.gpios}}};

  for (auto &[component_type, components] : component_map) {
    for (auto &component : components) {
      if (component.name.substr(0, strip_prefix_.length()) != strip_prefix_) {
        throw std::invalid_argument(fmt::format("{} {} lacks prefix {}",
                                                component_type, component.name,
                                                strip_prefix_));
      }
      auto stripped_name = component.name.substr(strip_prefix_.length());
      push_interfaces(component_type, component.name, stripped_name,
                      component.state_interfaces, state_interfaces_,
                      nested_state_interfaces_);
      push_interfaces(component_type, component.name, stripped_name,
                      component.command_interfaces, state_interfaces_,
                      nested_command_interfaces_);
    }
  }
}

std::string HardwareModel::build_canonical() const {
  std::ostringstream stream{};
  stream << "packert:" << PACKERT_VERSION << "|" << name_ << "|state";
  for (auto &interface : state_interfaces_) {
    if (const auto *intPtr = std::get_if(interface)) {
      std::cout << "It's an int: " << *intPtr << '\n';
    } else if (const auto *strPtr = std::get_if<std::string>(&v)) {
      std::cout << "It's a string: " << *strPtr << '\n';
    }
  }
  stream << "|command";
  for (auto &command : commands) {
    stream << "/" << command;
  }
}

}  // namespace packert_hardware
