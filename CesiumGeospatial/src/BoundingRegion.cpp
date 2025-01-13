#include <CesiumGeometry/CullingResult.h>
#include <CesiumGeometry/IntersectionTests.h>
#include <CesiumGeometry/Plane.h>
#include <CesiumGeometry/Ray.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/EllipsoidTangentPlane.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumUtility/Math.h>

#include <glm/common.hpp>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>

#include <optional>
#include <stdexcept>

using namespace CesiumUtility;
using namespace CesiumGeometry;

namespace CesiumGeospatial {
BoundingRegion::BoundingRegion(
    const GlobeRectangle& rectangle,
    double minimumHeight,
    double maximumHeight,
    const Ellipsoid& ellipsoid)
    : _rectangle(rectangle),
      _minimumHeight(minimumHeight),
      _maximumHeight(maximumHeight),
      _boundingBox(BoundingRegion::_computeBoundingBox(
          rectangle,
          minimumHeight,
          maximumHeight,
          ellipsoid)),
      _southwestCornerCartesian(
          ellipsoid.cartographicToCartesian(rectangle.getSouthwest())),
      _northeastCornerCartesian(
          ellipsoid.cartographicToCartesian(rectangle.getNortheast())),
      _westNormal(),
      _eastNormal(),
      _southNormal(),
      _northNormal(),
      _planesAreInvalid(false) {
  // The middle latitude on the western edge.
  const glm::dvec3 westernMidpointCartesian =
      ellipsoid.cartographicToCartesian(Cartographic(
          rectangle.getWest(),
          (rectangle.getSouth() + rectangle.getNorth()) * 0.5,
          0.0));

  // Compute the normal of the plane on the western edge of the tile.
  this->_westNormal = glm::normalize(
      glm::cross(westernMidpointCartesian, glm::dvec3(0.0, 0.0, 1.0)));

  // The middle latitude on the eastern edge.
  const glm::dvec3 easternMidpointCartesian =
      ellipsoid.cartographicToCartesian(Cartographic(
          rectangle.getEast(),
          (rectangle.getSouth() + rectangle.getNorth()) * 0.5,
          0.0));

  // Compute the normal of the plane on the eastern edge of the tile.
  this->_eastNormal = glm::normalize(
      glm::cross(glm::dvec3(0.0, 0.0, 1.0), easternMidpointCartesian));

  // Compute the normal of the plane bounding the southern edge of the tile.
  const glm::dvec3 westVector =
      westernMidpointCartesian - easternMidpointCartesian;
  const glm::dvec3 eastWestNormal = glm::normalize(westVector);

  if (!Math::equalsEpsilon(glm::length(eastWestNormal), 1.0, Math::Epsilon6)) {
    this->_planesAreInvalid = true;
    return;
  }

  const double south = rectangle.getSouth();
  glm::dvec3 southSurfaceNormal;

  if (south > 0.0) {
    // Compute a plane that doesn't cut through the tile.
    const glm::dvec3 southCenterCartesian =
        ellipsoid.cartographicToCartesian(Cartographic(
            (rectangle.getWest() + rectangle.getEast()) * 0.5,
            south,
            0.0));
    const Plane westPlane(this->_southwestCornerCartesian, this->_westNormal);

    // Find a point that is on the west and the south planes
    std::optional<glm::dvec3> intersection = IntersectionTests::rayPlane(
        Ray(southCenterCartesian, eastWestNormal),
        westPlane);

    if (!intersection) {
      this->_planesAreInvalid = true;
      return;
    }

    this->_southwestCornerCartesian = intersection.value();
    southSurfaceNormal = ellipsoid.geodeticSurfaceNormal(southCenterCartesian);
  } else {
    southSurfaceNormal =
        ellipsoid.geodeticSurfaceNormal(rectangle.getSoutheast());
  }
  this->_southNormal =
      glm::normalize(glm::cross(southSurfaceNormal, westVector));

  // Compute the normal of the plane bounding the northern edge of the tile.
  const double north = rectangle.getNorth();
  glm::dvec3 northSurfaceNormal;
  if (north < 0.0) {
    // Compute a plane that doesn't cut through the tile.
    const glm::dvec3 northCenterCartesian =
        ellipsoid.cartographicToCartesian(Cartographic(
            (rectangle.getWest() + rectangle.getEast()) * 0.5,
            north,
            0.0));

    const Plane eastPlane(this->_northeastCornerCartesian, this->_eastNormal);

    // Find a point that is on the east and the north planes
    std::optional<glm::dvec3> intersection = IntersectionTests::rayPlane(
        Ray(northCenterCartesian, -eastWestNormal),
        eastPlane);

    if (!intersection) {
      this->_planesAreInvalid = true;
      return;
    }

    this->_northeastCornerCartesian = intersection.value();
    northSurfaceNormal = ellipsoid.geodeticSurfaceNormal(northCenterCartesian);
  } else {
    northSurfaceNormal =
        ellipsoid.geodeticSurfaceNormal(rectangle.getNorthwest());
  }
  this->_northNormal =
      glm::normalize(glm::cross(westVector, northSurfaceNormal));
}

CullingResult
BoundingRegion::intersectPlane(const Plane& plane) const noexcept {
  return this->_boundingBox.intersectPlane(plane);
}

double BoundingRegion::computeDistanceSquaredToPosition(
    const glm::dvec3& position,
    const Ellipsoid& ellipsoid) const noexcept {
  std::optional<Cartographic> cartographic =
      ellipsoid.cartesianToCartographic(position);
  if (!cartographic) {
    return this->_boundingBox.computeDistanceSquaredToPosition(position);
  }

  return this->computeDistanceSquaredToPosition(cartographic.value(), position);
}

double BoundingRegion::computeDistanceSquaredToPosition(
    const Cartographic& position,
    const Ellipsoid& ellipsoid) const noexcept {
  return this->computeDistanceSquaredToPosition(
      position,
      ellipsoid.cartographicToCartesian(position));
}

double BoundingRegion::computeDistanceSquaredToPosition(
    const Cartographic& cartographicPosition,
    const glm::dvec3& cartesianPosition) const noexcept {
  double result = 0.0;

  if (!this->_planesAreInvalid) {
    if (!this->_rectangle.contains(cartographicPosition)) {
      const glm::dvec3& southwestCornerCartesian =
          this->_southwestCornerCartesian;
      const glm::dvec3& northeastCornerCartesian =
          this->_northeastCornerCartesian;
      const glm::dvec3& westNormal = this->_westNormal;
      const glm::dvec3& southNormal = this->_southNormal;
      const glm::dvec3& eastNormal = this->_eastNormal;
      const glm::dvec3& northNormal = this->_northNormal;

      const glm::dvec3 vectorFromSouthwestCorner =
          cartesianPosition - southwestCornerCartesian;
      const double distanceToWestPlane =
          glm::dot(vectorFromSouthwestCorner, westNormal);
      const double distanceToSouthPlane =
          glm::dot(vectorFromSouthwestCorner, southNormal);

      const glm::dvec3 vectorFromNortheastCorner =
          cartesianPosition - northeastCornerCartesian;
      const double distanceToEastPlane =
          glm::dot(vectorFromNortheastCorner, eastNormal);
      const double distanceToNorthPlane =
          glm::dot(vectorFromNortheastCorner, northNormal);

      if (distanceToWestPlane > 0.0) {
        result += distanceToWestPlane * distanceToWestPlane;
      } else if (distanceToEastPlane > 0.0) {
        result += distanceToEastPlane * distanceToEastPlane;
      }

      if (distanceToSouthPlane > 0.0) {
        result += distanceToSouthPlane * distanceToSouthPlane;
      } else if (distanceToNorthPlane > 0.0) {
        result += distanceToNorthPlane * distanceToNorthPlane;
      }
    }

    const double cameraHeight = cartographicPosition.height;
    const double minimumHeight = this->_minimumHeight;
    const double maximumHeight = this->_maximumHeight;

    if (cameraHeight > maximumHeight) {
      const double distanceAboveTop = cameraHeight - maximumHeight;
      result += distanceAboveTop * distanceAboveTop;
    } else if (cameraHeight < minimumHeight) {
      const double distanceBelowBottom = minimumHeight - cameraHeight;
      result += distanceBelowBottom * distanceBelowBottom;
    }
  }

  const double bboxDistanceSquared =
      this->getBoundingBox().computeDistanceSquaredToPosition(
          cartesianPosition);
  return glm::max(bboxDistanceSquared, result);
}

BoundingRegion BoundingRegion::computeUnion(
    const BoundingRegion& other,
    const CesiumGeospatial::Ellipsoid& ellipsoid) const noexcept {
  return BoundingRegion(
      this->_rectangle.computeUnion(other._rectangle),
      glm::min(this->_minimumHeight, other._minimumHeight),
      glm::max(this->_maximumHeight, other._maximumHeight),
      ellipsoid);
}

namespace {
OrientedBoundingBox fromPlaneExtents(
    const glm::dvec3& planeOrigin,
    const glm::dvec3& planeXAxis,
    const glm::dvec3& planeYAxis,
    const glm::dvec3& planeZAxis,
    double minimumX,
    double maximumX,
    double minimumY,
    double maximumY,
    double minimumZ,
    double maximumZ) noexcept {
  glm::dmat3 halfAxes(planeXAxis, planeYAxis, planeZAxis);

  const glm::dvec3 centerOffset(
      (minimumX + maximumX) / 2.0,
      (minimumY + maximumY) / 2.0,
      (minimumZ + maximumZ) / 2.0);

  const glm::dvec3 scale(
      (maximumX - minimumX) / 2.0,
      (maximumY - minimumY) / 2.0,
      (maximumZ - minimumZ) / 2.0);

  const glm::dmat3 scaledHalfAxes(
      halfAxes[0] * scale.x,
      halfAxes[1] * scale.y,
      halfAxes[2] * scale.z);

  return OrientedBoundingBox(
      planeOrigin + (halfAxes * centerOffset),
      scaledHalfAxes);
}
} // namespace

/*static*/ OrientedBoundingBox BoundingRegion::_computeBoundingBox(
    const GlobeRectangle& rectangle,
    double minimumHeight,
    double maximumHeight,
    const Ellipsoid& ellipsoid) {
  //>>includeStart('debug', pragmas.debug);
  if (!Math::equalsEpsilon(
          ellipsoid.getRadii().x,
          ellipsoid.getRadii().y,
          Math::Epsilon15)) {
    throw std::runtime_error(
        "Ellipsoid must be an ellipsoid of revolution (radii.x == radii.y)");
  }
  //>>includeEnd('debug');

  double minX, maxX, minY, maxY, minZ, maxZ;
  Plane plane(glm::dvec3(0.0, 0.0, 1.0), 0.0);

  if (rectangle.computeWidth() <= Math::OnePi) {
    // The bounding box will be aligned with the tangent plane at the center of
    // the rectangle.
    const Cartographic tangentPointCartographic = rectangle.computeCenter();
    const glm::dvec3 tangentPoint =
        ellipsoid.cartographicToCartesian(tangentPointCartographic);
    const EllipsoidTangentPlane tangentPlane(tangentPoint, ellipsoid);
    plane = tangentPlane.getPlane();

    // If the rectangle spans the equator, CW is instead aligned with the
    // equator (because it sticks out the farthest at the equator).
    const double lonCenter = tangentPointCartographic.longitude;
    const double latCenter =
        rectangle.getSouth() < 0.0 && rectangle.getNorth() > 0.0
            ? 0.0
            : tangentPointCartographic.latitude;

    // Compute XY extents using the rectangle at maximum height
    const Cartographic perimeterCartographicNC(
        lonCenter,
        rectangle.getNorth(),
        maximumHeight);
    Cartographic perimeterCartographicNW(
        rectangle.getWest(),
        rectangle.getNorth(),
        maximumHeight);
    const Cartographic perimeterCartographicCW(
        rectangle.getWest(),
        latCenter,
        maximumHeight);
    Cartographic perimeterCartographicSW(
        rectangle.getWest(),
        rectangle.getSouth(),
        maximumHeight);
    const Cartographic perimeterCartographicSC(
        lonCenter,
        rectangle.getSouth(),
        maximumHeight);

    const glm::dvec3 perimeterCartesianNC =
        ellipsoid.cartographicToCartesian(perimeterCartographicNC);
    glm::dvec3 perimeterCartesianNW =
        ellipsoid.cartographicToCartesian(perimeterCartographicNW);
    const glm::dvec3 perimeterCartesianCW =
        ellipsoid.cartographicToCartesian(perimeterCartographicCW);
    glm::dvec3 perimeterCartesianSW =
        ellipsoid.cartographicToCartesian(perimeterCartographicSW);
    const glm::dvec3 perimeterCartesianSC =
        ellipsoid.cartographicToCartesian(perimeterCartographicSC);

    const glm::dvec2 perimeterProjectedNC =
        tangentPlane.projectPointToNearestOnPlane(perimeterCartesianNC);
    const glm::dvec2 perimeterProjectedNW =
        tangentPlane.projectPointToNearestOnPlane(perimeterCartesianNW);
    const glm::dvec2 perimeterProjectedCW =
        tangentPlane.projectPointToNearestOnPlane(perimeterCartesianCW);
    const glm::dvec2 perimeterProjectedSW =
        tangentPlane.projectPointToNearestOnPlane(perimeterCartesianSW);
    const glm::dvec2 perimeterProjectedSC =
        tangentPlane.projectPointToNearestOnPlane(perimeterCartesianSC);

    minX = glm::min(
        glm::min(perimeterProjectedNW.x, perimeterProjectedCW.x),
        perimeterProjectedSW.x);

    maxX = -minX; // symmetrical

    maxY = glm::max(perimeterProjectedNW.y, perimeterProjectedNC.y);
    minY = glm::min(perimeterProjectedSW.y, perimeterProjectedSC.y);

    // Compute minimum Z using the rectangle at minimum height, since it will be
    // deeper than the maximum height
    perimeterCartographicNW.height = perimeterCartographicSW.height =
        minimumHeight;
    perimeterCartesianNW =
        ellipsoid.cartographicToCartesian(perimeterCartographicNW);
    perimeterCartesianSW =
        ellipsoid.cartographicToCartesian(perimeterCartographicSW);

    minZ = glm::min(
        plane.getPointDistance(perimeterCartesianNW),
        plane.getPointDistance(perimeterCartesianSW));
    maxZ = maximumHeight; // Since the tangent plane touches the surface at
                          // height = 0, this is okay

    // Esure our box is at least a millimeter in each direction to avoid
    // problems with degenerate or nearly-degenerate bounding regions.
    const double oneMillimeter = 0.001;
    if (maxX - minX < oneMillimeter) {
      minX -= oneMillimeter * 0.5;
      maxX += oneMillimeter * 0.5;
    }
    if (maxY - minY < oneMillimeter) {
      minY -= oneMillimeter * 0.5;
      maxY += oneMillimeter * 0.5;
    }
    if (maxZ - minZ < oneMillimeter) {
      minZ -= oneMillimeter * 0.5;
      maxZ += oneMillimeter * 0.5;
    }

    return fromPlaneExtents(
        tangentPlane.getOrigin(),
        tangentPlane.getXAxis(),
        tangentPlane.getYAxis(),
        tangentPlane.getZAxis(),
        minX,
        maxX,
        minY,
        maxY,
        minZ,
        maxZ);
  }

  // Handle the case where rectangle width is greater than PI (wraps around more
  // than half the ellipsoid).
  const bool fullyAboveEquator = rectangle.getSouth() > 0.0;
  const bool fullyBelowEquator = rectangle.getNorth() < 0.0;
  const double latitudeNearestToEquator =
      fullyAboveEquator   ? rectangle.getSouth()
      : fullyBelowEquator ? rectangle.getNorth()
                          : 0.0;
  const double centerLongitude = rectangle.computeCenter().longitude;

  // Plane is located at the rectangle's center longitude and the rectangle's
  // latitude that is closest to the equator. It rotates around the Z axis. This
  // results in a better fit than the obb approach for smaller rectangles, which
  // orients with the rectangle's center normal.
  glm::dvec3 planeOrigin = ellipsoid.cartographicToCartesian(
      Cartographic(centerLongitude, latitudeNearestToEquator, maximumHeight));
  planeOrigin.z = 0.0; // center the plane on the equator to simpify plane
                       // normal calculation
  const bool isPole = glm::abs(planeOrigin.x) < Math::Epsilon10 &&
                      glm::abs(planeOrigin.y) < Math::Epsilon10;
  const glm::dvec3 planeNormal =
      !isPole ? glm::normalize(planeOrigin) : glm::dvec3(1.0, 0.0, 0.0);
  const glm::dvec3 planeYAxis(0.0, 0.0, 1.0);
  const glm::dvec3 planeXAxis = glm::cross(planeNormal, planeYAxis);
  plane = Plane(planeOrigin, planeNormal);

  // Get the horizon point relative to the center. This will be the farthest
  // extent in the plane's X dimension.
  const glm::dvec3 horizonCartesian =
      ellipsoid.cartographicToCartesian(Cartographic(
          centerLongitude + Math::PiOverTwo,
          latitudeNearestToEquator,
          maximumHeight));
  maxX = glm::dot(plane.projectPointOntoPlane(horizonCartesian), planeXAxis);
  minX = -maxX; // symmetrical

  // Get the min and max Y, using the height that will give the largest extent
  maxY = ellipsoid
             .cartographicToCartesian(Cartographic(
                 0.0,
                 rectangle.getNorth(),
                 fullyBelowEquator ? minimumHeight : maximumHeight))
             .z;
  minY = ellipsoid
             .cartographicToCartesian(Cartographic(
                 0.0,
                 rectangle.getSouth(),
                 fullyAboveEquator ? minimumHeight : maximumHeight))
             .z;
  const glm::dvec3 farZ = ellipsoid.cartographicToCartesian(Cartographic(
      rectangle.getEast(),
      latitudeNearestToEquator,
      maximumHeight));
  minZ = plane.getPointDistance(farZ);
  maxZ = 0.0; // plane origin starts at maxZ already

  // min and max are local to the plane axes
  return fromPlaneExtents(
      planeOrigin,
      planeXAxis,
      planeYAxis,
      planeNormal,
      minX,
      maxX,
      minY,
      maxY,
      minZ,
      maxZ);
}

} // namespace CesiumGeospatial
