#include <Cesium3DTilesSelection/Exp_ImplicitTilesetGenerator.h>

namespace Cesium3DTilesSelection {
OctreeSubtree::OctreeSubtree(uint32_t subtreeLevels) {
  uint64_t numOfTiles = ((1 << (3 * (subtreeLevels + 1))) - 1) / (8 - 1);

  _tileAvailable.resize(numOfTiles / 8);
  _contentAvailable.resize(numOfTiles / 8);
  _subtreeAvailable.resize(1 << (3 * (subtreeLevels)));
}

Octree::Octree(uint64_t maxLevels, uint32_t subtreeLevels)
    : _maxLevels{maxLevels}, _subtreeLevels{subtreeLevels} {}

void Octree::markTileContent(const CesiumGeometry::OctreeTileID& octreeID) {}

void Octree::markTileEmptyContent(
    const CesiumGeometry::OctreeTileID& octreeID) {}
} // namespace Cesium3DTilesSelection
