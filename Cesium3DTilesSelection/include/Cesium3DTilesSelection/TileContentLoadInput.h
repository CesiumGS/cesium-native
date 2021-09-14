#pragma once

#include "Cesium3DTilesSelection/BoundingVolume.h"
#include "Cesium3DTilesSelection/Library.h"
#include "Cesium3DTilesSelection/Tile.h"
#include "Cesium3DTilesSelection/TileContext.h"
#include "Cesium3DTilesSelection/TileID.h"
#include "Cesium3DTilesSelection/TileRefine.h"
#include "Cesium3DTilesSelection/TilesetOptions.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/IAssetRequest.h"

#include <gsl/span>
#include <spdlog/fwd.h>

#include <cstddef>
#include <memory>

namespace Cesium3DTilesSelection {
/**
 * @brief The information that is passed to a {@link TileContentLoader} to
 * create a {@link TileContentLoadResult}.
 *
 * For many types of tile content, only the `data` field is required. The other
 * members are used for content that can generate child tiles, like external
 * tilesets or composite tiles. These members are usually initialized from
 * the corresponding members of the {@link Tile} that the content belongs to.
 */
struct CESIUM3DTILESSELECTION_API TileContentLoadInput {

  /**
   * @brief Creates a new, uninitialized instance for the given tile.
   *
   * The `data`, `contentType` and `url` will have default values,
   * and have to be initialized before this instance is passed to
   * one of the loader functions.
   *
   * @param asyncSystem The async system to use for tile content loading.
   * @param pLogger The logger that will be used
   * @param pAssetAccessor The asset accessor to make further requests with.
   * @param pRequest The original tile request and its response.
   * @param data The content data to use. The response data will be used
   * instead if this is left as `std::nullopt`.
   * @param tile The {@link Tile} that the content belongs to.
   */
  TileContentLoadInput(
      const CesiumAsync::AsyncSystem& asyncSystem,
      std::shared_ptr<spdlog::logger>&& pLogger,
      std::shared_ptr<CesiumAsync::IAssetAccessor>&& pAssetAccessor,
      std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest,
      const std::optional<gsl::span<const std::byte>>& data,
      const Tile& tile);

  /**
   * @brief Creates a new instance.
   *
   * For many types of tile content, only the `data` field is required. The
   * other parameters are used for content that can generate child tiles, like
   * external tilesets or composite tiles.
   *
   * @param asyncSystem The async system to use for tile content loading.
   * @param pLogger The logger that will be used
   * @param pAssetAccessor The asset accessor to make further requests with.
   * @param pRequest The original tile request and its response.
   * @param data The content data to use. The response data will be used
   * instead if this is left as `std::nullopt`.
   * @param tileID The {@link TileID}
   * @param tileBoundingVolume The tile {@link BoundingVolume}
   * @param tileContentBoundingVolume The tile content {@link BoundingVolume}
   * @param tileRefine The {@link TileRefine} strategy
   * @param tileGeometricError The geometric error of the tile
   * @param tileTransform The tile transform
   */
  TileContentLoadInput(
      const CesiumAsync::AsyncSystem& asyncSystem,
      std::shared_ptr<spdlog::logger>&& pLogger,
      std::shared_ptr<CesiumAsync::IAssetAccessor>&& pAssetAccessor,
      std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest,
      const std::optional<gsl::span<const std::byte>>& data,
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
   * @brief The asset request and response data for the tile.
   */
  std::shared_ptr<CesiumAsync::IAssetRequest> pRequest;

  /**
   * @brief The content data to use.
   */
  gsl::span<const std::byte> data;

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
