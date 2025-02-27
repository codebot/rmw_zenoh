# External Dependency Quality declaration zenoh-c

This document is a declaration of software quality for the `zenoh-c` external dependency, based on the guidelines in [REP-2004](https://github.com/ros-infrastructure/rep/blob/rep-2004/rep-2004.rst).

The [zenoh-c](https://github.com/eclipse-zenoh/zenoh-c) external dependency is a C binding based on the main [Zenoh implementation written in Rust](https://github.com/eclipse-zenoh/zenoh).
It is maintained by [Eclipse Zenoh GitHub](https://github.com/eclipse-zenoh) organization together with other repositories.
First, a summary discussing how this library is qualified is presented, and then it will be listed how this library matches the standards defined for ROS packages.

## Summary

The `zenoh-c` meets the basic requirements for a software platform in terms of testing its basic functionality, providing a [valid license](https://github.com/eclipse-zenoh/zenoh-c/blob/main/LICENSE) for the code used and a public GitHub repository with the changes made to the code over time.

TODO(zettascale): Include paragraph How are we going to asure API/ABI policy ?

There is no explicit support for any OS platform, however their [GitHub repository](https://github.com/eclipse-zenoh/zenoh-c) installation appears to be targeting Linux.
The first version of this library was developed in 2020, and it is used widely.

TODO(zettascale): Some generic statidistics about how many people use this package and if there is any other open source
project that use this and Why we use this library here

Considering the previously mentioned reasons, we consider this library to be robust and reliable and at Quality Level 1.

# Comparison with ROS packages quality standards

## Version policy [1]

### Version Scheme [1.i]

TODO(zettascale): Explain version scheme

### Version Stability [1.ii]

TODO(zettascale): Explain version Stability

### Public API Declaration [1.iii]

As a C library, elements available in [zenoh.h](https://github.com/eclipse-zenoh/zenoh-c/blob/main/include/zenoh.h) are considered to be the library's public API.

### API Stability Policy [1.iv]

TODO(zettascale): Explain API Stability

### ABI Stability Policy [1.v]

TODO(zettascale): Explain ABI Stability

### ABI and ABI Stability Within a Released ROS Distribution [1.vi]

TODO(zettascale): Explain ABI and ABI Stability  Within a Released ROS Distribution

## Change Control Process [2]

### Change Requests [2.i]

Checking through the commits history, it can be seen is not the case.

### Contributor Origin [2.ii]

Does not have it (or it does not seem like it’s the case).

### Peer Review Policy [2.iii]

Seems to be followed for pull requests on the GitHub repository, but as not all code changes occur through change requests, this can not be confirmed for these changes.

### Continuous Integration [2.iv]

`zenoh-c` is compiled in the ROS 2 buildfarm for all [tier 1 platforms](https://www.ros.org/reps/rep-2000.html#support-tiers).

Currently nightly results can be seen here:
* [linux-aarch64_release](https://ci.ros2.org/view/nightly/job/nightly_linux-aarch64_release/lastBuild/testReport/zenoh_cpp_vendor/)
* [linux_release](https://ci.ros2.org/view/nightly/job/nightly_linux_release/lastBuild/testReport/zenoh_cpp_vendor/)
* [mac_osx_release](https://ci.ros2.org/view/nightly/job/nightly_osx_release/lastBuild/testReport/zenoh_cpp_vendor/)
* [windows_release](https://ci.ros2.org/view/nightly/job/nightly_win_rel/lastBuild/testReport/zenoh_cpp_vendor/)

### Documentation Policy [2.v]

Not available.

## Documentation [3]

### Feature Documentation [3.i]

Provided [doxygen documentation](https://zenoh-c.readthedocs.io/en) for the whole project.

### Public API Documentation [3.ii]

Yes, doxygen documentation is available for library [here](https://zenoh-c.readthedocs.io/en).

### License [3.iii]

Apache 2.0 license declared for the repository, it can be found [here](https://github.com/eclipse-zenoh/zenoh-c/blob/main/LICENSE).

### Copyright Statements [3.iv]

Is not available.

### Quality Declaration [3.v]

This document represents the Quality Declaration document for the `zenoh-c` ROS dependency.

## Testing [4]

### Feature Testing [4.i]

Tests provided to cover the expected usage of the library, for the version of the library used can be found [here](https://github.com/eclipse-zenoh/zenoh-c?tab=readme-ov-file).

### Public API Testing [4.ii]

Not clear without coverage results to check if all the API is covered.

### Coverage [4.iii]

Code coverage and internal policies are not public, if any.

### Performance [4.iv]

Performance tests are defined in the vendored package.

### Linters and Static Analysis [4.v]

Not available publicly, if any.

## Dependencies [5]

### Direct Runtime ROS Dependencies [5.i]

The `zenoh-c` library does not add additional dependencies, it only requires C++ standard libraries to be built and used.

### Optional Direct Runtime ROS Dependencies [5.ii]

Does not apply for external dependencies.

### Direct Runtime non-ROS Dependency [5.iii]

The `zenoh-c` library require RUST and cargo to be able to compile it.

## Platform Support [6]

This library does not state support for any specific platform, but it is built in the ROS 2 buildfarm for all tier 1 platforms:

TODO(ahcorde): Review links
Currently nightly results can be seen here:
* [linux-aarch64_release](https://ci.ros2.org/view/nightly/job/nightly_linux-aarch64_release/lastBuild/testReport/zenoh_cpp_vendor/)
* [linux_release](https://ci.ros2.org/view/nightly/job/nightly_linux_release/lastBuild/testReport/zenoh_cpp_vendor/)
* [mac_osx_release](https://ci.ros2.org/view/nightly/job/nightly_osx_release/lastBuild/testReport/zenoh_cpp_vendor/)
* [windows_release](https://ci.ros2.org/view/nightly/job/nightly_win_rel/lastBuild/testReport/zenoh_cpp_vendor/)

## Security [7]

### Vulnerability Disclosure Policy [7.i]

TODO(zettascale): Vulnerability Disclosure Policy
The `zenoh-c` library does not have a Vulnerability Disclosure Policy. But for ROS 2's purposes, see the policy defined in the Quality Declaration of the `zenoh_cpp_vendor` package.
