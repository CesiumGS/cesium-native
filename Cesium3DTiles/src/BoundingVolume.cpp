#include "Cesium3DTiles/BoundingVolume.h"
#include "CesiumGeospatial/Transforms.h"
#include "CesiumGeospatial/Cartographic.h"
#include <glm/gtc/matrix_inverse.hpp>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace Cesium3DTiles {

BoundingVolume transformBoundingVolume(
    const glm::dmat4x4& transform,
    const BoundingVolume& boundingVolume) {
  struct Operation {
    const glm::dmat4x4& transform;

    BoundingVolume operator()(const OrientedBoundingBox& boundingBox) {
      glm::dvec3 center = transform * glm::dvec4(boundingBox.getCenter(), 1.0);
      glm::dmat3 halfAxes = glm::dmat3(transform) * boundingBox.getHalfAxes();
      return OrientedBoundingBox(center, halfAxes);
    }

    BoundingVolume operator()(const BoundingRegion& boundingRegion) {
      // Regions are not transformed.
      return boundingRegion;
    }

    BoundingVolume operator()(const BoundingSphere& boundingSphere) {
      glm::dvec3 center =
          transform * glm::dvec4(boundingSphere.getCenter(), 1.0);

      double uniformScale = glm::max(
          glm::max(
              glm::length(glm::dvec3(transform[0])),
              glm::length(glm::dvec3(transform[1]))),
          glm::length(glm::dvec3(transform[2])));

      return BoundingSphere(center, boundingSphere.getRadius() * uniformScale);
    }

    BoundingVolume
    operator()(const BoundingRegionWithLooseFittingHeights& boundingRegion) {
      // Regions are not transformed.
      return boundingRegion;
    }
  };

  return std::visit(Operation{transform}, boundingVolume);
}

glm::dvec3 getBoundingVolumeCenter(const BoundingVolume& boundingVolume) {
  struct Operation {
    glm::dvec3 operator()(const OrientedBoundingBox& boundingBox) {
      return boundingBox.getCenter();
    }

    glm::dvec3 operator()(const BoundingRegion& boundingRegion) {
      return boundingRegion.getBoundingBox().getCenter();
    }

    glm::dvec3 operator()(const BoundingSphere& boundingSphere) {
      return boundingSphere.getCenter();
    }

    glm::dvec3
    operator()(const BoundingRegionWithLooseFittingHeights& boundingRegion) {
      return boundingRegion.getBoundingRegion().getBoundingBox().getCenter();
    }
  };

  return std::visit(Operation{}, boundingVolume);
}

// TODO: Test this more thoroughly
std::optional<GlobeRectangle> getGlobeRectangle(const BoundingVolume& boundingVolume) {
  struct Operation {
    std::optional<GlobeRectangle>
    operator()(const BoundingSphere& boundingSphere) {
      const glm::dvec3& centerEcef = boundingSphere.getCenter();
      double radius = boundingSphere.getRadius();

      glm::dmat4 enuToEcef =
          Transforms::eastNorthUpToFixedFrame(
              centerEcef /*, ellipsoid*/);
      glm::dmat4 ecefBounds = enuToEcef * glm::dmat4(
                                              radius,
                                              0.0,
                                              0.0,
                                              1.0,
                                              -radius,
                                              0.0,
                                              0.0,
                                              1.0,
                                              0.0,
                                              radius,
                                              0.0,
                                              1.0,
                                              0.0,
                                              -radius,
                                              0.0,
                                              1.0);

      std::optional<Cartographic> east =
          Ellipsoid::WGS84.cartesianToCartographic(
              ecefBounds[0]);
      std::optional<Cartographic> west =
          Ellipsoid::WGS84.cartesianToCartographic(
              ecefBounds[1]);
      std::optional<Cartographic> north =
          Ellipsoid::WGS84.cartesianToCartographic(
              ecefBounds[2]);
      std::optional<Cartographic> south =
          Ellipsoid::WGS84.cartesianToCartographic(
              ecefBounds[3]);

      if (!east || !west || !north || !south) {
        return std::nullopt;
      }

      return GlobeRectangle(
          west->longitude,
          south->latitude,
          east->longitude,
          north->latitude);
    }

    std::optional<GlobeRectangle>
    operator()(const OrientedBoundingBox& orientedBoundingBox) {
      const glm::dvec3& centerEcef = orientedBoundingBox.getCenter();
      glm::dmat3 enuToEcef =
          Transforms::eastNorthUpToFixedFrame(
              centerEcef /*, ellipsoid*/);
      const glm::dmat3& halfAxes = orientedBoundingBox.getHalfAxes();
      glm::dmat3 obbToEnu = glm::affineInverse(enuToEcef) * halfAxes;

      constexpr glm::dvec3 obbVertices[] = {
          glm::dvec3(1.0, 1.0, 1.0),
          glm::dvec3(1.0, 1.0, -1.0),
          glm::dvec3(1.0, -1.0, 1.0),
          glm::dvec3(1.0, -1.0, -1.0),
          glm::dvec3(-1.0, 1.0, 1.0),
          glm::dvec3(-1.0, 1.0, -1.0),
          glm::dvec3(-1.0, -1.0, 1.0),
          glm::dvec3(-1.0, -1.0, -1.0)};

      glm::dvec3 vert0Enu = obbToEnu * obbVertices[0];

      int8_t eastmostIndex = 0;
      double eastmostProjection = vert0Enu.x;
      int8_t westmostIndex = 0;
      double westmostProjection = vert0Enu.x;
      int8_t northmostIndex = 0;
      double northmostProjection = vert0Enu.y;
      int8_t southmostIndex = 0;
      double southmostProjection = vert0Enu.y;

      for (int8_t i = 1; i < 8; ++i) {
        glm::dvec3 vertEnu = obbToEnu * obbVertices[i];
        if (vertEnu.x > eastmostProjection) {
          eastmostProjection = vertEnu.x;
          eastmostIndex = i;
        }
        if (vertEnu.x < westmostProjection) {
          westmostProjection = vertEnu.x;
          westmostIndex = i;
        }
        if (vertEnu.y > northmostProjection) {
          northmostProjection = vertEnu.y;
          northmostIndex = i;
        }
        if (vertEnu.y < southmostProjection) {
          southmostProjection = vertEnu.y;
          southmostIndex = i;
        }
      }

      std::optional<Cartographic> east =
          Ellipsoid::WGS84.cartesianToCartographic(
              halfAxes * obbVertices[eastmostIndex] + centerEcef);
      std::optional<Cartographic> west =
          Ellipsoid::WGS84.cartesianToCartographic(
              halfAxes * obbVertices[westmostIndex] + centerEcef);
      // TODO: why do these have to be switched??
      std::optional<Cartographic> north =
          Ellipsoid::WGS84.cartesianToCartographic(
              halfAxes * obbVertices[southmostIndex] + centerEcef);
      std::optional<Cartographic> south =
          Ellipsoid::WGS84.cartesianToCartographic(
              halfAxes * obbVertices[northmostIndex] + centerEcef);

      if (!east || !west || !north || !south) {
        return std::nullopt;
      }

      return GlobeRectangle(
          west->longitude,
          south->latitude,
          east->longitude,
          north->latitude);
    }

    std::optional<GlobeRectangle>
    operator()(const BoundingRegion& boundingRegion) {
      return boundingRegion.getRectangle();
    }

    std::optional<GlobeRectangle>
    operator()(const BoundingRegionWithLooseFittingHeights&
                  boundingRegionWithLooseFittingHeights) {
      return boundingRegionWithLooseFittingHeights.getBoundingRegion()
          .getRectangle();
    }
  };

  return std::visit(Operation{}, boundingVolume);
}

} // namespace Cesium3DTiles
