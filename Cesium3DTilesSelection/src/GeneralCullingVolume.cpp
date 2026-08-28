#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/GeneralCullingVolume.h>
#include <CesiumGeometry/BoundingCylinderRegion.h>
#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/CullingResult.h>
#include <CesiumGeometry/CullingVolume.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/BoundingRegionWithLooseFittingHeights.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>

#include <variant>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace Cesium3DTilesSelection {

namespace {
template <class T>
bool isBoundingVolumeVisible(
    const T& boundingVolume,
    const CullingVolume& cullingVolume) noexcept {
  const CullingResult left =
      boundingVolume.intersectPlane(cullingVolume.leftPlane);
  if (left == CullingResult::Outside) {
    return false;
  }

  const CullingResult right =
      boundingVolume.intersectPlane(cullingVolume.rightPlane);
  if (right == CullingResult::Outside) {
    return false;
  }

  const CullingResult top =
      boundingVolume.intersectPlane(cullingVolume.topPlane);
  if (top == CullingResult::Outside) {
    return false;
  }

  const CullingResult bottom =
      boundingVolume.intersectPlane(cullingVolume.bottomPlane);
  if (bottom == CullingResult::Outside) {
    return false;
  }

  return true;
}

bool isBoundingVolumeVisible(
    const CullingVolume& cullingVolume,
    const BoundingVolume& boundingVolume) noexcept {
  struct Operation {
    const CullingVolume& cullingVolume;

    bool operator()(const OrientedBoundingBox& boundingBox) noexcept {
      return isBoundingVolumeVisible(boundingBox, cullingVolume);
    }

    bool operator()(const BoundingRegion& boundingRegion) noexcept {
      return isBoundingVolumeVisible(boundingRegion, cullingVolume);
    }

    bool operator()(const BoundingSphere& boundingSphere) noexcept {
      return isBoundingVolumeVisible(boundingSphere, cullingVolume);
    }

    bool operator()(
        const BoundingRegionWithLooseFittingHeights& boundingRegion) noexcept {
      return isBoundingVolumeVisible(
          boundingRegion.getBoundingRegion(),
          cullingVolume);
    }

    bool operator()(const S2CellBoundingVolume& s2Cell) noexcept {
      return isBoundingVolumeVisible(s2Cell, cullingVolume);
    }

    bool
    operator()(const BoundingCylinderRegion& boundingCylinderRegion) noexcept {
      return isBoundingVolumeVisible(boundingCylinderRegion, cullingVolume);
    }
  };
  return std::visit(Operation{cullingVolume}, boundingVolume);
}
} // namespace

bool isBoundingVolumeVisible(
    const GeneralCullingVolume& cullingVolume,
    const BoundingVolume& boundingVolume) {
  struct Operation {
    const BoundingVolume& boundingVolume;

    bool operator()(const CesiumGeometry::CullingVolume& cv) {
      return isBoundingVolumeVisible(cv, boundingVolume);
    }

    bool operator()(const BoundingVolume& cv) {
      return testIntersection(cv, boundingVolume);
    }
  };
  return std::visit(Operation{boundingVolume}, cullingVolume);
}

} // namespace Cesium3DTilesSelection
