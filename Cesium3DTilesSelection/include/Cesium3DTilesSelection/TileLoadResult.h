#pragma once

#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGltf/Model.h>
#include <CesiumRasterOverlays/RasterOverlayDetails.h>

#include <functional>
#include <memory>
#include <optional>
#include <variant>

namespace Cesium3DTilesSelection {

class Tile;

/**
 * @brief Store the content of the tile after finishing
 * loading tile using {@link TilesetContentLoader::loadTileContent}:
 *
 * 1. Returning {@link TileUnknownContent} means that the loader doesn't know
 * the content of the tile. This content type is useful when loader fails to
 * load the tile content; or a background task is running to determine the tile
 * content and the loader wants the client to retry later at some point in the
 * future
 *
 * 2. Returning {@link TileEmptyContent} means that this tile has no content and mostly used
 * for efficient culling during the traversal process
 *
 * 3. Returning {@link TileExternalContent} means that this tile points to an external tileset
 *
 * 4. Returning {@link CesiumGltf::Model} means that this tile has glTF model
 */
using TileContentKind = std::variant<
    TileUnknownContent,
    TileEmptyContent,
    TileExternalContent,
    CesiumGltf::Model>;

/**
 * @brief Indicate the status of {@link Cesium3DTilesSelection::TilesetContentLoader::loadTileContent} and
 * {@link Cesium3DTilesSelection::TilesetContentLoader::createTileChildren} operations
 */
enum class TileLoadResultState {
  /**
   * @brief The operation is successful and all the fields in {@link TileLoadResult}
   * or {@link TileChildrenResult} are applied to the tile
   */
  Success,

  /**
   * @brief The operation is failed and __none__ of the fields in {@link TileLoadResult}
   * or {@link TileChildrenResult} are applied to the tile
   */
  Failed,

  /**
   * @brief The operation requires the client to retry later due to some
   * background work happenning and
   * __none__ of the fields in {@link TileLoadResult} or {@link TileChildrenResult} are applied to the tile
   */
  RetryLater
};

/**
 * @brief Store the result of loading a tile content after
 * invoking {@link TilesetContentLoader::loadTileContent}
 */
struct CESIUM3DTILESSELECTION_API TileLoadResult {
  /**
   * @brief The content type of the tile.
   */
  TileContentKind contentKind;

  /**
   * @brief The up axis of glTF content.
   */
  CesiumGeometry::Axis glTFUpAxis;

  /**
   * @brief A tile can potentially store a more fit bounding volume along with
   * its content. If this field is set, the tile's bounding volume will be
   * updated after the loading is finished.
   */
  std::optional<BoundingVolume> updatedBoundingVolume;

  /**
   * @brief A tile can potentially store a more fit content bounding volume
   * along with its content. If this field is set, the tile's content bounding
   * volume will be updated after the loading is finished.
   */
  std::optional<BoundingVolume> updatedContentBoundingVolume;

  /**
   * @brief Holds details of the {@link TileRenderContent} that are useful
   * for raster overlays.
   */
  std::optional<CesiumRasterOverlays::RasterOverlayDetails>
      rasterOverlayDetails;

  /**
   * @brief The asset accessor that was used to retrieve this tile, and that
   * should be used to retrieve further resources referenced by the tile.
   */
  std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor;

  /**
   * @brief The request that is created to download the tile content.
   */
  std::shared_ptr<CesiumAsync::IAssetRequest> pCompletedRequest;

  /**
   * @brief A callback that is invoked in the main thread immediately when the
   * loading is finished. This callback is useful when the content request has
   * other fields like geometric error,
   * children (in the case of {@link TileExternalContent}), etc, to override the existing fields.
   */
  std::function<void(Tile&)> tileInitializer;

  /**
   * @brief The result of loading a tile. Note that if the state is Failed or
   * RetryLater, __none__ of the fields above (including {@link TileLoadResult::tileInitializer}) will be
   * applied to a tile when the loading is finished
   */
  TileLoadResultState state;

  /**
   * @brief The ellipsoid that this tile uses.
   *
   * This value is only guaranteed to be accurate when {@link TileLoadResult::state} is equal to {@link TileLoadResultState::Success}.
   */
  CesiumGeospatial::Ellipsoid ellipsoid =
      CesiumGeospatial::Ellipsoid::UNIT_SPHERE;

  /**
   * @brief Create a result with Failed state
   *
   * @param pAssetAccessor The \ref CesiumAsync::IAssetAccessor "IAssetAccessor"
   * used to load tiles.
   * @param pCompletedRequest The failed request
   */
  static TileLoadResult createFailedResult(
      std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor,
      std::shared_ptr<CesiumAsync::IAssetRequest> pCompletedRequest);

  /**
   * @brief Create a result with RetryLater state
   *
   * @param pAssetAccessor The \ref CesiumAsync::IAssetAccessor "IAssetAccessor"
   * used to load tiles.
   * @param pCompletedRequest The failed request
   */
  static TileLoadResult createRetryLaterResult(
      std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor,
      std::shared_ptr<CesiumAsync::IAssetRequest> pCompletedRequest);
};

} // namespace Cesium3DTilesSelection
