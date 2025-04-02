#include <CesiumITwinClient/IModelMeshExport.h>

#include <string>

namespace CesiumITwinClient {
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