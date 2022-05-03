#include <Cesium3DTilesSelection/Exp_ImplicitQuadtreeLoader.h>
#include <variant>

namespace Cesium3DTilesSelection {
CesiumAsync::Future<TileLoadResult> ImplicitQuadtreeLoader::loadTileContent(
    TilesetContentLoader& currentLoader,
    const TileContentLoadInfo& loadInfo,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders) {
  const CesiumGeometry::QuadtreeTileID* quadtreeID =
      std::get_if<CesiumGeometry::QuadtreeTileID>(&loadInfo.tileID);
  assert(
      quadtreeID != nullptr && "Implicit Quadtree Loader cannot support tile "
                               "that does not have quadtree ID");

  // check that the subtree for this tile is loaded. If not, we load the subtree, then load the tile content
  uint32_t subtreeLevelIdx = quadtreeID->level / _subtreeLevels;
  if (subtreeLevelIdx >= _loadedSubtrees.size()) {
    return loadInfo.asyncSystem.createResolvedFuture<TileLoadResult>(
        TileLoadResult{
            TileUnknownContent{},
            TileLoadResultState::Failed,
            nullptr,
            {}});
  }

  uint64_t subtreeMortonIdx = 0; // TODO: calculate morton idx
  auto subtreeIt = _loadedSubtrees[subtreeLevelIdx].find(subtreeMortonIdx);
  if (subtreeIt == _loadedSubtrees[subtreeLevelIdx].end()) {
    // the subtree is not loaded, so load it now, then load the tile content
  }
}
} // namespace Cesium3DTilesSelection
