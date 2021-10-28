#include "CesiumGeospatial/Projection.h"

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

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

  // If either projected coordinate crosses zero, also check the distance at
  // zero. This is not completely robust, but works well enough for currently
  // supported projections at least.
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
  }

  return glm::dvec2(x, y);
}

double computeApproximateConversionFactorToMetersNearPosition(
    const Projection& projection,
    const glm::dvec2& position) {
  struct Operation {
    const glm::dvec2& position;

    double operator()(const GeographicProjection& /*geographic*/) noexcept {
      return 1.0;
    }

    double operator()(const WebMercatorProjection& webMercator) noexcept {
      // TODO: is there a better estimate?
      return glm::cos(webMercator.unproject(position).latitude);
    }
  };

  return std::visit(Operation{position}, projection);
}
} // namespace CesiumGeospatial
