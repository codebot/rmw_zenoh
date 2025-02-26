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
  VCS_VERSION fd8567b631c8836f1239714029c1f395213bb095
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

execute_process(
  COMMAND "${PYTHON_EXECUTABLE}" -c "if True:
    from distutils import sysconfig as sc
    print(sc.get_python_lib(prefix='', plat_specific=True))"
  OUTPUT_VARIABLE PYTHON_SITE
  OUTPUT_STRIP_TRAILING_WHITESPACE)

message("ALex: " ${OUTPUT_VARIABLE})
message("ALex: " ${OUTPUT_STRIP_TRAILING_WHITESPACE})
message("ALex: " ${PYTHON_SITE})

ament_get_python_install_dir(PYTHON_DIR_INSTALL)
message("ALex: " ${PYTHON_DIR_INSTALL})
message("ALex: " ${CMAKE_INSTALL_PREFIX})

include(ExternalProject)
ExternalProject_Add(zenoh_python_vendor
  GIT_REPOSITORY    https://github.com/eclipse-zenoh/zenoh-python
  GIT_TAG           e117aee2af831485aefc4f51bf482e5324fe1a53
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     maturin build --release
  INSTALL_COMMAND   wheel unpack target/wheels/eclipse_zenoh-1.2.1-cp38-abi3-manylinux_2_34_x86_64.whl --dest ${CMAKE_INSTALL_PREFIX}/${PYTHON_DIR_INSTALL}/zenoh
  # TEST_COMMAND      ""
  BUILD_IN_SOURCE 1
)

externalproject_add_stepdependencies(zenoh_cpp_vendor configure zenoh_c_vendor)

ament_export_dependencies(zenohcxx)

ament_package()
