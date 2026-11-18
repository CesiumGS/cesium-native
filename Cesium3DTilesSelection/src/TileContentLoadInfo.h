#pragma once

#include <Cesium3DTiles/BoundingVolume.h>
#include <Cesium3DTiles/TileRefine.h>
#include <Cesium3DTilesSelection/IPrepareRendererResources.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileID.h>
#include <Cesium3DTilesSelection/TilesetContentOptions.h>
#include <Cesium3DTilesSelection/TilesetSharedAssetSystem.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/Axis.h>

#include <spdlog/fwd.h>

#include <cstddef>
#include <memory>
#include <span>

using namespace Cesium3DTiles;

namespace Cesium3DTilesSelection {
struct TileContentLoadInfo {
  TileContentLoadInfo(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumUtility::IntrusivePointer<TilesetSharedAssetSystem>&
          pSharedAssetSystem,
      const TilesetContentOptions& contentOptions,
      const Tile& tile);

  CesiumAsync::AsyncSystem asyncSystem;

  std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor;

  std::shared_ptr<spdlog::logger> pLogger;

  std::shared_ptr<IPrepareRendererResources> pPrepareRendererResources;

  TileID tileID;

  BoundingVolume tileBoundingVolume;

  std::optional<BoundingVolume> tileContentBoundingVolume;
  CesiumUtility::IntrusivePointer<TilesetSharedAssetSystem> pSharedAssetSystem;

  TileRefine tileRefine;

  double tileGeometricError;

  glm::dmat4 tileTransform;

  TilesetContentOptions contentOptions;
};
} // namespace Cesium3DTilesSelection
