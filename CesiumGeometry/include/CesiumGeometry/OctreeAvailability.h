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

  /**
   * @brief Attempts to add a child subtree onto the given parent subtree.
   *
   * Priming with a known parent subtree node avoids the need to traverse the
   * entire availability tree so far. If the parent node is nullptr and the
   * tile ID indicates this is the root tile, the subtree will be attached to
   * the root.
   *
   * @param tileID The root tile's ID of the subtree we are trying to add.
   * @param pParentNode The parent subtree node. The tileID should fall exactly
   * at the end of this parent subtree.
   * @param newSubtree The new subtree to add to the overall availability tree.
   *
   * @return Whether the insertion was successful.
   */
  bool addSubtree(
      const OctreeTileID& tileID,
      AvailabilityNode* pParentNode,
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
  AvailabilityNode*
  findChildNode(const OctreeTileID& tileID, AvailabilityNode* pParentNode);

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
  AvailabilityNode* getRootNode() noexcept { return this->_pRoot.get(); }

private:
  OctreeTilingScheme _tilingScheme;
  uint32_t _subtreeLevels;
  uint32_t _maximumLevel;
  uint32_t _maximumChildrenSubtrees;
  std::unique_ptr<AvailabilityNode> _pRoot;
};

} // namespace CesiumGeometry