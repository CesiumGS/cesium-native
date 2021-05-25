#pragma once

#include "CesiumGeometry/Library.h"

#include <optional>

#include <glm/vec3.hpp>

namespace CesiumGeometry {

/**
 * @brief A ray that extends infinitely from the provided origin in the provided
 * direction.
 */
class CESIUMGEOMETRY_API Ray final {
public:
  /**
   * @brief Creates a ray from the given parameters.
   *
   * The caller is responsible to make sure that the given direction vector is
   * normalized.
   *
   * @param origin The origin.
   * @param direction The direction.
   * @return The ray.
   */
  static Ray createUnchecked(
      const glm::dvec3& origin,
      const glm::dvec3& direction) noexcept;

  /**
   * @brief Creates a ray from the given parameters.
   *
   * If the given direction is not normalized (i.e. when it does not have a
   * length of 1.0, within a small machine epsilon), then an empty optional will
   * be returned.
   *
   * @param origin The origin.
   * @param direction The direction.
   * @return The ray.
   */
  static std::optional<Ray> createOptional(
      const glm::dvec3& origin,
      const glm::dvec3& direction) noexcept;

  /**
   * @brief Creates a ray from the given parameters.
   *
   * If the given direction is not normalized (i.e. when it does not have a
   * length of 1.0, within a small machine epsilon), then an exception will be
   * thrown.
   *
   * @param origin The origin.
   * @param direction The direction.
   * @return The ray.
   * @exception std::exception If the direction is not normalized
   */
  static Ray
  createThrowing(const glm::dvec3& origin, const glm::dvec3& direction);

  /**
   * @brief Gets the origin of the ray.
   */
  const glm::dvec3& getOrigin() const noexcept { return this->_origin; }

  /**
   * @brief Gets the direction of the ray.
   */
  const glm::dvec3& getDirection() const noexcept { return this->_direction; }

  /**
   * @brief Constructs a new ray with its direction opposite this one.
   */
  Ray operator-() const noexcept;

private:
  /**
   * @brief Construct a new ray.
   *
   * @param origin The origin of the ray.
   * @param direction The direction of the ray
   */
  Ray(const glm::dvec3& origin, const glm::dvec3& direction);

  glm::dvec3 _origin;
  glm::dvec3 _direction;
};
} // namespace CesiumGeometry
