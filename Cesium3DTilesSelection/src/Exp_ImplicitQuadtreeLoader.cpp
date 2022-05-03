#include <Cesium3DTilesSelection/Exp_ImplicitQuadtreeLoader.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <libmorton/morton.h>
#include <variant>
#include <type_traits>

namespace Cesium3DTilesSelection {
CesiumAsync::Future<TileLoadResult> ImplicitQuadtreeLoader::loadTileContent(
    TilesetContentLoader& currentLoader,
    const TileContentLoadInfo& loadInfo,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders) {
  // Ensure CesiumGeometry::QuadtreeTileID only has 32-bit components. There are
  // solutions below if the ID has more than 32-bit components.
  static_assert(
      std::is_same_v<
          decltype(std::declval<CesiumGeometry::QuadtreeTileID>().x),
          uint32_t>,
      "This loader cannot support more than 32-bit ID");
  static_assert(
      std::is_same_v<
          decltype(std::declval<CesiumGeometry::QuadtreeTileID>().y),
          uint32_t>,
      "This loader cannot support more than 32-bit ID");
  static_assert(
      std::is_same_v<
          decltype(std::declval<CesiumGeometry::QuadtreeTileID>().level),
          uint32_t>,
      "This loader cannot support more than 32-bit ID");

  // make sure the tile is a quadtree tile
  const CesiumGeometry::QuadtreeTileID* quadtreeID =
      std::get_if<CesiumGeometry::QuadtreeTileID>(&loadInfo.tileID);
  if (!quadtreeID) {
    return loadInfo.asyncSystem.createResolvedFuture<TileLoadResult>(
        TileLoadResult{
            TileUnknownContent{},
            TileLoadResultState::Failed,
            nullptr,
            {}});
  }

  // check that the subtree for this tile is loaded. If not, we load the
  // subtree, then load the tile content
  size_t subtreeLevelIdx = quadtreeID->level / _subtreeLevels;
  if (subtreeLevelIdx >= _loadedSubtrees.size()) {
    return loadInfo.asyncSystem.createResolvedFuture<TileLoadResult>(
        TileLoadResult{
            TileUnknownContent{},
            TileLoadResultState::Failed,
            nullptr,
            {}});
  }

  // the below morton index hash to the subtree assumes that tileID's components
  // x and y never exceed 32-bit. In other words, the max levels this loader can
  // support is 33 which will have 4^32 tiles in the level 32th. The 64-bit
  // morton index below can support that much tiles without overflow. More than
  // 33 levels, this loader will fail. One solution for that is to create
  // multiple new ImplicitQuadtreeLoaders and assign them to any tiles that have
  // levels exceeding the maximum 33. Those new loaders will be added to the
  // current loader, and thus, create a hierarchical tree of loaders where each
  // loader will serve up to 33 levels with the level 0 being relative to the
  // parent loader. The solution isn't implemented at the moment, as implicit
  // tilesets that exceeds 33 levels are expected to be very rare
  uint64_t subtreeMortonIdx =
      libmorton::morton2D_64_encode(quadtreeID->x, quadtreeID->y);
  auto subtreeIt = _loadedSubtrees[subtreeLevelIdx].find(subtreeMortonIdx);
  if (subtreeIt == _loadedSubtrees[subtreeLevelIdx].end()) {
    // the subtree is not loaded, so load it now, then load the tile content
  }
}
} // namespace Cesium3DTilesSelection
