#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <CesiumGeometry/BoundingCylinderRegion.h>
#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/BoundingRegionWithLooseFittingHeights.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/GlobeTransforms.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>

#include <glm/common.hpp>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/vector_double3.hpp>

#include <cstdint>
#include <optional>
#include <type_traits>
#include <variant>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace Cesium3DTilesSelection {

BoundingVolume transformBoundingVolume(
    const glm::dmat4x4& transform,
    const BoundingVolume& boundingVolume) {
  struct Operation {
    const glm::dmat4x4& transform;

    BoundingVolume operator()(const OrientedBoundingBox& boundingBox) {
      return boundingBox.transform(transform);
    }

    BoundingVolume operator()(const BoundingRegion& boundingRegion) noexcept {
      // Regions are not transformed.
      return boundingRegion;
    }

    BoundingVolume operator()(const BoundingSphere& boundingSphere) {
      return boundingSphere.transform(transform);
    }

    BoundingVolume operator()(
        const BoundingRegionWithLooseFittingHeights& boundingRegion) noexcept {
      // Regions are not transformed.
      return boundingRegion;
    }

    BoundingVolume
    operator()(const S2CellBoundingVolume& s2CellBoundingVolume) noexcept {
      // S2 Cells are not transformed.
      return s2CellBoundingVolume;
    }

    BoundingVolume
    operator()(const BoundingCylinderRegion& boundingCylinderRegion) {
      return boundingCylinderRegion.transform(transform);
    }
  };

  return std::visit(Operation{transform}, boundingVolume);
}

glm::dvec3 getBoundingVolumeCenter(const BoundingVolume& boundingVolume) {
  struct Operation {
    glm::dvec3 operator()(const OrientedBoundingBox& boundingBox) noexcept {
      return boundingBox.getCenter();
    }

    glm::dvec3 operator()(const BoundingRegion& boundingRegion) noexcept {
      return boundingRegion.getBoundingBox().getCenter();
    }

    glm::dvec3 operator()(const BoundingSphere& boundingSphere) noexcept {
      return boundingSphere.getCenter();
    }

    glm::dvec3 operator()(
        const BoundingRegionWithLooseFittingHeights& boundingRegion) noexcept {
      return boundingRegion.getBoundingRegion().getBoundingBox().getCenter();
    }

    glm::dvec3 operator()(const S2CellBoundingVolume& s2Cell) noexcept {
      return s2Cell.getCenter();
    }

    glm::dvec3
    operator()(const BoundingCylinderRegion& boundingCylinderRegion) {
      return boundingCylinderRegion.getCenter();
    }
  };

  return std::visit(Operation{}, boundingVolume);
}

std::optional<GlobeRectangle> estimateGlobeRectangle(
    const BoundingVolume& boundingVolume,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  struct Operation {
    const CesiumGeospatial::Ellipsoid& ellipsoid;

    std::optional<GlobeRectangle>
    operator()(const BoundingSphere& boundingSphere) {
      if (boundingSphere.contains(glm::dvec3(0.0))) {
        return CesiumGeospatial::GlobeRectangle::MAXIMUM;
      }

      const glm::dvec3& centerEcef = boundingSphere.getCenter();
      double radius = boundingSphere.getRadius();

      glm::dmat4 enuToEcef =
          GlobeTransforms::eastNorthUpToFixedFrame(centerEcef, ellipsoid);
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
          this->ellipsoid.cartesianToCartographic(glm::dvec3(ecefBounds[0]));
      std::optional<Cartographic> west =
          this->ellipsoid.cartesianToCartographic(glm::dvec3(ecefBounds[1]));
      std::optional<Cartographic> north =
          this->ellipsoid.cartesianToCartographic(glm::dvec3(ecefBounds[2]));
      std::optional<Cartographic> south =
          this->ellipsoid.cartesianToCartographic(glm::dvec3(ecefBounds[3]));

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
      if (orientedBoundingBox.contains(glm::dvec3(0.0))) {
        return CesiumGeospatial::GlobeRectangle::MAXIMUM;
      }

      const glm::dvec3& centerEcef = orientedBoundingBox.getCenter();
      const glm::dmat3& halfAxes = orientedBoundingBox.getHalfAxes();

      constexpr glm::dvec3 obbVertices[] = {
          glm::dvec3(1.0, 1.0, 1.0),
          glm::dvec3(1.0, 1.0, -1.0),
          glm::dvec3(1.0, -1.0, 1.0),
          glm::dvec3(1.0, -1.0, -1.0),
          glm::dvec3(-1.0, 1.0, 1.0),
          glm::dvec3(-1.0, 1.0, -1.0),
          glm::dvec3(-1.0, -1.0, 1.0),
          glm::dvec3(-1.0, -1.0, -1.0)};

      glm::dvec3 vert0Ecef = centerEcef + halfAxes * obbVertices[0];
      std::optional<Cartographic> c =
          this->ellipsoid.cartesianToCartographic(vert0Ecef);
      if (!c) {
        return std::nullopt;
      }

      double west = c->longitude;
      double south = c->latitude;
      double east = c->longitude;
      double north = c->latitude;

      for (int8_t i = 1; i < 8; ++i) {
        glm::dvec3 vertEcef = centerEcef + halfAxes * obbVertices[i];
        c = this->ellipsoid.cartesianToCartographic(vertEcef);
        if (!c) {
          return std::nullopt;
        }

        west = glm::min(west, c->longitude);
        south = glm::min(south, c->latitude);
        east = glm::max(east, c->longitude);
        north = glm::max(north, c->latitude);
      }

      return GlobeRectangle(west, south, east, north);
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

    std::optional<GlobeRectangle>
    operator()(const S2CellBoundingVolume& s2Cell) {
      return s2Cell.getCellID().computeBoundingRectangle();
    }

    std::optional<GlobeRectangle>
    operator()(const BoundingCylinderRegion& boundingCylinderRegion) {
      return operator()(boundingCylinderRegion.toOrientedBoundingBox());
    }
  };

  return std::visit(Operation{ellipsoid}, boundingVolume);
}

const CesiumGeospatial::BoundingRegion*
getBoundingRegionFromBoundingVolume(const BoundingVolume& boundingVolume) {
  const BoundingRegion* pResult = std::get_if<BoundingRegion>(&boundingVolume);
  if (!pResult) {
    const BoundingRegionWithLooseFittingHeights* pLoose =
        std::get_if<BoundingRegionWithLooseFittingHeights>(&boundingVolume);
    if (pLoose) {
      pResult = &pLoose->getBoundingRegion();
    }
  }
  return pResult;
}

OrientedBoundingBox getOrientedBoundingBoxFromBoundingVolume(
    const BoundingVolume& boundingVolume,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  struct Operation {
    const CesiumGeospatial::Ellipsoid& ellipsoid;

    OrientedBoundingBox operator()(const BoundingSphere& sphere) const {
      return OrientedBoundingBox::fromSphere(sphere);
    }

    OrientedBoundingBox
    operator()(const CesiumGeometry::OrientedBoundingBox& box) const {
      return box;
    }

    OrientedBoundingBox
    operator()(const CesiumGeospatial::BoundingRegion& region) const {
      return region.getBoundingBox();
    }

    OrientedBoundingBox operator()(
        const CesiumGeospatial::BoundingRegionWithLooseFittingHeights& region)
        const {
      return region.getBoundingRegion().getBoundingBox();
    }

    OrientedBoundingBox
    operator()(const CesiumGeospatial::S2CellBoundingVolume& s2) const {
      return s2.computeBoundingRegion(ellipsoid).getBoundingBox();
    }

    OrientedBoundingBox
    operator()(const BoundingCylinderRegion& cylinderRegion) const {
      return cylinderRegion.toOrientedBoundingBox();
    }
  };

  return std::visit(Operation{ellipsoid}, boundingVolume);
}

namespace {

template <typename BV>
bool intersectsImpl(const BoundingSphere& bs, const BV& bv1) {
  double d2 = bv1.computeDistanceSquaredToPosition(bs.getCenter());
  return d2 < bs.getRadius() * bs.getRadius();
}
bool intersectsImpl(
    const BoundingSphere& bs,
    const BoundingRegionWithLooseFittingHeights& bv1) {
  double d2 = bv1.computeConservativeDistanceSquaredToPosition(bs.getCenter());
  return d2 < bs.getRadius() * bs.getRadius();
}

bool intersectsImpl(
    const OrientedBoundingBox& obb0,
    const OrientedBoundingBox& obb1) {
  return intersects(obb0, obb1);
}

bool intersectsImpl(const OrientedBoundingBox& obb, const BoundingRegion& br) {
  return intersects(obb, br.getBoundingBox());
}

bool intersectsImpl(
    const OrientedBoundingBox& obb,
    const BoundingCylinderRegion& bcr) {
  return intersects(obb, bcr.toOrientedBoundingBox());
}

template <typename BV>
bool intersectsImpl(const OrientedBoundingBox& obb, const BV& bv) {
  return intersectsImpl(obb.toSphere(), bv);
}

template <typename BV>
bool intersectsImpl(const BoundingRegion& br, const BV& bv) {
  return intersectsImpl(br.getBoundingBox(), bv);
}

} // namespace

bool testIntersection(
    const BoundingVolume& volume0,
    const BoundingVolume& volume1) {
  return visit(
      [&](auto&& bv0, auto&& bv1) -> bool {
        // If one of the volumes is a Bounding sphere, then don't bother to
        // convert; just test against the sphere.
        if constexpr (std::is_same_v<decltype(bv0), const BoundingSphere&>) {
          return intersectsImpl(bv0, bv1);
        } else if constexpr (std::is_same_v<
                                 decltype(bv1),
                                 const BoundingSphere&>) {
          return intersectsImpl(bv1, bv0);
          ;
        }
        if constexpr (std::is_same_v<
                          decltype(bv0),
                          const OrientedBoundingBox&>) {
          return intersectsImpl(bv0, bv1);
        } else if constexpr (std::is_same_v<
                                 decltype(bv1),
                                 const OrientedBoundingBox&>) {
          return intersectsImpl(bv1, bv0);
        }
        if constexpr (std::is_same_v<decltype(bv0), const BoundingRegion&>) {
          return intersectsImpl(bv0, bv1);
        } else if constexpr (std::is_same_v<
                                 decltype(bv1),
                                 const BoundingRegion&>) {
          return intersectsImpl(bv1, bv0);
        }
        if constexpr (std::is_same_v<
                          decltype(bv0),
                          const BoundingRegionWithLooseFittingHeights&>) {
          return intersectsImpl(bv0.getBoundingRegion(), bv1);
        } else if constexpr (
            std::is_same_v<
                decltype(bv1),
                const BoundingRegionWithLooseFittingHeights&>) {
          return intersectsImpl(bv1.getBoundingRegion(), bv0);
        }
        if constexpr (
            std::is_same_v<decltype(bv0), const S2CellBoundingVolume&> ||
            std::is_same_v<decltype(bv1), const S2CellBoundingVolume&>) {
          // not dealing yet
          return false;
        }
        if constexpr (std::is_same_v<
                          decltype(bv0),
                          const BoundingCylinderRegion&>) {
          return intersectsImpl(bv0.toOrientedBoundingBox(), bv1);
        } else if constexpr (std::is_same_v<
                                 decltype(bv1),
                                 const BoundingCylinderRegion&>) {
          return intersectsImpl(bv1.toOrientedBoundingBox(), bv0);
        }
        return false;
      },
      volume0,
      volume1);
}
} // namespace Cesium3DTilesSelection
