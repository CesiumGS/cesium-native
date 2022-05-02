#include <Cesium3DTilesSelection/Exp_ImplicitQuadtreeLoader.h>

namespace Cesium3DTilesSelection {
CesiumAsync::Future<TileLoadResult> ImplicitQuadtreeLoader::loadTileContent(
    [[maybe_unused]] TilesetContentLoader& currentLoader,
    [[maybe_unused]] const TileContentLoadInfo& loadInfo,
    [[maybe_unused]] const std::vector<CesiumAsync::IAssetAccessor::THeader>&
        requestHeaders) {
  return loadInfo.asyncSystem.createResolvedFuture<TileLoadResult>(
      TileLoadResult{});
}
} // namespace Cesium3DTilesSelection
