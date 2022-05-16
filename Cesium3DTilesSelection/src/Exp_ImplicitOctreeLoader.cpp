#include <Cesium3DTilesSelection/Exp_ImplicitOctreeLoader.h>

namespace Cesium3DTilesSelection {
CesiumAsync::Future<TileLoadResult> ImplicitOctreeLoader::loadTileContent(
    TilesetContentLoader& currentLoader,
    const TileContentLoadInfo& loadInfo,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders) {
  (void)(currentLoader);
  (void)(requestHeaders);
  return loadInfo.asyncSystem.createResolvedFuture<TileLoadResult>(
      TileLoadResult{});
}
} // namespace Cesium3DTilesSelection
