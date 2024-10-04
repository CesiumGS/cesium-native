#include "TileContentLoadInfo.h"

#include <Cesium3DTilesSelection/TilesetContentLoader.h>

namespace Cesium3DTilesSelection {
TileContentLoadInfo::TileContentLoadInfo(
    const CesiumAsync::AsyncSystem& asyncSystem_,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor_,
    const std::shared_ptr<IPrepareRendererResources>&
        pPrepareRendererResources_,
    const std::shared_ptr<spdlog::logger>& pLogger_,
    const CesiumUtility::IntrusivePointer<CesiumGltf::SharedAssetDepots>
        pAssetDepot_,
    const TilesetContentOptions& contentOptions_,
    const Tile& tile)
    : asyncSystem(asyncSystem_),
      pAssetAccessor(pAssetAccessor_),
      pLogger(pLogger_),
      pPrepareRendererResources{pPrepareRendererResources_},
      tileID(tile.getTileID()),
      tileBoundingVolume(tile.getBoundingVolume()),
      tileContentBoundingVolume(tile.getContentBoundingVolume()),
      pAssetDepot{pAssetDepot_},
      tileRefine(tile.getRefine()),
      tileGeometricError(tile.getGeometricError()),
      tileTransform(tile.getTransform()),
      contentOptions(contentOptions_) {}
} // namespace Cesium3DTilesSelection
