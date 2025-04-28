#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/ViewState.h>
#include <CesiumGeometry/BoundingCylinderRegion.h>
#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/CullingResult.h>
#include <CesiumGeometry/CullingVolume.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeometry/Transforms.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/BoundingRegionWithLooseFittingHeights.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>

#include <glm/common.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_double4.hpp>
#include <glm/geometric.hpp>

#include <cmath>
#include <limits>
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
      ellipsoid);
}

namespace {
glm::dvec3 positionFromView(const glm::dmat4& viewMatrix) {
  // Back out the world position by multiplying the view matrix translation by
  // the rotation transpose (inverse) and negating.
  glm::dvec3 position(0.0);
  position.x = -glm::dot(glm::dvec3(viewMatrix[0]), glm::dvec3(viewMatrix[3]));
  position.y = -glm::dot(glm::dvec3(viewMatrix[1]), glm::dvec3(viewMatrix[3]));
  position.z = -glm::dot(glm::dvec3(viewMatrix[2]), glm::dvec3(viewMatrix[3]));
  return position;
}

glm::dvec3 directionFromView(const glm::dmat4& viewMatrix) {
  return glm::dvec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]) *
         -1.0;
}
} // namespace

// The near and far plane values don't matter for frustum culling.
ViewState::ViewState(
    const glm::dvec3& position,
    const glm::dvec3& direction,
    const glm::dvec3& up,
    const glm::dvec2& viewportSize,
    double horizontalFieldOfView,
    double verticalFieldOfView,
    const CesiumGeospatial::Ellipsoid& ellipsoid)
    : ViewState(
          Transforms::createViewMatrix(position, direction, up),
          Transforms::createPerspectiveMatrix(
              horizontalFieldOfView,
              verticalFieldOfView,
              100.0,
              std::numeric_limits<double>::infinity()),
          viewportSize,
          ellipsoid) {}

ViewState::ViewState(
    const glm::dmat4& viewMatrix,
    const glm::dmat4& projectionMatrix,
    const glm::dvec2& viewportSize,
    const CesiumGeospatial::Ellipsoid& ellipsoid)
    : _position(positionFromView(viewMatrix)),
      _direction(directionFromView(viewMatrix)),
      _viewportSize(viewportSize),
      _ellipsoid(ellipsoid),
      _positionCartographic(
          ellipsoid.cartesianToCartographic(positionFromView(viewMatrix))),
      _cullingVolume(createCullingVolume(projectionMatrix * viewMatrix)),
      _viewMatrix(viewMatrix),
      _projectionMatrix(projectionMatrix) {}

ViewState::ViewState(
    const glm::dvec3& position,
    const glm::dvec3& direction,
    const glm::dvec3& up,
    const glm::dvec2& viewportSize,
    double left,
    double right,
    double bottom,
    double top,
    const CesiumGeospatial::Ellipsoid& ellipsoid)
    : ViewState(
          Transforms::createViewMatrix(position, direction, up),
          Transforms::createOrthographicMatrix(
              left,
              right,
              bottom,
              top,
              100.0,
              std::numeric_limits<double>::infinity()),
          viewportSize,
          ellipsoid) {}

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

    bool
    operator()(const BoundingCylinderRegion& boundingCylinderRegion) noexcept {
      return Cesium3DTilesSelection::isBoundingVolumeVisible(
          boundingCylinderRegion,
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

    double
    operator()(const BoundingCylinderRegion& boundingCylinderRegion) noexcept {
      return boundingCylinderRegion.computeDistanceSquaredToPosition(
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
  // This is a simplified version of the projection transform and homogeneous
  // division. We transform the coordinate (0.0, geometric error, -distance,
  // 1) and use the resulting NDC to find the screen space error.  That's not
  // quite right: the distance argument is actually the slant distance to the
  // tile, whereas view-space Z is perpendicular to the near plane.
  const glm::dmat4& projMat = this->_projectionMatrix;
  glm::dvec4 centerNdc = projMat * glm::dvec4(0.0, 0.0, -distance, 1.0);
  centerNdc /= centerNdc.w;
  glm::dvec4 errorOffsetNdc =
      projMat * glm::dvec4(0.0, geometricError, -distance, 1.0);
  errorOffsetNdc /= errorOffsetNdc.w;

  double ndcError = (errorOffsetNdc - centerNdc).y;
  // ndc bounds are [-1.0, 1.0]. Our projection matrix has the top of the
  // screen at -1.0, so ndcError is negative.
  return -ndcError * this->_viewportSize.y / 2.0;
}

double ViewState::getHorizontalFieldOfView() const noexcept {
  return std::atan(1.0 / this->_projectionMatrix[0][0]) * 2.0;
}

double ViewState::getVerticalFieldOfView() const noexcept {
  return std::atan(-1.0 / this->_projectionMatrix[1][1]) * 2.0;
}
} // namespace Cesium3DTilesSelection
