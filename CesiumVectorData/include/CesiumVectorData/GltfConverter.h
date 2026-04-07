#pragma once

#include <CesiumGltf/Model.h>
#include <CesiumVectorData/GeoJsonDocument.h>

namespace CesiumVectorData {

// Modeled after (copied from?) Cesium3DTilesContent::GltfConverterResult

struct CESIUMVECTORDATA_API ConverterResult {
  /**
   * @brief The gltf model converted from GeoJSON. This is empty if
   * there are errors during the conversion
   */
  std::optional<CesiumGltf::Model> model;

  /**
   * @brief The error and warning list when converting to a gltf
   * model. This is empty if there are no errors during the conversion
   */
  CesiumUtility::ErrorList errors;
};

class CESIUMVECTORDATA_API GltfConverter {
public:
  ConverterResult operator()(const GeoJsonDocument& geoJson);
};
} // namespace CesiumVectorData
