#include "packert_hardware/hardware_model.hpp"
#include <fmt/format.h>
#include <algorithm>
#include <cstddef>
#include <hardware_interface/hardware_info.hpp>
#include <hardware_interface/lexical_casts.hpp>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "packert_hardware/layout.hpp"
#include "packert_hardware/packert.hpp"

namespace packert_hardware {

HardwareModel::HardwareModel(std::string_view source,
                             const hardware_interface::HardwareInfo &info)
    : name_(info.name), source_(source) {
  load_parameters(info);
  build_interfaces(info);
  canonical_ = build_canonical();
  hash_ = fmt::format("0x{:x}", packert_hardware::hash(canonical_));
}

json HardwareModel::to_json() const {
  json j;
  j["name"] = name_;
  j["source"] = source_;
  j["canonical"] = canonical_;
  j["hash"] = hash_;
  j["use_double"] = use_double_;
  if (!command_interfaces_.empty()) {
    json commands = {{"joint", json::array()},
                     {"sensor", json::array()},
                     {"gpio", json::array()}};
    for (const auto &interface : command_interfaces_) {
      auto component = component_type_as_string(std::visit(
          [](const auto &i) { return i.component_type; }, *interface));
      if (const auto *single = std::get_if<SingleInterface>(interface.get())) {
        json interface_json = {{"type", "single"},
                               {"interface_name", single->interface_name},
                               {"data_type", single->data_type.to_string()}};
      }
      if (const auto *nested = std::get_if<NestedInterface>(interface.get())) {
      }
      j["commands"] = commands;
    }
    return j;
  }

  hardware_interface::HandleDataType HardwareModel::map_data_type(
      std::string_view data_type) {
    hardware_interface::HandleDataType type{std::string{data_type}};

    if (type == hardware_interface::HandleDataType::DOUBLE && !use_double_) {
      type = hardware_interface::HandleDataType::FLOAT32;
    }

    return type;
  }

  bool HardwareModel::compare_interfaces(const Interface &a,
                                         const Interface &b) {
    return std::visit(
        [](const auto &x, const auto &y) {
          if (x.component_type != y.component_type) {
            return x.component_type < y.component_type;
          }
          if (x.component_name != y.component_name) {
            return x.component_name < y.component_name;
          }
          if (x.interface_name != y.interface_name) {
            return x.interface_name < y.interface_name;
          }

          // Only can reach this point if above are not sorted, which shouldn't
          // be possible given HardwareInfo parser prevents duplicates.
          // Additionally, this static method expects that nested interfaces are
          // already sorted.
          return true;
        },
        a, b);
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

        auto nested_interface =
            nested_map.find({component_name, interface_name});

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
    std::vector<std::pair<ComponentType,
                          std::vector<hardware_interface::ComponentInfo>>>
        component_map{{{ComponentType::JOINT, info.joints},
                       {ComponentType::SENSOR, info.sensors},
                       {ComponentType::GPIO, info.gpios}}};

    for (auto &[component_type, components] : component_map) {
      for (auto &component : components) {
        if (component.name.substr(0, strip_prefix_.length()) != strip_prefix_) {
          throw std::invalid_argument(
              fmt::format("{} {} lacks prefix {}", component_type,
                          component.name, strip_prefix_));
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

    // Ensure fields of nested interfaces are sorted for determinism
    for (const auto &nested : nested_state_interfaces_) {
      nested.second.lock()->sort();
    }
    for (const auto &nested : nested_command_interfaces_) {
      nested.second.lock()->sort();
    }

    // Interfaces themselves need to be sorted lexicographically to guarantee
    // determinism
    std::sort(state_interfaces_.begin(), state_interfaces_.end(),
              compare_interfaces);
    std::sort(command_interfaces_.begin(), command_interfaces_.end(),
              compare_interfaces);
  }

  std::string HardwareModel::build_canonical() const {
    std::ostringstream os{};
    os << "packert:" << PACKERT_VERSION << "|" << name_;
    if (state_interfaces_.size() > 0) {
      os << "|state";
      for (auto &interface : state_interfaces_) {
        std::visit([&os](const auto &val) { os << "," << val; }, *interface);
      }
    }
    if (command_interfaces_.size() > 0) {
      os << "|command";
      for (auto &interface : command_interfaces_) {
        std::visit([&os](const auto &val) { os << "," << val; }, *interface);
      }
    }
    return os.str();
  }

}  // namespace packert_hardware
