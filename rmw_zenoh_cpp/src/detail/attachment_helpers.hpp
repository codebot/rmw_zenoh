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
#include <string>

#include <zenoh.hxx>

#include "rmw/types.h"

namespace rmw_zenoh_cpp
{
///=============================================================================
class AttachmentData final
{
public:
  /// @brief Constructor.
  /// @param sequence_number A monotonically increasing count.
  /// @param source_timestamp The time when the attachment was originally created.
  /// @param source_gid GID of the entity that originally created this attachment.
  /// @param zid The zenoh session id of the entity that originally created this attachment.
  AttachmentData(
    const int64_t sequence_number,
    const int64_t source_timestamp,
    const std::array<uint8_t, RMW_GID_STORAGE_SIZE> source_gid,
    const std::string & zid);

  explicit AttachmentData(const zenoh::Bytes & bytes);
  explicit AttachmentData(AttachmentData && data);

  int64_t sequence_number() const;
  int64_t source_timestamp() const;
  std::array<uint8_t, RMW_GID_STORAGE_SIZE> copy_gid() const;
  std::string zid() const;

  zenoh::Bytes serialize_to_zbytes();

private:
  int64_t sequence_number_;
  int64_t source_timestamp_;
  std::array<uint8_t, RMW_GID_STORAGE_SIZE> source_gid_;
  std::string zid_;
};
}  // namespace rmw_zenoh_cpp

#endif  // DETAIL__ATTACHMENT_HELPERS_HPP_
