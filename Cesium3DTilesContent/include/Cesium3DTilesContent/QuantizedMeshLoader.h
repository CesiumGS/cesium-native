#pragma once

#include "Library.h"

#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTileRectangularRange.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGltf/Model.h>
#include <CesiumUtility/ErrorList.h>

#include <gsl/span>
#include <rapidjson/document.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace CesiumAsync {
class IAssetRequest;
}

namespace Cesium3DTilesContent {

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

  CesiumUtility::ErrorList errors;
};

struct QuantizedMeshMetadataResult {
  std::vector<CesiumGeometry::QuadtreeTileRectangularRange> availability;
  CesiumUtility::ErrorList errors;
};

/**
 * @brief Loads `quantized-mesh-1.0` terrain data.
 */
class CESIUM3DTILESCONTENT_API QuantizedMeshLoader final {
public:
  /**
   * @brief Create a {@link QuantizedMeshLoadResult} from the given data.
   *
   * @param tileID The tile ID
   * @param tileBoundingVoume The tile bounding volume
   * @param url The URL
   * @param data The actual input data
   * @return The {@link QuantizedMeshLoadResult}
   */
  static QuantizedMeshLoadResult load(
      const CesiumGeometry::QuadtreeTileID& tileID,
      const CesiumGeospatial::BoundingRegion& tileBoundingVolume,
      const std::string& url,
      const gsl::span<const std::byte>& data,
      bool enableWaterMask);

  static QuantizedMeshMetadataResult loadMetadata(
      const gsl::span<const std::byte>& data,
      const CesiumGeometry::QuadtreeTileID& tileID);

  static QuantizedMeshMetadataResult loadAvailabilityRectangles(
      const rapidjson::Document& metadata,
      uint32_t startingLevel);
};

} // namespace Cesium3DTilesContent
