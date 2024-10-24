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

#include "zenoh_utils.hpp"

#include <chrono>

#include "attachment_helpers.hpp"
#include "rmw/types.h"

namespace rmw_zenoh_cpp
{
///=============================================================================
void create_map_and_set_sequence_num(
  z_owned_bytes_t * out_bytes, int64_t sequence_number, uint8_t gid[RMW_GID_STORAGE_SIZE])
{
  auto now = std::chrono::system_clock::now().time_since_epoch();
  auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
  int64_t source_timestamp = now_ns.count();

  rmw_zenoh_cpp::attachement_data_t data(sequence_number, source_timestamp, gid);
  data.serialize_to_zbytes(out_bytes);
}

///=============================================================================
ZenohQuery::ZenohQuery(z_owned_query_t query) { query_ = query; }

///=============================================================================
ZenohQuery::~ZenohQuery() { z_drop(z_move(query_)); }

///=============================================================================
const z_loaned_query_t * ZenohQuery::get_query() const { return z_loan(query_); }

///=============================================================================
ZenohReply::ZenohReply(z_owned_reply_t reply) { reply_ = reply; }

///=============================================================================
ZenohReply::~ZenohReply() { z_drop(z_move(reply_)); }

///=============================================================================
std::optional<const z_loaned_sample_t *> ZenohReply::get_sample() const
{
  if (z_reply_is_ok(z_loan(reply_))) {
    return z_reply_ok(z_loan(reply_));
  }

  return std::nullopt;
}
}  // namespace rmw_zenoh_cpp
