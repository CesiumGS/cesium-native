#pragma once

#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTileRectangularRange.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGltf/Model.h>
#include <CesiumQuantizedMeshTerrain/Library.h>
#include <CesiumUtility/ErrorList.h>

#include <rapidjson/document.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace CesiumAsync {
class IAssetRequest;
}

namespace CesiumQuantizedMeshTerrain {

/**
 * @brief The results of a \ref QuantizedMeshLoader::load operation, containing
 * either the loaded model, an improved bounding region for the tile, and
 * available quadtree tiles discovered, if the load succeeded - or the request
 * made and the errors that were returned, if the load failed.
 */
struct QuantizedMeshLoadResult {
  /**
   * @brief The glTF model to be rendered for this tile.
   *
   * If this is `std::nullopt`, the tile cannot be rendered.
   * If it has a value but the model is blank, the tile can
   * be "rendered", but it is rendered as nothing.
   */
  std::optional<CesiumGltf::Model> model;

  /**
   * @brief An improved bounding region for this tile.
   *
   * If this is available, then it is more accurate than the one the tile used
   * originally.
   */
  std::optional<CesiumGeospatial::BoundingRegion> updatedBoundingVolume{};

  /**
   * @brief Available quadtree tiles discovered as a result of loading this
   * tile.
   */
  std::vector<CesiumGeometry::QuadtreeTileRectangularRange>
      availableTileRectangles{};

  /**
   * @brief The request that was used to download the tile content, if any.
   *
   * This field is only populated when there are request-related errors.
   */
  std::shared_ptr<CesiumAsync::IAssetRequest> pRequest;

  /**
   * @brief The errors and warnings reported while loading this tile.
   */
  CesiumUtility::ErrorList errors;
};

/**
 * @brief The metadata of a Quantized Mesh tile, returned by \ref
 * QuantizedMeshLoader::loadMetadata.
 */
struct QuantizedMeshMetadataResult {
  /**
   * @brief Information about the availability of child tiles.
   */
  std::vector<CesiumGeometry::QuadtreeTileRectangularRange> availability;

  /**
   * @brief The errors and warnings reported while loading this tile, if any.
   */
  CesiumUtility::ErrorList errors;
};

/**
 * @brief Loads `quantized-mesh-1.0` terrain data.
 */
class CESIUMQUANTIZEDMESHTERRAIN_API QuantizedMeshLoader final {
public:
  /**
   * @brief Create a {@link QuantizedMeshLoadResult} from the given data.
   *
   * @param tileID The tile ID.
   * @param tileBoundingVolume The tile bounding volume.
   * @param url The URL from which the data was loaded.
   * @param data The actual tile data.
   * @param enableWaterMask If true, will attempt to load a water mask from the
   * quantized mesh data.
   * @param ellipsoid The ellipsoid to use for this quantized mesh.
   * @return The {@link QuantizedMeshLoadResult}
   */
  static QuantizedMeshLoadResult load(
      const CesiumGeometry::QuadtreeTileID& tileID,
      const CesiumGeospatial::BoundingRegion& tileBoundingVolume,
      const std::string& url,
      const std::span<const std::byte>& data,
      bool enableWaterMask,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  /**
   * @brief Parses the metadata (tile availability) from the given
   * quantized-mesh terrain tile data.
   *
   * @param data The actual tile data.
   * @param tileID The tile ID.
   * @return The parsed metadata.
   */
  static QuantizedMeshMetadataResult loadMetadata(
      const std::span<const std::byte>& data,
      const CesiumGeometry::QuadtreeTileID& tileID);

  /**
   * @brief Extracts tile availability information from a parsed layer.json
   * or tile metadata extension.
   *
   * The actual availability information will be found in a property called
   * `available`.
   *
   * @param layerJson The RapidJSON document containing the layer.json.
   * @param startingLevel The first tile level number to which the availability
   * information applies.
   * @return The availability.
   */
  static QuantizedMeshMetadataResult loadAvailabilityRectangles(
      const rapidjson::Document& layerJson,
      uint32_t startingLevel);
};

} // namespace CesiumQuantizedMeshTerrain
