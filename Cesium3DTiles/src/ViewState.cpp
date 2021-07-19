
#include "Cesium3DTiles/ViewState.h"

#include <glm/trigonometric.hpp>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace Cesium3DTiles {

/* static */ ViewState ViewState::create(
    const FrustumType& frustumType,
    const glm::dvec3& position,
    const glm::dvec3& direction,
    const glm::dvec3& up,
    const glm::dvec2& viewportSize,
    double horizontalFieldOfView,
    double verticalFieldOfView,
    double orthographicWidth,
    double orthographicHeight,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  return ViewState(
      frustumType,
      position,
      direction,
      up,
      viewportSize,
      horizontalFieldOfView,
      verticalFieldOfView,
      orthographicWidth,
      orthographicHeight,
      ellipsoid.cartesianToCartographic(position));
}

/* static */ CullingVolume ViewState::createCullingVolume(
    const FrustumType& frustumType,
    const glm::dvec3& position,
    const glm::dvec3& direction,
    const glm::dvec3& up,
    const double horizontalFieldOfView,
    const double verticalFieldOfView,
    const double orthographicWidth,
    const double orthographicHeight) {

  if (frustumType == FrustumType::Perspective) {
    return createPerspectiveCullingVolume(
        position,
        direction,
        up,
        horizontalFieldOfView,
        verticalFieldOfView);
  } else if (frustumType == FrustumType::Orthographic) {
    return createOrthographicCullingVolume(
        position,
        direction,
        up,
        orthographicWidth,
        orthographicHeight);
  } else {
    return CullingVolume({});
  }
}

ViewState::ViewState(
    const FrustumType& frustumType,
    const glm::dvec3& position,
    const glm::dvec3& direction,
    const glm::dvec3& up,
    const glm::dvec2& viewportSize,
    double horizontalFieldOfView,
    double verticalFieldOfView,
    double orthographicWidth,
    double orthographicHeight,
    const std::optional<CesiumGeospatial::Cartographic>& positionCartographic)
    : _frustumType(frustumType),
      _position(position),
      _direction(direction),
      _up(up),
      _viewportSize(viewportSize),
      _horizontalFieldOfView(horizontalFieldOfView),
      _verticalFieldOfView(verticalFieldOfView),
      _orthographicWidth(orthographicWidth),
      _orthographicHeight(orthographicHeight),
      _sseDenominator(2.0 * glm::tan(0.5 * verticalFieldOfView)),
      _positionCartographic(positionCartographic),
      _cullingVolume(createCullingVolume(
          frustumType,
          position,
          direction,
          up,
          horizontalFieldOfView,
          verticalFieldOfView,
          orthographicWidth,
          orthographicHeight)) {}

template <class T>
static bool isBoundingVolumeVisible(
    const T& boundingVolume,
    const CullingVolume& cullingVolume) noexcept {

  for (const CesiumGeometry::Plane& plane : cullingVolume.getPlanes()) {
    if (boundingVolume.intersectPlane(plane) == CullingResult::Outside) {
      return false;
    }
  }

  return true;
}

bool ViewState::isBoundingVolumeVisible(
    const BoundingVolume& boundingVolume) const noexcept {
  // TODO: use plane masks
  struct Operation {
    const ViewState& viewState;

    bool operator()(const OrientedBoundingBox& boundingBox) {
      return Cesium3DTiles::isBoundingVolumeVisible(
          boundingBox,
          viewState._cullingVolume);
    }

    bool operator()(const BoundingRegion& boundingRegion) {
      return Cesium3DTiles::isBoundingVolumeVisible(
          boundingRegion,
          viewState._cullingVolume);
    }

    bool operator()(const BoundingSphere& boundingSphere) {
      return Cesium3DTiles::isBoundingVolumeVisible(
          boundingSphere,
          viewState._cullingVolume);
    }

    bool
    operator()(const BoundingRegionWithLooseFittingHeights& boundingRegion) {
      return Cesium3DTiles::isBoundingVolumeVisible(
          boundingRegion.getBoundingRegion(),
          viewState._cullingVolume);
    }
  };

  return std::visit(Operation{*this}, boundingVolume);
}

double ViewState::computeDistanceSquaredToBoundingVolume(
    const BoundingVolume& boundingVolume) const noexcept {
  struct Operation {
    const ViewState& viewState;

    double operator()(const OrientedBoundingBox& boundingBox) {
      return boundingBox.computeDistanceSquaredToPosition(viewState._position);
    }

    double operator()(const BoundingRegion& boundingRegion) {
      if (viewState._positionCartographic) {
        return boundingRegion.computeDistanceSquaredToPosition(
            viewState._positionCartographic.value(),
            viewState._position);
      }
      return boundingRegion.computeDistanceSquaredToPosition(
          viewState._position);
    }

    double operator()(const BoundingSphere& boundingSphere) {
      return boundingSphere.computeDistanceSquaredToPosition(
          viewState._position);
    }

    double
    operator()(const BoundingRegionWithLooseFittingHeights& boundingRegion) {
      if (viewState._positionCartographic) {
        return boundingRegion.computeConservativeDistanceSquaredToPosition(
            viewState._positionCartographic.value(),
            viewState._position);
      }
      return boundingRegion.computeConservativeDistanceSquaredToPosition(
          viewState._position);
    }
  };

  return std::visit(Operation{*this}, boundingVolume);
}

double ViewState::computeScreenSpaceError(
    double geometricError,
    double distance) const noexcept {
  // Avoid divide by zero when viewer is inside the tile
  distance = glm::max(distance, 1e-7);

  if (this->_frustumType == FrustumType::Orthographic) {
    return geometricError * this->_viewportSize.y / this->_orthographicHeight;
  }

  double sseDenominator = this->_sseDenominator;
  return (geometricError * this->_viewportSize.y) / (distance * sseDenominator);
}
} // namespace Cesium3DTiles
