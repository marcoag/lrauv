
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

#include <memory>
#include <sdf/sdf.hh>

#include <gz/common/Profiler.hh>
#include <gz/plugin/Register.hh>
#include "gz/sim/comms/Broker.hh"
#include "gz/sim/comms/MsgManager.hh"
#include "gz/sim/Util.hh"
#include "AcousticCommsPluginRefactor.hh"

using namespace tethys;
using namespace gz;
using namespace sim;

class AcousticCommsPluginRefactor::AcousticCommsRefactorPrivateData
{
};

AcousticCommsPluginRefactor::AcousticCommsPluginRefactor()
  : dataPtr(std::make_unique<AcousticCommsRefactorPrivateData>())
{
}

void AcousticCommsPluginRefactor::Load(
    const Entity &_entity,
    std::shared_ptr<const sdf::Element> _sdf,
    EntityComponentManager &_ecm,
    EventManager &_eventMgr)
{
}

void AcousticCommsPluginRefactor::Step(
    const UpdateInfo &_info,
    const comms::Registry &_currentRegistry,
    comms::Registry &_newRegistry,
    EntityComponentManager &_ecm)
{
}

GZ_ADD_PLUGIN(AcousticCommsPluginRefactor,
              System,
              comms::ICommsModel::ISystemConfigure,
              comms::ICommsModel::ISystemPreUpdate)
