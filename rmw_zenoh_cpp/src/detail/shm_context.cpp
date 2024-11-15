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

#include <stdexcept>

#include "shm_context.hpp"

namespace rmw_zenoh_cpp
{
///=============================================================================
#ifdef RMW_ZENOH_BUILD_WITH_SHARED_MEMORY
ShmContext::ShmContext(size_t alloc_size, size_t msgsize_threshold)
:msgsize_threshold(msgsize_threshold)
{
    // Create Layout for provider's memory
    // Provider's alignment will be 1 byte as we are going to make only 1-byte aligned allocations
    // TODO(yellowhatter): use zenoh_shm_message_size_threshold as base for alignment
  z_alloc_alignment_t alignment = {0};
  z_owned_memory_layout_t layout;
  if (z_memory_layout_new(&layout, alloc_size, alignment) != Z_OK) {
    throw std::runtime_error("Unable to create a Layout for SHM provider.");
  }
    // Create SHM provider
  const auto provider_creation_result =
    z_posix_shm_provider_new(&shm_provider, z_loan(layout));
  z_drop(z_move(layout));
  if (provider_creation_result != Z_OK) {
    throw std::runtime_error("Unable to create an SHM provider.");
  }
}


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
