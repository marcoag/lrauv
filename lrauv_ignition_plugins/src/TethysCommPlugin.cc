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

#include <chrono>

#include <ignition/plugin/Register.hh>

#include "lrauv_command.pb.h"
#include "lrauv_state.pb.h"

#include "TethysCommPlugin.hh"

using namespace tethys_comm_plugin;

TethysCommPlugin::TethysCommPlugin()
{
}

void TethysCommPlugin::Configure(
  const ignition::gazebo::Entity &_entity,
  const std::shared_ptr<const sdf::Element> &_sdf,
  ignition::gazebo::EntityComponentManager &_ecm,
  ignition::gazebo::EventManager &_eventMgr)
{
  auto model = ignition::gazebo::Model(_entity);
  ignmsg << "TethysCommPlugin::Configure" << std::endl;

  // Parse SDF parameters
  if (_sdf->HasElement("command_topic"))
  {
    this->commandTopic = _sdf->Get<std::string>("command_topic");
  }
  if (_sdf->HasElement("state_topic"))
  {
    this->stateTopic = _sdf->Get<std::string>("state_topic");
  }
  if (_sdf->HasElement("model_link"))
  {
    this->base_link = _sdf->Get<std::string>("model_link");
  }

  // Initialize transport
  if (!this->node.Subscribe(this->commandTopic,
      &TethysCommPlugin::CommandCallback, this))
  {
    ignerr << "Error subscribing to topic " << "[" << commandTopic << "]. "
      << std::endl;
    return;
  }

  this->statePub =
    this->node.Advertise<lrauv_ignition_plugins::msgs::LRAUVState>(
    this->stateTopic);
  if (!this->statePub)
  {
    ignerr << "Error advertising topic [" << stateTopic << "]"
      << std::endl;
  }

  this->elapsed = std::chrono::steady_clock::now();
}

void TethysCommPlugin::CommandCallback(
  const lrauv_ignition_plugins::msgs::LRAUVCommand &_msg)
{
  ignmsg << "Received command: " << _msg.propomega_() << std::endl;
}

void TethysCommPlugin::PreUpdate(
  const ignition::gazebo::UpdateInfo &_info,
  ignition::gazebo::EntityComponentManager &_ecm)
{
  // ignmsg << "TethysCommPlugin::PreUpdate" << std::endl;
}

void TethysCommPlugin::PostUpdate(
  const ignition::gazebo::UpdateInfo &_info,
  const ignition::gazebo::EntityComponentManager &_ecm)
{
  // ignmsg << "TethysCommPlugin::PostUpdate" << std::endl;

  if (std::chrono::steady_clock::now() - this->elapsed
      > std::chrono::seconds(1))
  {
    auto model_pose = worldPose(modelLink);

    // Publish state
    lrauv_ignition_plugins::msgs::LRAUVState stateMsg;
    //stateMsg.set_propomega_(counter);
    stateMsg.set_rph_(model_pose.Rot());
    stateMsg.set_depth_(-model_pose.Pos().Z());



    this->statePub.Publish(stateMsg);
    ignmsg << "Published state: " << stateMsg.propomega_() << std::endl;

    this->elapsed = std::chrono::steady_clock::now();
  }
}

IGNITION_ADD_PLUGIN(
  tethys_comm_plugin::TethysCommPlugin,
  ignition::gazebo::System,
  tethys_comm_plugin::TethysCommPlugin::ISystemConfigure,
  tethys_comm_plugin::TethysCommPlugin::ISystemPreUpdate,
  tethys_comm_plugin::TethysCommPlugin::ISystemPostUpdate)
