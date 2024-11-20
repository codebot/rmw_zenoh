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

#include <zenoh.hxx>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "event.hpp"
#include "liveliness_utils.hpp"
#include "message_type_support.hpp"

#include "rmw/ret_types.h"

namespace rmw_zenoh_cpp
{
///=============================================================================
class PublisherData final
{
public:
  // Make a shared_ptr of PublisherData.
  static std::shared_ptr<PublisherData> make(
    const std::shared_ptr<zenoh::Session> & session,
    const rmw_node_t * const node,
    liveliness::NodeInfo node_info,
    std::size_t node_id,
    std::size_t publisher_id,
    const std::string & topic_name,
    const rosidl_message_type_support_t * type_support,
    const rmw_qos_profile_t * qos_profile);

  // Publish a ROS message.
  rmw_ret_t publish(
    const void * ros_message,
    std::optional<z_owned_shm_provider_t> & shm_provider);

  // Publish a serialized ROS message.
  rmw_ret_t publish_serialized_message(
    const rmw_serialized_message_t * serialized_message,
    std::optional<z_owned_shm_provider_t> & shm_provider);

  // Get a copy of the keyexpr_hash of this PublisherData's liveliness::Entity.
  std::size_t keyexpr_hash() const;

  // Get a copy of the TopicInfo of this PublisherData.
  liveliness::TopicInfo topic_info() const;

  // Copy the GID of this PublisherData into an rmw_gid_t.
  void copy_gid(uint8_t out_gid[RMW_GID_STORAGE_SIZE]) const;

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
  PublisherData(
    const rmw_node_t * rmw_node,
    std::shared_ptr<liveliness::Entity> entity,
    zenoh::Publisher pub,
    std::optional<zenoh::ext::PublicationCache> pub_cache,
    zenoh::LivelinessToken token,
    const void * type_support_impl,
    std::unique_ptr<MessageTypeSupport> type_support);

  // Internal mutex.
  mutable std::mutex mutex_;
  // The parent node.
  const rmw_node_t * rmw_node_;
  // The Entity generated for the publisher.
  std::shared_ptr<liveliness::Entity> entity_;
  // An owned publisher.
  zenoh::Publisher pub_;
  // Optional publication cache when durability is transient_local.
  std::optional<zenoh::ext::PublicationCache> pub_cache_;
  // Liveliness token for the publisher.
  std::optional<zenoh::LivelinessToken> token_;
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
