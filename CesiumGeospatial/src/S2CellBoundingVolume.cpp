#include "CesiumGeospatial/S2CellBoundingVolume.h"

#include <CesiumUtility/Math.h>

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

#include <array>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace {

// Computes bounding planes of the kDOP.
std::array<Plane, 6> computeBoundingPlanes(
    const S2CellBoundingVolume& s2Cell,
    const Ellipsoid& ellipsoid) {
  std::array<Plane, 6> planes;
  glm::dvec3 centerPoint = s2Cell.getCenter();

  // Compute top plane.
  // - Get geodetic surface normal at the center of the S2 cell.
  // - Get center point at maximum height of bounding volume.
  // - Create top plane from surface normal and top point.
  glm::dvec3 centerSurfaceNormal = ellipsoid.geodeticSurfaceNormal(centerPoint);
  std::optional<Cartographic> maybeTopCartographic =
      ellipsoid.cartesianToCartographic(centerPoint);
  assert(maybeTopCartographic);
  Cartographic& topCartographic = *maybeTopCartographic;
  topCartographic.height = s2Cell.getMaximumHeight();
  glm::dvec3 top = ellipsoid.cartographicToCartesian(topCartographic);
  planes[0] = Plane(top, centerSurfaceNormal);
  const Plane& topPlane = planes[0];

  // Compute bottom plane.
  // - Iterate through bottom vertices
  //   - Get distance from vertex to top plane
  // - Find longest distance from vertex to top plane
  // - Translate top plane by the distance
  GlobeRectangle rectangle = s2Cell.getCellID().getRectangle();
  Cartographic verticesCartographic[4] = {
      rectangle.getSouthwest(),
      rectangle.getSoutheast(),
      rectangle.getNortheast(),
      rectangle.getNorthwest()};
  glm::dvec3 verticesCartesian[4];

  double maxDistance = 0;
  for (int i = 0; i < 4; ++i) {
    verticesCartographic[i].height = s2Cell.getMinimumHeight();
    verticesCartesian[i] =
        ellipsoid.cartographicToCartesian(verticesCartographic[i]);

    double distance = topPlane.getPointDistance(verticesCartesian[i]);
    if (distance < maxDistance) {
      maxDistance = distance;
    }
  }

  // Negate the normal of the bottom plane since we want all normals to point
  // "outwards".
  planes[1] =
      Plane(-topPlane.getNormal(), -topPlane.getDistance() + maxDistance);

  // Compute side planes.
  // - Iterate through vertices (in CCW order, by default)
  //   - Get a vertex and another vertex adjacent to it.
  //   - Compute geodetic surface normal at one vertex.
  //   - Compute vector between vertices.
  //   - Compute normal of side plane. (cross product of top dir and side dir)
  for (int i = 0; i < 4; ++i) {
    const glm::dvec3& vertex = verticesCartesian[i];
    const glm::dvec3& adjacentVertex = verticesCartesian[(i + 1) % 4];
    const glm::dvec3 geodeticNormal = ellipsoid.geodeticSurfaceNormal(vertex);
    glm::dvec3 side = adjacentVertex - vertex;
    glm::dvec3 sideNormal = glm::normalize(glm::cross(side, geodeticNormal));
    planes[i + 2] = Plane(vertex, sideNormal);
  }

  return planes;
}

/**
 * Computes intersection of 3 planes.
 */
glm::dvec3
computeIntersection(const Plane& p0, const Plane& p1, const Plane& p2) {
  const glm::dvec3& n0 = p0.getNormal();
  const glm::dvec3& n1 = p1.getNormal();
  const glm::dvec3& n2 = p2.getNormal();

  glm::dmat3x3 matrix(
      glm::dvec3(n0.x, n1.x, n2.x),
      glm::dvec3(n0.y, n1.y, n2.y),
      glm::dvec3(n0.z, n1.z, n2.z));

  double determinant = glm::determinant(matrix);

  glm::dvec3 x0 = n0 * -p0.getDistance();
  glm::dvec3 x1 = n1 * -p1.getDistance();
  glm::dvec3 x2 = n2 * -p2.getDistance();

  glm::dvec3 f0 = glm::cross(n1, n2) * glm::dot(x0, n0);
  glm::dvec3 f1 = glm::cross(n2, n0) * glm::dot(x1, n1);
  glm::dvec3 f2 = glm::cross(n0, n1) * glm::dot(x2, n2);

  glm::dvec3 s = f0 + f1 + f2;

  return s / determinant;
}

std::array<glm::dvec3, 8>
computeVertices(const std::array<Plane, 6>& boundingPlanes) {
  std::array<glm::dvec3, 8> vertices;

  for (int i = 0; i < 4; ++i) {
    // Vertices on the top plane.
    vertices[i] = computeIntersection(
        boundingPlanes[0],
        boundingPlanes[2 + ((i + 3) % 4)],
        boundingPlanes[2 + (i % 4)]);
    // Vertices on the bottom plane.
    vertices[i + 4] = computeIntersection(
        boundingPlanes[1],
        boundingPlanes[2 + ((i + 3) % 4)],
        boundingPlanes[2 + (i % 4)]);
  }

  return vertices;
}

} // namespace

S2CellBoundingVolume::S2CellBoundingVolume(
    const S2CellID& cellID,
    double minimumHeight,
    double maximumHeight,
    const Ellipsoid& ellipsoid)
    : _cellID(cellID),
      _minimumHeight(minimumHeight),
      _maximumHeight(maximumHeight) {
  Cartographic result = this->_cellID.getCenter();
  result.height = (this->_minimumHeight + this->_maximumHeight) * 0.5;
  this->_center = ellipsoid.cartographicToCartesian(result);
  this->_boundingPlanes = computeBoundingPlanes(*this, ellipsoid);
  this->_vertices = computeVertices(this->_boundingPlanes);
}

glm::dvec3 S2CellBoundingVolume::getCenter() const noexcept {
  return this->_center;
}

CesiumGeometry::CullingResult S2CellBoundingVolume::intersectPlane(
    const CesiumGeometry::Plane& plane) const noexcept {
  int32_t plusCount = 0;
  int32_t negCount = 0;
  for (size_t i = 0; i < this->_vertices.size(); ++i) {
    double distanceToPlane =
        glm::dot(plane.getNormal(), this->_vertices[i]) + plane.getDistance();
    if (distanceToPlane < 0.0) {
      negCount++;
    } else {
      plusCount++;
    }
  }

  if (plusCount == this->_vertices.size()) {
    return CullingResult::Inside;
  } else if (negCount == this->_vertices.size()) {
    return CullingResult::Outside;
  }
  return CullingResult::Intersecting;
}

double S2CellBoundingVolume::computeDistanceSquaredToPosition(
    const glm::dvec3& /* position */) const noexcept {
  return 0.0;
}
