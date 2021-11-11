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

/*
 * Development of this module has been funded by the Monterey Bay Aquarium
 * Research Institute (MBARI) and the David and Lucile Packard Foundation
 */

/*
* This test evaluates whether the hydrodynamic plugin successfully performs
* damping when a thrust is applied. Based on previous discussion with MBARI
* at 300rpm, we should be moving at 1ms^-1. This test checks this behaviour.
* Furthermore, by starting 4 vehicles out in different positions and
* orientations, this test makes sure that the transforms of the hydrodynamics
* plugin are correct.
*/

#include <chrono>
#include <thread>
#include <gtest/gtest.h>

#include <ignition/gazebo/TestFixture.hh>
#include <ignition/gazebo/Util.hh>
#include <ignition/gazebo/World.hh>
#include <ignition/gazebo/Model.hh>
#include <ignition/gazebo/Link.hh>
#include <ignition/math/SphericalCoordinates.hh>
#include <ignition/transport/Node.hh>

#include "lrauv_init.pb.h"
#include "TestConstants.hh"

//////////////////////////////////////////////////
void commandVehicleForward(const std::string &name)
{
  using namespace ignition;

  transport::Node node;
  auto pub =
      node.Advertise<msgs::Double>(
          "/model/" + name + "/joint/propeller_joint/cmd_pos");

  msgs::Double thrustCmd;
  thrustCmd.set_data(-6.7);

  for (int i = 0; i < 3; i++)
  {
    pub.Publish(thrustCmd);
  }
}

//////////////////////////////////////////////////
TEST(SpawnTest, Spawn)
{
  using namespace ignition;
  common::Console::SetVerbosity(4);

  // Setup fixture
  auto fixture = std::make_unique<ignition::gazebo::TestFixture>(
      ignition::common::joinPaths(
        std::string(PROJECT_SOURCE_PATH), "worlds", "star_world.sdf"));

  gazebo::Entity vehicle1{gazebo::kNullEntity};
  gazebo::Entity vehicle2{gazebo::kNullEntity};
  gazebo::Entity vehicle3{gazebo::kNullEntity};
  gazebo::Entity vehicle4{gazebo::kNullEntity};

  std::vector<math::Vector3d> velocitiesV1;
  std::vector<math::Vector3d> velocitiesV2;
  std::vector<math::Vector3d> velocitiesV3;
  std::vector<math::Vector3d> velocitiesV4;

  std::vector<math::Pose3d> posesV1;
  std::vector<math::Pose3d> posesV2;
  std::vector<math::Pose3d> posesV3;
  std::vector<math::Pose3d> posesV4;

  fixture->OnPostUpdate(
      [&](const gazebo::UpdateInfo &_info,
          const gazebo::EntityComponentManager &_ecm)
      {
        auto worldEntity = gazebo::worldEntity(_ecm);
        gazebo::World world(worldEntity);

        vehicle1 = world.ModelByName(_ecm, "tethys");
        if (gazebo::kNullEntity != vehicle1)
        {
          auto baselink = gazebo::Model(vehicle1).LinkByName(_ecm, "base_link");
          gazebo::Link link(baselink);
          auto velocity = link.WorldLinearVelocity(_ecm);
          auto pose = link.WorldPose(_ecm);
          if(!pose.has_value() || !velocity.has_value())
          {
            ignerr << "No pose/velocity\n";
            return;
          }
          velocitiesV1.push_back(velocity.value());
          posesV1.push_back(pose.value());
        }
        else
        {
          ignerr << "Model tethys not found\n";
          return;
        }

        vehicle2 = world.ModelByName(_ecm, "tethys2");
        if (gazebo::kNullEntity != vehicle2)
        {
          auto baselink = gazebo::Model(vehicle2).LinkByName(_ecm, "base_link");
          gazebo::Link link(baselink);
          auto velocity = link.WorldLinearVelocity(_ecm);
          auto pose = link.WorldPose(_ecm);
          if(!pose.has_value() || !velocity.has_value())
          {
            ignerr << "No pose/velocity\n";
          }
          velocitiesV2.push_back(velocity.value());
          posesV2.push_back(pose.value());
        }
        else
        {
          ignerr << "Model tethys2 not found\n";
          return;
        }

        vehicle3 = world.ModelByName(_ecm, "tethys3");
        if (gazebo::kNullEntity != vehicle3)
        {
          auto baselink = gazebo::Model(vehicle3).LinkByName(_ecm, "base_link");
          gazebo::Link link(baselink);
          auto velocity = link.WorldLinearVelocity(_ecm);
          auto pose = link.WorldPose(_ecm);
          if(!pose.has_value() || !velocity.has_value())
          {
            ignerr << "No pose/velocity\n";
            return;
          }
          velocitiesV3.push_back(velocity.value());
          posesV3.push_back(pose.value());
        }
        else
        {
          ignerr << "Model tethys3 not found\n";
          return;
        }

        vehicle4 = world.ModelByName(_ecm, "tethys4");
        if (ignition::gazebo::kNullEntity != vehicle4)
        {
          auto baselink = gazebo::Model(vehicle4).LinkByName(_ecm, "base_link");
          gazebo::Link link(baselink);
          auto velocity = link.WorldLinearVelocity(_ecm);
          auto pose = link.WorldPose(_ecm);
          if(!pose.has_value() || !velocity.has_value())
          {
            ignerr << "No pose/velocity\n";
            return;
          }
          velocitiesV4.push_back(velocity.value());
          posesV4.push_back(pose.value());
        }
        else
        {
          ignerr << "Model tethys4 not found\n";
          return;
        }
      });
  fixture->Finalize();

  commandVehicleForward("tethys");
  commandVehicleForward("tethys2");
  commandVehicleForward("tethys3");
  commandVehicleForward("tethys4");

  // Check that vehicles don't exist
  fixture->Server()->Run(true, 50000, false);

  ASSERT_EQ(velocitiesV1.size(), 50000);
  ASSERT_EQ(velocitiesV2.size(), 50000);
  ASSERT_EQ(velocitiesV3.size(), 50000);
  ASSERT_EQ(velocitiesV4.size(), 50000);

  ASSERT_EQ(posesV1.size(), 50000);
  ASSERT_EQ(posesV2.size(), 50000);
  ASSERT_EQ(posesV3.size(), 50000);
  ASSERT_EQ(posesV4.size(), 50000);

  // Expect all their final velocity lengths to be similar - around 1m/s as
  // specified early on.
  EXPECT_NEAR(
    velocitiesV1.rbegin()->Length(), velocitiesV2.rbegin()->Length(), 1e-3);
  EXPECT_NEAR(
    velocitiesV1.rbegin()->Length(), velocitiesV3.rbegin()->Length(), 1e-3);
  EXPECT_NEAR(
    velocitiesV1.rbegin()->Length(), velocitiesV4.rbegin()->Length(), 1e-3);
  
  // This value seems a little off. Possibly due to the sinking motion
  EXPECT_NEAR(
    velocitiesV1.rbegin()->Length(), 1.01, 1e-2);

  // Should not have a Z velocity.
  // TODO(arjo): We seem to have a very slight 0.3mm/s sinking motion
  EXPECT_NEAR(
    velocitiesV1.rbegin()->Z(), 0, 1e-3);

  // Rotations should not have changed much throuh the course of the test
  EXPECT_EQ(posesV1.rbegin()->Rot(), posesV1.begin()->Rot());
  EXPECT_EQ(posesV2.rbegin()->Rot(), posesV2.begin()->Rot());
  EXPECT_EQ(posesV3.rbegin()->Rot(), posesV3.begin()->Rot());
  EXPECT_EQ(posesV4.rbegin()->Rot(), posesV4.begin()->Rot());
  
}
