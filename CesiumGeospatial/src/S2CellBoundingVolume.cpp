#include <CesiumGeometry/CullingResult.h>
#include <CesiumGeometry/Plane.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>
#include <CesiumUtility/Assert.h>

#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/matrix.hpp>

#include <array>
#include <cstddef>
#include <limits>
#include <optional>
#include <span>

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
  CESIUM_ASSERT(maybeTopCartographic);
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
  std::array<Cartographic, 4> verticesCartographic =
      s2Cell.getCellID().getVertices();
  glm::dvec3 verticesCartesian[4];

  double maxDistance = 0;
  for (size_t i = 0; i < 4; ++i) {
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
  for (size_t i = 0; i < 4; ++i) {
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

  for (size_t i = 0; i < 4; ++i) {
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

std::span<const glm::dvec3> S2CellBoundingVolume::getVertices() const noexcept {
  return this->_vertices;
}

CesiumGeometry::CullingResult S2CellBoundingVolume::intersectPlane(
    const CesiumGeometry::Plane& plane) const noexcept {
  size_t plusCount = 0;
  size_t negCount = 0;
  for (const auto& vertex : this->_vertices) {
    double distanceToPlane =
        glm::dot(plane.getNormal(), vertex) + plane.getDistance();
    if (distanceToPlane < 0.0) {
      ++negCount;
    } else {
      ++plusCount;
    }
  }

  if (plusCount == this->_vertices.size()) {
    return CullingResult::Inside;
  } else if (negCount == this->_vertices.size()) {
    return CullingResult::Outside;
  }
  return CullingResult::Intersecting;
}

namespace {

std::array<glm::dvec3, 4>
getPlaneVertices(const std::array<glm::dvec3, 8>& vertices, size_t index) {
  if (index <= 1) {
    size_t start = index * 4;
    return {
        vertices[start],
        vertices[start + 1],
        vertices[start + 2],
        vertices[start + 3]};
  }

  size_t i = index - 2;
  return {
      vertices[i % 4],
      vertices[(i + 1) % 4],
      vertices[4 + ((i + 1) % 4)],
      vertices[4 + i]};
}

std::array<glm::dvec3, 4> computeEdgeNormals(
    const Plane& plane,
    const std::array<glm::dvec3, 4>& vertices,
    bool invert) {
  std::array<glm::dvec3, 4> result = {
      glm::normalize(glm::cross(plane.getNormal(), vertices[1] - vertices[0])),
      glm::normalize(glm::cross(plane.getNormal(), vertices[2] - vertices[1])),
      glm::normalize(glm::cross(plane.getNormal(), vertices[3] - vertices[2])),
      glm::normalize(glm::cross(plane.getNormal(), vertices[0] - vertices[3]))};

  if (invert) {
    result[0] = -result[0];
    result[1] = -result[1];
    result[2] = -result[2];
    result[3] = -result[3];
  }

  return result;
}

/**
 * Finds point on a line segment closest to a given point.
 * @private
 */
glm::dvec3 closestPointLineSegment(
    const glm::dvec3& p,
    const glm::dvec3& l0,
    const glm::dvec3& l1) {
  glm::dvec3 d = l1 - l0;
  double t = glm::dot(d, p - l0);

  if (t <= 0.0) {
    return l0;
  }

  double dMag = glm::dot(d, d);
  if (t >= dMag) {
    return l1;
  }

  t /= dMag;
  return glm::dvec3(
      (1 - t) * l0.x + t * l1.x,
      (1 - t) * l0.y + t * l1.y,
      (1 - t) * l0.z + t * l1.z);
}

/**
 * Finds closes point on the polygon, created by the given vertices, from
 * a point. The test point and the polygon are all on the same plane.
 * @private
 */
glm::dvec3 closestPointPolygon(
    const glm::dvec3& p,
    const std::array<glm::dvec3, 4>& vertices,
    const std::array<glm::dvec3, 4>& edgeNormals) {
  double minDistance = std::numeric_limits<double>::max();
  glm::dvec3 closestPoint = p;

  for (size_t i = 0; i < vertices.size(); ++i) {
    Plane edgePlane(vertices[i], edgeNormals[i]);
    double edgePlaneDistance = edgePlane.getPointDistance(p);

    // Skip checking against the edge if the point is not in the half-space that
    // the edgePlane's normal points towards i.e. if the edgePlane is facing
    // away from the point.
    if (edgePlaneDistance < 0.0) {
      continue;
    }

    glm::dvec3 closestPointOnEdge =
        closestPointLineSegment(p, vertices[i], vertices[(i + 1) % 4]);

    double distance = glm::distance(p, closestPointOnEdge);
    if (distance < minDistance) {
      minDistance = distance;
      closestPoint = closestPointOnEdge;
    }
  }

  return closestPoint;
}

} // namespace

/**
 * The distance to point check for this kDOP involves checking the signed
 * distance of the point to each bounding plane. A plane qualifies for a
 * distance check if the point being tested against is in the half-space in the
 * direction of the normal i.e. if the signed distance of the point from the
 * plane is greater than 0.
 *
 * There are 4 possible cases for a point if it is outside the polyhedron:
 *
 *   \     X     /     X \           /       \           /       \           /
 * ---\---------/---   ---\---------/---   ---X---------/---   ---\---------/---
 *     \       /           \       /           \       /           \       /
 *   ---\-----/---       ---\-----/---       ---\-----/---       ---\-----/---
 *       \   /               \   /               \   /               \   /
 *                                                                    \ /
 *                                                                     \
 *                                                                    / \
 *                                                                   / X \
 *
 *         I                  II                  III                 IV
 *
 * * Case I: There is only one plane selected.
 * In this case, we project the point onto the plane and do a point polygon
 * distance check to find the closest point on the polygon. The point may lie
 * inside the "face" of the polygon or outside. If it is outside, we need to
 * determine which edges to test against.
 *
 * * Case II: There are two planes selected.
 * In this case, the point will lie somewhere on the line created at the
 * intersection of the selected planes or one of the planes.
 *
 * * Case III: There are three planes selected.
 * In this case, the point will lie on the vertex, at the intersection of the
 * selected planes.
 *
 * * Case IV: There are more than three planes selected.
 * Since we are on an ellipsoid, this will only happen in the bottom plane,
 * which is what we will use for the distance test.
 */
double S2CellBoundingVolume::computeDistanceSquaredToPosition(
    const glm::dvec3& position) const noexcept {
  size_t numSelectedPlanes = 0;
  std::array<size_t, 6> selectedPlaneIndices;

  if (this->_boundingPlanes[0].getPointDistance(position) > 0.0) {
    selectedPlaneIndices[numSelectedPlanes++] = 0;
  } else if (this->_boundingPlanes[1].getPointDistance(position) > 0.0) {
    selectedPlaneIndices[numSelectedPlanes++] = 1;
  }

  for (size_t i = 0; i < 4; i++) {
    size_t sidePlaneIndex = 2 + i;
    if (this->_boundingPlanes[sidePlaneIndex].getPointDistance(position) >
        0.0) {
      selectedPlaneIndices[numSelectedPlanes++] = sidePlaneIndex;
    }
  }

  // Check if inside all planes.
  if (numSelectedPlanes == 0) {
    return 0.0;
  }

  // We use the skip variable when the side plane indices are non-consecutive.
  if (numSelectedPlanes == 1) {
    // Handles Case I
    size_t planeIndex = selectedPlaneIndices[0];
    const Plane& selectedPlane = this->_boundingPlanes[planeIndex];
    std::array<glm::dvec3, 4> vertices =
        getPlaneVertices(this->_vertices, planeIndex);
    std::array<glm::dvec3, 4> edgeNormals =
        computeEdgeNormals(selectedPlane, vertices, planeIndex == 0);
    glm::dvec3 facePoint = closestPointPolygon(
        selectedPlane.projectPointOntoPlane(position),
        vertices,
        edgeNormals);

    return glm::distance2(facePoint, position);
  } else if (numSelectedPlanes == 2) {
    // Handles Case II
    // Since we are on the ellipsoid, the dihedral angle between a top plane and
    // a side plane will always be acute, so we can do a faster check there.
    if (selectedPlaneIndices[0] == 0) {
      glm::dvec3 facePoint = closestPointLineSegment(
          position,
          this->_vertices
              [4 * selectedPlaneIndices[0] + (selectedPlaneIndices[1] - 2)],
          this->_vertices
              [4 * selectedPlaneIndices[0] +
               ((selectedPlaneIndices[1] - 2 + 1) % 4)]);
      return glm::distance2(facePoint, position);
    }
    double minimumDistanceSquared = std::numeric_limits<double>::max();
    for (size_t i = 0; i < 2; i++) {
      size_t planeIndex = selectedPlaneIndices[i];
      const Plane& selectedPlane = this->_boundingPlanes[planeIndex];
      std::array<glm::dvec3, 4> vertices =
          getPlaneVertices(this->_vertices, planeIndex);
      std::array<glm::dvec3, 4> edgeNormals =
          computeEdgeNormals(selectedPlane, vertices, planeIndex == 0);
      glm::dvec3 facePoint = closestPointPolygon(
          selectedPlane.projectPointOntoPlane(position),
          vertices,
          edgeNormals);

      double distanceSquared = glm::distance2(facePoint, position);
      if (distanceSquared < minimumDistanceSquared) {
        minimumDistanceSquared = distanceSquared;
      }
    }
    return minimumDistanceSquared;
  } else if (numSelectedPlanes > 3) {
    // Handles Case IV
    std::array<glm::dvec3, 4> vertices = getPlaneVertices(this->_vertices, 1);
    glm::dvec3 facePoint = closestPointPolygon(
        this->_boundingPlanes[1].projectPointOntoPlane(position),
        vertices,
        computeEdgeNormals(this->_boundingPlanes[1], vertices, false));
    return glm::distance2(facePoint, position);
  }

  // Handles Case III
  size_t skip =
      selectedPlaneIndices[1] == 2 && selectedPlaneIndices[2] == 5 ? 0 : 1;

  // Vertex is on top plane.
  if (selectedPlaneIndices[0] == 0) {
    return glm::distance2(
        position,
        this->_vertices[(selectedPlaneIndices[1] - 2 + skip) % 4]);
  }

  // Vertex is on bottom plane.
  return glm::distance2(
      position,
      this->_vertices[4 + ((selectedPlaneIndices[1] - 2 + skip) % 4)]);
}

std::span<const CesiumGeometry::Plane>
S2CellBoundingVolume::getBoundingPlanes() const noexcept {
  return this->_boundingPlanes;
}

BoundingRegion S2CellBoundingVolume::computeBoundingRegion(
    const CesiumGeospatial::Ellipsoid& ellipsoid) const noexcept {
  return BoundingRegion(
      this->_cellID.computeBoundingRectangle(),
      this->_minimumHeight,
      this->_maximumHeight,
      ellipsoid);
}
