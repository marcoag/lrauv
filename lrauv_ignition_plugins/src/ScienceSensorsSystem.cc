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

#include <ignition/msgs/pointcloud_packed.pb.h>

#include <ignition/plugin/Register.hh>
#include <ignition/common/SystemPaths.hh>
#include <ignition/math/SphericalCoordinates.hh>
#include <ignition/msgs/Utility.hh>
#include <ignition/transport/Node.hh>

#include <pcl/conversions.h>
#include <pcl/point_cloud.h>
#include <pcl/PCLPointCloud2.h>
#include <pcl/octree/octree_search.h>

#include "ScienceSensorsSystem.hh"

using namespace tethys;

class tethys::ScienceSensorsSystemPrivate
{
  /// \brief Shift point cloud with respect to the world origin in spherical
  /// coordinates, if available.
  public: void GetWorldOriginSphericalCoords();

  /// \brief Reads csv file and populate various data fields
  public: void ReadData();

  /// \brief Generate octree from spatial data, for searching
  public: void GenerateOctrees();

  /// \brief Publish the latest point cloud
  public: void PublishData();

  /// \brief Service callback for a point cloud with the latest science data.
  /// \param[in] _res Point cloud to return
  /// \return True
  public: bool ScienceDataService(ignition::msgs::PointCloudPacked &_res);

  /// \brief Returns a point cloud message populated with the latest sensor data
  public: ignition::msgs::PointCloudPacked PointCloudMsg();

  /// \brief Interpolate floating point data based on distance
  /// \param[in] _arr Array of data from which to find elements to interpolate
  /// \param[in] _spatialIdx Indices in _arr
  /// \param[in] _spatialSqrDist Distances of elements in _arr
  /// \return Interpolated value, or quiet NaN if inputs invalid.
  public: float InterpolateData(
    std::vector<float> _arr,
    std::vector<int> &_inds,
    std::vector<float> &_dists);

  /// \brief csv field name for timestamp of data
  public: const std::string TIME {"elapsed_time_second"};

  /// \brief csv field name for latitude
  public: const std::string LATITUDE {"latitude_degree"};

  /// \brief csv field name for longitude
  public: const std::string LONGITUDE {"longitude_degree"};

  /// \brief csv field name for depth
  public: const std::string DEPTH {"depth_meter"};

  /// \brief csv field name for temperature
  public: const std::string TEMPERATURE {"sea_water_temperature_degC"};

  /// \brief csv field name for salinity
  public: const std::string SALINITY {"sea_water_salinity_psu"};

  /// \brief csv field name for chlorophyll
  public: const std::string CHLOROPHYLL {
    "mass_concentration_of_chlorophyll_in_sea_water_ugram_per_liter"};

  /// \brief csv field name for ocean current velocity eastward
  public: const std::string EAST_CURRENT {
    "eastward_sea_water_velocity_meter_per_sec"};

  /// \brief csv field name for ocean current velocity northward
  public: const std::string NORTH_CURRENT {
    "northward_sea_water_velocity_meter_per_sec"};

  /// \brief Service name for getting the spherical coordinates associated with
  /// the world origin
  public: std::string getWorldOriginSphericalService
    {"/world_origin_spherical"};

  /// \brief Input data file name, relative to a path Ignition can find in its
  /// environment variables.
  public: std::string dataPath {"2003080103_mb_l3_las.csv"};

  /// \brief Indicates whether data has been loaded
  public: bool initialized = false;

  /// World origin in spherical coordinates (latitude, longitude, elevation)
  public: ignition::math::Vector3d worldOriginSphericalCoords = {0, 0, 0};

  /// World origin in Cartesian coordinates converted from spherical coordinates
  public: ignition::math::Vector3d worldOriginEarthCartesianCoords = {0, 0, 0};

  /// \brief Whether using more than one time slices of data
  public: bool multipleTimeSlices = false;

  /// \brief Index of the latest time slice
  public: int timeIdx = 0;

  /// \brief Timestamps to index slices of data
  public: std::vector<float> timestamps;

  /// \brief Surface type, for converting spherical coordinates to Cartesian
  public: ignition::math::SphericalCoordinates::SurfaceType surfaceType =
    ignition::math::SphericalCoordinates::EARTH_WGS84;

  /// \brief Spatial coordinates of data
  /// Vector size: number of time slices. Indices correspond to those of
  /// this->timestamps.
  /// Point cloud: spatial coordinates to index science data by location
  public: std::vector<pcl::PointCloud<pcl::PointXYZ>::Ptr> timeSpaceCoords;

  /// \brief Science data.
  /// Outer vector size: number of time slices. Indices correspond to those of
  /// this->timestamps.
  /// Inner vector: indices correspond to those of timeSpaceCoords.
  public: std::vector<std::vector<float>> temperatureArr;

  /// \brief Science data. Same size as temperatureArr.
  public: std::vector<std::vector<float>> salinityArr;

  /// \brief Science data. Same size as temperatureArr.
  public: std::vector<std::vector<float>> chlorophyllArr;

  /// \brief Science data. Same size as temperatureArr.
  public: std::vector<std::vector<float>> eastCurrentArr;

  /// \brief Science data. Same size as temperatureArr.
  public: std::vector<std::vector<float>> northCurrentArr;

  /// \brief Resolution of spatial coordinates in meters in octree, for data
  /// search.
  public: float spatialRes = 0.1f;

  /// \brief Octree for data search based on spatial location of sensor. One
  /// octree per point cloud in timeSpaceCoords.
  public: std::vector<pcl::octree::OctreePointCloudSearch<pcl::PointXYZ>>
    spatialOctrees;

  /// \brief Transport node
  public: ignition::transport::Node node;

  /// \brief Publisher for point clouds representing positions for science data
  public: ignition::transport::Node::Publisher cloudPub;

  // TODO TEMPORARY HACK publish float arrays instead of point cloud fields
  public: ignition::transport::Node::Publisher tempPub;
  public: ignition::transport::Node::Publisher chlorPub;
  public: ignition::transport::Node::Publisher salPub;
  public: ignition::msgs::Float_V tempMsg;
  public: ignition::msgs::Float_V chlorMsg;
  public: ignition::msgs::Float_V salMsg;

  // TODO TEMPORARY HACK
  /// \brief Publish a few more times for visualization plugin to get them
  public: int repeatPubTimes = 1;

  // TODO TEMPORARY HACK scale down to see in view to skip orbit tool limits
  // For 2003080103_mb_l3_las_1x1km.csv
  //public: const float MINIATURE_SCALE = 0.01;
  // For 2003080103_mb_l3_las.csv
  public: const float MINIATURE_SCALE = 0.0001;

  // TODO TEMPORARY HACK Skip depths below this z, so have memory to
  // visualize higher layers at higher resolution.
  // This is only for visualization, so that MAX_PTS_VIS can calculate close
  // to the actual number of points visualized.
  // Sensors shouldn't use this.
  public: const float SKIP_Z_BELOW = -20;
};

/////////////////////////////////////////////////
/// \brief Helper function to create a sensor according to its type
/// \tparam SensorType A sensor type
/// \param[in] _system Pointer to the science sensors system
/// \param[in] _ecm Mutable reference to the ECM
/// \param[in] _entity Sensor entity
/// \param[in] _custom Custom sensor component
/// \param[in] _parent Parent entity component
template<typename SensorType>
void createSensor(ScienceSensorsSystem *_system,
    ignition::gazebo::EntityComponentManager &_ecm,
    const ignition::gazebo::Entity &_entity,
    const ignition::gazebo::components::CustomSensor *_custom,
    const ignition::gazebo::components::ParentEntity *_parent)
{
  auto type = ignition::sensors::customType(_custom->Data());
  if (SensorType::kTypeStr != type)
  {
    return;
  }
  // Get sensor's scoped name without the world
  auto sensorScopedName = ignition::gazebo::removeParentScope(
      ignition::gazebo::scopedName(_entity, _ecm, "::", false), "::");
  sdf::Sensor data = _custom->Data();
  data.SetName(sensorScopedName);

  // Default to scoped name as topic
  if (data.Topic().empty())
  {
    std::string topic = scopedName(_entity, _ecm) + "/" + SensorType::kTypeStr;
    data.SetTopic(topic);
  }

  ignition::sensors::SensorFactory sensorFactory;
  auto sensor = sensorFactory.CreateSensor<SensorType>(data);
  if (nullptr == sensor)
  {
    ignerr << "Failed to create sensor [" << sensorScopedName << "]"
           << std::endl;
    return;
  }

  // Set sensor parent
  auto parentName = _ecm.Component<ignition::gazebo::components::Name>(
      _parent->Data())->Data();
  sensor->SetParent(parentName);

  // Set topic on Gazebo
  _ecm.CreateComponent(_entity,
      ignition::gazebo::components::SensorTopic(sensor->Topic()));

  // Keep track of this sensor
  _system->entitySensorMap.insert(std::make_pair(_entity,
      std::move(sensor)));

  igndbg << "Created sensor [" << sensorScopedName << "]"
         << std::endl;
}

/////////////////////////////////////////////////
ScienceSensorsSystem::ScienceSensorsSystem()
  : dataPtr(std::make_unique<ScienceSensorsSystemPrivate>())
{
}

/////////////////////////////////////////////////
void ScienceSensorsSystemPrivate::ReadData()
{
  std::fstream fs;
  fs.open(this->dataPath, std::ios::in);

  std::vector<std::string> fieldnames;
  std::string line, word, temp;

  // Read field names in first line
  std::getline(fs, line);

  std::stringstream ss(line);

  // Spherical coordinates object for Cartesian conversions
  ignition::math::SphericalCoordinates sc(this->surfaceType);

  // Tokenize header line into columns
  while (std::getline(ss, word, ','))
  {
    fieldnames.push_back(word);
  }

  // Read file line by line
  while (std::getline(fs, line))
  {
    std::stringstream ss(line);

    int i = 0;

    // Index of the timestamp in this line of data. Init to invalid index
    int lineTimeIdx = -1;

    // Spatial coordinates of this line of data. Init to NaN before populating
    float latitude = std::numeric_limits<float>::quiet_NaN();
    float longitude = std::numeric_limits<float>::quiet_NaN();
    float depth = std::numeric_limits<float>::quiet_NaN();

    // Science data. Init to NaN before knowing whether timestamp is valid, so
    // that we do not assume timestamp column precedes data columns in each line
    // in the file.
    float temp = std::numeric_limits<float>::quiet_NaN();
    float sal = std::numeric_limits<float>::quiet_NaN();
    float chlor = std::numeric_limits<float>::quiet_NaN();
    float nCurr = std::numeric_limits<float>::quiet_NaN();
    float eCurr = std::numeric_limits<float>::quiet_NaN();

    // Tokenize the line into columns
    while (std::getline(ss, word, ','))
    {
      float val = 0.0f;
      try
      {
        // stof handles NaNs and Infs
        val = stof(word);
      }
      catch (const std::invalid_argument &ia)
      {
        ignerr << "Line [" << line << "] contains invalid word. Skipping. "
               << ia.what() << std::endl;
        continue;
      }
      catch (const std::out_of_range &oor)
      {
        ignerr << "Line [" << line << "] contains invalid word. Skipping. "
               << oor.what() << std::endl;
        continue;
      }

      // Time index
      if (fieldnames[i] == TIME)
      {
        // Does not account for floating point error. Assumes time specified in
        // csv file is same accuracy for each line.
        std::vector<float>::iterator it =
          std::find(this->timestamps.begin(), this->timestamps.end(), val);
        // If the timestamp is new
        if (it == this->timestamps.end())
        {
          // Insert new timestamp into 1D array
          this->timestamps.push_back(val);

          // Create a new time slice of data
          auto newCloud = pcl::PointCloud<pcl::PointXYZ>::Ptr(
              new pcl::PointCloud<pcl::PointXYZ>);
          this->timeSpaceCoords.push_back(newCloud->makeShared());
          // Is this valid memory management?
          this->temperatureArr.push_back(std::vector<float>());
          this->salinityArr.push_back(std::vector<float>());
          this->chlorophyllArr.push_back(std::vector<float>());
          this->eastCurrentArr.push_back(std::vector<float>());
          this->northCurrentArr.push_back(std::vector<float>());

          lineTimeIdx = this->timestamps.size() - 1;
        }
        // If the timestamp exists, find the index of its corresponding time
        // slice
        else
        {
          lineTimeIdx = it - this->timestamps.begin();
        }
      }
      // Spatial index: latitude
      else if (fieldnames[i] == LATITUDE)
      {
        latitude = val;
      }
      // Spatial index: longitude
      else if (fieldnames[i] == LONGITUDE)
      {
        longitude = val;
      }
      // Spatial index: depth
      else if (fieldnames[i] == DEPTH)
      {
        depth = val;
      }
      // Science data
      else if (fieldnames[i] == TEMPERATURE)
      {
        temp = val;
      }
      else if (fieldnames[i] == SALINITY)
      {
        sal = val;
      }
      else if (fieldnames[i] == CHLOROPHYLL)
      {
        chlor = val;
      }
      else if (fieldnames[i] == EAST_CURRENT)
      {
        eCurr = val;
      }
      else if (fieldnames[i] == NORTH_CURRENT)
      {
        nCurr = val;
      }
      else
      {
        ignerr << "Unrecognized science data field name [" << fieldnames[i]
               << "]. Skipping column." << std::endl;
      }

      i += 1;
    }

    // Check validity of timestamp
    // If no timestamp was provided for this line, cannot index the datum.
    if (lineTimeIdx == -1)
    {
      ignerr << "Line [" << line << "] timestamp invalid. Skipping."
             << std::endl;
      continue;
    }
    else
    {
      // Check validity of spatial coordinates
      if (!std::isnan(latitude) && !std::isnan(longitude) && !std::isnan(depth))
      {
        // TODO TEMPORARY HACK skip points below a certain depth
        if (-depth < this->SKIP_Z_BELOW)
        {
          continue;
        }

        // Convert spherical coordinates to Cartesian
        ignition::math::Vector3d cart = sc.LocalFromSphericalPosition(
          {latitude, longitude, 0});

        // Shift to be relative to world origin spherical coordinates
        cart -= this->worldOriginEarthCartesianCoords;

        // TODO TEMPORARY HACK scale down to see in view
        cart *= this->MINIATURE_SCALE;

        // TODO TEMPORARY HACK skip points beyond some distance from origin
        if (abs(cart.X()) > 1000 || abs(cart.Y()) > 1000)
        {
          continue;
        }

        // Gather spatial coordinates, 3 fields in the line, into point cloud
        // for indexing this time slice of data.
        // Flip sign of z, because positive depth is negative z.
        this->timeSpaceCoords[lineTimeIdx]->push_back(
          pcl::PointXYZ(cart.X(), cart.Y(), -depth));

        // Populate science data
        this->temperatureArr[lineTimeIdx].push_back(temp);
        this->salinityArr[lineTimeIdx].push_back(sal);
        this->chlorophyllArr[lineTimeIdx].push_back(chlor);
        this->eastCurrentArr[lineTimeIdx].push_back(eCurr);
        this->northCurrentArr[lineTimeIdx].push_back(nCurr);
      }
      // If spatial coordinates invalid, cannot use to index this datum
      else
      {
        ignerr << "Line [" << line << "] has invalid spatial coordinates "
               << "(latitude, longitude, and/or depth). Skipping." << std::endl;
        continue;
      }
    }
  }

  // Make sure the number of timestamps in the 1D indexing array, and the
  // number of time slices of data, are the same.
  assert(this->timestamps.size() == this->timeSpaceCoords.size());

  for (int i = 0; i < this->timeSpaceCoords.size(); i++)
  {
    igndbg << "At time slice " << this->timestamps[i] << ", populated "
           << this->timeSpaceCoords[i]->size()
           << " spatial coordinates." << std::endl;
  }

  this->initialized = true;
}

/////////////////////////////////////////////////
void ScienceSensorsSystemPrivate::GetWorldOriginSphericalCoords()
{
  // TODO TEMPORARY HACK hard code lat long.
  // When have time, init default value from SDF <spherical_coordinates> tag
  this->worldOriginSphericalCoords = ignition::math::Vector3d(
    36.8024781413352, -121.829647676843, 0);
  // Convert spherical coordinates to Cartesian
  ignition::math::SphericalCoordinates sc(this->surfaceType);
  this->worldOriginEarthCartesianCoords = sc.LocalFromSphericalPosition(
    this->worldOriginSphericalCoords);

  /*
  ignition::msgs::Vector3d res;
  bool success;

  ignmsg << "Waiting for " << this->getWorldOriginSphericalService << "..."
    << std::endl;

  // Get world origin in spherical coordinates, if available.
  if (!this->node.Request(this->getWorldOriginSphericalService, 5000, res,
    success))
  {
    ignerr << "Failed to request service ["
      << this->getWorldOriginSphericalService << "]" << std::endl;
  }

  // If unsuccessful, a first vehicle hasn't been spawned, no long/lat info
  if (!success)
  {
    ignwarn << "No long/lat information. "
      << "Has a vehicle been spawned using the WorldCommPlugin service? "
      << "Science data will be loaded without correcting for long/lat at world "
      << "origin."
      << std::endl;
  }
  else
  {
    this->worldOriginSphericalCoords = ignition::math::Vector3d(
      res.x(), res.y(), res.z());

    // Convert spherical coordinates to Cartesian
    ignition::math::SphericalCoordinates sc(this->surfaceType);
    this->worldOriginEarthCartesianCoords = sc.LocalFromSphericalPosition(
      this->worldOriginSphericalCoords);
  }
  */
}


/*
  if (!this->node.Request(this->getWorldOriginSphericalService, 5000,
      &ScienceSensorsSystemPrivate::WorldOriginSphericalCoordsCb(), this))
  {
    ignerr << "Failed to request service ["
      << this->getWorldOriginSphericalService << "]" << std::endl;
  }
}

/////////////////////////////////////////////////
void ScienceSensorsSystemPrivate::WorldOriginSphericalCoordsCb(
  const ignition::msgs::Vector3d &_res,
  const bool _success)
{
  if (_success)
  {
    this->worldOriginSphericalCoords = _res;

    // Convert spherical coordinates to Cartesian
    this->worldOriginEarthCartesianCoords = this->sc.LocalFromSphericalPosition(
      this->worldOriginSphericalCoords);
  }
}
*/

/////////////////////////////////////////////////
void ScienceSensorsSystemPrivate::GenerateOctrees()
{
  // For each time slice, create an octree for the spatial coordinates
  for (int i = 0; i < this->timeSpaceCoords.size(); ++i)
  {
    this->spatialOctrees.push_back(
      pcl::octree::OctreePointCloudSearch<pcl::PointXYZ>(this->spatialRes));

    // Populate octree with spatial coordinates
    this->spatialOctrees[i].setInputCloud(this->timeSpaceCoords[i]);
    this->spatialOctrees[i].addPointsFromInputCloud();
  }
}

/////////////////////////////////////////////////
void ScienceSensorsSystemPrivate::PublishData()
{
  this->cloudPub.Publish(this->PointCloudMsg());
  this->tempPub.Publish(this->tempMsg);
  this->chlorPub.Publish(this->chlorMsg);
  this->salPub.Publish(this->salMsg);
}

/////////////////////////////////////////////////
float ScienceSensorsSystemPrivate::InterpolateData(
  std::vector<float> _arr,
  std::vector<int> &_inds,
  std::vector<float> &_dists)
{
  // Sanity checks
  if (_inds.size() == 0 || _dists.size() == 0)
  {
    ignwarn << "InterpolateData(): Invalid neighbors aray size ("
            << _inds.size() << " and " << _dists.size()
            << "). No neighbors to use for interpolation. Returning NaN."
            << std::endl;
    return std::numeric_limits<float>::quiet_NaN();
  }
  if (_inds.size() != _dists.size())
  {
    ignwarn << "InterpolateData(): Number of neighbors != number of distances."
            << "Invalid input. Returning NaN." << std::endl;
    return std::numeric_limits<float>::quiet_NaN();
  }

  // Find closest neighbor
  int nnIdx = _inds[0];
  float minDist = _dists[0];
  for (int i = 0; i < _inds.size(); ++i)
  {
    if (_dists[i] < minDist)
    {
      nnIdx = _inds[i];
      minDist = _dists[i];
    }
  }

  return _arr[nnIdx];

  // TODO Return a weighted sum, based on distance, of the elements to
  // interpolate among
  // TODO: Look at x and y only? Depth resolution much finer and tips kNN
  // search. Or use radiusSearch().



}

/////////////////////////////////////////////////
void ScienceSensorsSystem::Configure(
  const ignition::gazebo::Entity &_entity,
  const std::shared_ptr<const sdf::Element> &_sdf,
  ignition::gazebo::EntityComponentManager &_ecm,
  ignition::gazebo::EventManager &_eventMgr)
{
  if (_sdf->HasElement("data_path"))
  {
    this->dataPtr->dataPath = _sdf->Get<std::string>("data_path");
  }

  ignition::common::SystemPaths sysPaths;
  std::string fullPath = sysPaths.FindFile(this->dataPtr->dataPath);
  if (fullPath.empty())
  {
     ignerr << "Data file [" << this->dataPtr->dataPath << "] not found."
            << std::endl;
     return;
  }
  else
  {
    this->dataPtr->dataPath = fullPath;
    ignmsg << "Loading science data from [" << this->dataPtr->dataPath << "]"
           << std::endl;
  }

  this->dataPtr->cloudPub = this->dataPtr->node.Advertise<
      ignition::msgs::PointCloudPacked>("/science_data");

  this->dataPtr->node.Advertise("/science_data_srv",
      &ScienceSensorsSystemPrivate::ScienceDataService, this->dataPtr.get());

  // TODO TEMPORARY HACK publish float array instead of point cloud fields
  this->dataPtr->tempPub = this->dataPtr->node.Advertise<
      ignition::msgs::Float_V>("/temperature");
  this->dataPtr->chlorPub = this->dataPtr->node.Advertise<
      ignition::msgs::Float_V>("/chlorophyll");
  this->dataPtr->salPub = this->dataPtr->node.Advertise<
      ignition::msgs::Float_V>("/salinity");

  // TODO: Offer a way to shift after ReadData(), in case the first vehicle is
  // spawned much later than when ScienceSensorsSystem plugin is loaded.
  this->dataPtr->GetWorldOriginSphericalCoords();

  this->dataPtr->ReadData();
  this->dataPtr->GenerateOctrees();

  // Publish science data at the initial timestamp
  this->dataPtr->PublishData();
}

/////////////////////////////////////////////////
void ScienceSensorsSystem::PreUpdate(
  const ignition::gazebo::UpdateInfo &,
  ignition::gazebo::EntityComponentManager &_ecm)
{
  _ecm.EachNew<ignition::gazebo::components::CustomSensor,
               ignition::gazebo::components::ParentEntity>(
    [&](const ignition::gazebo::Entity &_entity,
        const ignition::gazebo::components::CustomSensor *_custom,
        const ignition::gazebo::components::ParentEntity *_parent)->bool
      {
        createSensor<SalinitySensor>(this, _ecm, _entity, _custom, _parent);
        createSensor<TemperatureSensor>(this, _ecm, _entity, _custom, _parent);
        createSensor<ChlorophyllSensor>(this, _ecm, _entity, _custom, _parent);
        createSensor<CurrentSensor>(this, _ecm, _entity, _custom, _parent);
        return true;
      });
}

/////////////////////////////////////////////////
void ScienceSensorsSystem::PostUpdate(const ignition::gazebo::UpdateInfo &_info,
  const ignition::gazebo::EntityComponentManager &_ecm)
{
  // Only update and publish if data has been loaded and simulation is not
  // paused.
  if (this->dataPtr->initialized && !_info.paused)
  {
    double simTimeSeconds = std::chrono::duration_cast<std::chrono::seconds>(
      _info.simTime).count();

    // Update time index
    if (this->dataPtr->multipleTimeSlices)
    {
      // Only update if sim time exceeds the elapsed timestamp in data
      if (!this->dataPtr->timestamps.empty() &&
        simTimeSeconds >= this->dataPtr->timestamps[this->dataPtr->timeIdx])
      {
        // Increment for next point in time
        this->dataPtr->timeIdx++;

        // Publish science data at the next timestamp
        this->dataPtr->PublishData();
      }
    }

    // TODO TEMPORARY DEBUG publishing more frequently to help debugging.
    // Publish every n iters so that VisualizePointCloud plugin gets it.
    if (this->dataPtr->repeatPubTimes % 10000 == 0)
    {
      this->dataPtr->PublishData();
      // Reset
      this->dataPtr->repeatPubTimes = 1;
    }
    else
    {
      this->dataPtr->repeatPubTimes++;
    }

    // For each sensor, get its pose, search in the octree for the closest
    // neighbors, and interpolate to get approximate data at this sensor pose.
    for (auto &[entity, sensor] : this->entitySensorMap)
    {
      // Sensor pose in lat/lon, used to search for data by spatial coordinates
      // TODO convert to Cartesian
      auto sensorLatLon = ignition::gazebo::sphericalCoordinates(entity, _ecm);
      /*
      // TODO TEMP DEBUG what is the sensor attached to?? World? Not robot?
      ignerr << "sensor lat long: "
             << sensorLatLon.value().X() << ", " << sensorLatLon.value().Y()
             << std::endl;
      */
      if (!sensorLatLon)
      {
        static std::unordered_set<ignition::gazebo::Entity> warnedEntities;
        if (warnedEntities.find(entity) != warnedEntities.end())
        {
          ignwarn << "Failed to get spherical coordinates for sensor entity ["
                  << entity << "]" << std::endl;
          warnedEntities.insert(entity);
        }
        continue;
      }
      pcl::PointXYZ searchPoint(
          sensorLatLon.value().X(),
          sensorLatLon.value().Y(),
          sensorLatLon.value().Z());

      // kNN search (alternatives are voxel search and radius search. kNN
      // search is good for variable resolution when the distance to the next
      // neighbor is unknown).
      int k = 8;

      // Indices and distances of neighboring points in the search results
      std::vector<int> spatialIdx;
      std::vector<float> spatialSqrDist;

      // Search in octree to find spatial index of science data
      if (this->dataPtr->spatialOctrees[this->dataPtr->timeIdx].getLeafCount()
        > 0)
      {
        if (this->dataPtr->spatialOctrees[this->dataPtr->timeIdx].nearestKSearch(
          searchPoint, k, spatialIdx, spatialSqrDist) <= 0)
        {
          ignwarn << "No data found near sensor location " << sensorLatLon.value()
                  << std::endl;
          continue;
        }
        // Debug output
        /*
        else
        {
          igndbg << "kNN search for sensor pose (" << sensorPose.X() << ", "
                 << sensorPose.Y() << ", " << sensorPose.Z() << "):"
                 << std::endl;

          for (std::size_t i = 0; i < spatialIdx.size(); i++)
          {
            // Index the point cloud at the current time slice
            pcl::PointXYZ nbrPt = (*(this->dataPtr->timeSpaceCoords[
              this->dataPtr->timeIdx]))[spatialIdx[i]];

            igndbg << "Neighbor at (" << nbrPt.x << ", " << nbrPt.y << ", "
                   << nbrPt.z << "), squared distance " << spatialSqrDist[i]
                   << " m" << std::endl;
          }
        }
        */

        // For the correct sensor, grab closest neighbors and interpolate
        if (auto casted = std::dynamic_pointer_cast<SalinitySensor>(sensor))
        {
          float sal = this->dataPtr->InterpolateData(
            this->dataPtr->salinityArr[this->dataPtr->timeIdx], spatialIdx,
            spatialSqrDist);
          casted->SetData(sal);
        }
        else if (auto casted = std::dynamic_pointer_cast<TemperatureSensor>(
          sensor))
        {
          float temp = this->dataPtr->InterpolateData(
            this->dataPtr->temperatureArr[this->dataPtr->timeIdx], spatialIdx,
            spatialSqrDist);

          ignition::math::Temperature tempC;
          tempC.SetCelsius(temp);
          casted->SetData(tempC);
        }
        else if (auto casted = std::dynamic_pointer_cast<ChlorophyllSensor>(
          sensor))
        {
          float chlor = this->dataPtr->InterpolateData(
            this->dataPtr->chlorophyllArr[this->dataPtr->timeIdx], spatialIdx,
            spatialSqrDist);
          casted->SetData(chlor);
        }
        else if (auto casted = std::dynamic_pointer_cast<CurrentSensor>(sensor))
        {
          float eCurr = this->dataPtr->InterpolateData(
            this->dataPtr->eastCurrentArr[this->dataPtr->timeIdx], spatialIdx,
            spatialSqrDist);
          float nCurr = this->dataPtr->InterpolateData(
            this->dataPtr->northCurrentArr[this->dataPtr->timeIdx], spatialIdx,
            spatialSqrDist);

          auto curr = ignition::math::Vector3d(eCurr, nCurr, 0.0);
          casted->SetData(curr);
        }
        else
        {
          ignerr << "Unsupported sensor type, failed to set data" << std::endl;
        }
      }
      sensor->Update(_info.simTime, false);
    }
  }

  this->RemoveSensorEntities(_ecm);
}

//////////////////////////////////////////////////
void ScienceSensorsSystem::RemoveSensorEntities(
    const ignition::gazebo::EntityComponentManager &_ecm)
{
  _ecm.EachRemoved<ignition::gazebo::components::CustomSensor>(
    [&](const ignition::gazebo::Entity &_entity,
        const ignition::gazebo::components::CustomSensor *)->bool
      {
        auto sensorId = this->entitySensorMap.find(_entity);
        if (sensorId == this->entitySensorMap.end())
        {
          ignerr << "Internal error, missing sensor for entity ["
                 << _entity << "]" << std::endl;
          return true;
        }

        this->entitySensorMap.erase(sensorId);

        igndbg << "Removed sensor entity [" << _entity << "]" << std::endl;

        return true;
      });
}

//////////////////////////////////////////////////
bool ScienceSensorsSystemPrivate::ScienceDataService(
    ignition::msgs::PointCloudPacked &_res)
{
  _res = this->PointCloudMsg();
  return true;
}

//////////////////////////////////////////////////
ignition::msgs::PointCloudPacked ScienceSensorsSystemPrivate::PointCloudMsg()
{
  ignition::msgs::PointCloudPacked msg;

  if (this->timeIdx < 0 || this->timeIdx >= this->timestamps.size())
  {
    ignerr << "Invalid time index [" << this->timeIdx << "]."
           << std::endl;
    return msg;
  }

  ignition::msgs::InitPointCloudPacked(msg, "world", true,
    {
      {"xyz", ignition::msgs::PointCloudPacked::Field::FLOAT32},
    });

  // TODO optimization for visualization:
  // Use PCL methods to chop off points beyond some distance from sensor
  // pose. Don't need to visualize beyond that. Might want to put that on a
  // different topic specifically for visualization.

  msg.mutable_header()->mutable_stamp()->set_sec(this->timestamps[this->timeIdx]);

  pcl::PCLPointCloud2 pclPC2;
  pcl::toPCLPointCloud2(*this->timeSpaceCoords[this->timeIdx].get(), pclPC2);

  msg.set_height(pclPC2.height);
  msg.set_width(pclPC2.width);
  msg.set_is_bigendian(pclPC2.is_bigendian);
  msg.set_point_step(pclPC2.point_step);
  msg.set_row_step(pclPC2.row_step);
  msg.set_is_dense(pclPC2.is_dense);

  // FIXME: Include more than position data
  msg.mutable_data()->resize(pclPC2.data.size());
  //  + temperatureArr[this->timeIdx].size());
  memcpy(msg.mutable_data()->data(), pclPC2.data.data(), pclPC2.data.size());

  /*
  // Interleaf fields to each point
  for (int c = 0; c < pclPC2.width; ++c)
  {
    for (int r = 0; r < pclPC2.height; ++r)
    {
      memcpy(msg.mutable_data()->data() + ,
        pclPC2.data.data() + ,
        pclPC2.data.size());


    }
  }
  */

  // Wrong layout
  // Temperature field, offset from beginning by previous data's size
  //memcpy(msg.mutable_data()->data() + pclPC2.data.size(),
  //  temperatureArr[this->timeIdx].data(), temperatureArr[this->timeIdx].size());

  // TODO TEMPORARY HACK publish float arrays instead of point cloud fields
  //this->tempMsg.mutable_data()->Resize(temperatureArr[this->timeIdx].size(), 0.0f);
  //memcpy(this->tempMsg.mutable_data(),
  //  temperatureArr[this->timeIdx].data(), temperatureArr[this->timeIdx].size());
  // TODO did memory leak start after I started publishing float array?
  *this->tempMsg.mutable_data() = {temperatureArr[this->timeIdx].begin(),
    temperatureArr[this->timeIdx].end()};
  *this->chlorMsg.mutable_data() = {chlorophyllArr[this->timeIdx].begin(),
    chlorophyllArr[this->timeIdx].end()};
  *this->salMsg.mutable_data() = {salinityArr[this->timeIdx].begin(),
    salinityArr[this->timeIdx].end()};

  return msg;
}

IGNITION_ADD_PLUGIN(
  tethys::ScienceSensorsSystem,
  ignition::gazebo::System,
  tethys::ScienceSensorsSystem::ISystemConfigure,
  tethys::ScienceSensorsSystem::ISystemPreUpdate,
  tethys::ScienceSensorsSystem::ISystemPostUpdate)

IGNITION_ADD_PLUGIN_ALIAS(tethys::ScienceSensorsSystem,
    "tethys::ScienceSensorsSystem")
