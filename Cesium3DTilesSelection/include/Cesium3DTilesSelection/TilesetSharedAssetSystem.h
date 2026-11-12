#pragma once

#include <Cesium3DTilesSelection/MaxarContentGeoJsonSchemaAssetDescriptor.h>
#include <CesiumAsync/SharedAssetDepot.h>
#include <CesiumGltf/Schema.h>
#include <CesiumGltfReader/GltfSharedAssetSystem.h>
#include <CesiumUtility/IntrusivePointer.h>

namespace Cesium3DTilesSelection {

/**
 * @brief Contains assets that are potentially shared across multiple Tilesets.
 */
class TilesetSharedAssetSystem
    : public CesiumGltfReader::GltfSharedAssetSystem {
public:
  /**
   * @brief Obtains an `IntrusivePointer` to the `TilesetSharedAssetSystem`
   * singleton.
   */
  static CesiumUtility::IntrusivePointer<TilesetSharedAssetSystem> getDefault();

  virtual ~TilesetSharedAssetSystem() = default;

  /**
   * @brief A depot of glTF schemas converted from `MAXAR_content_geojson`
   * properties schemas.
   *
   * The depot key is the schema's resolved URL plus request headers. The
   * factory downloads the Maxar document and converts it; it does not parse
   * the bytes as a glTF schema. Loaders that resolve the same key receive the
   * same @ref CesiumGltf::Schema object.
   */
  using MaxarContentGeoJsonSchemaDepot = CesiumAsync::SharedAssetDepot<
      CesiumGltf::Schema,
      MaxarContentGeoJsonSchemaAssetDescriptor>;

  /**
   * @brief The asset depot for converted `MAXAR_content_geojson` schemas.
   */
  CesiumUtility::IntrusivePointer<MaxarContentGeoJsonSchemaDepot>
      pMaxarContentGeoJsonSchema;
};

} // namespace Cesium3DTilesSelection
