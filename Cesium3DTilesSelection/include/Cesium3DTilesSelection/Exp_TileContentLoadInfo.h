#pragma once

#include "BoundingVolume.h"
#include "Tile.h"
#include "TileID.h"
#include "TileRefine.h"
#include "TilesetOptions.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>

#include <gsl/span>
#include <spdlog/fwd.h>

#include <cstddef>
#include <memory>

namespace Cesium3DTilesSelection {
struct TileContentLoadInfo {
  TileContentLoadInfo(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const TilesetContentOptions& contentOptions,
      const Tile& tile);

  CesiumAsync::AsyncSystem asyncSystem;

  std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor;

  std::shared_ptr<spdlog::logger> pLogger;

  TileID tileID;

  BoundingVolume tileBoundingVolume;

  std::optional<BoundingVolume> tileContentBoundingVolume;

  TileRefine tileRefine;

  double tileGeometricError;

  glm::dmat4 tileTransform;

  TilesetContentOptions contentOptions;
};
} // namespace Cesium3DTilesSelection
