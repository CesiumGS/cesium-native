#include "CesiumGeometry/AxisTransforms.h"
#include <glm/mat3x3.hpp>

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

const glm::dmat4 AxisTransforms::Y_UP_TO_Z_UP = createYupToZup();
const glm::dmat4 AxisTransforms::Z_UP_TO_Y_UP = createZupToYup();
const glm::dmat4 AxisTransforms::X_UP_TO_Z_UP = createXupToZup();
const glm::dmat4 AxisTransforms::Z_UP_TO_X_UP = createZupToXup();
const glm::dmat4 AxisTransforms::X_UP_TO_Y_UP = createXupToYup();
const glm::dmat4 AxisTransforms::Y_UP_TO_X_UP = createYupToXup();

} // namespace CesiumGeometry
