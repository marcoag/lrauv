/*
 * Copyright (C) 2022 Open Source Robotics Foundation
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

/*
 * Development of this module has been funded by the Monterey Bay Aquarium
 * Research Institute (MBARI) and the David and Lucile Packard Foundation
 */

#ifndef TETHYS_ACOUSTICCOMMSREFACTOR_ENGINE_
#define TETHYS_ACOUSTICCOMMSREFACTOR_ENGINE_

#include <memory>

#include <sdf/sdf.hh>
#include "gz/sim/comms/ICommsModel.hh"
#include <gz/sim/System.hh>
#include <gz/sim/Entity.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>

namespace tethys
{
///////////////////////////////////
/// \brief 
/// * `<address>` : The address parameter. This is a 32 bit unsigned int which
/// reflects the address which belongs to this vehicle. This will also create 
/// the topics  `/comms/external/<address>/rx` on which this plugin will publish
/// received messages and `/comms/external/<address>/tx` which this plugin 
/// subscribes to and publishes data.
/// * `<model_plugin_file>` : This is the name of library containing the 
/// environmental comms Plugin. The file should be in your `$TETHYS_COMMS_MODEL`
/// directory [Required].
/// * `<model_name>` : This is the name of the environmental communications 
/// model. [Required]
/// * `<link_name>` : The link to which the wireless transponder is attached
/// to. [Required]
/// * `<external_comms_prefix>` : Prefix on which to handle interaction between
/// the application . [Optional, Defaults to `/comms/external`]
/// * `<internal_comms_prefix>` : Prefix on which to send and listen for
/// internal packets.  [Optional, Defaults to `/comms/internal`]
/// * `<broadcast>` : This defaults to true. When enabled all receivers will
/// receive all packets. If false, then packets will be directed only to 
/// specified receivers.  [Optional, Defaults to `/comms/internal`]
/// * `<timestep>` : If defined this will allow the comms client to run at a
/// higher clock resolution than the physics engine. This is useful when dealing
/// with ranging. If the timestep is set larger than the physics engine dt then
/// the engine will default to dt. Note: for consistency it is adviced that the
/// dt is a multiple of timestep. [Optional. Seconds]
/// 
/// ```
///  <plugin
///         filename="AcousticCommsPlugin"
///         name="tethys::AcousticCommsPlugin">
///         <address>2</address>
///         <model_plugin_file>simple_acoustic_model</model_plugin_file>
///         <model_name>tethys::SimpleAcousticModel</model_name>
///         <link_name>base_link</link_name>
///         <timestep>0.00001</timestep>
///        ...
///  </plugin>
/// ```
/// All other parameters are forwarded to the specific communication model.
class AcousticCommsPluginRefactor:
  public gz::sim::comms::ICommsModel
{
  public: explicit AcousticCommsPluginRefactor();

  public: ~AcousticCommsPluginRefactor() override = default;

  // Documentation inherited.
  public: void Load(const gz::sim::Entity &_entity,
                      std::shared_ptr<const sdf::Element> _sdf,
                      gz::sim::EntityComponentManager &_ecm,
                      gz::sim::EventManager &_eventMgr) override;

  // Documentation inherited.
  public: void Step(const gz::sim::UpdateInfo &_info,
                      const gz::sim::comms::Registry &_currentRegistry,
                      gz::sim::comms::Registry &_newRegistry,
                      gz::sim::EntityComponentManager &_ecm) override;

  // Impl pointer
  private: class AcousticCommsRefactorPrivateData;
  private: std::unique_ptr<AcousticCommsRefactorPrivateData> dataPtr;
};
}

#endif
