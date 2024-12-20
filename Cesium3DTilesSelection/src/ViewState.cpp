#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/ViewState.h>
#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/CullingResult.h>
#include <CesiumGeometry/CullingVolume.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/BoundingRegionWithLooseFittingHeights.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>

#include <glm/common.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/trigonometric.hpp>

#include <optional>
#include <variant>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace Cesium3DTilesSelection {

/* static */ ViewState ViewState::create(
    const glm::dvec3& position,
    const glm::dvec3& direction,
    const glm::dvec3& up,
    const glm::dvec2& viewportSize,
    double horizontalFieldOfView,
    double verticalFieldOfView,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  return ViewState(
      position,
      direction,
      up,
      viewportSize,
      horizontalFieldOfView,
      verticalFieldOfView,
      ellipsoid.cartesianToCartographic(position),
      ellipsoid);
}

ViewState::ViewState(
    const glm::dvec3& position,
    const glm::dvec3& direction,
    const glm::dvec3& up,
    const glm::dvec2& viewportSize,
    double horizontalFieldOfView,
    double verticalFieldOfView,
    const std::optional<CesiumGeospatial::Cartographic>& positionCartographic,
    const CesiumGeospatial::Ellipsoid& ellipsoid)
    : _position(position),
      _direction(direction),
      _up(up),
      _viewportSize(viewportSize),
      _horizontalFieldOfView(horizontalFieldOfView),
      _verticalFieldOfView(verticalFieldOfView),
      _ellipsoid(ellipsoid),
      _sseDenominator(2.0 * glm::tan(0.5 * verticalFieldOfView)),
      _positionCartographic(positionCartographic),
      _cullingVolume(createCullingVolume(
          position,
          direction,
          up,
          horizontalFieldOfView,
          verticalFieldOfView)) {}

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
} // namespace

bool ViewState::isBoundingVolumeVisible(
    const BoundingVolume& boundingVolume) const noexcept {
  // TODO: use plane masks
  struct Operation {
    const ViewState& viewState;

    bool operator()(const OrientedBoundingBox& boundingBox) noexcept {
      return Cesium3DTilesSelection::isBoundingVolumeVisible(
          boundingBox,
          viewState._cullingVolume);
    }

    bool operator()(const BoundingRegion& boundingRegion) noexcept {
      return Cesium3DTilesSelection::isBoundingVolumeVisible(
          boundingRegion,
          viewState._cullingVolume);
    }

    bool operator()(const BoundingSphere& boundingSphere) noexcept {
      return Cesium3DTilesSelection::isBoundingVolumeVisible(
          boundingSphere,
          viewState._cullingVolume);
    }

    bool operator()(
        const BoundingRegionWithLooseFittingHeights& boundingRegion) noexcept {
      return Cesium3DTilesSelection::isBoundingVolumeVisible(
          boundingRegion.getBoundingRegion(),
          viewState._cullingVolume);
    }

    bool operator()(const S2CellBoundingVolume& s2Cell) noexcept {
      return Cesium3DTilesSelection::isBoundingVolumeVisible(
          s2Cell,
          viewState._cullingVolume);
    }
  };

  return std::visit(Operation{*this}, boundingVolume);
}

double ViewState::computeDistanceSquaredToBoundingVolume(
    const BoundingVolume& boundingVolume) const noexcept {
  struct Operation {
    const ViewState& viewState;

    double operator()(const OrientedBoundingBox& boundingBox) noexcept {
      return boundingBox.computeDistanceSquaredToPosition(viewState._position);
    }

    double operator()(const BoundingRegion& boundingRegion) noexcept {
      if (viewState._positionCartographic) {
        return boundingRegion.computeDistanceSquaredToPosition(
            viewState._positionCartographic.value(),
            viewState._position);
      }
      return boundingRegion.computeDistanceSquaredToPosition(
          viewState._position,
          viewState._ellipsoid);
    }

    double operator()(const BoundingSphere& boundingSphere) noexcept {
      return boundingSphere.computeDistanceSquaredToPosition(
          viewState._position);
    }

    double operator()(
        const BoundingRegionWithLooseFittingHeights& boundingRegion) noexcept {
      if (viewState._positionCartographic) {
        return boundingRegion.computeConservativeDistanceSquaredToPosition(
            viewState._positionCartographic.value(),
            viewState._position);
      }
      return boundingRegion.computeConservativeDistanceSquaredToPosition(
          viewState._position,
          viewState._ellipsoid);
    }

    double operator()(const S2CellBoundingVolume& s2Cell) noexcept {
      return s2Cell.computeDistanceSquaredToPosition(viewState._position);
    }
  };

  return std::visit(Operation{*this}, boundingVolume);
}

double ViewState::computeScreenSpaceError(
    double geometricError,
    double distance) const noexcept {
  // Avoid divide by zero when viewer is inside the tile
  distance = glm::max(distance, 1e-7);
  const double sseDenominator = this->_sseDenominator;
  return (geometricError * this->_viewportSize.y) / (distance * sseDenominator);
}
} // namespace Cesium3DTilesSelection
