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

#include "rmw_context_impl_s.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include "graph_cache.hpp"
#include "guard_condition.hpp"
#include "identifier.hpp"
#include "logging_macros.hpp"
#include "rmw_node_data.hpp"
#include "zenoh_config.hpp"
#include "zenoh_router_check.hpp"

#include "rcpputils/scope_exit.hpp"
#include "rmw/error_handling.h"
#include "zenoh_utils.hpp"

// Megabytes of SHM to reserve.
// TODO(clalancette): Make this configurable, or get it from the configuration
#define SHM_BUFFER_SIZE_MB 10

// This global mapping of raw Data pointers to Data shared pointers allows graph_sub_data_handler()
// to lookup the pointer, and gain a reference to a shared_ptr if it exists.
// This guarantees that the Data object will not be destroyed while we are using it.
static std::mutex data_to_data_shared_ptr_map_mutex;
static std::unordered_map<rmw_context_impl_s::Data *,
  std::shared_ptr<rmw_context_impl_s::Data>> data_to_data_shared_ptr_map;

static void graph_sub_data_handler(z_loaned_sample_t * sample, void * data);

// Bundle all class members into a data struct which can be passed as a
// weak ptr to various threads for thread-safe access without capturing
// "this" ptr by reference.
class rmw_context_impl_s::Data final
{
public:
  // Constructor.
  Data(
    std::size_t domain_id,
    const std::string & enclave)
  : domain_id_(std::move(domain_id)),
    enclave_(std::move(enclave)),
    is_shutdown_(false),
    next_entity_id_(0),
    nodes_({})
  {
    // Initialize the zenoh configuration.
    z_owned_config_t config;
    rmw_ret_t ret;
    if ((ret =
      rmw_zenoh_cpp::get_z_config(
        rmw_zenoh_cpp::ConfigurableEntity::Session,
        &config)) != RMW_RET_OK)
    {
      throw std::runtime_error("Error configuring Zenoh session.");
    }

#ifndef _MSC_VER
    // Check if shm is enabled.
    z_owned_string_t shm_enabled;
    zc_config_get_from_str(z_loan(config), Z_CONFIG_SHARED_MEMORY_KEY, &shm_enabled);
    auto always_free_shm_enabled = rcpputils::make_scope_exit(
      [&shm_enabled]() {
        z_drop(z_move(shm_enabled));
      });
#endif

    // Initialize the zenoh session.
    z_owned_session_t raw_session;
    if (z_open(&raw_session, z_move(config), NULL) != Z_OK) {
      RMW_SET_ERROR_MSG("Error setting up zenoh session.");
      throw std::runtime_error("Error setting up zenoh session.");
    }
    if (session_ != nullptr) {
      session_.reset();
    }
    session_ = std::make_shared<rmw_zenoh_cpp::ZenohSession>(raw_session);
    auto close_session = rcpputils::make_scope_exit(
      [&raw_session]() {
        z_close(z_loan_mut(raw_session), NULL);
      });

    // Verify if the zenoh router is running if configured.
    const std::optional<uint64_t> configured_connection_attempts =
      rmw_zenoh_cpp::zenoh_router_check_attempts();
    if (configured_connection_attempts.has_value()) {
      uint64_t connection_attempts = 0;
      constexpr std::chrono::milliseconds sleep_time(1000);
      constexpr int64_t ticks_between_print(std::chrono::milliseconds(1000) / sleep_time);
      while ((ret = rmw_zenoh_cpp::zenoh_router_check(session_->loan())) != RMW_RET_OK) {
        if ((connection_attempts % ticks_between_print) == 0) {
          RMW_ZENOH_LOG_WARN_NAMED(
            "rmw_zenoh_cpp",
            "Unable to connect to a Zenoh router. "
            "Have you started a router with `ros2 run rmw_zenoh_cpp rmw_zenohd`?");
        }
        if (++connection_attempts >= configured_connection_attempts.value()) {
          break;
        }
        std::this_thread::sleep_for(sleep_time);
      }
    }

    // Initialize the graph cache.
    const z_id_t zid = z_info_zid(session_->loan());
    graph_cache_ = std::make_shared<rmw_zenoh_cpp::GraphCache>(zid);
    // Setup liveliness subscriptions for discovery.
    std::string liveliness_str = rmw_zenoh_cpp::liveliness::subscription_token(domain_id);

    // Query router/liveliness participants to get graph information before the session was started.
    // We create a blocking channel that is unbounded, ie. `bound` = 0, to receive
    // replies for the z_liveliness_get() call. This is necessary as if the `bound`
    // is too low, the channel may starve the zenoh executor of its threads which
    // would lead to deadlocks when trying to receive replies and block the
    // execution here.
    // The blocking channel will return when the sender end is closed which is
    // the moment the query finishes.
    // The non-blocking fifo exists only for the use case where we don't want to
    // block the thread between responses (including the request termination response).
    // In general, unless we want to cooperatively schedule other tasks on the same
    // thread as reading the fifo, the blocking fifo will be more appropriate as
    // the code will be simpler, and if we're just going to spin over the non-blocking
    // reads until we obtain responses, we'll just be hogging CPU time by convincing
    // the OS that we're doing actual work when it could instead park the thread.
    z_owned_fifo_handler_reply_t handler;
    z_owned_closure_reply_t closure;
    z_fifo_channel_reply_new(&closure, &handler, SIZE_MAX - 1);

    z_view_keyexpr_t keyexpr;
    z_view_keyexpr_from_str(&keyexpr, liveliness_str.c_str());
    z_liveliness_get(
      session_->loan(), z_loan(keyexpr),
      z_move(closure), NULL);
    z_owned_reply_t reply;
    while (z_recv(z_loan(handler), &reply) == Z_OK) {
      if (z_reply_is_ok(z_loan(reply))) {
        const z_loaned_sample_t * sample = z_reply_ok(z_loan(reply));
        z_view_string_t keystr;
        z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
        std::string liveliness_str(z_string_data(z_loan(keystr)), z_string_len(z_loan(keystr)));
        // Ignore tokens from the same session to avoid race conditions from this
        // query and the liveliness subscription.
        graph_cache_->parse_put(std::move(liveliness_str), true);
      } else {
        RMW_ZENOH_LOG_DEBUG_NAMED(
          "rmw_zenoh_cpp", "[rmw_context_impl_s] z_call received an invalid reply.\n");
      }
      z_drop(z_move(reply));
    }
    z_drop(z_move(handler));

    // Initialize the shm manager if shared_memory is enabled in the config.
    shm_provider_ = std::nullopt;
#ifndef _MSC_VER
    if (strncmp(
        z_string_data(z_loan(shm_enabled)),
        "true",
        z_string_len(z_loan(shm_enabled))) == 0)
    {
      // TODO(yuyuan): determine the default alignment of SHM
      z_alloc_alignment_t alignment = {5};
      z_owned_memory_layout_t layout;
      z_memory_layout_new(&layout, SHM_BUFFER_SIZE_MB * 1024 * 1024, alignment);

      z_owned_shm_provider_t provider;
      if (z_posix_shm_provider_new(&provider, z_loan(layout)) != Z_OK) {
        RMW_ZENOH_LOG_ERROR_NAMED("rmw_zenoh_cpp", "Unable to create an SHM provider.");
        throw std::runtime_error("Unable to create an SHM provider.");
      }
      shm_provider_ = provider;
    }
    auto free_shm_provider = rcpputils::make_scope_exit(
      [this]() {
        if (shm_provider_.has_value()) {
          z_drop(z_move(shm_provider_.value()));
        }
      });
#endif
    graph_guard_condition_ = std::make_unique<rmw_guard_condition_t>();
    graph_guard_condition_->implementation_identifier = rmw_zenoh_cpp::rmw_zenoh_identifier;
    graph_guard_condition_->data = &guard_condition_data_;

    // Setup the liveliness subscriber to receives updates from the ROS graph
    // and update the graph cache.
    z_liveliness_subscriber_options_t sub_options;
    z_liveliness_subscriber_options_default(&sub_options);
    z_owned_closure_sample_t callback;
    z_closure(&callback, graph_sub_data_handler, nullptr, this);
    z_view_keyexpr_t liveliness_ke;
    z_view_keyexpr_from_str(&liveliness_ke, liveliness_str.c_str());
    if (z_liveliness_declare_subscriber(
        session_->loan(),
        &graph_subscriber_, z_loan(liveliness_ke),
        z_move(callback), &sub_options) != Z_OK)
    {
      RMW_SET_ERROR_MSG("unable to create zenoh subscription");
      throw std::runtime_error("Unable to subscribe to ROS graph updates.");
    }
    auto undeclare_z_sub = rcpputils::make_scope_exit(
      [this]() {
        z_undeclare_subscriber(z_move(this->graph_subscriber_));
      });

    close_session.cancel();
#ifndef _MSC_VER
    free_shm_provider.cancel();
#endif
    undeclare_z_sub.cancel();
  }

  // Shutdown the Zenoh session.
  rmw_ret_t shutdown()
  {
    {
      std::lock_guard<std::recursive_mutex> lock(mutex_);
      rmw_ret_t ret = RMW_RET_OK;
      if (is_shutdown_) {
        return ret;
      }

      z_undeclare_subscriber(z_move(graph_subscriber_));
      if (shm_provider_.has_value()) {
        z_drop(z_move(shm_provider_.value()));
      }
      is_shutdown_ = true;

      // We specifically do *not* hold the mutex_ while tearing down the session; this allows us
      // to avoid an AB/BA deadlock if shutdown is racing with graph_sub_data_handler().
    }

    // Drop the shared session.
    session_.reset();

    return RMW_RET_OK;
  }

  std::string enclave() const
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return enclave_;
  }

  std::shared_ptr<rmw_zenoh_cpp::ZenohSession> session() const
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return session_;
  }

  std::optional<z_owned_shm_provider_t> & shm_provider()
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return shm_provider_;
  }

  rmw_guard_condition_t * graph_guard_condition()
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return graph_guard_condition_.get();
  }

  std::size_t get_next_entity_id()
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return next_entity_id_++;
  }

  bool is_shutdown() const
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return is_shutdown_;
  }

  bool session_is_valid() const
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return !z_session_is_closed(session_->loan());
  }

  std::shared_ptr<rmw_zenoh_cpp::GraphCache> graph_cache()
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return graph_cache_;
  }

  bool create_node_data(
    const rmw_node_t * const node,
    const std::string & ns,
    const std::string & node_name)
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (nodes_.count(node) > 0) {
      // Node already exists.
      return false;
    }

    // Check that the Zenoh session is still valid.
    if (z_session_is_closed(session_->loan())) {
      RMW_ZENOH_LOG_ERROR_NAMED(
        "rmw_zenoh_cpp",
        "Unable to create NodeData as Zenoh session is invalid.");
      return false;
    }

    auto node_data = rmw_zenoh_cpp::NodeData::make(
      node,
      this->get_next_entity_id(),
      session_->loan(),
      domain_id_,
      ns,
      node_name,
      enclave_);
    if (node_data == nullptr) {
      // Error already handled.
      return false;
    }

    auto node_insertion = nodes_.insert(std::make_pair(node, std::move(node_data)));
    if (!node_insertion.second) {
      return false;
    }

    return true;
  }

  std::shared_ptr<rmw_zenoh_cpp::NodeData> get_node_data(const rmw_node_t * const node)
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto node_it = nodes_.find(node);
    if (node_it == nodes_.end()) {
      return nullptr;
    }
    return node_it->second;
  }

  void delete_node_data(const rmw_node_t * const node)
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    nodes_.erase(node);
  }

  void update_graph_cache(z_sample_kind_t sample_kind, const std::string & keystr)
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (is_shutdown_) {
      return;
    }
    switch (sample_kind) {
      case z_sample_kind_t::Z_SAMPLE_KIND_PUT:
        graph_cache_->parse_put(keystr);
        break;
      case z_sample_kind_t::Z_SAMPLE_KIND_DELETE:
        graph_cache_->parse_del(keystr);
        break;
      default:
        return;
    }

    // Trigger the ROS graph guard condition.
    rmw_ret_t rmw_ret = rmw_trigger_guard_condition(graph_guard_condition_.get());
    if (RMW_RET_OK != rmw_ret) {
      RMW_ZENOH_LOG_WARN_NAMED(
        "rmw_zenoh_cpp",
        "[graph_sub_data_handler] Unable to trigger graph guard condition."
      );
    }
  }

  // Destructor.
  ~Data()
  {
    auto ret = this->shutdown();
    nodes_.clear();
    static_cast<void>(ret);
  }

private:
  // Mutex to lock when accessing members.
  mutable std::recursive_mutex mutex_;
  // The ROS domain id of this context.
  std::size_t domain_id_;
  // Enclave, name used to find security artifacts in a sros2 keystore.
  std::string enclave_;
  // A shared session.
  std::shared_ptr<rmw_zenoh_cpp::ZenohSession> session_{nullptr};
  // An optional SHM manager that is initialized of SHM is enabled in the
  // zenoh session config.
  std::optional<z_owned_shm_provider_t> shm_provider_;
  // Graph cache.
  std::shared_ptr<rmw_zenoh_cpp::GraphCache> graph_cache_;
  // ROS graph liveliness subscriber.
  z_owned_subscriber_t graph_subscriber_;
  // Equivalent to rmw_dds_common::Context's guard condition.
  // Guard condition that should be triggered when the graph changes.
  std::unique_ptr<rmw_guard_condition_t> graph_guard_condition_;
  // The GuardCondition data structure.
  rmw_zenoh_cpp::GuardCondition guard_condition_data_;
  // Shutdown flag.
  bool is_shutdown_;
  // A counter to assign a local id for every entity created in this session.
  std::size_t next_entity_id_;
  // Nodes created from this context.
  std::unordered_map<const rmw_node_t *, std::shared_ptr<rmw_zenoh_cpp::NodeData>> nodes_;
};

///=============================================================================
static void graph_sub_data_handler(z_loaned_sample_t * sample, void * data)
{
  z_view_string_t keystr;
  z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);

  auto data_ptr = static_cast<rmw_context_impl_s::Data *>(data);
  if (data_ptr == nullptr) {
    RMW_ZENOH_LOG_ERROR_NAMED(
      "rmw_zenoh_cpp",
      "[graph_sub_data_handler] Invalid data_ptr."
    );
    return;
  }

  // Look up the data shared_ptr in the global map.  If it is in there, use it.
  // If not, it is being shutdown so we can just ignore this update.
  std::shared_ptr<rmw_context_impl_s::Data> data_shared_ptr{nullptr};
  {
    std::lock_guard<std::mutex> lk(data_to_data_shared_ptr_map_mutex);
    if (data_to_data_shared_ptr_map.count(data_ptr) == 0) {
      return;
    }
    data_shared_ptr = data_to_data_shared_ptr_map[data_ptr];
  }

  // Update the graph cache.
  std::string liveliness_str(z_string_data(z_loan(keystr)), z_string_len(z_loan(keystr)));
  data_shared_ptr->update_graph_cache(z_sample_kind(sample), std::move(liveliness_str));
}

///=============================================================================
rmw_context_impl_s::rmw_context_impl_s(
  const std::size_t domain_id,
  const std::string & enclave)
{
  data_ = std::make_shared<Data>(domain_id, std::move(enclave));

  std::lock_guard<std::mutex> lk(data_to_data_shared_ptr_map_mutex);
  data_to_data_shared_ptr_map.emplace(data_.get(), data_);
}

///=============================================================================
rmw_context_impl_s::~rmw_context_impl_s()
{
  this->shutdown();
}

///=============================================================================
std::string rmw_context_impl_s::enclave() const
{
  return data_->enclave();
}

///=============================================================================
std::shared_ptr<rmw_zenoh_cpp::ZenohSession> rmw_context_impl_s::session() const
{
  return data_->session();
}

///=============================================================================
std::optional<z_owned_shm_provider_t> & rmw_context_impl_s::shm_provider()
{
  return data_->shm_provider();
}

///=============================================================================
rmw_guard_condition_t * rmw_context_impl_s::graph_guard_condition()
{
  return data_->graph_guard_condition();
}

///=============================================================================
std::size_t rmw_context_impl_s::get_next_entity_id()
{
  return data_->get_next_entity_id();
}

///=============================================================================
rmw_ret_t rmw_context_impl_s::shutdown()
{
  {
    std::lock_guard<std::mutex> lk(data_to_data_shared_ptr_map_mutex);
    data_to_data_shared_ptr_map.erase(data_.get());
  }

  return data_->shutdown();
}

///=============================================================================
bool rmw_context_impl_s::is_shutdown() const
{
  return data_->is_shutdown();
}

///=============================================================================
bool rmw_context_impl_s::session_is_valid() const
{
  return data_->session_is_valid();
}

///=============================================================================
std::shared_ptr<rmw_zenoh_cpp::GraphCache> rmw_context_impl_s::graph_cache()
{
  return data_->graph_cache();
}

///=============================================================================
bool rmw_context_impl_s::create_node_data(
  const rmw_node_t * const node,
  const std::string & ns,
  const std::string & node_name)
{
  return data_->create_node_data(node, ns, node_name);
}

///=============================================================================
std::shared_ptr<rmw_zenoh_cpp::NodeData> rmw_context_impl_s::get_node_data(
  const rmw_node_t * const node)
{
  return data_->get_node_data(node);
}

///=============================================================================
void rmw_context_impl_s::delete_node_data(const rmw_node_t * const node)
{
  data_->delete_node_data(node);
}
