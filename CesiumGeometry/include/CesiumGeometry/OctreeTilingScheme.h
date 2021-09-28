#pragma once

#include "CesiumGeometry/Library.h"
#include "CesiumGeometry/OctreeTileID.h"
#include "CesiumGeometry/OrientedBoundingBox.h"
#include <glm/vec3.hpp>
#include <optional>

namespace CesiumGeometry {

/**
 * @brief Defines how an {@link OrientedBoundingBox} is divided into octree
 * tiles.
 */
class CESIUMGEOMETRY_API OctreeTilingScheme {
public:
  /**
   * @brief Constructs a new instance.
   *
   * @param box The overall OBB that is tiled, expressed in
   * projected coordinates.
   * @param rootTilesX The number of tiles at the root of the quadtree in the X
   * direction.
   * @param rootTilesY The nubmer of tiles at the root of the quadtree in the Y
   * direction.
   * @param rootTilesZ The number of tiles at the root of the quadtree in the Z
   * direction.
   */
  OctreeTilingScheme(
      const CesiumGeometry::OrientedBoundingBox& box,
      uint32_t rootTilesX,
      uint32_t rootTilesY,
      uint32_t rootTilesZ);

  /**
   * @brief Return the overall box that is tiled.
   *
   * The box is expressed in projected coordinates.
   */
  const CesiumGeometry::OrientedBoundingBox& getBox() const noexcept {
    return this->_box;
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
   * @brief Returns the number of root tiles, in z-direction.
   */
  uint32_t getRootTilesZ() const noexcept { return this->_rootTilesZ; }

  /**
   * @brief Returns the number of tiles, in x-direction, at the given level.
   */
  uint32_t getNumberOfXTilesAtLevel(uint32_t level) const noexcept;

  /**
   * @brief Returns the number of tiles, in y-direction, at the given level.
   */
  uint32_t getNumberOfYTilesAtLevel(uint32_t level) const noexcept;

  /**
   * @brief Returns the number of tiles, in z-direction, at the given level.
   */
  uint32_t getNumberOfZTilesAtLevel(uint32_t level) const noexcept;

  /**
   * @brief Computes the {@link CesiumGeometry::OctreeTileID} for a given
   * position and level.
   *
   * If the given position is within the {@link getBox} of this tiling
   * scheme, then this will compute the octree tile ID for the tile that
   * contains the given position at the given level. Otherwise, `nullopt`
   * is returned.
   *
   * @param position The 3D position
   * @param level The level
   * @return The tile ID, or `nullopt`.
   */
  std::optional<CesiumGeometry::OctreeTileID>
  positionToTile(const glm::dvec3& position, uint32_t level) const noexcept;

  /**
   * @brief Returns the {@link CesiumGeometry::OrientedBoundingBox} that is 
   * covered by the specified tile.
   *
   * The box that is covered by the tile that is identified with
   * the given {@link CesiumGeometry::OctreeTileID} will be computed,
   * based on the {@link getBox} of this tiling scheme.
   *
   * @param tileID The tile ID
   * @return The box
   */
  CesiumGeometry::OrientedBoundingBox
  tileToBoundingBox(const CesiumGeometry::OctreeTileID& tileID) const noexcept;

private:
  CesiumGeometry::OrientedBoundingBox _box;
  uint32_t _rootTilesX;
  uint32_t _rootTilesY;
  uint32_t _rootTilesZ;
};

} // namespace CesiumGeometry
