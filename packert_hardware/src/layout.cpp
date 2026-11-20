#include "packert_hardware/layout.hpp"
#include <array>
#include <hardware_interface/hardware_info.hpp>
#include <regex>
#include <stdexcept>
#include <string_view>
#include <vector>
#include "hardware_interface/lexical_casts.hpp"
#include "packert_hardware/packert.hpp"

namespace packert_hardware {

inline constexpr const char *SNAKE_CASE{"^[a-z][a-z0-9_]*$,"};

inline bool is_snake_case(std::string_view name) {
  static const std::regex re{SNAKE_CASE};
  return std::regex_match(name.begin(), name.end(), re);
}

inline constexpr std::array<std::string_view, 136> KEYWORDS{
    {"_Alignas",
     "_Alignof",
     "_Atomic",
     "_BitInt",
     "_Bool",
     "_Complex",
     "_Decimal128",
     "_Decimal32",
     "_Decimal64",
     "_Generic",
     "_Imaginary",
     "_Noreturn",
     "_Pragma",
     "_Static_assert",
     "_Thread_local",
     "__has_c_attribute",
     "__has_cpp_attribute",
     "__has_embed",
     "__has_include",
     "alignas",
     "alignof",
     "and",
     "and_eq",
     "asm",
     "audit",
     "auto",
     "axiom",
     "bitand",
     "bitor",
     "bool",
     "break",
     "case",
     "catch",
     "char",
     "char16_t",
     "char32_t",
     "char8_t",
     "class",
     "co_await",
     "co_return",
     "co_yield",
     "compl",
     "concept",
     "const",
     "const_cast",
     "consteval",
     "constexpr",
     "constinit",
     "continue",
     "decltype",
     "default",
     "define",
     "defined",
     "delete",
     "do",
     "double",
     "dynamic_cast",
     "elif",
     "elifdef",
     "elifndef",
     "else",
     "embed",
     "endif",
     "enum",
     "error",
     "explicit",
     "export",
     "extern",
     "false",
     "final",
     "float",
     "for",
     "friend",
     "goto",
     "if",
     "ifdef",
     "ifndef",
     "import",
     "include",
     "inline",
     "int",
     "line",
     "long",
     "module",
     "mutable",
     "namespace",
     "new",
     "noexcept",
     "not",
     "not_eq",
     "nullptr",
     "operator",
     "or",
     "or_eq",
     "override",
     "posix",
     "pragma",
     "private",
     "protected",
     "public",
     "register",
     "reinterpret_cast",
     "requires",
     "restrict",
     "return",
     "short",
     "signed",
     "sizeof",
     "static",
     "static_assert",
     "static_cast",
     "std",
     "struct",
     "switch",
     "template",
     "this",
     "thread_local",
     "throw",
     "transaction_safe",
     "true",
     "try",
     "typedef",
     "typeid",
     "typename",
     "undef",
     "union",
     "unsigned",
     "using",
     "virtual",
     "void",
     "volatile",
     "warning",
     "wchar_t",
     "while",
     "xor",
     "xor_eq"}};

constexpr bool is_keyword(std::string_view name) {
  for (auto k : KEYWORDS) {
    if (k == name) return true;
  }
  return false;
}

inline constexpr std::array<std::pair<std::string_view, std::string_view>, 9>
    TYPES{{{"double", "f64"},
           {"float32", "f32"},
           {"bool", "b"},
           {"uint8", "u8"},
           {"int8", "i8"},
           {"uint16", "u16"},
           {"int16", "i16"},
           {"uint32", "u32"},
           {"int32", "i32"}}};

constexpr std::string_view map_type(std::string_view type,
                                    bool float64_as_float32) {
  if (type == "double" && float64_as_float32) {
    return "f32";
  }
  for (auto &[from, to] : TYPES) {
    if (type == from) {
      return to;
    }
  }
  return {};
}

bool validate(const hardware_interface::HardwareInfo &info) {
  if (!is_snake_case(info.name) || is_keyword(info.name)) {
    return false;
  }

  for (auto &components : {info.joints, info.sensors, info.gpios}) {
    for (auto &component : components) {
      if (!is_snake_case(component.name) || is_keyword(component.name)) {
        return false;
      }

      for (auto &command : component.command_interfaces) {
        if (!is_snake_case(command.name) || is_keyword(command.name)) {
          return false;
        }
      }

      for (auto &state : component.state_interfaces) {
        if (!is_snake_case(state.name) || is_keyword(state.name)) {
          return false;
        }
      }
    }
  }

  return true;
}

std::pair<std::vector<std::string>, std::vector<std::string>> get_interfaces(
    const hardware_interface::HardwareInfo &info, std::string_view strip_prefix,
    bool float64_as_float32) {
  std::vector<std::string> states;
  std::vector<std::string> commands;
  std::vector<std::pair<std::string_view,
                        std::vector<hardware_interface::ComponentInfo>>>
      component_map{{{"joint", info.joints},
                     {"sensor", info.sensors},
                     {"gpio", info.gpios}}};

  for (auto &[component_type, components] : component_map) {
    for (auto &component : components) {
      if (component.name.substr(0, strip_prefix.length()) != strip_prefix) {
        throw std::invalid_argument(fmt::format("{} {} lacks prefix {}",
                                                component_type, component.name,
                                                strip_prefix));
      }
      auto stripped_name = component.name.substr(strip_prefix.length());
      for (auto &interface : component.state_interfaces) {
        auto type = map_type(interface.data_type, float64_as_float32);
        states.push_back(fmt::format("{}/{}/{}:{}", component_type,
                                     stripped_name, interface.name, type));
      }
      for (auto &interface : component.command_interfaces) {
        auto type = map_type(interface.data_type, float64_as_float32);
        states.push_back(fmt::format("{}/{}/{}:{}", component_type,
                                     stripped_name, interface.name, type));
      }
    }
  }

  return {states, commands};
}

std::string canonical(const hardware_interface::HardwareInfo &info) {
  std::string_view strip_prefix{""};
  bool float64_as_float32{false};
  std::ostringstream stream{};

  if (info.hardware_parameters.find("strip_prefix") !=
      info.hardware_parameters.end()) {
    strip_prefix = info.hardware_parameters.at("strip_prefix");
  }

  if (info.hardware_parameters.find("float64_as_float32") !=
      info.hardware_parameters.end()) {
    float64_as_float32 = hardware_interface::parse_bool(
        info.hardware_parameters.at("float64_as_float32"));
  }

  stream << "packert:" << PACKERT_VERSION << "|" << info.name << "|state";
  auto [states, commands] =
      get_interfaces(info, strip_prefix, float64_as_float32);
  for (auto &state : states) {
    stream << "/" << state;
  }
  stream << "|command";
  for (auto &command : commands) {
    stream << "/" << command;
  }

  return stream.str();
}

}  // namespace packert_hardware
