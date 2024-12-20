#pragma once

#include <CesiumGeometry/Plane.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/Library.h>

#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace CesiumGeospatial {

/**
 * @brief A plane tangent to an {@link Ellipsoid} at a certain origin position.
 *
 * If the origin is not on the surface of the ellipsoid, its surface projection
 * will be used.
 */
class CESIUMGEOSPATIAL_API EllipsoidTangentPlane final {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param origin The origin, in cartesian coordinates.
   * @param ellipsoid The ellipsoid. Default value: {@link Ellipsoid::WGS84}.
   * @throws An `std::invalid_argument` if the given origin is at the
   * center of the ellipsoid.
   */
  EllipsoidTangentPlane(
      const glm::dvec3& origin,
      const Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  /**
   * @brief Creates a new instance.
   *
   * @param eastNorthUpToFixedFrame A transform that was computed with
   * {@link GlobeTransforms::eastNorthUpToFixedFrame}.
   * @param ellipsoid The ellipsoid. Default value: {@link Ellipsoid::WGS84}.
   */
  EllipsoidTangentPlane(
      const glm::dmat4& eastNorthUpToFixedFrame,
      const Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  /**
   * @brief Returns the {@link Ellipsoid}.
   */
  const Ellipsoid& getEllipsoid() const noexcept { return this->_ellipsoid; }

  /**
   * @brief Returns the origin, in cartesian coordinates.
   */
  const glm::dvec3& getOrigin() const noexcept { return this->_origin; }

  /**
   * @brief Returns the x-axis of this plane.
   */
  const glm::dvec3& getXAxis() const noexcept { return this->_xAxis; }

  /**
   * @brief Returns the y-axis of this plane.
   */
  const glm::dvec3& getYAxis() const noexcept { return this->_yAxis; }

  /**
   * @brief Returns the z-axis (i.e. the normal) of this plane.
   */
  const glm::dvec3& getZAxis() const noexcept {
    return this->_plane.getNormal();
  }

  /**
   * @brief Returns a {@link CesiumGeometry::Plane} representation of this
   * plane.
   */
  const CesiumGeometry::Plane& getPlane() const noexcept {
    return this->_plane;
  }

  /**
   * @brief Computes the position of the projection of the given point on this
   * plane.
   *
   * Projects the given point on this plane, along the normal. The result will
   * be a 2D point, referring to the local coordinate system of the plane that
   * is given by the x- and y-axis.
   *
   * @param cartesian The point in cartesian coordinates.
   * @return The 2D representation of the point on the plane that is closest to
   * the given position.
   */
  glm::dvec2
  projectPointToNearestOnPlane(const glm::dvec3& cartesian) const noexcept;

private:
  /**
   * Computes the matrix that is used for the constructor (if the origin
   * and ellipsoid are given), but throws an `std::invalid_argument` if
   * the origin is at the center of the ellipsoid.
   */
  static glm::dmat4 computeEastNorthUpToFixedFrame(
      const glm::dvec3& origin,
      const Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID);

  Ellipsoid _ellipsoid;
  glm::dvec3 _origin;
  glm::dvec3 _xAxis;
  glm::dvec3 _yAxis;
  CesiumGeometry::Plane _plane;
};

} // namespace CesiumGeospatial
