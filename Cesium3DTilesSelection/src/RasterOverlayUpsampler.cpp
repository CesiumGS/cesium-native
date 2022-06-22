#include "RasterOverlayUpsampler.h"

namespace Cesium3DTilesSelection {
CesiumAsync::Future<TileLoadResult> RasterOverlayUpsampler::loadTileContent(
    Tile& tile,
    const TilesetContentOptions& contentOptions,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders)
{

}

bool RasterOverlayUpsampler::updateTileContent([[maybe_unused]] Tile& tile) {
  return false;
}
} // namespace Cesium3DTilesSelection
