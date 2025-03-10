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

#ifndef DETAIL__RMW_NODE_DATA_HPP_
#define DETAIL__RMW_NODE_DATA_HPP_

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <zenoh.hxx>

#include "graph_cache.hpp"
#include "liveliness_utils.hpp"
#include "rmw_client_data.hpp"
#include "rmw_publisher_data.hpp"
#include "rmw_subscription_data.hpp"
#include "rmw_service_data.hpp"

#include "rmw/rmw.h"

namespace rmw_zenoh_cpp
{
///=============================================================================
// The NodeData can only be created via rmw_context_impl_s::create_node_data().
class NodeData final
{
public:
  // Make a shared_ptr of NodeData. Returns nullptr if construction fails.
  static std::shared_ptr<NodeData> make(
    const rmw_node_t * const node,
    std::size_t id,
    std::shared_ptr<zenoh::Session> session,
    std::size_t domain_id,
    const std::string & namespace_,
    const std::string & node_name,
    const std::string & enclave);

  // Get the id of this node.
  std::size_t id() const;

  // Create a new PublisherData for a given rmw_publisher_t.
  bool create_pub_data(
    const rmw_publisher_t * const publisher,
    std::shared_ptr<zenoh::Session> session,
    std::size_t id,
    const std::string & topic_name,
    const rosidl_message_type_support_t * type_support,
    const rmw_qos_profile_t * qos_profile);

  // Retrieve the PublisherData for a given rmw_publisher_t if present.
  PublisherDataPtr get_pub_data(const rmw_publisher_t * const publisher);

  // Delete the PublisherData for a given rmw_publisher_t if present.
  void delete_pub_data(const rmw_publisher_t * const publisher);

  // Create a new SubscriptionData for a given rmw_subscription_t.
  bool create_sub_data(
    const rmw_subscription_t * const subscription,
    std::shared_ptr<zenoh::Session> session,
    std::shared_ptr<GraphCache> graph_cache,
    std::size_t id,
    const std::string & topic_name,
    const rosidl_message_type_support_t * type_support,
    const rmw_qos_profile_t * qos_profile,
    const rmw_subscription_options_t & sub_options);

  // Retrieve the SubscriptionData for a given rmw_subscription_t if present.
  SubscriptionDataPtr get_sub_data(const rmw_subscription_t * const subscription);

  // Delete the SubscriptionData for a given rmw_subscription_t if present.
  void delete_sub_data(const rmw_subscription_t * const subscription);

  // Create a new ServiceData for a given rmw_service_t.
  bool create_service_data(
    const rmw_service_t * const service,
    std::shared_ptr<zenoh::Session> session,
    std::size_t id,
    const std::string & service_name,
    const rosidl_service_type_support_t * type_support,
    const rmw_qos_profile_t * qos_profile);

  // Retrieve the ServiceData for a given rmw_service_t if present.
  ServiceDataPtr get_service_data(const rmw_service_t * const service);

  // Delete the ServiceData for a given rmw_service_t if present.
  void delete_service_data(const rmw_service_t * const service);

  // Create a new ClientData for a given rmw_client_t.
  bool create_client_data(
    const rmw_client_t * const client,
    std::shared_ptr<zenoh::Session> session,
    std::size_t id,
    const std::string & service_name,
    const rosidl_service_type_support_t * type_support,
    const rmw_qos_profile_t * qos_profile);

  // Retrieve the ClientData for a given rmw_client_t if present.
  ClientDataPtr get_client_data(const rmw_client_t * const client);

  // Delete the ClientData for a given rmw_client_t if present.
  void delete_client_data(const rmw_client_t * const client);

  // Shutdown this NodeData.
  rmw_ret_t shutdown();

  // Check if this NodeData is shutdown.
  bool is_shutdown() const;

  // Destructor.
  ~NodeData();

private:
  // Constructor.
  NodeData(
    const rmw_node_t * const node,
    std::size_t id,
    std::shared_ptr<liveliness::Entity> entity,
    zenoh::LivelinessToken token);
  // Internal mutex.
  mutable std::recursive_mutex mutex_;
  // The rmw_node_t associated with this NodeData.
  const rmw_node_t * node_;
  // The entity id of this node as generated by get_next_entity_id().
  // Every interface created by this node will include this id in its liveliness token.
  std::size_t id_;
  // The Entity generated for the node.
  std::shared_ptr<liveliness::Entity> entity_;
  // Liveliness token for the node.
  std::optional<zenoh::LivelinessToken> token_;
  // Shutdown flag.
  bool is_shutdown_;
  // Map of publishers.
  std::unordered_map<const rmw_publisher_t *, PublisherDataPtr> pubs_;
  // Map of subscriptions.
  std::unordered_map<const rmw_subscription_t *, SubscriptionDataPtr> subs_;
  // Map of services.
  std::unordered_map<const rmw_service_t *, ServiceDataPtr> services_;
  // Map of clients.
  std::unordered_map<const rmw_client_t *, ClientDataPtr> clients_;
};
}  // namespace rmw_zenoh_cpp

#endif  // DETAIL__RMW_NODE_DATA_HPP_
