#pragma once

#include <cstdint>
#include <string>

namespace CesiumITwinClient {

enum class ITwinCesiumCuratedContentType : uint8_t {
  Invalid = 0,
  Cesium3DTiles = 1,
  Gltf = 2,
  Imagery = 3,
  Terrain = 4,
  Kml = 5,
  Czml = 6,
  GeoJson = 7,
};

ITwinCesiumCuratedContentType cesiumCuratedContentTypeFromString(const std::string& str);

enum class ITwinCesiumCuratedContentStatus : uint8_t {
  Invalid = 0,
  AwaitingFiles = 1,
  NotStarted = 2,
  InProgress = 3,
  Complete = 4,
  Error = 5,
  DataError = 6
};

ITwinCesiumCuratedContentStatus cesiumCuratedContentStatusFromString(const std::string& str);

struct ITwinCesiumCuratedContentItem {
  uint64_t id;
  ITwinCesiumCuratedContentType type;
  std::string name;
  std::string description;
  std::string attribution;
  ITwinCesiumCuratedContentStatus status;
};

enum class ITwinStatus : uint8_t {
  Invalid = 0,
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

}