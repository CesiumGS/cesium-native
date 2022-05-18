#pragma once

#include <CesiumGeometry/OctreeTileID.h>
#include <cstddef>
#include <vector>

namespace Cesium3DTilesSelection {
class OctreeSubtree {
public:
  OctreeSubtree(uint32_t subtreeLevels);

  void
  markTileContent(uint32_t relativeTileLevel, uint64_t relativeTileMortonId);

  void markTileEmptyContent(
      uint32_t relativeTileLevel,
      uint64_t relativeTileMortonId);

private:
  std::vector<std::byte> _tileAvailable;
  std::vector<std::byte> _contentAvailable;
  std::vector<std::byte> _subtreeAvailable;
};

class Octree {
public:
  Octree(uint64_t maxLevels, uint32_t subtreeLevels);

  void markTileContent(const CesiumGeometry::OctreeTileID &octreeID);

  void markTileEmptyContent(const CesiumGeometry::OctreeTileID &octreeID);

private:
  uint64_t _maxLevels;
  uint32_t _subtreeLevels;
};
} // namespace Cesium3DTilesSelection
