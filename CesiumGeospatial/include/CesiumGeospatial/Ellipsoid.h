#pragma once

#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Library.h>
#include <CesiumUtility/Math.h>

#include <glm/vec3.hpp>

#include <optional>

// The comments are copied here so that the doc comment always shows up in
// Intellisense whether the default is toggled or not.
#ifndef CESIUM_DISABLE_DEFAULT_ELLIPSOID
// To keep from breaking our API, a lot of things now need default Ellipsoid
// parameters. However, we shouldn't rely on these defaults internally, so
// disabling them is a good way to get a compile-time check that they're not
// being used. This macro allows us to toggle this check.
#define CESIUM_DEFAULT_ELLIPSOID = CesiumGeospatial::Ellipsoid::WGS84
#else
// To keep from breaking our API, a lot of things now need default Ellipsoid
// parameters. However, we shouldn't rely on these defaults internally, so
// disabling them is a good way to get a compile-time check that they're not
// being used. This macro allows us to toggle this check.
#define CESIUM_DEFAULT_ELLIPSOID
#endif

namespace CesiumGeospatial {

/**
 * @brief A quadratic surface defined in Cartesian coordinates.
 *
 * The surface is defined by the equation `(x / a)^2 + (y / b)^2 + (z / c)^2 =
 * 1`. This is primarily used by Cesium to represent the shape of planetary
 * bodies. Rather than constructing this object directly, one of the provided
 * constants is normally used.
 *
 * @see \ref what-is-an-ellipsoid
 */
class CESIUMGEOSPATIAL_API Ellipsoid final {
public:
  /**
   * @brief An Ellipsoid instance initialized to the WGS84 standard.
   *
   * The ellipsoid is initialized to the  World Geodetic System (WGS84)
   * standard, as defined in
   * https://earth-info.nga.mil/GandG/publications/tr8350.2/wgs84fin.pdf.
   */
  static /*constexpr*/ const Ellipsoid WGS84;

  /**
   * @brief An Ellipsoid with all three radii set to one meter.
   */
  static const Ellipsoid UNIT_SPHERE;

  /**
   * @brief Creates a new instance.
   *
   * @param x The radius in x-direction.
   * @param y The radius in y-direction.
   * @param z The radius in z-direction.
   */
  constexpr Ellipsoid(double x, double y, double z) noexcept
      : Ellipsoid(glm::dvec3(x, y, z)) {}

  /**
   * @brief Creates a new instance.
   *
   * @param radii The radii in x-, y-, and z-direction.
   */
  constexpr Ellipsoid(const glm::dvec3& radii) noexcept
      : _radii(radii),
        _radiiSquared(radii.x * radii.x, radii.y * radii.y, radii.z * radii.z),
        _oneOverRadii(1.0 / radii.x, 1.0 / radii.y, 1.0 / radii.z),
        _oneOverRadiiSquared(
            1.0 / (radii.x * radii.x),
            1.0 / (radii.y * radii.y),
            1.0 / (radii.z * radii.z)),
        _centerToleranceSquared(CesiumUtility::Math::Epsilon1) {}

  /**
   * @brief Returns the radii in x-, y-, and z-direction.
   */
  constexpr const glm::dvec3& getRadii() const noexcept { return this->_radii; }

  /**
   * @brief Computes the normal of the plane tangent to the surface of the
   * ellipsoid at the provided position.
   *
   * @param position The cartesian position for which to to determine the
   * surface normal.
   * @return The normal.
   */
  glm::dvec3 geodeticSurfaceNormal(const glm::dvec3& position) const noexcept;

  /**
   * @brief Computes the normal of the plane tangent to the surface of the
   * ellipsoid at the provided position.
   *
   * @param cartographic The {@link Cartographic} position for which to to
   * determine the surface normal.
   * @return The normal.
   */
  glm::dvec3
  geodeticSurfaceNormal(const Cartographic& cartographic) const noexcept;

  /**
   * @brief Converts the provided {@link Cartographic} to cartesian
   * representation.
   *
   * @param cartographic The {@link Cartographic} position.
   * @return The cartesian representation.
   */
  glm::dvec3
  cartographicToCartesian(const Cartographic& cartographic) const noexcept;

  /**
   * @brief Converts the provided cartesian to a {@link Cartographic}
   * representation.
   *
   * The result will be the empty optional if the given cartesian is at the
   * center of this ellipsoid.
   *
   * @param cartesian The cartesian position.
   * @return The {@link Cartographic} representation, or the empty optional if
   * the cartesian is at the center of this ellipsoid.
   */
  std::optional<Cartographic>
  cartesianToCartographic(const glm::dvec3& cartesian) const noexcept;

  /**
   * @brief Scales the given cartesian position along the geodetic surface
   * normal so that it is on the surface of this ellipsoid.
   *
   * The result will be the empty optional if the position is at the center of
   * this ellipsoid.
   *
   * @param cartesian The cartesian position.
   * @return The scaled position, or the empty optional if
   * the cartesian is at the center of this ellipsoid.
   */
  std::optional<glm::dvec3>
  scaleToGeodeticSurface(const glm::dvec3& cartesian) const noexcept;

  /**
   * @brief Scales the provided cartesian position along the geocentric
   * surface normal so that it is on the surface of this ellipsoid.
   *
   * @param cartesian The cartesian position to scale.
   * @returns The scaled position, or the empty optional if the cartesian is at
   * the center of this ellipsoid.
   */
  std::optional<glm::dvec3>
  scaleToGeocentricSurface(const glm::dvec3& cartesian) const noexcept;

  /**
   * @brief The maximum radius in any dimension.
   *
   * @return The maximum radius.
   */
  constexpr double getMaximumRadius() const noexcept {
    return glm::max(this->_radii.x, glm::max(this->_radii.y, this->_radii.z));
  }

  /**
   * @brief The minimum radius in any dimension.
   *
   * @return The minimum radius.
   */
  constexpr double getMinimumRadius() const noexcept {
    return glm::min(this->_radii.x, glm::min(this->_radii.y, this->_radii.z));
  }

  /**
   * @brief Returns `true` if two elliposids are equal.
   */
  constexpr bool operator==(const Ellipsoid& rhs) const noexcept {
    return this->_radii == rhs._radii;
  };

  /**
   * @brief Returns `true` if two elliposids are *not* equal.
   */
  constexpr bool operator!=(const Ellipsoid& rhs) const noexcept {
    return !(*this == rhs);
  };

private:
  glm::dvec3 _radii;
  glm::dvec3 _radiiSquared;
  glm::dvec3 _oneOverRadii;
  glm::dvec3 _oneOverRadiiSquared;
  double _centerToleranceSquared;
};
} // namespace CesiumGeospatial
