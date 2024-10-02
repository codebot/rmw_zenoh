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

#include <zenoh.h>

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>

#include "liveliness_utils.hpp"

namespace rmw_zenoh_cpp
{
///=============================================================================
// The NodeData can only be created via rmw_context_impl_s::create_node_data().
class NodeData final
{
public:
  // Make a shared_ptr of NodeData. Returns nullptr if construction fails.
  static std::shared_ptr<NodeData> make(
    std::size_t id,
    const z_loaned_session_t * session,
    std::size_t domain_id,
    const std::string & namespace_,
    const std::string & node_name,
    const std::string & enclave);

  // Get the id of this node.
  std::size_t id() const;

  // Destructor.
  ~NodeData();

private:
  // Constructor.
  NodeData(
    std::size_t id,
    std::shared_ptr<liveliness::Entity> entity,
    zc_owned_liveliness_token_t token);
  // Internal mutex.
  mutable std::mutex mutex_;
  // The entity id of this node as generated by get_next_entity_id().
  // Every interface created by this node will include this id in its liveliness token.
  std::size_t id_;
  // The Entity generated for the node.
  std::shared_ptr<liveliness::Entity> entity_;
  // Liveliness token for the node.
  zc_owned_liveliness_token_t token_;
};
}  // namespace rmw_zenoh_cpp

#endif  // DETAIL__RMW_NODE_DATA_HPP_
