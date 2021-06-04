/*
 * Copyright (C) 2021 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef TETHYS_BUOYANCY_ENGINE_
#define TETHYS_BUOYANCY_ENGINE_

#include <ignition/gazebo/components.hh>
#include <ignition/gazebo/Link.hh>
#include <ignition/gazebo/Model.hh>
#include <ignition/gazebo/System.hh>
#include <ignition/gazebo/Util.hh>
#include <ignition/msgs.hh>
#include <ignition/plugin/Register.hh>
#include <ignition/transport/Node.hh>

#include <mutex>
#include <string>

namespace tethys
{
  class BuoyancyEnginePrivateData;

  /// \brief This class provides a simple mechanical bladder which is used to
  /// control the buoyancy of an underwater glider. It uses archimedes principal
  /// to apply an upward force based on the volume of the bladder. It listens to
  /// the topic `buoyancy_engine` or `/model/{namespace}/buoyancy_engine`
  /// topic for the volume of the bladder in *cubicentimers*.
  ///
  /// ## Parameters
  /// <link_name> - The link which the plugin is attached to [required, string]
  /// <namespace> - The namespace for the topic. If empty the plugin will listen
  ///   on `buoyancy_engine` otherwise it listens on
  ///   `/model/{namespace}/buoyancy_engine` [optional, string]
  /// <min_volume> - Minimum volume of the engine [optional, float,
  ///   default=30cc]
  /// <neutral_volume> - At this volume the engine has neutral buoyancy. Used to
  ///   estimate the weight of the engine [optional, float, default=300cc]
  /// <default_volume> - The volume which the engine starts at [optional, float,
  ///   default=300cc]
  /// <max_volume> - Maximum volume of the engine [optional, float,
  ///   default=1000cc]
  /// <max_inflation_rate> - Maximum inflation rate for bladder [optional,
  ///   float, default=3cc/s]
  ///
  /// ## Topics
  /// * Subscribes to a ignition::msgs::Double on `buoyancy_engine` or
  ///  `/model/{namespace}/buoyancy_engine`. This is the set point for the
  ///  engine.
  /// * Publishes a ignition::msgs::Double on `buoyancy_engine` or
  ///  `/model/{namespace}/buoyancy_engine` on the currents
  class BuoyancyEnginePlugin:
    public ignition::gazebo::System,
    public ignition::gazebo::ISystemConfigure,
    public ignition::gazebo::ISystemPreUpdate
  {
    public: BuoyancyEnginePlugin();

    public: ~BuoyancyEnginePlugin() override;

    /// Inherits documentation from parent class
    public: void Configure(
        const ignition::gazebo::Entity &_entity,
        const std::shared_ptr<const sdf::Element> &_sdf,
        ignition::gazebo::EntityComponentManager &_ecm,
        ignition::gazebo::EventManager &/*_eventMgr*/
    );

    /// Inherits documentation from parent class
    public: void PreUpdate(
        const ignition::gazebo::UpdateInfo &_info,
        ignition::gazebo::EntityComponentManager &_ecm);

    /// Inherits documentation from parent class
    private: std::unique_ptr<BuoyancyEnginePrivateData> dataPtr;
  };
}

#endif