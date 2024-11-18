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
#include <zenoh.h>

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

// Megabytes of SHM to reserve.
// TODO(clalancette): Make this configurable, or get it from the configuration
#define SHM_BUFFER_SIZE_MB 10

// The variable is used to identify whether the process is trying to exit or not.
// The atexit function we registered will set the flag and prevent us from closing
// Zenoh Session. Zenoh API can't be used in atexit function, because Tokio context
// is already destroyed. It will cause panic if we do so.
static bool is_exiting = false;
void update_is_exiting()
{
  is_exiting = true;
}

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
    std::optional<zenoh::Config> config = rmw_zenoh_cpp::get_z_config(
        rmw_zenoh_cpp::ConfigurableEntity::Session);

    if (!config.has_value()) {
      throw std::runtime_error("Error configuring Zenoh session.");
    }

    // // Check if shm is enabled.
    // z_owned_string_t shm_enabled;
    // zc_config_get_from_str(z_loan(config), Z_CONFIG_SHARED_MEMORY_KEY, &shm_enabled);
    // auto always_free_shm_enabled = rcpputils::make_scope_exit(
    //   [&shm_enabled]() {
    //     z_drop(z_move(shm_enabled));
    //   });

    // Initialize the zenoh session.
    zenoh::ZResult result;
    session_ = std::make_shared<zenoh::Session>(
        std::move(config.value()),
        zenoh::Session::SessionOptions::create_default(),
        &result);
    if(result != Z_OK) {
      throw std::runtime_error("Error setting up zenoh session. ");
    }

    rmw_ret_t ret;

    // TODO(Yadunund) Move this check into a separate thread.
    // Verify if the zenoh router is running if configured.
    const std::optional<uint64_t> configured_connection_attempts =
      rmw_zenoh_cpp::zenoh_router_check_attempts();
    if (configured_connection_attempts.has_value()) {
      ret = RMW_RET_ERROR;
      uint64_t connection_attempts = 0;
      // Retry until the connection is successful.
      while (ret != RMW_RET_OK && connection_attempts < configured_connection_attempts.value()) {
        zenoh::ZResult err;
        this->session_->get_routers_z_id(&err);
        if (err != Z_OK) {
          ++connection_attempts;
        } else {
          ret = RMW_RET_OK;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      if (ret != RMW_RET_OK) {
        throw std::runtime_error(
                "Unable to connect to a Zenoh router after " +
                std::to_string(configured_connection_attempts.value()) +
                " retries.");
      }
    }

    // Initialize the graph cache.
    graph_cache_ = std::make_shared<rmw_zenoh_cpp::GraphCache>(this->session_->get_zid().to_string());
    // Setup liveliness subscriptions for discovery.
    std::string liveliness_str = rmw_zenoh_cpp::liveliness::subscription_token(domain_id);

    // Query router/liveliness participants to get graph information before the session was started.
    // We create a blocking channel that is unbounded, ie. `bound` = 0, to receive
    // replies for the zc_liveliness_get() call. This is necessary as if the `bound`
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
    // z_owned_fifo_handler_reply_t handler;
    // z_owned_closure_reply_t closure;
    // z_fifo_channel_reply_new(&closure, &handler, SIZE_MAX - 1);
    // Intuitively use a FIFO channel rather than building it up with a closure and a handler to FIFO channel
    zenoh::KeyExpr keyexpr(liveliness_str);

    zenoh::Session::GetOptions options = zenoh::Session::GetOptions::create_default();
    options.target = zenoh::QueryTarget::Z_QUERY_TARGET_ALL;
    options.payload = "";

    zenoh::ZResult err;
    auto replies = session_->liveliness_get(
      keyexpr,
      zenoh::channels::FifoChannel(SIZE_MAX - 1),
      zenoh::Session::LivelinessGetOptions::create_default(),
      &err);

    if (err != Z_OK)
    {
      throw std::runtime_error("Error getting liveliness. ");
    }

    for (auto res = replies.recv(); std::holds_alternative<zenoh::Reply>(res); res = replies.recv()) {

        const zenoh::Reply &reply = std::get<zenoh::Reply>(res);
        if (reply.is_ok()) {
            const auto &sample = reply.get_ok();
            graph_cache_->parse_put(std::string(sample.get_keyexpr().as_string_view()), true);
        }
    }

    // Initialize the shm manager if shared_memory is enabled in the config.
    shm_provider_ = std::nullopt;
    // if (strncmp(
    //   z_string_data(z_loan(shm_enabled)),
    //   "true",
    //   z_string_len(z_loan(shm_enabled))) == 0)
    // {
    //   // TODO(yuyuan): determine the default alignment of SHM
    //   z_alloc_alignment_t alignment = {5};
    //   z_owned_memory_layout_t layout;
    //   z_memory_layout_new(&layout, SHM_BUFFER_SIZE_MB * 1024 * 1024, alignment);

    //   z_owned_shm_provider_t provider;
    //   if (z_posix_shm_provider_new(&provider, z_loan(layout)) != Z_OK) {
    //     RMW_ZENOH_LOG_ERROR_NAMED("rmw_zenoh_cpp", "Unable to create a SHM provider.");
    //     throw std::runtime_error("Unable to create shm provider.");
    //   }
    //   shm_provider_ = provider;
    // }
    // auto free_shm_provider = rcpputils::make_scope_exit(
    //   [this]() {
    //     if (shm_provider_.has_value()) {
    //       z_drop(z_move(shm_provider_.value()));
    //     }
    //   });

    graph_guard_condition_ = std::make_unique<rmw_guard_condition_t>();
    graph_guard_condition_->implementation_identifier = rmw_zenoh_cpp::rmw_zenoh_identifier;
    graph_guard_condition_->data = &guard_condition_data_;

    // Setup the liveliness subscriber to receives updates from the ROS graph
    // and update the graph cache.
    zc_liveliness_subscriber_options_t sub_options;
    zc_liveliness_subscriber_options_default(&sub_options);
    z_owned_closure_sample_t callback;
    z_closure(&callback, graph_sub_data_handler, nullptr, this);
    z_view_keyexpr_t liveliness_ke;
    z_view_keyexpr_from_str(&liveliness_ke, liveliness_str.c_str());
    auto undeclare_z_sub = rcpputils::make_scope_exit(
      [this]() {
        z_undeclare_subscriber(z_move(this->graph_subscriber_));
      });
    if (zc_liveliness_declare_subscriber(
        z_loan(session_->_0),
        &graph_subscriber_, z_loan(liveliness_ke),
        z_move(callback), &sub_options) != Z_OK)
    {
      RMW_SET_ERROR_MSG("unable to create zenoh subscription");
      throw std::runtime_error("Unable to subscribe to ROS graph updates.");
    }

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

      // Shutdown all the nodes in this context.
      for (auto node_it = nodes_.begin(); node_it != nodes_.end(); ++node_it) {
        ret = node_it->second->shutdown();
        if (ret != RMW_RET_OK) {
          RMW_ZENOH_LOG_ERROR_NAMED(
            "rmw_zenoh_cpp",
            "Unable to shutdown node with id %zu. rmw_ret_t code: %zu.",
            node_it->second->id(),
            ret
          );
        }
      }

      z_undeclare_subscriber(z_move(graph_subscriber_));
      if (shm_provider_.has_value()) {
        z_drop(z_move(shm_provider_.value()));
      }
      is_shutdown_ = true;

      // We specifically do *not* hold the mutex_ while tearing down the session; this allows us
      // to avoid an AB/BA deadlock if shutdown is racing with graph_sub_data_handler().
    }

    // // Close the zenoh session
    // if (z_close(z_loan_mut(session_), NULL) != Z_OK) {
    //   RMW_SET_ERROR_MSG("Error while closing zenoh session");
    //   return RMW_RET_ERROR;
    // }
    session_.reset();
    return RMW_RET_OK;
  }

  std::string enclave() const
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return enclave_;
  }

  const z_loaned_session_t * session() const
  {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return z_loan(session_->_0);
  }

  const std::shared_ptr<zenoh::Session> session_cpp() const
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
    return !z_session_is_closed(z_loan(session_->_0));
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
    if (!session_is_valid()) {
      RMW_ZENOH_LOG_ERROR_NAMED(
        "rmw_zenoh_cpp",
        "Unable to create NodeData as Zenoh session is invalid.");
      return false;
    }

    auto node_data = rmw_zenoh_cpp::NodeData::make(
      node,
      this->get_next_entity_id(),
      session_cpp(),
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
  // An owned session.
  std::shared_ptr<zenoh::Session> session_;
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
  std::string str(z_string_data(z_loan(keystr)), z_string_len(z_loan(keystr)));
  data_shared_ptr->update_graph_cache(z_sample_kind(sample), str);
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
const z_loaned_session_t * rmw_context_impl_s::session() const
{
  return data_->session();
}

///=============================================================================
const std::shared_ptr<zenoh::Session> rmw_context_impl_s::session_cpp() const
{
return data_->session_cpp();
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
