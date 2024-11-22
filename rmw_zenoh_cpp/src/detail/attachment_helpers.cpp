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

#include <zenoh.h>

#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "rmw/types.h"

#include "attachment_helpers.hpp"

namespace rmw_zenoh_cpp
{

AttachmentData::AttachmentData(
  const int64_t _sequence_number,
  const int64_t _source_timestamp,
  const uint8_t _source_gid[RMW_GID_STORAGE_SIZE])
{
  sequence_number = _sequence_number;
  source_timestamp = _source_timestamp;
  for (size_t i = 0; i < RMW_GID_STORAGE_SIZE; ++i)
  {
    source_gid.push_back(_source_gid[RMW_GID_STORAGE_SIZE - 1 - i]);
  }
}

AttachmentData::AttachmentData(AttachmentData && data)
{
  sequence_number = std::move(data.sequence_number);
  source_timestamp = std::move(data.source_timestamp);
  source_gid = data.source_gid;
}

zenoh::Bytes AttachmentData::serialize_to_zbytes()
{
  auto serializer = zenoh::ext::Serializer();
  serializer.serialize(std::string("sequence_number"));
  serializer.serialize(this->sequence_number);
  serializer.serialize(std::string("source_timestamp"));
  serializer.serialize(this->source_timestamp);
  serializer.serialize(std::string("source_gid"));
  serializer.serialize(this->source_gid);
  return std::move(serializer).finish();
}

AttachmentData::AttachmentData(const zenoh::Bytes & attachment)
{
  zenoh::ext::Deserializer deserializer(std::move(attachment));
  const auto sequence_number_str = deserializer.deserialize<std::string>();
  if (sequence_number_str != "sequence_number")
  {
    throw std::runtime_error("sequence_number is not found in the attachment.");
  }
  this->sequence_number = deserializer.deserialize<int64_t>();

  const auto source_timestamp_str = deserializer.deserialize<std::string>();
  if (source_timestamp_str != "source_timestamp")
  {
    throw std::runtime_error("source_timestamp is not found in the attachment.");
  }
  this->source_timestamp = deserializer.deserialize<int64_t>();

  const auto source_gid_str = deserializer.deserialize<std::string>();
  if (source_gid_str != "source_gid")
  {
    throw std::runtime_error("source_gid is not found in the attachment.");
  }
  this->source_gid = deserializer.deserialize<std::vector<uint8_t>>();
}

attachment_data_t::attachment_data_t(
  const int64_t _sequence_number,
  const int64_t _source_timestamp,
  const uint8_t _source_gid[RMW_GID_STORAGE_SIZE])
{
  sequence_number = _sequence_number;
  source_timestamp = _source_timestamp;
  memcpy(source_gid, _source_gid, RMW_GID_STORAGE_SIZE);
}

attachment_data_t::attachment_data_t(attachment_data_t && data)
{
  sequence_number = std::move(data.sequence_number);
  source_timestamp = std::move(data.source_timestamp);
  memcpy(source_gid, data.source_gid, RMW_GID_STORAGE_SIZE);
}

void attachment_data_t::serialize_to_zbytes(z_owned_bytes_t * attachment)
{
  ze_owned_serializer_t serializer;
  ze_serializer_empty(&serializer);
  ze_serializer_serialize_str(z_loan_mut(serializer), "sequence_number");
  ze_serializer_serialize_int64(z_loan_mut(serializer), this->sequence_number);
  ze_serializer_serialize_str(z_loan_mut(serializer), "source_timestamp");
  ze_serializer_serialize_int64(z_loan_mut(serializer), this->source_timestamp);
  ze_serializer_serialize_str(z_loan_mut(serializer), "source_gid");
  ze_serializer_serialize_buf(z_loan_mut(serializer), this->source_gid, RMW_GID_STORAGE_SIZE);
  ze_serializer_finish(z_move(serializer), attachment);
}

attachment_data_t::attachment_data_t(const z_loaned_bytes_t * attachment)
{
  ze_deserializer_t deserializer = ze_deserializer_from_bytes(attachment);
  z_owned_string_t key;

  ze_deserializer_deserialize_string(&deserializer, &key);

  // Deserialize the sequence_number
  if (std::string_view(
      z_string_data(z_loan(key)),
      z_string_len(z_loan(key))) != "sequence_number")
  {
    throw std::runtime_error("sequence_number is not found in the attachment.");
  }
  z_drop(z_move(key));
  if (ze_deserializer_deserialize_int64(&deserializer, &this->sequence_number)) {
    throw std::runtime_error("Failed to deserialize the sequence_number.");
  }

  // Deserialize the source_timestamp
  ze_deserializer_deserialize_string(&deserializer, &key);
  if (std::string_view(
      z_string_data(z_loan(key)),
      z_string_len(z_loan(key))) != "source_timestamp")
  {
    throw std::runtime_error("source_timestamp is not found in the attachment");
  }
  z_drop(z_move(key));
  if (ze_deserializer_deserialize_int64(&deserializer, &this->source_timestamp)) {
    throw std::runtime_error("Failed to deserialize the source_timestamp.");
  }

  // Deserialize the source_gid
  ze_deserializer_deserialize_string(&deserializer, &key);
  if (std::string_view(z_string_data(z_loan(key)), z_string_len(z_loan(key))) != "source_gid") {
    throw std::runtime_error("Invalid attachment: the key source_gid is not found");
  }
  z_drop(z_move(key));
  z_owned_slice_t slice;
  if (ze_deserializer_deserialize_slice(&deserializer, &slice)) {
    throw std::runtime_error("Failed to deserialize the source_gid.");
  }
  if (z_slice_len(z_loan(slice)) != RMW_GID_STORAGE_SIZE) {
    throw std::runtime_error("The length of source_gid mismatched.");
  }
  memcpy(this->source_gid, z_slice_data(z_loan(slice)), z_slice_len(z_loan(slice)));
  z_drop(z_move(slice));
}
}  // namespace rmw_zenoh_cpp
