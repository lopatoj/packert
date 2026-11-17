#ifndef PACKERT_HARDWARE__TEMPLATE_HPP_
#define PACKERT_HARDWARE__TEMPLATE_HPP_

#include <map>
#include <memory>
#include <string_view>
#include <variant>
#include <vector>
#include "hardware_interface/hardware_info.hpp"
#include "packert_hardware/visibility_control.h"

namespace packert_hardware {

enum class ComponentType { JOINT = 0, SENSOR, GPIO };

struct SingleInterface {
  ComponentType component_type;
  std::string component_name;
  std::string interface_name;
  hardware_interface::HandleDataType data_type;
};

struct NestedInterface {
  ComponentType component_type;
  std::string component_name;
  std::string interface_name;
  std::vector<std::pair<std::string, hardware_interface::HandleDataType>>
      fields;
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

 private:
  hardware_interface::HandleDataType map_data_type(std::string_view data_type);

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
