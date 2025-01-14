#pragma once

#include <CesiumGeometry/CullingResult.h>
#include <CesiumGeometry/Plane.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/S2CellID.h>

#include <glm/vec3.hpp>

#include <array>
#include <span>
#include <string_view>

namespace CesiumGeospatial {

/**
 * A tile bounding volume specified as an S2 cell token with minimum and maximum
 * heights. The bounding volume is a k DOP. A k-DOP is the Boolean intersection
 * of extents along k directions.
 */
class CESIUMGEOSPATIAL_API S2CellBoundingVolume final {
public:
  /** @brief Creates a new \ref S2CellBoundingVolume.
   *
   * @param cellID The S2 cell ID.
   * @param minimumHeight The minimum height of the bounding volume.
   * @param maximumHeight The maximum height of the bounding volume.
   * @param ellipsoid The ellipsoid.
   */
  S2CellBoundingVolume(
      const S2CellID& cellID,
      double minimumHeight,
      double maximumHeight,
      const CesiumGeospatial::Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  /**
   * @brief Gets this bounding volume's cell ID.
   */
  const S2CellID& getCellID() const { return this->_cellID; }

  /**
   * @brief Gets the minimum height of the cell.
   */
  double getMinimumHeight() const noexcept { return this->_minimumHeight; }

  /**
   * @brief Gets the maximum height of the cell.
   */
  double getMaximumHeight() const noexcept { return this->_maximumHeight; }

  /**
   * @brief Gets the center of this bounding volume in ellipsoid-fixed (ECEF)
   * coordinates.
   */
  glm::dvec3 getCenter() const noexcept;

  /**
   * @brief Gets the either corners of the bounding volume, in ellipsoid-fixed
   * (ECEF) coordinates.
   *
   * @return An array of positions with a `size()` of 8.
   */
  std::span<const glm::dvec3> getVertices() const noexcept;

  /**
   * @brief Determines on which side of a plane the bounding volume is located.
   *
   * @param plane The plane to test against.
   * @return The {@link CesiumGeometry::CullingResult}
   *  * `Inside` if the entire region is on the side of the plane the normal is
   * pointing.
   *  * `Outside` if the entire region is on the opposite side.
   *  * `Intersecting` if the region intersects the plane.
   */
  CesiumGeometry::CullingResult
  intersectPlane(const CesiumGeometry::Plane& plane) const noexcept;

  /**
   * @brief Computes the distance squared from a given position to the closest
   * point on this bounding volume. The position must be expressed in
   * ellipsoid-centered (ECEF) coordinates.
   *
   * @param position The position
   * @return The estimated distance squared from the bounding box to the point.
   *
   * @snippet TestOrientedBoundingBox.cpp distanceSquaredTo
   */
  double
  computeDistanceSquaredToPosition(const glm::dvec3& position) const noexcept;

  /**
   * @brief Gets the six planes that bound the volume.
   *
   * @return An array of planes with a `size()` of 6.
   */
  std::span<const CesiumGeometry::Plane> getBoundingPlanes() const noexcept;

  /**
   * @brief Computes the bounding begion that best fits this S2 cell volume.
   *
   * @return The bounding region.
   */
  BoundingRegion
  computeBoundingRegion(const CesiumGeospatial::Ellipsoid& ellipsoid
                            CESIUM_DEFAULT_ELLIPSOID) const noexcept;

private:
  S2CellID _cellID;
  double _minimumHeight;
  double _maximumHeight;
  glm::dvec3 _center;
  std::array<CesiumGeometry::Plane, 6> _boundingPlanes;
  std::array<glm::dvec3, 8> _vertices;
};

} // namespace CesiumGeospatial
