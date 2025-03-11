#pragma once

#include <CesiumGeospatial/GlobeRectangle.h>

#include <cstdint>
#include <string>

namespace CesiumITwinClient {

struct UserProfile {
  std::string id;
  std::string displayName;
  std::string givenName;
  std::string surname;
  std::string email;
};

enum class ITwinCesiumCuratedContentType : uint8_t {
  Unknown = 0,
  Cesium3DTiles = 1,
  Gltf = 2,
  Imagery = 3,
  Terrain = 4,
  Kml = 5,
  Czml = 6,
  GeoJson = 7,
};

ITwinCesiumCuratedContentType
cesiumCuratedContentTypeFromString(const std::string& str);

enum class ITwinCesiumCuratedContentStatus : uint8_t {
  Unknown = 0,
  AwaitingFiles = 1,
  NotStarted = 2,
  InProgress = 3,
  Complete = 4,
  Error = 5,
  DataError = 6
};

ITwinCesiumCuratedContentStatus
cesiumCuratedContentStatusFromString(const std::string& str);

struct ITwinCesiumCuratedContentItem {
  uint64_t id;
  ITwinCesiumCuratedContentType type;
  std::string name;
  std::string description;
  std::string attribution;
  ITwinCesiumCuratedContentStatus status;
};

enum class ITwinStatus : uint8_t {
  Unknown = 0,
  Active = 1,
  Inactive = 2,
  Trial = 3
};

ITwinStatus iTwinStatusFromString(const std::string& str);

struct ITwin {
  std::string id;
  std::string iTwinClass;
  std::string subClass;
  std::string type;
  std::string number;
  std::string displayName;
  ITwinStatus status;
};

enum class IModelState : uint8_t {
  Unknown = 0,
  Initialized = 1,
  NotInitialized = 2
};

IModelState iModelStateFromString(const std::string& str);

struct IModel {
  std::string id;
  std::string displayName;
  std::string name;
  std::string description;
  IModelState state;
  CesiumGeospatial::GlobeRectangle extent;
};

enum class IModelMeshExportStatus : uint8_t {
  Unknown = 0,
  NotStarted = 1,
  InProgress = 2,
  Complete = 3,
  Invalid = 4
};

IModelMeshExportStatus iModelMeshExportStatusFromString(const std::string& str);

enum class IModelMeshExportType : uint8_t {
  Unknown = 0,
  /**
   * @brief iTwin "3D Fast Transmission" (3DFT) format.
   */
  ITwin3DFT = 1,
  IModel = 2,
  Cesium = 3,
  Cesium3DTiles = 4,
};

IModelMeshExportType iModelMeshExportTypeFromString(const std::string& str);

struct IModelMeshExport {
  std::string id;
  std::string displayName;
  IModelMeshExportStatus status;
  IModelMeshExportType exportType;
};

enum class ITwinRealityDataClassification : uint8_t {
  Unknown = 0,
  Terrain = 1,
  Imagery = 2,
  Pinned = 3,
  Model = 4,
  Undefined = 5
};

ITwinRealityDataClassification
iTwinRealityDataClassificationFromString(const std::string& str);

struct ITwinRealityData {
  std::string id;
  std::string displayName;
  std::string description;
  ITwinRealityDataClassification classification;
  std::string type;
  CesiumGeospatial::GlobeRectangle extent;
  bool authoring;
};

} // namespace CesiumITwinClient