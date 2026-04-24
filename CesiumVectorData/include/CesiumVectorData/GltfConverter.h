#pragma once

#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGltf/Model.h>
#include <CesiumUtility/Result.h>
#include <CesiumVectorData/GeoJsonDocument.h>

namespace CesiumVectorData {

using ConverterResult = CesiumUtility::Result<CesiumGltf::Model>;

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
   * @param ellipsoid The ellipsoid for GeoJSON coordinates
   * @returns A result object that includes the glTF Model and any errors.
   */
  static ConverterResult convert(
      const GeoJsonDocument& geoJson,
      const CesiumGeospatial::Ellipsoid& ellipsoid);
};
} // namespace CesiumVectorData
