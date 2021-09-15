#include "CesiumGeometry/CullingVolume.h"

#include <glm/glm.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

namespace Cesium3DTilesSelection {

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

  const double n = 1.0;

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
} // namespace Cesium3DTilesSelection
