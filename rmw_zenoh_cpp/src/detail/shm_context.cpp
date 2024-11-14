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

#include "shm_context.hpp"

namespace rmw_zenoh_cpp
{
///=============================================================================
#ifdef RMW_ZENOH_BUILD_WITH_SHARED_MEMORY
ShmContext::ShmContext(ShmContext && other)
: shm_provider(other.shm_provider),
  msgsize_threshold(other.msgsize_threshold)
{
  ::z_internal_null(&other.shm_provider);
}

ShmContext & ShmContext::operator=(ShmContext && other)
{
  if (this != &other) {
    ::z_drop(::z_move(shm_provider));
    shm_provider = other.shm_provider;
    ::z_internal_null(&other.shm_provider);
    msgsize_threshold = other.msgsize_threshold;
  }
  return *this;
}

ShmContext::~ShmContext()
{
  z_drop(z_move(shm_provider));
}
#endif
}  // namespace rmw_zenoh_cpp
