#pragma once

#include "Tile.h"

#include <CesiumGeometry/Availability.h>
#include <CesiumGeometry/OctreeTileID.h>
#include <CesiumGeometry/QuadtreeTileID.h>

#include <cstdint>
#include <optional>

namespace Cesium3DTilesSelection {

/**
 * @brief Useful implicit tiling information for traversal of the current
 * tile.
 */
struct ImplicitTraversalInfo {

  /**
   * @brief The parent subtree node to the current one.
   *
   * This is only useful when we have not yet loaded the current subtree.
   * Knowing the parent will let us easily add the new subtree once its
   * loaded.
   */
  CesiumGeometry::AvailabilityNode* pParentNode;

  /**
   * @brief The subtree that contains the current tile's availability.
   *
   * If the current tile is the root of a subtree that has not been loaded
   * yet, this will be nullptr.
   */
  CesiumGeometry::AvailabilityNode* pCurrentNode;

  /**
   * @brief The {@link CesiumGeometry::TileAvailabilityFlags} of the current
   * tile.
   */
  uint8_t availability;

  /**
   * @brief Whether this tile is using implicit quadtree tiling.
   *
   * This implies that the tile has a valid
   * {@link CesiumGeometry::QuadtreeTileID} and its tile context has a valid
   * {@link CesiumGeometry::QuadtreeAvailability} and
   * {@link CesiumGeometry::QuadtreeTilingScheme}.
   */
  bool usingImplicitQuadtreeTiling;

  /**
   * @brief Whether this tile is using implicit octree tiling.
   *
   * This implies that the tile has a valid
   * {@link CesiumGeometry::OctreeTileID} and its tile context has a valid
   * {@link CesiumGeometry::OctreeAvailability} and
   * {@link CesiumGeometry::OctreeTilingScheme}.
   */
  bool usingImplicitOctreeTiling;

  /**
   * @brief Whether we should queue up a subtree load corresponding to this
   * tile.
   *
   * This indicates that the traversal should queue up a subtree load for the
   * empty pCurrentNode this frame. If the queue decides to load this subtree,
   * we will use the tile's id and the pParentNode to create a node and kick
   * off its subtree load.
   */
  bool shouldQueueSubtreeLoad;

  /**
   * @brief Creates empty instance.
   */
  ImplicitTraversalInfo() noexcept;

  /**
   * @brief Attempt to initialize an instance for the given {@link Tile}.
   *
   * @param pTile The tile.
   * @param pParentInfo The parent tile's implicit info. If this does not exist
   * it can be assumed that the parent was not an implicit tile.
   */
  ImplicitTraversalInfo(
      Tile* pTile,
      const ImplicitTraversalInfo* pParentInfo = nullptr) noexcept;
};

namespace ImplicitTraversalUtilities {

CesiumGeometry::QuadtreeTileID getAvailabilityTile(
    const CesiumGeometry::QuadtreeTileID& tileID,
    uint32_t availabilityLevels);

void createImplicitChildrenIfNeeded(
    Tile& tile,
    const ImplicitTraversalInfo& implicitInfo);

void createImplicitQuadtreeTile(
    const TileContext* pTileContext,
    const ImplicitTilingContext& implicitContext,
    Tile& parent,
    Tile& child,
    const CesiumGeometry::QuadtreeTileID& childID,
    uint8_t availability);

void createImplicitOctreeTile(
    const ImplicitTilingContext& implicitContext,
    Tile& parent,
    Tile& child,
    const CesiumGeometry::OctreeTileID& childID,
    uint8_t availability);
} // namespace ImplicitTraversalUtilities
} // namespace Cesium3DTilesSelection
