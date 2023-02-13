#include "CesiumGeometry/Transforms.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>

namespace CesiumGeometry {

namespace {
// Initialize with a function instead of inline to avoid an
// internal compiler error in MSVC v14.27.29110.
glm::dmat4 createYupToZup() {
  return glm::dmat4(
      glm::dvec4(1.0, 0.0, 0.0, 0.0),
      glm::dvec4(0.0, 0.0, 1.0, 0.0),
      glm::dvec4(0.0, -1.0, 0.0, 0.0),
      glm::dvec4(0.0, 0.0, 0.0, 1.0));
}

glm::dmat4 createZupToYup() {
  return glm::dmat4(
      glm::dvec4(1.0, 0.0, 0.0, 0.0),
      glm::dvec4(0.0, 0.0, -1.0, 0.0),
      glm::dvec4(0.0, 1.0, 0.0, 0.0),
      glm::dvec4(0.0, 0.0, 0.0, 1.0));
}

glm::dmat4 createXupToZup() {
  return glm::dmat4(
      glm::dvec4(0.0, 0.0, 1.0, 0.0),
      glm::dvec4(0.0, 1.0, 0.0, 0.0),
      glm::dvec4(-1.0, 0.0, 0.0, 0.0),
      glm::dvec4(0.0, 0.0, 0.0, 1.0));
}

glm::dmat4 createZupToXup() {
  return glm::dmat4(
      glm::dvec4(0.0, 0.0, -1.0, 0.0),
      glm::dvec4(0.0, 1.0, 0.0, 0.0),
      glm::dvec4(1.0, 0.0, 0.0, 0.0),
      glm::dvec4(0.0, 0.0, 0.0, 1.0));
}

glm::dmat4 createXupToYup() {
  return glm::dmat4(
      glm::dvec4(0.0, 1.0, 0.0, 0.0),
      glm::dvec4(-1.0, 0.0, 0.0, 0.0),
      glm::dvec4(0.0, 0.0, 1.0, 0.0),
      glm::dvec4(0.0, 0.0, 0.0, 1.0));
}

glm::dmat4 createYupToXup() {
  return glm::dmat4(
      glm::dvec4(0.0, -1.0, 0.0, 0.0),
      glm::dvec4(1.0, 0.0, 0.0, 0.0),
      glm::dvec4(0.0, 0.0, 1.0, 0.0),
      glm::dvec4(0.0, 0.0, 0.0, 1.0));
}
} // namespace

const glm::dmat4 Transforms::Y_UP_TO_Z_UP = createYupToZup();
const glm::dmat4 Transforms::Z_UP_TO_Y_UP = createZupToYup();
const glm::dmat4 Transforms::X_UP_TO_Z_UP = createXupToZup();
const glm::dmat4 Transforms::Z_UP_TO_X_UP = createZupToXup();
const glm::dmat4 Transforms::X_UP_TO_Y_UP = createXupToYup();
const glm::dmat4 Transforms::Y_UP_TO_X_UP = createYupToXup();

glm::dmat4 CesiumGeometry::Transforms::translationRotationScale(
    const glm::dvec3& translation,
    const glm::dquat& rotation,
    const glm::dvec3& scale) {
  glm::dmat3 rotationScale =
      glm::mat3_cast(rotation) * glm::dmat3(
                                     glm::dvec3(scale.x, 0.0, 0.0),
                                     glm::dvec3(0.0, scale.y, 0.0),
                                     glm::dvec3(0.0, 0.0, scale.z));
  return glm::dmat4(
      glm::dvec4(rotationScale[0], 0.0),
      glm::dvec4(rotationScale[1], 0.0),
      glm::dvec4(rotationScale[2], 0.0),
      glm::dvec4(translation, 1.0));
}

} // namespace CesiumGeometry
