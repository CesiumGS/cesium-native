#pragma once

#include <cstdint>
#include <string>

namespace CesiumITwinClient {
/**
 * @brief The status of an iModel Mesh Export.
 *
 * See
 * https://developer.bentley.com/apis/mesh-export/operations/get-export/#exportstatus
 * for more information.
 */
enum class IModelMeshExportStatus : uint8_t {
  Unknown = 0,
  NotStarted = 1,
  InProgress = 2,
  Complete = 3,
  Invalid = 4
};

/**
 * @brief Obtains an \ref IModelMeshExportStatus value from the provided string.
 *
 * See
 * https://developer.bentley.com/apis/mesh-export/operations/get-export/#exportstatus
 * for a list of possible values.
 */
IModelMeshExportStatus iModelMeshExportStatusFromString(const std::string& str);

/**
 * @brief The type of mesh exported.
 *
 * See
 * https://developer.bentley.com/apis/mesh-export/operations/get-export/#startexport-exporttype
 * for more information.
 */
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

/**
 * @brief Obtains an \ref IModelMeshExportType value from the provided string.
 *
 * See
 * https://developer.bentley.com/apis/mesh-export/operations/get-export/#startexport-exporttype
 * for a list of possible values.
 */
IModelMeshExportType iModelMeshExportTypeFromString(const std::string& str);

/**
 * @brief An iModel mesh export.
 *
 * See
 * https://developer.bentley.com/apis/mesh-export/operations/get-export/#export
 * for more information.
 */
struct IModelMeshExport {
  /** @brief ID of the export request. */
  std::string id;
  /** @brief Name of the exported iModel. */
  std::string displayName;
  /** @brief The status of the export job. */
  IModelMeshExportStatus status;
  /** @brief The type of the export. */
  IModelMeshExportType exportType;
};
}; // namespace CesiumITwinClient