#pragma once

#include <CesiumGeometry/CullingResult.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Library.h>

namespace CesiumGeometry {
class Plane;
}

namespace CesiumGeospatial {
class Cartographic;

/**
 * @brief A bounding volume specified as a longitude/latitude bounding box and a
 * minimum and maximum height.
 */
class CESIUMGEOSPATIAL_API BoundingRegion final {
public:
  /**
   * @brief Constructs a new bounding region.
   *
   * @param rectangle The bounding rectangle of the region.
   * @param minimumHeight The minimum height in meters.
   * @param maximumHeight The maximum height in meters.
   * @param ellipsoid The ellipsoid on which this region is defined.
   */
  BoundingRegion(
      const GlobeRectangle& rectangle,
      double minimumHeight,
      double maximumHeight,
      const Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  /**
   * @brief Gets the bounding rectangle of the region.
   */
  const GlobeRectangle& getRectangle() const noexcept {
    return this->_rectangle;
  }

  /**
   * @brief Gets the minimum height of the region.
   */
  double getMinimumHeight() const noexcept { return this->_minimumHeight; }

  /**
   * @brief Gets the maximum height of the region.
   */
  double getMaximumHeight() const noexcept { return this->_maximumHeight; }

  /**
   * @brief Gets an oriented bounding box containing this region.
   */
  const CesiumGeometry::OrientedBoundingBox& getBoundingBox() const noexcept {
    return this->_boundingBox;
  }

  /**
   * @brief Determines on which side of a plane the bounding region is located.
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
   * @brief Computes the distance-squared from a position in ellipsoid-centered
   * Cartesian coordinates to the closest point in this bounding region.
   *
   * If the position cannot be converted into cartograpic coordinates for the
   * given ellipsoid (because it is close to the center of the ellipsoid),
   * then this function will return the squared distance between the given
   * position and the closest point of the bounding box that is enclosed in
   * this region.
   *
   * @param position The position.
   * @param ellipsoid The ellipsoid on which this region is defined.
   * @return The distance-squared from the position to the closest point in the
   * bounding region.
   */
  double computeDistanceSquaredToPosition(
      const glm::dvec3& position,
      const Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID) const noexcept;

  /**
   * @brief Computes the distance-squared from a longitude-latitude-height
   * position to the closest point in this bounding region.
   *
   * @param position The position.
   * @param ellipsoid The ellipsoid on which this region is defined.
   * @return The distance-squared from the position to the closest point in the
   * bounding region.
   */
  double computeDistanceSquaredToPosition(
      const Cartographic& position,
      const Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID) const noexcept;

  /**
   * @brief Computes the distance-squared from a position to the closest point
   * in this bounding region, when the longitude-latitude-height and
   * ellipsoid-centered Cartesian coordinates of the position are both already
   * known.
   *
   * @param cartographicPosition The position as a longitude-latitude-height.
   * @param cartesianPosition The position as ellipsoid-centered Cartesian
   * coordinates.
   * @return The distance-squared from the position to the closest point in the
   * bounding region.
   */
  double computeDistanceSquaredToPosition(
      const Cartographic& cartographicPosition,
      const glm::dvec3& cartesianPosition) const noexcept;

  /**
   * @brief Computes the union of this bounding region with another.
   *
   * @param other The other bounding region.
   * @param ellipsoid The {@link CesiumGeospatial::Ellipsoid}.
   * @return The union.
   */
  BoundingRegion computeUnion(
      const BoundingRegion& other,
      const CesiumGeospatial::Ellipsoid& ellipsoid
          CESIUM_DEFAULT_ELLIPSOID) const noexcept;

private:
  static CesiumGeometry::OrientedBoundingBox _computeBoundingBox(
      const GlobeRectangle& rectangle,
      double minimumHeight,
      double maximumHeight,
      const Ellipsoid& ellipsoid);

  GlobeRectangle _rectangle;
  double _minimumHeight;
  double _maximumHeight;
  CesiumGeometry::OrientedBoundingBox _boundingBox;

  glm::dvec3 _southwestCornerCartesian;
  glm::dvec3 _northeastCornerCartesian;
  glm::dvec3 _westNormal;
  glm::dvec3 _eastNormal;
  glm::dvec3 _southNormal;
  glm::dvec3 _northNormal;
  bool _planesAreInvalid;
};

} // namespace CesiumGeospatial
