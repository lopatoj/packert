cmake_minimum_required(VERSION 3.8)
project(packert_hardware)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_ros REQUIRED)
find_package(hardware_interface REQUIRED)
find_package(fmt REQUIRED)
find_package(tobas_inja_vendor REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(packert_system src/packert_system.cpp)
add_library(packert_hardware::packert_system ALIAS packert_system)
target_compile_features(packert_system PUBLIC c_std_99 cxx_std_17)
target_include_directories(packert_system PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>)
target_link_libraries(packert_system PUBLIC
  hardware_interface::hardware_interface
  fmt::fmt
  tobas_inja_vendor::inja
  nlohmann_json::nlohmann_json)

# Causes the visibility macros to use dllexport rather than dllimport,
# which is appropriate when building the dll but not consuming it.
target_compile_definitions(packert_system PRIVATE
  "PACKERT_HARDWARE_BUILDING_LIBRARY")

install(
  DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
)
install(
  TARGETS packert_system
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_export_include_directories(
  "include/${PROJECT_NAME}"
)
ament_export_libraries(
  packert_system
)
ament_export_targets(
  export_${PROJECT_NAME}
)

ament_package()
