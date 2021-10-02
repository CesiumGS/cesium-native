#pragma once

#include "CesiumGeometry/Library.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeometry/QuadtreeTileID.h"
#include <vector>
#include <memory>

namespace CesiumGeometry {

class CESIUMGEOMETRY_API QuadtreeSubtreeAvailability final {
public:
  struct Subtree {
    uint32_t tileAvailability;
    uint32_t contentAvailability;
    uint64_t subtreeAvailability;
  };

  QuadtreeSubtreeAvailability(
      const QuadtreeTilingScheme& tilingScheme, 
      uint32_t maximumLevel) noexcept;

  bool isTileAvailable(const QuadtreeTileID& tileID) const noexcept;

  void addSubtree(
      const QuadtreeTileID& tileID, 
      const Subtree& subtree) noexcept;

private:
  struct Node {
    Subtree subtree;
    std::vector<std::unique_ptr<Node>> childNodes;
  };

  QuadtreeTilingScheme _tilingScheme;
  uint32_t _maximumLevel;
  std::unique_ptr<Node> _pRoot;
};

} // namespace CesiumGeometry