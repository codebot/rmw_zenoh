// Copyright 2024 Open Source Robotics Foundation, Inc.
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

#ifndef DETAIL__ATTACHMENT_HELPERS_HPP_
#define DETAIL__ATTACHMENT_HELPERS_HPP_

#include <array>
#include <cstdint>

#include <zenoh.hxx>

#include "rmw/types.h"

namespace rmw_zenoh_cpp
{
///=============================================================================
class AttachmentData final
{
public:
  AttachmentData(
    const int64_t sequence_number,
    const int64_t source_timestamp,
    const std::array<uint8_t, 16> source_gid);

  explicit AttachmentData(const zenoh::Bytes & bytes);
  explicit AttachmentData(AttachmentData && data);

  int64_t sequence_number() const;
  int64_t source_timestamp() const;
  std::array<uint8_t, 16> copy_gid() const;
  size_t gid_hash() const;

  zenoh::Bytes serialize_to_zbytes();

private:
  int64_t sequence_number_;
  int64_t source_timestamp_;
  std::array<uint8_t, 16> source_gid_;
  size_t gid_hash_;
};
}  // namespace rmw_zenoh_cpp

#endif  // DETAIL__ATTACHMENT_HELPERS_HPP_
