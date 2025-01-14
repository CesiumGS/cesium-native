#pragma once

#include <CesiumGeometry/Availability.h>
#include <CesiumGeometry/Library.h>
#include <CesiumGeometry/OctreeTileID.h>
#include <CesiumGeometry/TileAvailabilityFlags.h>

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace CesiumGeometry {

/**
 * @brief An availability tree for an octree, where availability can be stored
 * and computed based on \ref OctreeTileID.
 */
class CESIUMGEOMETRY_API OctreeAvailability final {
public:
  /**
   * @brief Constructs a new instance.
   *
   * @param subtreeLevels The number of levels in each subtree.
   * @param maximumLevel The index of the maximum level in this tileset.
   */
  OctreeAvailability(uint32_t subtreeLevels, uint32_t maximumLevel) noexcept;

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
   * @brief Determines the currently known availability status of the given
   * tile.
   *
   * Priming with a known parent subtree node avoids the need to traverse the
   * entire availability tree so far. The node must have a loaded subtree
   *
   * @param tileID The tile ID to get the availability for.
   * @param pNode The subtree node to look for the tileID in. The tileID should
   * be within this subtree node.
   *
   * @return The {@link TileAvailabilityFlags} for this tile encoded into a
   * uint8_t.
   */
  uint8_t computeAvailability(
      const OctreeTileID& tileID,
      const AvailabilityNode* pNode) const noexcept;

  /**
   * @brief Attempts to add a child subtree node onto the given parent node.
   *
   * Priming with a known parent subtree node avoids the need to traverse the
   * entire availability tree so far. If the parent node is nullptr and the
   * tile ID indicates this is the root tile, the subtree will be attached to
   * the root.
   *
   * @param tileID The root tile's ID of the subtree we are trying to add.
   * @param pParentNode The parent subtree node. The tileID should fall exactly
   * at the end of this parent subtree.
   *
   * @return The newly created node if the insertion was successful, nullptr
   * otherwise.
   */
  AvailabilityNode*
  addNode(const OctreeTileID& tileID, AvailabilityNode* pParentNode) noexcept;

  /**
   * @brief Attempts to add a loaded subtree onto the given node.
   *
   * The node must have been created earlier from a call to addNode.
   *
   * @param pNode The node on which to add the subtree.
   * @param newSubtree The new subtree to add.
   *
   * @return Whether the insertion was successful.
   */
  bool addLoadedSubtree(
      AvailabilityNode* pNode,
      AvailabilitySubtree&& newSubtree) noexcept;
  /**
   * @brief Find the child node index corresponding to this tile ID and parent
   * node.
   *
   * Attempts to find the child node for the tile with the given ID and parent
   * node. The parent node is used to speed up the search significantly. Note
   * that if the given tile ID does not correspond exactly to an immediate
   * child node of the parent node, nullptr will be returned. If a tileID
   * outside the given parent node's subtree is given, an incorrect child index
   * may be returned.
   *
   * @param tileID The tile ID of the child node we are looking for.
   * @param pParentNode The immediate parent to the child node we are looking
   * for.
   * @return The child node index if found, std::nullopt otherwise.
   */
  std::optional<uint32_t> findChildNodeIndex(
      const OctreeTileID& tileID,
      const AvailabilityNode* pParentNode) const;

  /**
   * @brief Find the child node corresponding to this tile ID and parent node.
   *
   * Attempts to find the child node for the tile with the given ID and parent
   * node. The parent node is used to speed up the search significantly. Note
   * that if the given tile ID does not correspond exactly to an immediate
   * child node of the parent node, nullptr will be returned. If a tileID
   * outside the given parent node's subtree is given, an incorrect child index
   * may be returned.
   *
   * @param tileID The tile ID of the child node we are looking for.
   * @param pParentNode The immediate parent to the child node we are looking
   * for.
   * @return The child node if found, nullptr otherwise.
   */
  AvailabilityNode* findChildNode(
      const OctreeTileID& tileID,
      AvailabilityNode* pParentNode) const;

  /**
   * @brief Gets the number of levels in each subtree.
   *
   * @returns The number of levels in each subtree.
   */
  constexpr inline uint32_t getSubtreeLevels() const noexcept {
    return this->_subtreeLevels;
  }

  /**
   * @brief Gets the index of the maximum level in this implicit tileset.
   *
   * @returns The index of the maximum level.
   */
  constexpr inline uint32_t getMaximumLevel() const noexcept {
    return this->_maximumLevel;
  }

  /**
   * @brief Gets a pointer to the root subtree node of this implicit tileset.
   *
   * @returns The root node of the availability tree.
   */
  AvailabilityNode* getRootNode() noexcept { return this->_pRoot.get(); }

private:
  uint32_t _subtreeLevels;
  uint32_t _maximumLevel;
  uint32_t _maximumChildrenSubtrees;
  std::unique_ptr<AvailabilityNode> _pRoot;
};

} // namespace CesiumGeometry
