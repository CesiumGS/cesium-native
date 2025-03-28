#include <CesiumITwinClient/CesiumCuratedContent.h>

#include <string>

namespace CesiumITwinClient {
CesiumCuratedContentType
cesiumCuratedContentTypeFromString(const std::string& str) {
  if (str == "3DTILES") {
    return CesiumCuratedContentType::Cesium3DTiles;
  } else if (str == "GLTF") {
    return CesiumCuratedContentType::Gltf;
  } else if (str == "IMAGERY") {
    return CesiumCuratedContentType::Imagery;
  } else if (str == "TERRAIN") {
    return CesiumCuratedContentType::Terrain;
  } else if (str == "KML") {
    return CesiumCuratedContentType::Kml;
  } else if (str == "CZML") {
    return CesiumCuratedContentType::Czml;
  } else if (str == "GEOJSON") {
    return CesiumCuratedContentType::GeoJson;
  }

  return CesiumCuratedContentType::Unknown;
}

CesiumCuratedContentStatus
cesiumCuratedContentStatusFromString(const std::string& str) {
  if (str == "AWAITING_FILES") {
    return CesiumCuratedContentStatus::AwaitingFiles;
  } else if (str == "NOT_STARTED") {
    return CesiumCuratedContentStatus::NotStarted;
  } else if (str == "IN_PROGRESS") {
    return CesiumCuratedContentStatus::InProgress;
  } else if (str == "COMPLETE") {
    return CesiumCuratedContentStatus::Complete;
  } else if (str == "ERROR") {
    return CesiumCuratedContentStatus::Error;
  } else if (str == "DATA_ERROR") {
    return CesiumCuratedContentStatus::DataError;
  }

  return CesiumCuratedContentStatus::Unknown;
}
} // namespace CesiumITwinClient