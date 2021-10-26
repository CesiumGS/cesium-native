#pragma once

#include "Availability.h"
#include "Library.h"
#include "OctreeTileID.h"
#include "OctreeTilingScheme.h"
#include "TileAvailabilityFlags.h"

#include <gsl/span>

#include <cstddef>
#include <memory>
#include <vector>

namespace CesiumGeometry {

class CESIUMGEOMETRY_API OctreeAvailability final {
public:
  /**
   * @brief Constructs a new instance.
   *
   * @param tilingScheme The {@link OctreeTilingScheme} to use with this
   * availability.
   */
  OctreeAvailability(
      const OctreeTilingScheme& tilingScheme,
      uint32_t subtreeLevels,
      uint32_t maximumLevel) noexcept;

  /**
   * @brief Determines the currently known availability status of the given
   * tile.
   *
   * @param tileID The {@link CesiumGeometry::OctreeTileID} for the tile.
   *
   * @return The {@link TileAvailabilityFlags} for this tile encoded into a
   * uint8_t.
   */
  uint8_t computeAvailability(const OctreeTileID& tileID) const noexcept;

  /**
   * @brief Attempts to add an availability subtree into the existing overall
   * availability tree.
   *
   * @param tileID The {@link CesiumGeometry::OctreeTileID} for the tile.
   * @param newSubtree The {@link CesiumGeometry::AvailabilitySubtree} to add.
   *
   * @return Whether the insertion was successful.
   */
  bool addSubtree(
      const OctreeTileID& tileID,
      AvailabilitySubtree&& newSubtree) noexcept;

private:
  OctreeTilingScheme _tilingScheme;
  uint32_t _subtreeLevels;
  uint32_t _maximumLevel;
  uint32_t _maximumChildrenSubtrees;
  std::unique_ptr<AvailabilityNode> _pRoot;
};

} // namespace CesiumGeometry