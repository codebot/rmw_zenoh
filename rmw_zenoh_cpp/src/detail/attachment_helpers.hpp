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

#include <zenoh.h>

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
    const uint8_t source_gid[RMW_GID_STORAGE_SIZE]);
  explicit AttachmentData(const z_loaned_bytes_t *);
  explicit AttachmentData(AttachmentData && data);

  int64_t sequence_number() const;
  int64_t source_timestamp() const;
  void copy_gid(uint8_t out_gid[RMW_GID_STORAGE_SIZE]) const;
  size_t gid_hash() const;

  void serialize_to_zbytes(z_owned_bytes_t *);

private:
  int64_t sequence_number_;
  int64_t source_timestamp_;
  uint8_t source_gid_[RMW_GID_STORAGE_SIZE];
  size_t gid_hash_;
};
}  // namespace rmw_zenoh_cpp

#endif  // DETAIL__ATTACHMENT_HELPERS_HPP_
