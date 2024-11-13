// Copyright 2023 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef DETAIL__SHM_CONTEXT_HPP_
#define DETAIL__SHM_CONTEXT_HPP_


namespace rmw_zenoh_cpp
{
///=============================================================================
#ifdef RMW_ZENOH_BUILD_WITH_SHARED_MEMORY
struct ShmContext
{
  z_owned_shm_provider_t shm_provider;
  size_t msgsize_threshold;

  ~ShmContext();
};
#endif
}  // namespace rmw_zenoh_cpp

#endif  // DETAIL__SHM_CONTEXT_HPP_
