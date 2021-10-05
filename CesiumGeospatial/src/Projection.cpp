#include "CesiumGeospatial/Projection.h"

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
    const CesiumGeometry::AxisAlignedBox& box) {
  GlobeRectangle rectangle = unprojectRectangleSimple(
      projection,
      CesiumGeometry::Rectangle(
          box.minimumX,
          box.minimumY,
          box.maximumX,
          box.maximumY));
  return BoundingRegion(rectangle, box.minimumZ, box.maximumZ);
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
