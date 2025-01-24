#include <CesiumGeometry/AxisAlignedBox.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGeospatial/WebMercatorProjection.h>

#include <glm/common.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>

#include <variant>

namespace CesiumGeospatial {

glm::dvec3
projectPosition(const Projection& projection, const Cartographic& position) {
  struct Operation {
    const Cartographic& position;

    glm::dvec3 operator()(const GeographicProjection& geographic) noexcept {
      return geographic.project(position);
    }

    glm::dvec3 operator()(const WebMercatorProjection& webMercator) noexcept {
      return webMercator.project(position);
    }
  };

  return std::visit(Operation{position}, projection);
}

Cartographic
unprojectPosition(const Projection& projection, const glm::dvec3& position) {
  struct Operation {
    const glm::dvec3& position;

    Cartographic operator()(const GeographicProjection& geographic) noexcept {
      return geographic.unproject(position);
    }

    Cartographic operator()(const WebMercatorProjection& webMercator) noexcept {
      return webMercator.unproject(position);
    }
  };

  return std::visit(Operation{position}, projection);
}

CesiumGeometry::Rectangle projectRectangleSimple(
    const Projection& projection,
    const GlobeRectangle& rectangle) {
  struct Operation {
    const GlobeRectangle& rectangle;

    CesiumGeometry::Rectangle
    operator()(const GeographicProjection& geographic) noexcept {
      return geographic.project(rectangle);
    }

    CesiumGeometry::Rectangle
    operator()(const WebMercatorProjection& webMercator) noexcept {
      return webMercator.project(rectangle);
    }
  };

  return std::visit(Operation{rectangle}, projection);
}

GlobeRectangle unprojectRectangleSimple(
    const Projection& projection,
    const CesiumGeometry::Rectangle& rectangle) {
  struct Operation {
    const CesiumGeometry::Rectangle& rectangle;

    GlobeRectangle operator()(const GeographicProjection& geographic) noexcept {
      return geographic.unproject(rectangle);
    }

    GlobeRectangle
    operator()(const WebMercatorProjection& webMercator) noexcept {
      return webMercator.unproject(rectangle);
    }
  };

  return std::visit(Operation{rectangle}, projection);
}

CesiumGeometry::AxisAlignedBox projectRegionSimple(
    const Projection& projection,
    const BoundingRegion& region) {
  CesiumGeometry::Rectangle rectangle =
      projectRectangleSimple(projection, region.getRectangle());
  return CesiumGeometry::AxisAlignedBox(
      rectangle.minimumX,
      rectangle.minimumY,
      region.getMinimumHeight(),
      rectangle.maximumX,
      rectangle.maximumY,
      region.getMaximumHeight());
}

BoundingRegion unprojectRegionSimple(
    const Projection& projection,
    const CesiumGeometry::AxisAlignedBox& box,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  GlobeRectangle rectangle = unprojectRectangleSimple(
      projection,
      CesiumGeometry::Rectangle(
          box.minimumX,
          box.minimumY,
          box.maximumX,
          box.maximumY));
  return BoundingRegion(rectangle, box.minimumZ, box.maximumZ, ellipsoid);
}

glm::dvec2 computeProjectedRectangleSize(
    const Projection& projection,
    const CesiumGeometry::Rectangle& rectangle,
    double maxHeight,
    const Ellipsoid& ellipsoid) {
  glm::dvec3 ll = ellipsoid.cartographicToCartesian(unprojectPosition(
      projection,
      glm::dvec3(rectangle.getLowerLeft(), maxHeight)));
  glm::dvec3 lr = ellipsoid.cartographicToCartesian(unprojectPosition(
      projection,
      glm::dvec3(rectangle.getLowerRight(), maxHeight)));
  glm::dvec3 ul = ellipsoid.cartographicToCartesian(unprojectPosition(
      projection,
      glm::dvec3(rectangle.getUpperLeft(), maxHeight)));
  glm::dvec3 ur = ellipsoid.cartographicToCartesian(unprojectPosition(
      projection,
      glm::dvec3(rectangle.getUpperRight(), maxHeight)));

  double lowerDistance = glm::distance(ll, lr);
  double upperDistance = glm::distance(ul, ur);
  double leftDistance = glm::distance(ll, ul);
  double rightDistance = glm::distance(lr, ur);

  double x = glm::max(lowerDistance, upperDistance);
  double y = glm::max(leftDistance, rightDistance);

  // A rectangle that wraps around the whole globe might have an upper and lower
  // distance that are very small even though the rectangle is huge, because the
  // left and right edges of the rectangle are nearly on top of each other.
  // Detect that just by measuring the distance to a midpoint.
  // The bigger problem here is that we're measuring the Cartesian distance,
  // rather than the distance over the globe surface. But it's close enough
  // for our purposes except in this extreme case.
  glm::dvec3 lc = ellipsoid.cartographicToCartesian(unprojectPosition(
      projection,
      glm::dvec3(rectangle.getCenter().x, rectangle.minimumY, maxHeight)));
  glm::dvec3 uc = ellipsoid.cartographicToCartesian(unprojectPosition(
      projection,
      glm::dvec3(rectangle.getCenter().x, rectangle.maximumY, maxHeight)));

  double halfDistanceLA = glm::distance(ll, lc);
  double halfDistanceLB = glm::distance(lc, lr);
  double halfDistanceUA = glm::distance(ul, uc);
  double halfDistanceUB = glm::distance(uc, ur);

  if (halfDistanceLA > x || halfDistanceLB > x || halfDistanceUA > x ||
      halfDistanceUB > x) {
    x = glm::max(
        halfDistanceLA + halfDistanceLB,
        halfDistanceUA + halfDistanceUB);
  }

  // If either projected coordinate crosses zero, also check the distance at
  // zero. This is not completely robust, but works well enough for
  // currently supported projections at least.
  if (glm::sign(rectangle.minimumX) != glm::sign(rectangle.maximumX)) {
    glm::dvec3 top = ellipsoid.cartographicToCartesian(unprojectPosition(
        projection,
        glm::dvec3(0.0, rectangle.maximumY, maxHeight)));
    glm::dvec3 bottom = ellipsoid.cartographicToCartesian(unprojectPosition(
        projection,
        glm::dvec3(0.0, rectangle.minimumY, maxHeight)));
    double distance = glm::distance(top, bottom);
    y = glm::max(y, distance);
  }

  if (glm::sign(rectangle.minimumY) != glm::sign(rectangle.maximumY)) {
    glm::dvec3 left = ellipsoid.cartographicToCartesian(unprojectPosition(
        projection,
        glm::dvec3(rectangle.minimumX, 0.0, maxHeight)));
    glm::dvec3 right = ellipsoid.cartographicToCartesian(unprojectPosition(
        projection,
        glm::dvec3(rectangle.maximumX, 0.0, maxHeight)));
    double distance = glm::distance(left, right);
    x = glm::max(x, distance);

    // Also check for X (nearly) wrapping the whole globe.
    glm::dvec3 center = ellipsoid.cartographicToCartesian(unprojectPosition(
        projection,
        glm::dvec3(rectangle.getCenter().x, 0.0, maxHeight)));

    double halfDistanceL = glm::distance(left, center);
    double halfDistanceR = glm::distance(center, right);
    if (halfDistanceL > x || halfDistanceR > x) {
      x = halfDistanceL + halfDistanceR;
    }
  }

  return glm::dvec2(x, y);
}

const Ellipsoid& getProjectionEllipsoid(const Projection& projection) {
  struct Operation {
    const Ellipsoid&
    operator()(const GeographicProjection& geographic) noexcept {
      return geographic.getEllipsoid();
    }

    const Ellipsoid&
    operator()(const WebMercatorProjection& webMercator) noexcept {
      return webMercator.getEllipsoid();
    }
  };

  return std::visit(Operation{}, projection);
}
} // namespace CesiumGeospatial
