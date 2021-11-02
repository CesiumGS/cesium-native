#pragma once

#include "Availability.h"
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

  /**
   * @brief Find the child node corresponding to this tile ID and parent node.
   *
   * Attempts to find the child node for the tile with the given ID and parent
   * node. The parent node is used to speed up the search significantly. Note
   * that if the given tile ID does not correspond exactly to an immediate
   * child node of the parent node, nullptr will be returned. If a tileID
   * outside the given parent node's subtree is given, an incorrect child node
   * may be returned.
   *
   * @param tileID The tile ID of the child node we are looking for.
   * @param pParentNode The immediate parent to the child node we are looking
   * for.
   * @return The child node if found, nullptr otherwise.
   */
  const AvailabilityNode* findChildNode(
      const QuadtreeTileID& tileID,
      const AvailabilityNode* pParentNode) const;

  /**
   * @brief Gets the number of levels in each subtree.
   */
  constexpr inline uint32_t getSubtreeLevels() const noexcept {
    return this->_subtreeLevels;
  }

  /**
   * @brief Gets the index of the maximum level in this implicit tileset.
   */
  constexpr inline uint32_t getMaximumLevel() const noexcept {
    return this->_maximumLevel;
  }

  /**
   * @brief Gets a pointer to the root subtree node of this implicit tileset.
   */
  const AvailabilityNode* getRootNode() const noexcept {
    return this->_pRoot.get();
  }

private:
  QuadtreeTilingScheme _tilingScheme;
  uint32_t _subtreeLevels;
  uint32_t _maximumLevel;
  uint32_t _maximumChildrenSubtrees;
  std::unique_ptr<AvailabilityNode> _pRoot;
};

} // namespace CesiumGeometry