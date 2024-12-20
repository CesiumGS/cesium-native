#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/Transforms.h>

#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

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

glm::dmat4 Transforms::createTranslationRotationScaleMatrix(
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

void Transforms::computeTranslationRotationScaleFromMatrix(
    const glm::dmat4& matrix,
    glm::dvec3* pTranslation,
    glm::dquat* pRotation,
    glm::dvec3* pScale) {
  if (pRotation || pScale) {
    glm::dmat3 rotationAndScale = glm::dmat3(matrix);
    double lengthColumn0 = glm::length(rotationAndScale[0]);
    double lengthColumn1 = glm::length(rotationAndScale[1]);
    double lengthColumn2 = glm::length(rotationAndScale[2]);

    glm::dmat3 rotationMatrix(
        rotationAndScale[0] / lengthColumn0,
        rotationAndScale[1] / lengthColumn1,
        rotationAndScale[2] / lengthColumn2);

    glm::dvec3 scale(lengthColumn0, lengthColumn1, lengthColumn2);

    glm::dvec3 cross = glm::cross(rotationAndScale[0], rotationAndScale[1]);
    if (glm::dot(cross, rotationAndScale[2]) < 0.0) {
      rotationMatrix *= -1.0;
      scale *= -1.0;
    }

    if (pRotation)
      *pRotation = glm::quat_cast(rotationMatrix);
    if (pScale)
      *pScale = scale;
  }

  if (pTranslation)
    *pTranslation = glm::dvec3(matrix[3]);
}

namespace {
const glm::dmat4 Identity(1.0);
}

const glm::dmat4& Transforms::getUpAxisTransform(Axis from, Axis to) {
  switch (from) {
  case Axis::X:
    switch (to) {
    case Axis::X:
      return Identity;
    case Axis::Y:
      return X_UP_TO_Y_UP;
    case Axis::Z:
      return X_UP_TO_Z_UP;
    }
    break;
  case Axis::Y:
    switch (to) {
    case Axis::X:
      return Y_UP_TO_X_UP;
    case Axis::Y:
      return Identity;
    case Axis::Z:
      return Y_UP_TO_Z_UP;
    }
    break;
  case Axis::Z:
    switch (to) {
    case Axis::X:
      return Z_UP_TO_X_UP;
    case Axis::Y:
      return Z_UP_TO_Y_UP;
    case Axis::Z:
      return Identity;
    }
    break;
  }

  return Identity;
}

} // namespace CesiumGeometry
