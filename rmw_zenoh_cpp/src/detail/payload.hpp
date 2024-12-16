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

#ifndef DETAIL__PAYLOAD_HPP_
#define DETAIL__PAYLOAD_HPP_

#include <zenoh.h>

#include <variant>
#include <utility>
#include <vector>

namespace rmw_zenoh_cpp
{
///=============================================================================
class Payload
{
public:
  explicit Payload(const zenoh::Bytes & bytes)
  {
    auto slices = bytes.slice_iter();
    auto slice = slices.next();
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

  ~Payload() = default;

  const uint8_t * data()
  {
    if (std::holds_alternative<Empty>(bytes_)) {
      return nullptr;
    } else if (std::holds_alternative<NonContiguous>(bytes_)) {
      return std::get<NonContiguous>(bytes_).data();
    } else {
      return std::get<Contiguous>(bytes_).slice.data;
    }
  }

  size_t size()
  {
    if (std::holds_alternative<Empty>(bytes_)) {
      return 0;
    } else if (std::holds_alternative<NonContiguous>(bytes_)) {
      return std::get<NonContiguous>(bytes_).size();
    } else {
      return std::get<Contiguous>(bytes_).slice.len;
    }
  }

  bool empty()
  {
    return std::holds_alternative<Empty>(bytes_);
  }

private:
  struct Contiguous
  {
    zenoh::Slice slice;
    zenoh::Bytes bytes;
  };
  using NonContiguous = std::vector<uint8_t>;
  using Empty = std::nullptr_t;
  // Is `std::vector<uint8_t>` in case of a non-contiguous payload
  // and `zenoh::Slice` plus a `zenoh::Bytes` otherwise.
  std::variant<NonContiguous, Contiguous, Empty> bytes_;
};
}  // namespace rmw_zenoh_cpp

#endif  // DETAIL__PAYLOAD_HPP_
