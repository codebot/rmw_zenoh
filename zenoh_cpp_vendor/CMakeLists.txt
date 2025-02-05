cmake_minimum_required(VERSION 3.16)
project(zenoh_cpp_vendor)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_vendor_package REQUIRED)

# Disable default features and enable only the most useful ones. This reduces build time and footprint.
# For a complete list of features see: https://github.com/eclipse-zenoh/zenoh/blob/main/zenoh/Cargo.toml
# Note: We separate the two args needed for cargo with "$<SEMICOLON>" and not ";" as the
# latter is a list separater in cmake and hence the string will be split into two
# when expanded.
set(ZENOHC_CARGO_FLAGS "--no-default-features$<SEMICOLON>--features=shared-memory zenoh/transport_compression zenoh/transport_tcp zenoh/transport_tls")

# Set VCS_VERSION to include latest changes from zenoh/zenoh-c/zenoh-cpp to benefit from :
# - https://github.com/eclipse-zenoh/zenoh/pull/173 (Improve config to support priorities range in locators)
# - https://github.com/eclipse-zenoh/zenoh/pull/1736, https://github.com/eclipse-zenoh/zenoh/pull/1735,
#   https://github.com/eclipse-zenoh/zenoh/pull/1744, https://github.com/eclipse-zenoh/zenoh/pull/1749,
#   https://github.com/eclipse-zenoh/zenoh/pull/1751 (Improve performance of a large number of peers)
ament_vendor(zenoh_c_vendor
  VCS_URL https://github.com/eclipse-zenoh/zenoh-c.git
  VCS_VERSION 5fce7fb1d397e016ad02a50bde4262007d755424
  CMAKE_ARGS
    "-DZENOHC_CARGO_FLAGS=${ZENOHC_CARGO_FLAGS}"
    "-DZENOHC_BUILD_WITH_UNSTABLE_API=TRUE"
    "-DZENOHC_CUSTOM_TARGET=${ZENOHC_CUSTOM_TARGET}"
)

ament_export_dependencies(zenohc)

ament_vendor(zenoh_cpp_vendor
  VCS_URL https://github.com/eclipse-zenoh/zenoh-cpp
  VCS_VERSION bd4d741c6c4fa6509d8d745e22c3c50b4306bd65
  CMAKE_ARGS
    -DZENOHCXX_ZENOHC=OFF
)

externalproject_add_stepdependencies(zenoh_cpp_vendor configure zenoh_c_vendor)

ament_export_dependencies(zenohcxx)

ament_package()
