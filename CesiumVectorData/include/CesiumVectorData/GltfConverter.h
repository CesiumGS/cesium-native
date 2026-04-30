#pragma once

#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGltf/Model.h>
#include <CesiumUtility/Result.h>
#include <CesiumVectorData/GeoJsonDocument.h>

namespace CesiumVectorData {

/**
 * @brief The result of converting a GeoJSON document to glTF.
 */
using ConverterResult = CesiumUtility::Result<CesiumGltf::Model>;

/**
 * @brief Convert GeoJSON documents to glTF.
 */
class CESIUMVECTORDATA_API GltfConverter {
public:
  /**
   * @brief Convert geoJSON document to a result containing a glTF Model
   * object.
   *
   * @param geoJson The GeoJSON document.
   * @param ellipsoid The ellipsoid for GeoJSON coordinates.
   * @returns A result object that includes the glTF Model and any errors.
   */
  static ConverterResult convert(
      const GeoJsonDocument& geoJson,
      const CesiumGeospatial::Ellipsoid& ellipsoid);
};
} // namespace CesiumVectorData
