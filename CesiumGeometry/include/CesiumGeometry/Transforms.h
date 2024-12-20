#pragma once

#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/Library.h>

#include <glm/fwd.hpp>

namespace CesiumGeometry {

/**
 * @brief Coordinate system matrix constructions helpers.
 */
struct CESIUMGEOMETRY_API Transforms final {

  /**
   * @brief A matrix to convert from y-up to z-up orientation,
   * by rotating about PI/2 around the x-axis
   */
  static const glm::dmat4 Y_UP_TO_Z_UP;

  /**
   * @brief A matrix to convert from z-up to y-up orientation,
   * by rotating about -PI/2 around the x-axis
   */
  static const glm::dmat4 Z_UP_TO_Y_UP;

  /**
   * @brief A matrix to convert from x-up to z-up orientation,
   * by rotating about -PI/2 around the y-axis
   */
  static const glm::dmat4 X_UP_TO_Z_UP;

  /**
   * @brief A matrix to convert from z-up to x-up orientation,
   * by rotating about PI/2 around the y-axis
   */
  static const glm::dmat4 Z_UP_TO_X_UP;

  /**
   * @brief A matrix to convert from x-up to y-up orientation,
   * by rotating about PI/2 around the z-axis
   */
  static const glm::dmat4 X_UP_TO_Y_UP;

  /**
   * @brief A matrix to convert from y-up to x-up orientation,
   * by rotating about -PI/2 around the z-axis
   */
  static const glm::dmat4 Y_UP_TO_X_UP;

  /**
   * @brief Creates a translation-rotation-scale matrix, equivalent to
   * `translation * rotation * scale`. So if a vector is multiplied with the
   * resulting matrix, it will be first scaled, then rotated, then translated.
   *
   * @param translation The translation.
   * @param rotation The rotation.
   * @param scale The scale.
   */
  static glm::dmat4 createTranslationRotationScaleMatrix(
      const glm::dvec3& translation,
      const glm::dquat& rotation,
      const glm::dvec3& scale);

  /**
   * @brief Decomposes a matrix into translation, rotation, and scale
   * components. This is the reverse of
   * {@link createTranslationRotationScaleMatrix}.
   *
   * The scale may be negative (i.e. when switching from a right-handed to a
   * left-handed system), but skew and other funny business will result in
   * undefined behavior.
   *
   * @param matrix The matrix to decompose.
   * @param pTranslation A pointer to the vector in which to store the
   * translation, or nullptr if the translation is not needed.
   * @param pRotation A pointer to the quaternion in which to store the
   * rotation, or nullptr if the rotation is not needed.
   * @param pScale A pointer to the vector in which to store the scale, or
   * nullptr if the scale is not needed.
   */
  static void computeTranslationRotationScaleFromMatrix(
      const glm::dmat4& matrix,
      glm::dvec3* pTranslation,
      glm::dquat* pRotation,
      glm::dvec3* pScale);

  /**
   * @brief Gets a transform that converts from one up axis to another.
   *
   * @param from The up axis to convert from.
   * @param to The up axis to convert to.
   *
   * @returns The up axis transform.
   */
  static const glm::dmat4&
  getUpAxisTransform(CesiumGeometry::Axis from, CesiumGeometry::Axis to);
};

} // namespace CesiumGeometry
