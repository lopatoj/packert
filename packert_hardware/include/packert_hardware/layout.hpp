#ifndef PACKERT_HARDWARE__LAYOUT_HPP_
#define PACKERT_HARDWARE__LAYOUT_HPP_

#include <stddef.h>
#include <stdint.h>
#include "hardware_interface/hardware_info.hpp"
#include "packert_hardware/visibility_control.h"

namespace packert_hardware {

/// @brief This function checks the passed URDF description and ensures it
/// complies with packert's requirements.

PACKERT_HARDWARE_LOCAL
bool validate(const hardware_interface::HardwareInfo &info);

/// @brief This function produces the canonical string associated with a given
/// hardware description.
PACKERT_HARDWARE_LOCAL
std::string canonical(const hardware_interface::HardwareInfo &info);

PACKERT_HARDWARE_LOCAL
uint32_t hash(std::string_view canonical);

}  // namespace packert_hardware

#endif  // PACKERT_HARDWARE__LAYOUT_HPP_
