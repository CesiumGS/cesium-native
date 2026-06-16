#pragma once

#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Schema.h>
#include <CesiumUtility/Result.h>
#include <CesiumVectorData/GeoJsonDocument.h>

#include <rapidjson/document.h>

namespace CesiumVectorData {

/**
 * @brief The result of converting a GeoJSON document to glTF.
 */
using ConverterResult = CesiumUtility::Result<CesiumGltf::Model>;

/**
 * @brief The result of converting a MAXAR_content_geojson schema to a glTF
 * schema.
 */
using ConvertSchemaResult = CesiumUtility::Result<CesiumGltf::Schema>;

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
  /**
   * @brief Convert MAXAR_content_geojson schema into a glTF schema.
   * @param schemaJson The schema supplied with a GeoJson tileset
   * @returns A result object that includes the converted schema and any errors.
   */
  static ConvertSchemaResult
  convertSchema(const rapidjson::Document& schemaJson);
};
} // namespace CesiumVectorData
