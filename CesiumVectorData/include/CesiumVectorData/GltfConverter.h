#pragma once

#include <CesiumGltf/Model.h>
#include <CesiumVectorData/GeoJsonDocument.h>

namespace CesiumVectorData {

/**
 * @brief The result of converting a GeoJSON file to a glTF model.
 */
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

/**
 * @brief Functional for converting GeoJSON documents to glTF.
 */
class CESIUMVECTORDATA_API GltfConverter {
public:
  /**
   * @brief Convert geoJSON document to a result which includes a glTF Model
   * object.
   *
   * @param geoJson The GeoJSON document.
   * @returns A result object that includes the glTF Model and any errors.
   */
  ConverterResult operator()(const GeoJsonDocument& geoJson);
};
} // namespace CesiumVectorData
