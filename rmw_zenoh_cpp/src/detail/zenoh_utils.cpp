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
#include <cinttypes>

#include "attachment_helpers.hpp"
#include "rcpputils/scope_exit.hpp"

#include "rmw/error_handling.h"

namespace rmw_zenoh_cpp
{
/// Loan the zenoh session.
///=============================================================================
const z_loaned_session_t * ZenohSession::loan()
{
  return z_loan(inner_);
}

/// Close the zenoh session if destructed.
///=============================================================================
ZenohSession::~ZenohSession()
{
  z_close(z_loan_mut(inner_), NULL);
}

///=============================================================================
void create_map_and_set_sequence_num(
  z_owned_bytes_t * out_bytes, int64_t sequence_number, uint8_t gid[RMW_GID_STORAGE_SIZE])
{
  auto now = std::chrono::system_clock::now().time_since_epoch();
  auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
  int64_t source_timestamp = now_ns.count();

  AttachmentData data(sequence_number, source_timestamp, gid);
  data.serialize_to_zbytes(out_bytes);
}

///=============================================================================
ZenohQuery::ZenohQuery(
  const z_loaned_query_t * query,
  std::chrono::nanoseconds::rep received_timestamp)
{
  z_query_clone(&query_, query);
  received_timestamp_ = received_timestamp;
}

///=============================================================================
std::chrono::nanoseconds::rep ZenohQuery::get_received_timestamp() const
{
  return received_timestamp_;
}

///=============================================================================
ZenohQuery::~ZenohQuery()
{
  z_drop(z_move(query_));
}

///=============================================================================
const z_loaned_query_t * ZenohQuery::get_query() const {return z_loan(query_);}

///=============================================================================
ZenohReply::ZenohReply(
  const z_loaned_reply_t * reply,
  std::chrono::nanoseconds::rep received_timestamp)
{
  z_reply_clone(&reply_, reply);
  received_timestamp_ = received_timestamp;
}

///=============================================================================
ZenohReply::~ZenohReply()
{
  z_drop(z_move(reply_));
}

///=============================================================================
std::optional<const z_loaned_sample_t *> ZenohReply::get_sample() const
{
  if (z_reply_is_ok(z_loan(reply_))) {
    return z_reply_ok(z_loan(reply_));
  }

  return std::nullopt;
}

///=============================================================================
std::chrono::nanoseconds::rep ZenohReply::get_received_timestamp() const
{
  return received_timestamp_;
}
}  // namespace rmw_zenoh_cpp
