#ifndef PACKERT_HARDWARE__TEMPLATE_HPP_
#define PACKERT_HARDWARE__TEMPLATE_HPP_

#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <ostream>
#include <string_view>
#include <variant>
#include <vector>
#include "hardware_interface/hardware_info.hpp"
#include "packert_hardware/visibility_control.h"

namespace packert_hardware {

using namespace nlohmann;

enum class ComponentType { JOINT = 0, SENSOR, GPIO };

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

constexpr std::string_view to_c_type(
    const hardware_interface::HandleDataType &data_type) {
  switch (data_type) {
    case hardware_interface::HandleDataType::DOUBLE:
      return "float64_t";
    case hardware_interface::HandleDataType::FLOAT32:
      return "float32_t";
    case hardware_interface::HandleDataType::BOOL:
      return "bool";
    case hardware_interface::HandleDataType::UINT8:
      return "uint8_t";
    case hardware_interface::HandleDataType::INT8:
      return "int8_t";
    case hardware_interface::HandleDataType::UINT16:
      return "uint16_t";
    case hardware_interface::HandleDataType::INT16:
      return "int16_t";
    case hardware_interface::HandleDataType::UINT32:
      return "uint32_t";
    case hardware_interface::HandleDataType::INT32:
      return "int32_t";
  }
  return {};
}

struct SingleInterface {
  ComponentType component_type;
  std::string component_name;
  std::string interface_name;
  hardware_interface::HandleDataType data_type;

  friend std::ostream &operator<<(std::ostream &os,
                                  const SingleInterface &interface) {
    os << component_type_as_string(interface.component_type) << "/"
       << interface.component_name << "/" << interface.interface_name << ":"
       << interface.data_type.to_string();
    return os;
  }
};

struct NestedInterface {
  ComponentType component_type;
  std::string component_name;
  std::string interface_name;
  std::vector<std::pair<std::string, hardware_interface::HandleDataType>>
      fields;

  void sort() {
    std::sort(
        fields.begin(), fields.end(),
        [](const std::pair<std::string, hardware_interface::HandleDataType> &a,
           const std::pair<std::string, hardware_interface::HandleDataType>
               &b) {
          if (a.first != b.first) {
            return a.first < b.first;
          }
          return a.second < b.second;
        });
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const NestedInterface &interface) {
    os << component_type_as_string(interface.component_type) << "/"
       << interface.component_name << "/" << interface.interface_name << "{";
    auto i = 0U;
    for (auto &[field_name, field_type] : interface.fields) {
      os << (i++ == 0 ? "" : ",") << field_name << ":"
         << field_type.to_string();
    }
    os << "}";
    return os;
  }
};

using Interface = std::variant<SingleInterface, NestedInterface>;

class PACKERT_HARDWARE_LOCAL HardwareModel {
 public:
  HardwareModel(std::string_view source,
                const hardware_interface::HardwareInfo &info);

  std::string_view name() const { return name_; }

  std::string_view source() const { return source_; }

  std::string_view canonical() const { return canonical_; }

  std::string_view hash() const { return hash_; }

  json to_json() const;

 private:
  hardware_interface::HandleDataType map_data_type(std::string_view data_type);

  static bool compare_interfaces(const Interface &a, const Interface &b);

  void load_parameters(const hardware_interface::HardwareInfo &info);

  void push_interfaces(ComponentType component_type,
                       const std::string &component_name,
                       const std::string &stripped_name,
                       std::vector<hardware_interface::InterfaceInfo> &from,
                       std::vector<std::shared_ptr<Interface>> &to,
                       std::map<std::pair<std::string, std::string>,
                                std::weak_ptr<NestedInterface>> &nested_map);

  void build_interfaces(const hardware_interface::HardwareInfo &info);

  std::string build_canonical() const;

  const std::string name_;
  const std::string source_;
  std::string canonical_;
  std::string hash_;

  bool use_double_{true};
  std::string strip_prefix_{""};

  std::vector<std::shared_ptr<Interface>> command_interfaces_;
  std::vector<std::shared_ptr<Interface>> state_interfaces_;

  std::map<std::pair<std::string, std::string>, std::weak_ptr<NestedInterface>>
      nested_command_interfaces_;
  std::map<std::pair<std::string, std::string>, std::weak_ptr<NestedInterface>>
      nested_state_interfaces_;
};

}  // namespace packert_hardware

#endif  // PACKERT_HARDWARE__TEMPLATE_HPP_
