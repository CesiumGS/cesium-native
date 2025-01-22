#pragma once

#include <CesiumGeometry/Library.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/Rectangle.h>

#include <glm/vec2.hpp>

#include <optional>

namespace CesiumGeometry {

/**
 * @brief Defines how a rectangular region is divided into quadtree tiles.
 */
class CESIUMGEOMETRY_API QuadtreeTilingScheme final {
public:
  /**
   * @brief Constructs a new instance.
   *
   * @param rectangle The overall rectangle that is tiled, expressed in
   * projected coordinates.
   * @param rootTilesX The number of tiles at the root of the quadtree in the X
   * direction.
   * @param rootTilesY The number of tiles at the root of the quadtree in the Y
   * direction.
   */
  QuadtreeTilingScheme(
      const CesiumGeometry::Rectangle& rectangle,
      uint32_t rootTilesX,
      uint32_t rootTilesY) noexcept;

  /**
   * @brief Return the overall rectangle that is tiled.
   *
   * The rectangle is expressed in projected coordinates.
   */
  const CesiumGeometry::Rectangle& getRectangle() const noexcept {
    return this->_rectangle;
  }

  /**
   * @brief Returns the number of root tiles, in x-direction.
   */
  uint32_t getRootTilesX() const noexcept { return this->_rootTilesX; }

  /**
   * @brief Returns the number of root tiles, in y-direction.
   */
  uint32_t getRootTilesY() const noexcept { return this->_rootTilesY; }

  /**
   * @brief Returns the number of tiles, in x-direction, at the given level.
   */
  uint32_t getNumberOfXTilesAtLevel(uint32_t level) const noexcept;

  /**
   * @brief Returns the number of tiles, in y-direction, at the given level.
   */
  uint32_t getNumberOfYTilesAtLevel(uint32_t level) const noexcept;

  /**
   * @brief Computes the {@link CesiumGeometry::QuadtreeTileID} for a given
   * position and level.
   *
   * If the given position is within the {@link getRectangle} of this tiling
   * scheme, then this will compute the quadtree tile ID for the tile that
   * contains the given position at the given level. Otherwise, `nullopt`
   * is returned.
   *
   * @param position The 2D position
   * @param level The level
   * @return The tile ID, or `nullopt`.
   */
  std::optional<CesiumGeometry::QuadtreeTileID>
  positionToTile(const glm::dvec2& position, uint32_t level) const noexcept;

  /**
   * @brief Returns the {@link CesiumGeometry::Rectangle} that is covered by the
   * specified tile.
   *
   * The rectangle that is covered by the tile that is identified with
   * the given {@link CesiumGeometry::QuadtreeTileID} will be computed,
   * based on the {@link getRectangle} of this tiling scheme.
   *
   * @param tileID The tile ID
   * @return The rectangle
   */
  CesiumGeometry::Rectangle
  tileToRectangle(const CesiumGeometry::QuadtreeTileID& tileID) const noexcept;

private:
  CesiumGeometry::Rectangle _rectangle;
  uint32_t _rootTilesX;
  uint32_t _rootTilesY;
};

} // namespace CesiumGeometry
