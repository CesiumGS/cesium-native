#pragma once

#include "BoundingRegion.h"
#include "Ellipsoid.h"
#include "S2CellID.h"

#include <CesiumGeometry/CullingResult.h>
#include <CesiumGeometry/Plane.h>

#include <glm/vec3.hpp>
#include <gsl/span>

#include <array>
#include <string_view>

namespace CesiumGeospatial {

class CESIUMGEOSPATIAL_API S2CellBoundingVolume final {
public:
  S2CellBoundingVolume(
      const S2CellID& cellID,
      double minimumHeight,
      double maximumHeight,
      const Ellipsoid& ellipsoid = Ellipsoid::WGS84);

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

  gsl::span<const glm::dvec3> getVertices() const noexcept;

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

  gsl::span<const CesiumGeometry::Plane> getBoundingPlanes() const noexcept;

  const BoundingRegion& getBoundingRegion() const noexcept {
    return this->_boundingRegion;
  }

private:
  S2CellID _cellID;
  double _minimumHeight;
  double _maximumHeight;
  glm::dvec3 _center;
  std::array<CesiumGeometry::Plane, 6> _boundingPlanes;
  std::array<glm::dvec3, 8> _vertices;
  BoundingRegion _boundingRegion;
};

} // namespace CesiumGeospatial
