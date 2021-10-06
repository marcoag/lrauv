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
#include <gtest/gtest.h>

#include <ignition/common/Filesystem.hh>

#include "helper/LrauvTestFixture.hh"

//////////////////////////////////////////////////
TEST_F(LrauvTestFixture, PitchMass)
{
  // Reduce terminal output
  ignition::common::Console::SetVerbosity(3);

  ignition::common::chdir(std::string(LRAUV_APP_PATH));

  // Get initial X
  this->fixture->Server()->Run(true, 10, false);
  EXPECT_EQ(10, this->iterations);
  EXPECT_EQ(10, this->tethysPoses.size());
  EXPECT_NEAR(0.0, this->tethysPoses.back().Pos().X(), 1e-6);

  // Run non blocking
  this->fixture->Server()->Run(false, 0, false);

  // Launch mission
  std::atomic<bool> lrauvRunning{true};
  std::thread lrauvThread([&]()
  {
    LrauvTestFixture::ExecLRAUV(
        "/Missions/RegressionTests/IgnitionTests/testPitchMass.xml",
        lrauvRunning);
  });

  int maxSleep{100};
  int sleep{0};
  for (; sleep < maxSleep && lrauvRunning; ++sleep)
  {
    igndbg << "Ran [" << this->iterations << "] iterations." << std::endl;
    std::this_thread::sleep_for(1s);
  }
  EXPECT_LT(sleep, maxSleep);
  EXPECT_FALSE(lrauvRunning);

  lrauvThread.join();

  ignmsg << "Logged [" << this->tethysPoses.size() << "] poses" << std::endl;

  int maxIterations{28000};
  ASSERT_LT(maxIterations, this->tethysPoses.size());

  // Uncomment to get new expectations
  // for (int i = 2000; i <= maxIterations; i += 2000)
  // {
  //   auto pose = this->tethysPoses[i];
  //   std::cout << "this->CheckRange(" << i << ", {"
  //       << std::setprecision(2) << std::fixed
  //       << pose.Pos().X() << ", "
  //       << pose.Pos().Y() << ", "
  //       << pose.Pos().Z() << ", "
  //       << pose.Rot().Roll() << ", "
  //       << pose.Rot().Pitch() << ", "
  //       << pose.Rot().Yaw() << "}, {0.5, 0.5});"
  //       << std::endl;
  // }

  this->CheckRange(2000, {0.00, 0.00, -0.01, -0.00, -0.59, -0.00}, {0.5, 0.5});
  this->CheckRange(4000, {-0.01, -0.00, -0.01, -0.00, -0.68, -0.00}, {0.5, 0.5});
  this->CheckRange(6000, {-0.02, -0.00, -0.01, 0.00, -0.69, -0.00}, {0.5, 0.5});
  this->CheckRange(8000, {-0.02, -0.00, -0.01, 0.00, -0.71, -0.00}, {0.5, 0.5});
  this->CheckRange(10000, {-0.03, -0.00, -0.01, 0.00, -0.72, -0.00}, {0.5, 0.5});
  this->CheckRange(12000, {-0.03, -0.00, -0.02, -0.00, -0.69, -0.00}, {0.5, 0.5});
  this->CheckRange(14000, {-0.04, -0.00, -0.02, -0.00, -0.69, -0.00}, {0.5, 0.5});
  this->CheckRange(16000, {-0.04, -0.00, -0.02, 0.00, -0.70, -0.00}, {0.5, 0.5});
  this->CheckRange(18000, {-0.04, -0.00, -0.02, 0.00, -0.71, -0.00}, {0.5, 0.5});
  this->CheckRange(20000, {-0.04, -0.00, -0.02, -0.00, -0.70, -0.00}, {0.5, 0.5});
  this->CheckRange(22000, {-0.04, -0.00, -0.02, -0.00, -0.69, -0.00}, {0.5, 0.5});
  this->CheckRange(24000, {-0.05, -0.00, -0.02, 0.00, -0.70, -0.00}, {0.5, 0.5});
  this->CheckRange(26000, {-0.05, -0.00, -0.02, -0.00, -0.71, -0.00}, {0.5, 0.5});
  this->CheckRange(28000, {-0.05, -0.00, -0.02, -0.00, -0.71, -0.00}, {0.5, 0.5});
}
