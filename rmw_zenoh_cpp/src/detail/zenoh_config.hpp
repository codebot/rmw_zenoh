// Copyright 2023 Open Source Robotics Foundation, Inc.
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

#ifndef DETAIL__ZENOH_CONFIG_HPP_
#define DETAIL__ZENOH_CONFIG_HPP_

#include <zenoh.h>

#include <unordered_map>
#include <utility>

#include "rmw/ret_types.h"

///==============================================================================
enum class ConfigurableEntity : uint8_t
{
  Invalid = 0,
  Session,
  Router
};

///==============================================================================
/// Map the configurable entity to a pair of environment variable name that
/// stores the absolute path to the Zenoh config and the default config filename.
/// Note: The default config file should be located within rmw_zenoh_cpp/config/.
static const std::unordered_map<ConfigurableEntity,
  std::pair<const char *, const char *>> envar_map = {
  {ConfigurableEntity::Session,
    {"ZENOH_SESSION_CONFIG_URI", "DEFAULT_RMW_ZENOH_SESSION_CONFIG.json5"}},
  {ConfigurableEntity::Router, {"ZENOH_ROUTER_CONFIG_URI", "DEFAULT_RMW_ZENOH_ROUTER_CONFIG.json5"}}
};

/// Get the zenoh configuration for a configurable entity.
/// @details The behavior is as follows:
///   - If the environment variable for the entity is set, the z_owned_config_t
///     is configured as per the configuration file retrieved.
///   - If the environment variable is not set, the z_owned_config_t
///     is configured using the rmw_zenoh default configuration file.
/// @param entity The zenoh entity to be configured.
/// @param config The zenoh configuration to be filled.
/// @returns `RMW_RET_OK` if the configuration was successfully loaded.
[[nodiscard]]
rmw_ret_t get_z_config(const ConfigurableEntity & entity, z_owned_config_t * config);

#endif  // DETAIL__ZENOH_CONFIG_HPP_
