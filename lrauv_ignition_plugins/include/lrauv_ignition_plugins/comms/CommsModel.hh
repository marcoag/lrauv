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

/* Development of this module has been funded by the Monterey Bay Aquarium
Research Institute (MBARI) and the David and Lucile Packard Foundation */

#ifndef __LRAUV_IGNITION_PLUGINS_COMMS_COMMSMODEL_HH__
#define __LRAUV_IGNITION_PLUGINS_COMMS_COMMSMODEL_HH__

#include <ignition/gazebo/EntityComponentManager.hh>

#include "CommsPacket.hh"
#include "MessageManager.hh"

namespace tethys
{
/////////////////////////////////////
/// \brief Abstract interface to define how the environment should handle
/// communication dropouts
class ICommsModel
{
  /// \brief This method is called when the message bus delivers a message.
  /// You should override this method to determine when a message is coming in.
  /// \param[in] _packet - Incoming coMessageManagermms packet.
  public: virtual void enqueue_msg(const CommsPacket &_packet);

  /// \brief This method is called when the message bus 
  public: virtual void step(
    const ignition::gazebo::UpdateInfo &_info,
    ignition::gazebo::EntityComponentManager &_ecm,
    MessageManager &_messageMgr);
  
  /// \brief
  public: virtual ~ICommsModel() = default;
};
}

#endif