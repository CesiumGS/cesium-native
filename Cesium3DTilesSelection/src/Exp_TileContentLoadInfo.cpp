#include <Cesium3DTilesSelection/Exp_TileContentLoadInfo.h>

namespace Cesium3DTilesSelection {
TileContentLoadInfo::TileContentLoadInfo(
    const CesiumAsync::AsyncSystem& asyncSystem_,
    const std::shared_ptr<spdlog::logger>& pLogger_,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor_,
    const TilesetContentOptions& contentOptions_,
    const Tile& tile)
    : asyncSystem(asyncSystem_),
      pLogger(pLogger_),
      pAssetAccessor(pAssetAccessor_),
      tileID(tile.getTileID()),
      tileBoundingVolume(tile.getBoundingVolume()),
      tileContentBoundingVolume(tile.getContentBoundingVolume()),
      tileRefine(tile.getRefine()),
      tileGeometricError(tile.getGeometricError()),
      tileTransform(tile.getTransform()),
      contentOptions(contentOptions_) {}
} // namespace Cesium3DTilesSelection
