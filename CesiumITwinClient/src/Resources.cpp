#include <CesiumITwinClient/Resources.h>

namespace CesiumITwinClient {
ITwinCesiumCuratedContentType cesiumCuratedContentTypeFromString(const std::string &str) {
  if(str == "3DTILES") {
    return ITwinCesiumCuratedContentType::Cesium3DTiles;
  } else if(str == "GLTF") {
    return ITwinCesiumCuratedContentType::Gltf;
  } else if(str == "IMAGERY") {
    return ITwinCesiumCuratedContentType::Imagery;
  } else if(str == "TERRAIN") {
    return ITwinCesiumCuratedContentType::Terrain;
  } else if(str == "KML") {
    return ITwinCesiumCuratedContentType::Kml;
  } else if(str == "CZML") {
    return ITwinCesiumCuratedContentType::Czml;
  } else if(str == "GEOJSON") {
    return ITwinCesiumCuratedContentType::GeoJson;
  }

  return ITwinCesiumCuratedContentType::Invalid;
}

ITwinCesiumCuratedContentStatus
cesiumCuratedContentStatusFromString(const std::string& str) {
  if (str == "AWAITING_FILES") {
    return ITwinCesiumCuratedContentStatus::AwaitingFiles;
  } else if (str == "NOT_STARTED") {
    return ITwinCesiumCuratedContentStatus::NotStarted;
  } else if (str == "IN_PROGRESS") {
    return ITwinCesiumCuratedContentStatus::InProgress;
  } else if (str == "COMPLETE") {
    return ITwinCesiumCuratedContentStatus::Complete;
  } else if (str == "ERROR") {
    return ITwinCesiumCuratedContentStatus::Error;
  } else if (str == "DATA_ERROR") {
    return ITwinCesiumCuratedContentStatus::DataError;
  }

  return ITwinCesiumCuratedContentStatus::Invalid;
}
ITwinStatus iTwinStatusFromString(const std::string& str) {
  if (str == "Active") {
    return ITwinStatus::Active;
  } else if (str == "Inactive") {
    return ITwinStatus::Inactive;
  } else if (str == "Trial") {
    return ITwinStatus::Trial;
  }

  return ITwinStatus::Invalid;
}
} // namespace CesiumITwinClient