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
# - https://github.com/eclipse-zenoh/zenoh/pull/1742, https://github.com/eclipse-zenoh/zenoh/pull/1765
#    (Add autoconnect_strategy config allowing to optimize peers interconnections)
# - https://github.com/eclipse-zenoh/zenoh/pull/1753
#    (Improve AdvancedSub for faster delivery of first receveived data)
# - https://github.com/eclipse-zenoh/zenoh-cpp/pull/407, https://github.com/eclipse-zenoh/zenoh-c/pull/913
#    (Fix potential loss of request/reply messages in case of network congestion)
ament_vendor(zenoh_c_vendor
  VCS_URL https://github.com/eclipse-zenoh/zenoh-c.git
  VCS_VERSION 261493682c7dc54db3a07079315e009a2e7c1573
  CMAKE_ARGS
    "-DZENOHC_CARGO_FLAGS=${ZENOHC_CARGO_FLAGS}"
    "-DZENOHC_BUILD_WITH_UNSTABLE_API=TRUE"
    "-DZENOHC_CUSTOM_TARGET=${ZENOHC_CUSTOM_TARGET}"
)

ament_export_dependencies(zenohc)

ament_vendor(zenoh_cpp_vendor
  VCS_URL https://github.com/eclipse-zenoh/zenoh-cpp
  VCS_VERSION 5dfb68c9ac966925e59bcb52f39b9bc26c0ad6d3
  CMAKE_ARGS
    -DZENOHCXX_ZENOHC=OFF
)

externalproject_add_stepdependencies(zenoh_cpp_vendor configure zenoh_c_vendor)

ament_export_dependencies(zenohcxx)

ament_package()
