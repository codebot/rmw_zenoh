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
const zenoh::Reply & ZenohReply::get_sample() const
{
  return reply_.value();
}

///=============================================================================
std::chrono::nanoseconds::rep ZenohReply::get_received_timestamp() const
{
  return received_timestamp_;
}

int64_t get_system_time_in_ns()
{
  auto now = std::chrono::system_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}

///=============================================================================
Payload::Payload(const zenoh::Bytes & bytes)
{
  // NOTE(fuzzypixelz): `zenoh::Bytes` is an list of reference-couted buffers. When the list of
  // buffers contains exactly one element, it is not necessary to concatenate the list of buffers.
  // In this case, we store a clone of the bytes object to maintain a non-zero reference-count on
  // the buffer. This ensures that the slice into said buffer stays valid until we drop our copy
  // of the bytes object (at the very least). This case corresponds to the `Contiguous`
  // alternative of the `bytes_` variant and aims to optimize away a memcpy during "session-local"
  // communication.

  zenoh::Bytes::SliceIterator slices = bytes.slice_iter();
  std::optional<zenoh::Slice> slice = slices.next();
  if (!slice.has_value()) {
    bytes_ = nullptr;
  } else {
    if (!slices.next().has_value()) {
      bytes_ = Contiguous {slice.value(), bytes.clone()};
    } else {
      bytes_ = bytes.as_vector();
    }
  }
}

const uint8_t * Payload::data() const
{
  if (std::holds_alternative<Empty>(bytes_)) {
    return nullptr;
  } else if (std::holds_alternative<NonContiguous>(bytes_)) {
    return std::get<NonContiguous>(bytes_).data();
  } else {
    return std::get<Contiguous>(bytes_).slice.data;
  }
}

size_t Payload::size() const
{
  if (std::holds_alternative<Empty>(bytes_)) {
    return 0;
  } else if (std::holds_alternative<NonContiguous>(bytes_)) {
    return std::get<NonContiguous>(bytes_).size();
  } else {
    return std::get<Contiguous>(bytes_).slice.len;
  }
}

bool Payload::empty() const
{
  return std::holds_alternative<Empty>(bytes_);
}

///=============================================================================
BufferPool::BufferPool()
: buffers_(), mutex_()
{
  const char * env_value;
  const char * error_str = rcutils_get_env("RMW_ZENOH_BUFFER_POOL_MAX_SIZE_BYTES", &env_value);
  if (error_str != nullptr) {
    RMW_ZENOH_LOG_WARN_NAMED(
      "rmw_zenoh_cpp",
      "Unable to read maximum buffer pool size, falling back to default.");
    max_size_ = DEFAULT_MAX_SIZE;
  } else if (strcmp(env_value, "") == 0) {
    max_size_ = DEFAULT_MAX_SIZE;
  } else {
    max_size_ = std::atoll(env_value);
  }
  size_ = 0;
}

///=============================================================================
BufferPool::~BufferPool()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  for (Buffer & buffer : buffers_) {
    allocator.deallocate(buffer.data, allocator.state);
  }
}

///=============================================================================
BufferPool::Buffer BufferPool::allocate(size_t size)
{
  std::lock_guard<std::mutex> guard(mutex_);

  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  if (buffers_.empty()) {
    if (size_ + size > max_size_) {
      return {};
    } else {
      size_ += size;
    }
    uint8_t * data = static_cast<uint8_t *>(allocator.allocate(size, allocator.state));
    if (data == nullptr) {
      return {};
    } else {
      return Buffer {data, size};
    }
  } else {
    Buffer buffer = buffers_.back();
    buffers_.pop_back();
    if (buffer.size < size) {
      size_t size_diff = size - buffer.size;
      if (size_ + size_diff > max_size_) {
        return {};
      }
      uint8_t * data = static_cast<uint8_t *>(allocator.reallocate(
          buffer.data, size, allocator.state));
      if (data == nullptr) {
        return {};
      }
      size_ += size_diff;
      buffer.data = data;
      buffer.size = size;
    }
    return buffer;
  }
}

///=============================================================================
void BufferPool::deallocate(BufferPool::Buffer buffer)
{
  std::lock_guard<std::mutex> guard(mutex_);
  buffers_.push_back(buffer);
}

}  // namespace rmw_zenoh_cpp
