#pragma once

#include <CesiumGeometry/Library.h>
#include <CesiumGeometry/QuadtreeTileRectangularRange.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeometry/Rectangle.h>

#include <glm/vec2.hpp>

#include <memory>
#include <vector>

namespace CesiumGeometry {

/**
 * @brief Manages information about the availability of tiles in a quadtree.
 */
class CESIUMGEOMETRY_API QuadtreeRectangleAvailability final {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param tilingScheme The {@link QuadtreeTilingScheme}.
   * @param maximumLevel The maximum level (height of the tree) for which
   * the availability should be tracked.
   */
  QuadtreeRectangleAvailability(
      const QuadtreeTilingScheme& tilingScheme,
      uint32_t maximumLevel) noexcept;

  /**
   * @brief Adds the specified range to the set of available tiles.
   *
   * @param range The {@link QuadtreeTileRectangularRange} that describes
   * the range of available tiles.
   */
  void
  addAvailableTileRange(const QuadtreeTileRectangularRange& range) noexcept;

  /**
   * @brief Computes the maximum level for the given 2D position.
   *
   * This will compute the maximum level of any available tile for
   * the given position. The position refers to the 2D space that
   * is covered by the nodes of the quadtree.
   *
   * @param position The 2D position.
   * @return The maximum level at the given position. This may be 0 if
   * the position is not covered by the quadtree at all.
   */
  uint32_t
  computeMaximumLevelAtPosition(const glm::dvec2& position) const noexcept;

  /**
   * @brief Returns whether a certain tile is available.
   *
   * This checks the availability of the tile that is described by the
   * given {@link QuadtreeTileID}, which consists of the level and
   * the x- and y- coordinates of the queried tile.
   *
   * @param id The quadtree tile ID.
   * @returns The {@link CesiumGeometry::TileAvailabilityFlags} for this tile,
   * encoded into an uint8_t.
   */
  uint8_t isTileAvailable(const QuadtreeTileID& id) const noexcept;

private:
  struct RectangleWithLevel {
    uint32_t level;
    Rectangle rectangle;
  };

  struct QuadtreeNode {
    QuadtreeNode(
        const QuadtreeTileID& id_,
        const Rectangle& extent_,
        QuadtreeNode* pParent_) noexcept
        : id(id_),
          extent(extent_),
          pParent(pParent_),
          ll(),
          lr(),
          ul(),
          ur(),
          rectangles() {}

    QuadtreeTileID id;

    CesiumGeometry::Rectangle extent;

    QuadtreeNode* pParent;
    std::unique_ptr<QuadtreeNode> ll;
    std::unique_ptr<QuadtreeNode> lr;
    std::unique_ptr<QuadtreeNode> ul;
    std::unique_ptr<QuadtreeNode> ur;

    std::vector<RectangleWithLevel> rectangles;
  };

  QuadtreeTilingScheme _tilingScheme;
  uint32_t _maximumLevel;
  std::vector<std::unique_ptr<QuadtreeNode>> _rootNodes;

  static void putRectangleInQuadtree(
      const QuadtreeTilingScheme& tilingScheme,
      uint32_t maximumLevel,
      QuadtreeRectangleAvailability::QuadtreeNode& node,
      const QuadtreeRectangleAvailability::RectangleWithLevel&
          rectangle) noexcept;
  static bool rectangleLevelComparator(
      const QuadtreeRectangleAvailability::RectangleWithLevel& a,
      const QuadtreeRectangleAvailability::RectangleWithLevel& b) noexcept;
  static uint32_t findMaxLevelFromNode(
      QuadtreeNode* pStopNode,
      QuadtreeNode& node,
      const glm::dvec2& position) noexcept;
  static void createNodeChildrenIfNecessary(
      QuadtreeNode& node,
      const QuadtreeTilingScheme& tilingScheme) noexcept;
};
} // namespace CesiumGeometry
