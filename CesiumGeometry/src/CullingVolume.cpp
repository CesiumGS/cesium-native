#include <CesiumGeometry/CullingVolume.h>
#include <CesiumGeometry/Plane.h>
#include <CesiumGeometry/Transforms.h>

#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace CesiumGeometry {

CullingVolume createCullingVolume(
    const glm::dvec3& position,
    const glm::dvec3& direction,
    const glm::dvec3& up,
    const double fovx,
    const double fovy) noexcept {
  const double t = glm::tan(0.5 * fovy);
  const double b = -t;
  const double r = glm::tan(0.5 * fovx);
  const double l = -r;

  const double positionLen = glm::length(position);
  const double n = std::max(
      1.0,
      std::nextafter(positionLen, std::numeric_limits<double>::max()) -
          positionLen);

  // TODO: this is all ported directly from CesiumJS, can probably be refactored
  // to be more efficient with GLM.

  const glm::dvec3 right = glm::cross(direction, up);

  glm::dvec3 nearCenter = direction * n;
  nearCenter = position + nearCenter;

  // Left plane computation
  glm::dvec3 normal = right * l;
  normal = nearCenter + normal;
  normal = normal - position;
  normal = glm::normalize(normal);
  normal = glm::cross(normal, up);
  normal = glm::normalize(normal);

  const CesiumGeometry::Plane leftPlane(normal, -glm::dot(normal, position));

  // Right plane computation
  normal = right * r;
  normal = nearCenter + normal;
  normal = normal - position;
  normal = glm::cross(up, normal);
  normal = glm::normalize(normal);

  const CesiumGeometry::Plane rightPlane(normal, -glm::dot(normal, position));

  // Bottom plane computation
  normal = up * b;
  normal = nearCenter + normal;
  normal = normal - position;
  normal = glm::cross(right, normal);
  normal = glm::normalize(normal);

  const CesiumGeometry::Plane bottomPlane(normal, -glm::dot(normal, position));

  // Top plane computation
  normal = up * t;
  normal = nearCenter + normal;
  normal = normal - position;
  normal = glm::cross(normal, right);
  normal = glm::normalize(normal);

  const CesiumGeometry::Plane topPlane(normal, -glm::dot(normal, position));

  return {leftPlane, rightPlane, topPlane, bottomPlane};
}

CullingVolume createCullingVolume(const glm::dmat4& clipMatrix) {
  // Gribb / Hartmann method.
  // See
  // https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
  // Here's a sketch of the method for the left clipping plane. Given:
  // v incoming eye-space vertex
  // x', w' coordinate values after matrix multiplication
  //
  // The clipping test is -w' < x'
  // or -(row3 dot v) < (row0 dot v)
  //  0 < (row3 dot v) + (row0 dot v)
  //  0 < (row3 + row0) dot v
  // This is a test against the plane a*x + b*y + c*z + d*w = 0, but the
  // incoming w = 1, so we have a standard plane a*x + b*y + c*z + d = 0 where
  // the factors are the sums of the elements of the matrix rows.
  //
  auto normalizePlane = [](double a, double b, double c, double d) {
    double len = sqrt(a * a + b * b + c * c);
    return Plane(glm::dvec3(a / len, b / len, c / len), d / len);
  };
  const Plane leftPlane = normalizePlane(
      clipMatrix[0][3] + clipMatrix[0][0],
      clipMatrix[1][3] + clipMatrix[1][0],
      clipMatrix[2][3] + clipMatrix[2][0],
      clipMatrix[3][3] + clipMatrix[3][0]);
  const Plane rightPlane = normalizePlane(
      clipMatrix[0][3] - clipMatrix[0][0],
      clipMatrix[1][3] - clipMatrix[1][0],
      clipMatrix[2][3] - clipMatrix[2][0],
      clipMatrix[3][3] - clipMatrix[3][0]);
  const Plane bottomPlane = normalizePlane(
      clipMatrix[0][3] - clipMatrix[0][1],
      clipMatrix[1][3] - clipMatrix[1][1],
      clipMatrix[2][3] - clipMatrix[2][1],
      clipMatrix[3][3] - clipMatrix[3][1]);
  const Plane topPlane = normalizePlane(
      clipMatrix[0][3] + clipMatrix[0][1],
      clipMatrix[1][3] + clipMatrix[1][1],
      clipMatrix[2][3] + clipMatrix[2][1],
      clipMatrix[3][3] + clipMatrix[3][1]);
  return {leftPlane, rightPlane, topPlane, bottomPlane};
}

CullingVolume createCullingVolume(
    const glm::dvec3& position,
    const glm::dvec3& direction,
    const glm::dvec3& up,
    double l,
    double r,
    double b,
    double t,
    double n) noexcept {
  glm::dmat4 projMatrix = Transforms::createPerspectiveMatrix(
      l,
      r,
      b,
      t,
      n,
      std::numeric_limits<double>::infinity());
  glm::dmat4 viewMatrix = glm::lookAt(position, position + direction, up);
  glm::dmat4 clipMatrix = projMatrix * viewMatrix;
  return createCullingVolume(clipMatrix);
}

CullingVolume createOrthographicCullingVolume(
    const glm::dvec3& position,
    const glm::dvec3& direction,
    const glm::dvec3& up,
    double l,
    double r,
    double b,
    double t,
    double n) noexcept {
  glm::dmat4 projMatrix = Transforms::createOrthographicMatrix(
      l,
      r,
      b,
      t,
      n,
      std::numeric_limits<double>::infinity());
  glm::dmat4 viewMatrix = glm::lookAt(position, position + direction, up);
  glm::dmat4 clipMatrix = projMatrix * viewMatrix;
  return createCullingVolume(clipMatrix);
}
} // namespace CesiumGeometry
