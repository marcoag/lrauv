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

#include <chrono>
#include <thread>
#include <gtest/gtest.h>

#include <ignition/gazebo/TestFixture.hh>
#include <ignition/gazebo/Util.hh>
#include <ignition/gazebo/World.hh>
#include <ignition/math/SphericalCoordinates.hh>
#include <ignition/transport/Node.hh>

#include "lrauv_init.pb.h"

#include "TestConstants.hh"

using namespace std::chrono_literals;

void ChlorophyllCb(const ignition::msgs::Float &_msg)
{
  igndbg << "got reading " << _msg.data() << std::endl;
}

//////////////////////////////////////////////////
TEST(SensorTest, Sensor)
{
  ignition::common::Console::SetVerbosity(4);

  // Setup fixture
  auto fixture = std::make_unique<ignition::gazebo::TestFixture>(
      ignition::common::joinPaths(
      std::string(PROJECT_SOURCE_PATH), "worlds", "empty_environment.sdf"));

  ignition::gazebo::Entity vehicle1{ignition::gazebo::kNullEntity};
  unsigned int iterations{0u};
  std::vector<ignition::math::Pose3d> poses1;
  std::vector<ignition::math::Vector3d> latLon1;

  fixture->OnPostUpdate(
    [&](const ignition::gazebo::UpdateInfo &_info,
    const ignition::gazebo::EntityComponentManager &_ecm)
    {
      auto worldEntity = ignition::gazebo::worldEntity(_ecm);
      ignition::gazebo::World world(worldEntity);

      vehicle1 = world.ModelByName(_ecm, "vehicle1");
      if (ignition::gazebo::kNullEntity != vehicle1)
      {
        poses1.push_back(ignition::gazebo::worldPose(vehicle1, _ecm));

        auto latLon = ignition::gazebo::sphericalCoordinates(vehicle1, _ecm);
        EXPECT_TRUE(latLon);
        latLon1.push_back(latLon.value());
      }

      iterations++;
    });
  fixture->Finalize();

  // Check that vehicles don't exist
  fixture->Server()->RunOnce();
  EXPECT_EQ(ignition::gazebo::kNullEntity, vehicle1);
  EXPECT_EQ(1, iterations);
  EXPECT_EQ(0, poses1.size());
  EXPECT_EQ(0, latLon1.size());

  // Spawn first vehicle
  ignition::transport::Node node;
  node.Subscribe("/model/vehicle1/chlorophyll", &ChlorophyllCb);
  auto spawnPub = node.Advertise<lrauv_ignition_plugins::msgs::LRAUVInit>(
    "/lrauv/init");

  int sleep{0};
  int maxSleep{30};
  for (; !spawnPub.HasConnections() && sleep < maxSleep; ++sleep)
  {
    std::this_thread::sleep_for(100ms);
  }
  ASSERT_LE(sleep, maxSleep);

  // No specific orientation, vehicle will face North
  ignition::math::Angle lat1 = IGN_DTOR(36.7999992370605);
  ignition::math::Angle lon1 = IGN_DTOR(-122.720001220703);
  {
    lrauv_ignition_plugins::msgs::LRAUVInit spawnMsg;
    spawnMsg.mutable_id_()->set_data("vehicle1");
    spawnMsg.set_initlat_(lat1.Degree());
    spawnMsg.set_initlon_(lon1.Degree());
    spawnMsg.set_acommsaddress_(201);

    spawnPub.Publish(spawnMsg);
  }

  // Check that vehicle was spawned
  int expectedIterations = iterations;
  for (sleep = 0; ignition::gazebo::kNullEntity == vehicle1 && sleep < maxSleep;
      ++sleep)
  {
    std::this_thread::sleep_for(100ms);
    // Run paused so we avoid the physics moving the vehicles
    fixture->Server()->RunOnce(true);
    expectedIterations++;
    igndbg << "Waiting for vehicle1" << std::endl;
  }
  EXPECT_LE(sleep, maxSleep);
  EXPECT_NE(ignition::gazebo::kNullEntity, vehicle1);
  EXPECT_EQ(expectedIterations, iterations);
  EXPECT_LT(0, poses1.size());
  EXPECT_LT(0, latLon1.size());

  double tightTol{1e-5};

  // Check vehicle positions in ENU
  // World origin
  EXPECT_NEAR(0.0, poses1.back().Pos().X(), tightTol);
  EXPECT_NEAR(0.0, poses1.back().Pos().Y(), tightTol);
  EXPECT_NEAR(0.0, poses1.back().Pos().Z(), tightTol);
  // Facing North (-90 rotation from default West orientation)
  EXPECT_NEAR(0.0, poses1.back().Rot().Roll(), tightTol);
  EXPECT_NEAR(0.0, poses1.back().Rot().Pitch(), tightTol);
  EXPECT_NEAR(-IGN_PI*0.5, poses1.back().Rot().Yaw(), tightTol);

  EXPECT_NEAR(lat1.Degree(), latLon1.back().X(), tightTol);
  EXPECT_NEAR(lon1.Degree(), latLon1.back().Y(), tightTol);
  EXPECT_NEAR(0.0, latLon1.back().Z(), tightTol);

  // Higher tolerance for lat/lon because of the conversions
  double latLonTol{2e-2};

  fixture->Server()->Run(true, 1000, false);

}