#pragma once

#include "CesiumGeometry/Library.h"
#include "CesiumGeometry/TileAvailabilityFlags.h"
#include "CesiumGeometry/QuadtreeTileID.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include <cstddef>
#include <gsl/span>
#include <memory>
#include <vector>

namespace CesiumGeometry {

class CESIUMGEOMETRY_API QuadtreeSubtreeAvailability final {
public:
  struct Subtree {
    uint32_t levels;
    uint32_t maximumLevel;
    std::vector<std::byte> bitstream;
    gsl::span<const std::byte> tileAvailability;
    gsl::span<const std::byte> contentAvailability;
    gsl::span<const std::byte> subtreeAvailability;

    Subtree(void* data, uint32_t levels, uint32_t maximumLevel);
  };

  /**
   * @brief Constructs a new instance.
   * 
   * @param tilingScheme The {@link QuadtreeTilingScheme} to use with this
   * availability.
   */
  QuadtreeSubtreeAvailability(
      const QuadtreeTilingScheme& tilingScheme) noexcept;

  /**
   * @brief Determines the currently known availability status of the given 
   * tile.
   * 
   * @param tileID The {@link CesiumGeometry::QuadtreeTileID} for the tile.
   * 
   * @return The {@link TileAvailabilityFlags} for this tile encoded into a 
   * uint8_t.
   */
  uint8_t computeAvailability(const QuadtreeTileID& tileID) const noexcept;
  
  /**
   * @brief Attempts to add an availability subtree into the existing overall
   * availability tree.
   * 
   * @param tileID The {@link CesiumGeometry::QuadtreeTileID} for the tile.
   * 
   * @return Whether the insertion was successful.
   */
  bool addSubtree(const QuadtreeTileID& tileID, Subtree&& subtree) noexcept;

private:
  struct Node {
    Subtree subtree;
    std::vector<std::unique_ptr<Node>> childNodes;

    Node(Subtree&& subtree_);
  };

  QuadtreeTilingScheme _tilingScheme;
  uint32_t _maximumLevel;
  std::unique_ptr<Node> _pRoot;
};

} // namespace CesiumGeometry