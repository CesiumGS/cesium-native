#pragma once

#include <CesiumGeometry/Library.h>

#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

namespace CesiumGeometry {

/**
 * @brief A ray that extends infinitely from the provided origin in the provided
 * direction.
 */
class CESIUMGEOMETRY_API Ray final {
public:
  /**
   * @brief Construct a new ray.
   *
   * @param origin The origin of the ray.
   * @param direction The direction of the ray (normalized).
   *
   * @exception std::exception `direction` must be normalized.
   */
  Ray(const glm::dvec3& origin, const glm::dvec3& direction);

  /**
   * @brief Gets the origin of the ray.
   */
  const glm::dvec3& getOrigin() const noexcept { return this->_origin; }

  /**
   * @brief Gets the direction of the ray.
   */
  const glm::dvec3& getDirection() const noexcept { return this->_direction; }

  /**
   * @brief Calculates a point on the ray that corresponds to the given
   * distance from origin. Can be positive, negative, or 0.
   *
   * @param distance Desired distance from origin
   * @return The point along the ray.
   */
  glm::dvec3 pointFromDistance(double distance) const noexcept;

  /**
   * @brief Transforms the ray using a given 4x4 transformation matrix.
   *
   * @param transformation The 4x4 transformation matrix used to transform the
   * ray.
   * @return The transformed ray.
   */
  Ray transform(const glm::dmat4x4& transformation) const noexcept;

  /**
   * @brief Constructs a new ray with its direction opposite this one.
   */
  Ray operator-() const noexcept;

private:
  glm::dvec3 _origin;
  glm::dvec3 _direction;
};
} // namespace CesiumGeometry
