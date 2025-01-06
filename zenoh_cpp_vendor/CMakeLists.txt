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
set(ZENOHC_CARGO_FLAGS "--no-default-features$<SEMICOLON>--features=zenoh/transport_compression zenoh/transport_tcp zenoh/transport_tls")

set(RMW_ZENOH_BUILD_WITH_SHARED_MEMORY TRUE CACHE BOOL "Compile Zenoh RMW with Shared Memory support")

ament_vendor(zenoh_c_vendor
  VCS_URL https://github.com/eclipse-zenoh/zenoh-c.git
  VCS_VERSION 85ca060fa4037239ca4102a3a61f96626cc6b434
  CMAKE_ARGS
    "-DZENOHC_CARGO_FLAGS=${ZENOHC_CARGO_FLAGS}"
    "-DZENOHC_BUILD_WITH_SHARED_MEMORY:BOOL=${RMW_ZENOH_BUILD_WITH_SHARED_MEMORY}"
    "-DZENOHC_BUILD_WITH_UNSTABLE_API=TRUE"
    "-DZENOHC_CUSTOM_TARGET=${ZENOHC_CUSTOM_TARGET}"
)

ament_export_dependencies(zenohc)

# Set VCS_VERSION to include latest changes from zenoh-c to benefit from :
# - https://github.com/eclipse-zenoh/zenoh-cpp/pull/342 (Fix include what you use)
ament_vendor(zenoh_cpp_vendor
  VCS_URL https://github.com/eclipse-zenoh/zenoh-cpp
  VCS_VERSION 8f094255bea28f48d75e2be456e1b06875a8cd04
  CMAKE_ARGS
      -DZENOHCXX_ZENOHC=ON
      -DZENOHCXX_ZENOHPICO=OFF
  )

externalproject_add_stepdependencies(zenoh_cpp_vendor configure zenoh_c_vendor)

ament_export_dependencies(zenohcxx)

ament_package()
