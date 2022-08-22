#pragma once

#include "BoundingVolume.h"
#include "Library.h"
#include "RasterOverlayDetails.h"
#include "TileContent.h"
#include "TilesetOptions.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGltf/Model.h>

#include <spdlog/logger.h>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace Cesium3DTilesSelection {
class Tile;

/**
 * @brief Indicate the status of {@link TilesetContentLoader::loadTileContent} and
 * {@link TilesetContentLoader::createTileChildren} operations
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
 * @brief Store the parameters that are needed to load a tile
 */
struct CESIUM3DTILESSELECTION_API TileLoadInput {
  /**
   * @brief Creates a new instance
   *
   * @param tile The {@link Tile} that the content belongs to.
   * @param contentOptions The content options the {@link TilesetContentLoader} will use to process the content of the tile.
   * @param asyncSystem The async system to use for tile content loading.
   * @param pAssetAccessor The asset accessor to make further requests with.
   * @param pLogger The logger that will be used
   * @param requestHeaders The request headers that will be attached to the
   * request.
   */
  TileLoadInput(
      const Tile& tile,
      const TilesetContentOptions& contentOptions,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders);

  /**
   * @brief The tile that the {@link TilesetContentLoader} will request the server for the content.
   */
  const Tile& tile;

  /**
   * @brief The content options the {@link TilesetContentLoader} will use to process the content of the tile.
   */
  const TilesetContentOptions& contentOptions;

  /**
   * @brief The async system to run the loading in worker thread or main thread.
   */
  const CesiumAsync::AsyncSystem& asyncSystem;

  /**
   * @brief The asset accessor to make requests for the tile content over the
   * wire.
   */
  const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor;

  /**
   * @brief The logger that receives details of loading errors and warnings.
   */
  const std::shared_ptr<spdlog::logger>& pLogger;

  /**
   * @brief The request headers that will be attached to the request.
   */
  const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders;
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
  std::optional<RasterOverlayDetails> rasterOverlayDetails;

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
   * RetryLater,
   * __none__ of the fields above (including {@link TileLoadResult::tileInitializer}) will be
   * applied to a tile when the loading is finished
   */
  TileLoadResultState state;
};

/**
 * @brief Store the result of creating tile's children after
 * invoking {@link TilesetContentLoader::createTileChildren}
 */
struct CESIUM3DTILESSELECTION_API TileChildrenResult {
  /**
   * @brief The children of this tile.
   */
  std::vector<Tile> children;

  /**
   * @brief The result of creating children for
   * this tile. Note that: when receiving RetryLater status, client
   * needs to load this tile's content first or its parent's content. The reason
   * is that some tileset formats store the tile's children along with its
   * content or store a whole subtree for every n-th level tile (e.g Quantized
   * Mesh format). So unless the tile's content or the root tile of the subtree
   * the tile is in is loaded, the loader won't know how to create the tile's
   * children.
   */
  TileLoadResultState state;
};

/**
 * @brief The loader interface to load the tile content
 */
class CESIUM3DTILESSELECTION_API TilesetContentLoader {
public:
  /**
   * @brief Default virtual destructor
   */
  virtual ~TilesetContentLoader() = default;

  /**
   * @brief Load the tile content.
   *
   * @param input The {@link TileLoadInput} that has the tile info and loading systems to load this tile's content
   * @return The future of {@link TileLoadResult} that stores the tile's content
   */
  virtual CesiumAsync::Future<TileLoadResult>
  loadTileContent(const TileLoadInput& input) = 0;

  /**
   * @brief Create the tile's children.
   *
   * Note that: when receiving RetryLater status, client
   * needs to load this tile's content first or its parent's content. The reason
   * is that some tileset formats store the tile's children along with its
   * content or store a whole subtree for every n-th level tile (e.g Quantized
   * Mesh format). So unless the tile's content or the root tile of the subtree
   * the tile is in is loaded, the loader won't know how to create the tile's
   * children.
   *
   * @param tile The tile to create children for.
   * @return The {@link TileChildrenResult} that stores the tile's children
   */
  virtual TileChildrenResult createTileChildren(const Tile& tile) = 0;

  virtual CesiumGeometry::Axis
  getTileUpAxis(const Tile& tile) const noexcept = 0;
};
} // namespace Cesium3DTilesSelection
