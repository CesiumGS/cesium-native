#pragma once

#include "AvailabilityTree.h"
#include "Library.h"
#include "QuadtreeTileID.h"
#include "QuadtreeTilingScheme.h"
#include "TileAvailabilityFlags.h"

#include <gsl/span>

#include <cstddef>
#include <memory>
#include <vector>

namespace CesiumGeometry {

class CESIUMGEOMETRY_API QuadtreeAvailability final {
public:
  /**
   * @brief Constructs a new instance.
   *
   * @param tilingScheme The {@link QuadtreeTilingScheme} to use with this
   * availability.
   */
  QuadtreeAvailability(
      const QuadtreeTilingScheme& tilingScheme,
      uint32_t subtreeLevels,
      uint32_t maximumLevel) noexcept;

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
   * @param newSubtree The {@link CesiumGeometry::AvailabilitySubtree} to add.
   *
   * @return Whether the insertion was successful.
   */
  bool addSubtree(
      const QuadtreeTileID& tileID,
      AvailabilitySubtree&& newSubtree) noexcept;

private:
  QuadtreeTilingScheme _tilingScheme;
  uint32_t _subtreeLevels;
  uint32_t _maximumLevel;
  uint32_t _maximumChildrenSubtrees;
  std::unique_ptr<AvailabilityNode> _pRoot;
};

} // namespace CesiumGeometry