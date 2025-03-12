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

  return ITwinCesiumCuratedContentType::Unknown;
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

  return ITwinCesiumCuratedContentStatus::Unknown;
}
ITwinStatus iTwinStatusFromString(const std::string& str) {
  if (str == "Active") {
    return ITwinStatus::Active;
  } else if (str == "Inactive") {
    return ITwinStatus::Inactive;
  } else if (str == "Trial") {
    return ITwinStatus::Trial;
  }

  return ITwinStatus::Unknown;
}
IModelState iModelStateFromString(const std::string& str) {
  if (str == "initialized") {
    return IModelState::Initialized;
  } else if (str == "notInitialized") {
    return IModelState::NotInitialized;
  }

  return IModelState::Unknown;
}
IModelMeshExportStatus
iModelMeshExportStatusFromString(const std::string& str) {
  if (str == "NotStarted") {
    return IModelMeshExportStatus::NotStarted;
  } else if (str == "InProgress") {
    return IModelMeshExportStatus::InProgress;
  } else if (str == "Complete") {
    return IModelMeshExportStatus::Complete;
  } else if (str == "Invalid") {
    return IModelMeshExportStatus::Invalid;
  }

  return IModelMeshExportStatus::Unknown;
}
ITwinRealityDataClassification
iTwinRealityDataClassificationFromString(const std::string& str) {
  if (str == "Terrain") {
    return ITwinRealityDataClassification::Terrain;
  } else if (str == "Imagery") {
    return ITwinRealityDataClassification::Imagery;
  } else if (str == "Pinned") {
    return ITwinRealityDataClassification::Pinned;
  } else if (str == "Model") {
    return ITwinRealityDataClassification::Model;
  } else if (str == "Undefined") {
    return ITwinRealityDataClassification::Undefined;
  }

  return ITwinRealityDataClassification::Unknown;
}
IModelMeshExportType iModelMeshExportTypeFromString(const std::string& str) {
  if (str == "3DFT") {
    return IModelMeshExportType::ITwin3DFT;
  } else if (str == "IMODEL") {
    return IModelMeshExportType::IModel;
  } else if (str == "CESIUM") {
    return IModelMeshExportType::Cesium;
  } else if (str == "3DTiles") {
    return IModelMeshExportType::Cesium3DTiles;
  }

  return IModelMeshExportType::Unknown;
}
} // namespace CesiumITwinClient