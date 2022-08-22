#pragma once

#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/IPrepareRendererResources.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileID.h>
#include <Cesium3DTilesSelection/TileRefine.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/Axis.h>

#include <gsl/span>
#include <spdlog/fwd.h>

#include <cstddef>
#include <memory>

namespace Cesium3DTilesSelection {
struct TileContentLoadInfo {
  TileContentLoadInfo(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const TilesetContentOptions& contentOptions,
      const Tile& tile);

  CesiumAsync::AsyncSystem asyncSystem;

  std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor;

  std::shared_ptr<spdlog::logger> pLogger;

  std::shared_ptr<IPrepareRendererResources> pPrepareRendererResources;

  TileID tileID;

  BoundingVolume tileBoundingVolume;

  std::optional<BoundingVolume> tileContentBoundingVolume;

  TileRefine tileRefine;

  double tileGeometricError;

  glm::dmat4 tileTransform;

  TilesetContentOptions contentOptions;
};
} // namespace Cesium3DTilesSelection
