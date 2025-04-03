#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/Transforms.h>

#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>
#include <limits>

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

glm::dmat4 Transforms::createViewMatrix(
    const glm::dvec3& position,
    const glm::dvec3& direction,
    const glm::dvec3& up) {
  const glm::dvec3 forward = -1.0 * direction;
  const glm::dvec3 side = glm::normalize(glm::cross(up, forward));
  const glm::dvec3 poseUp = glm::normalize(glm::cross(forward, side));
  // Invert the pose to create the view matrix. Transpose the rotation part and
  // invert the position "by hand."
  glm::dmat4 result(1.0);
  result[0][0] = side.x;
  result[1][0] = side.y;
  result[2][0] = side.z;
  result[0][1] = poseUp.x;
  result[1][1] = poseUp.y;
  result[2][1] = poseUp.z;
  result[0][2] = forward.x;
  result[1][2] = forward.y;
  result[2][2] = forward.z;
  result[3][0] = -glm::dot(side, position);
  result[3][1] = -glm::dot(poseUp, position);
  result[3][2] = -glm::dot(forward, position);
  return result;
}

glm::dmat4 Transforms::createPerspectiveMatrix(
    double fovx,
    double fovy,
    double zNear,
    double zFar) {
  double m22;
  double m32;
  if (zFar == std::numeric_limits<double>::infinity()) {
    m22 = 0.0;
    m32 = zNear;
  } else {
    m22 = zNear / (zFar - zNear);
    m32 = zNear * zFar / (zFar - zNear);
  }
  return glm::dmat4(
      glm::dvec4(1.0 / std::tan(fovx * 0.5), 0.0, 0.0, 0.0),
      glm::dvec4(0.0, -1.0 / std::tan(fovy * 0.5), 0.0, 0.0),
      glm::dvec4(0.0, 0.0, m22, -1.0),
      glm::dvec4(0.0, 0.0, m32, 0.0));
}

glm::dmat4 Transforms::createPerspectiveMatrix(
    double left,
    double right,
    double bottom,
    double top,
    double zNear,
    double zFar) {
  double m22;
  double m32;
  if (zFar == std::numeric_limits<double>::infinity()) {
    m22 = 0.0;
    m32 = zNear;
  } else {
    m22 = zNear / (zFar - zNear);
    m32 = zNear * zFar / (zFar - zNear);
  }
  return glm::dmat4(
      glm::dvec4(2.0 * zNear / (right - left), 0.0, 0.0, 0.0),
      glm::dvec4(0.0, 2.0 * zNear / (bottom - top), 0.0, 0.0),
      glm::dvec4(
          (right + left) / (right - left),
          (bottom + top) / (bottom - top),
          m22,
          -1.0),
      glm::dvec4(0.0, 0.0, m32, 0.0));
}

glm::dmat4 Transforms::createOrthographicMatrix(
    double left,
    double right,
    double bottom,
    double top,
    double zNear,
    double zFar) {
  double m22;
  double m32;
  if (zFar == std::numeric_limits<double>::infinity()) {
    m22 = 0.0;
    m32 = 1.0;
  } else {
    m22 = 1.0 / (zFar - zNear);
    m32 = zFar / (zFar - zNear);
  }
  return glm::dmat4(
      glm::dvec4(2.0 / (right - left), 0.0, 0.0, 0.0),
      glm::dvec4(0.0, 2.0 / (bottom - top), 0.0, 0.0),
      glm::dvec4(0.0, 0.0, m22, 0.0),
      glm::dvec4(
          -(right + left) / (right - left),
          -(bottom + top) / (bottom - top),
          m32,
          1.0));
}

} // namespace CesiumGeometry
