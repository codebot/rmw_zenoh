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

#ifndef DETAIL__RMW_PUBLISHER_DATA_HPP_
#define DETAIL__RMW_PUBLISHER_DATA_HPP_

#include <zenoh.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "event.hpp"
#include "liveliness_utils.hpp"
#include "message_type_support.hpp"
#include "type_support_common.hpp"

#include "rcutils/allocator.h"

#include "rmw/rmw.h"
#include "rmw/ret_types.h"

namespace rmw_zenoh_cpp
{
///=============================================================================
class PublisherData final
{
public:
  // Make a shared_ptr of PublisherData.
  static std::shared_ptr<PublisherData> make(
    z_session_t session,
    const rmw_node_t * const node,
    liveliness::NodeInfo node_info,
    std::size_t node_id,
    std::size_t publisher_id,
    const std::string & topic_name,
    const rosidl_message_type_support_t * type_support,
    const rmw_qos_profile_t * qos_profile);

  // Get a copy of the actual qos_profile used by this publisher.
  rmw_qos_profile_t adapted_qos_profile() const;

  // Publish a ROS message.
  rmw_ret_t publish(
    const void * ros_message,
    std::optional<zc_owned_shm_manager_t> & shm_manager);

  // Get the next sequence number.
  size_t get_next_sequence_number();

  // Get a copy of the GUID of this PublisherData's liveliness::Entity.
  std::size_t guid() const;

  // Get a copy of the TopicInfo of this PublisherData.
  liveliness::TopicInfo topic_info() const;

  // Get the GID of this PublisherData.
  const uint8_t * gid() const;

  // Get the Zenoh publisher.
  z_publisher_t publisher() const;

  // Returns true if liveliness token is still valid.
  bool liveliness_is_valid() const;

  // Get the events manager of this PublisherData.
  std::shared_ptr<EventsManager> events_mgr() const;

  // Shutdown this PublisherData.
  rmw_ret_t shutdown();

  // Check if this PublisherData is shutdown.
  bool is_shutdown() const;

  // Destructor.
  ~PublisherData();

private:
  // Constructor.
  PublisherData();

  // Internal mutex.
  mutable std::mutex mutex_;
  // The parent node
  const rmw_node_t * rmw_node_;
  // The Entity generated for the publisher.
  std::shared_ptr<liveliness::Entity> entity_;
  uint8_t gid_[RMW_GID_STORAGE_SIZE];
  // An owned publisher.
  z_owned_publisher_t pub_;
  // Optional publication cache when durability is transient_local.
  std::optional<ze_owned_publication_cache_t> pub_cache_;
  // Liveliness token for the publisher.
  zc_owned_liveliness_token_t token_;
  // Type support fields
  const void * type_support_impl_;
  std::unique_ptr<MessageTypeSupport> type_support_;
  std::shared_ptr<EventsManager> events_mgr_;
  size_t sequence_number_;
  // Shutdown flag.
  bool is_shutdown_;
};
using PublisherDataPtr = std::shared_ptr<PublisherData>;
using PublisherDataConstPtr = std::shared_ptr<const PublisherData>;
}  // namespace rmw_zenoh_cpp

#endif  // DETAIL__RMW_PUBLISHER_DATA_HPP_
