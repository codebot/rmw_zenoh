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

#include <array>
#include <chrono>
#include <cinttypes>
#include <utility>

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
zenoh::Bytes create_map_and_set_sequence_num(
  int64_t sequence_number, std::array<uint8_t, RMW_GID_STORAGE_SIZE> gid)
{
  auto now = std::chrono::system_clock::now().time_since_epoch();
  auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
  int64_t source_timestamp = now_ns.count();

  rmw_zenoh_cpp::AttachmentData data(sequence_number, source_timestamp, gid);
  return data.serialize_to_zbytes();
}

///=============================================================================
ZenohQuery::ZenohQuery(
  const zenoh::Query & query,
  std::chrono::nanoseconds::rep received_timestamp)
: query_(query.clone())
{
  received_timestamp_ = received_timestamp;
}

///=============================================================================
std::chrono::nanoseconds::rep ZenohQuery::get_received_timestamp() const
{
  return received_timestamp_;
}

///=============================================================================
ZenohQuery::~ZenohQuery() {}

///=============================================================================
const zenoh::Query & ZenohQuery::get_query() const {return query_;}

///=============================================================================
ZenohReply::ZenohReply(
  const zenoh::Reply & reply,
  std::chrono::nanoseconds::rep received_timestamp)
{
  reply_ = reply.clone();
  received_timestamp_ = received_timestamp;
}

///=============================================================================
ZenohReply::~ZenohReply() {}

///=============================================================================
const zenoh::Reply & ZenohReply::get_sample() const
{
  return reply_.value();
}

///=============================================================================
std::chrono::nanoseconds::rep ZenohReply::get_received_timestamp() const
{
  return received_timestamp_;
}
}  // namespace rmw_zenoh_cpp
