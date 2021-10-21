#pragma once

#include "BoundingVolume.h"
#include "Library.h"
#include "Tile.h"
#include "TileContext.h"
#include "TileID.h"
#include "TileRefine.h"
#include "TilesetOptions.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>

#include <gsl/span>
#include <spdlog/fwd.h>

#include <cstddef>
#include <memory>

namespace Cesium3DTilesSelection {
/**
 * @brief The information that is passed to a {@link TileContentLoader} to
 * create a {@link TileContentLoadResult}.
 *
 * For many types of tile content, only the `pRequest` field is required. The
 * other members are used for content that can generate child tiles, like
 * external tilesets or composite tiles. These members are usually initialized
 * from
 * the corresponding members of the {@link Tile} that the content belongs to.
 */
struct CESIUM3DTILESSELECTION_API TileContentLoadInput {

  /**
   * @brief Creates a new, uninitialized instance for the given tile.
   *
   * The `asyncSystem`, `pLogger`, `pAssetAccessor` and `pRequest` will have
   * default values, and have to be initialized before this instance is passed
   * to one of the loader functions.
   *
   * @param tile The {@link Tile} that the content belongs to
   */
  TileContentLoadInput(const Tile& tile);

  /**
   * @brief Creates a new instance
   *
   * @param asyncSystem The async system to use for tile content loading.
   * @param pLogger The logger that will be used
   * @param pAssetAccessor The asset accessor to make further requests with.
   * @param pRequest The original tile request and its response.
   * @param pSubtreeRequest The original subtree request and its response.
   * @param tile The {@link Tile} that the content belongs to.
   */
  TileContentLoadInput(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumAsync::IAssetRequest>& pRequest,
      const std::shared_ptr<CesiumAsync::IAssetRequest>& pSubtreeRequest,
      const Tile& tile);

  /**
   * @brief Creates a new instance.
   *
   * @param asyncSystem The async system to use for tile content loading.
   * @param pLogger The logger that will be used
   * @param pAssetAccessor The asset accessor to make further requests with.
   * @param pRequest The original tile request and its response.
   * @param pSubtreeRequest The original subtree request and its response.
   * @param tileID The {@link TileID}
   * @param tileBoundingVolume The tile {@link BoundingVolume}
   * @param tileContentBoundingVolume The tile content {@link BoundingVolume}
   * @param tileRefine The {@link TileRefine} strategy
   * @param tileGeometricError The geometric error of the tile
   * @param tileTransform The tile transform
   */
  TileContentLoadInput(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumAsync::IAssetRequest>& pRequest,
      const std::shared_ptr<CesiumAsync::IAssetRequest>& pSubtreeRequest,
      const TileID& tileID,
      const BoundingVolume& tileBoundingVolume,
      const std::optional<BoundingVolume>& tileContentBoundingVolume,
      TileRefine tileRefine,
      double tileGeometricError,
      const glm::dmat4& tileTransform,
      const TilesetContentOptions& contentOptions);

  /**
   * @brief The async system to use for tile content loading.
   */
  CesiumAsync::AsyncSystem asyncSystem;

  /**
   * @brief The logger that receives details of loading errors and warnings.
   */
  std::shared_ptr<spdlog::logger> pLogger;

  /**
   * @brief The asset accessor to make further requests with.
   */
  std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor;

  /**
   * @brief The content asset request and response data for the tile.
   */
  std::shared_ptr<CesiumAsync::IAssetRequest> pRequest;

  /**
   * @brief The subtree asset request and response data for the tile.
   *
   * Only applicable if implicit tiling is used and this tile is the root of an
   * availability subtree.
   */
  std::shared_ptr<CesiumAsync::IAssetRequest> pSubtreeRequest;

  /**
   * @brief The {@link TileID}.
   */
  TileID tileID;

  /**
   * @brief The tile {@link BoundingVolume}.
   */
  BoundingVolume tileBoundingVolume;

  /**
   * @brief Tile content {@link BoundingVolume}.
   */
  std::optional<BoundingVolume> tileContentBoundingVolume;

  /**
   * @brief The {@link TileRefine}.
   */
  TileRefine tileRefine;

  /**
   * @brief The geometric error.
   */
  double tileGeometricError;

  /**
   * @brief The tile transform
   */
  glm::dmat4 tileTransform;

  /**
   * @brief Options for parsing content and creating Gltf models.
   */
  TilesetContentOptions contentOptions;
};
} // namespace Cesium3DTilesSelection
