#pragma once

#include "BoundingVolume.h"
#include "Library.h"
#include "TileContent.h"
#include "TileLoadResult.h"
#include "TilesetOptions.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGltf/Model.h>
#include <CesiumRasterOverlays/RasterOverlayDetails.h>

#include <spdlog/logger.h>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace Cesium3DTilesSelection {
class Tile;

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
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

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

  /**
   * @brief The ellipsoid that this tileset uses.
   */
  const CesiumGeospatial::Ellipsoid& ellipsoid;
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
  virtual TileChildrenResult createTileChildren(
      const Tile& tile,
      const CesiumGeospatial::Ellipsoid& ellipsoid
          CESIUM_DEFAULT_ELLIPSOID) = 0;
};
} // namespace Cesium3DTilesSelection
