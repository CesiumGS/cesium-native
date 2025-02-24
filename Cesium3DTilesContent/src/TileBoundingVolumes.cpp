#include <Cesium3DTiles/BoundingVolume.h>
#include <Cesium3DTiles/Extension3dTilesBoundingVolumeCylinder.h>
#include <Cesium3DTiles/Extension3dTilesBoundingVolumeS2.h>
#include <Cesium3DTilesContent/TileBoundingVolumes.h>
#include <CesiumGeometry/BoundingCylinderRegion.h>
#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>
#include <CesiumGeospatial/S2CellID.h>

#include <glm/ext/quaternion_double.hpp>

#include <optional>
#include <vector>

using namespace Cesium3DTiles;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace Cesium3DTilesContent {

std::optional<OrientedBoundingBox> TileBoundingVolumes::getOrientedBoundingBox(
    const BoundingVolume& boundingVolume) {
  if (boundingVolume.box.size() < 12)
    return std::nullopt;

  const std::vector<double>& a = boundingVolume.box;
  return CesiumGeometry::OrientedBoundingBox(
      glm::dvec3(a[0], a[1], a[2]),
      glm::dmat3(a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11]));
}

void TileBoundingVolumes::setOrientedBoundingBox(
    Cesium3DTiles::BoundingVolume& boundingVolume,
    const CesiumGeometry::OrientedBoundingBox& boundingBox) {
  const glm::dvec3& center = boundingBox.getCenter();
  const glm::dmat3& halfAxes = boundingBox.getHalfAxes();
  boundingVolume.box = {
      center.x,
      center.y,
      center.z,
      halfAxes[0].x,
      halfAxes[0].y,
      halfAxes[0].z,
      halfAxes[1].x,
      halfAxes[1].y,
      halfAxes[1].z,
      halfAxes[2].x,
      halfAxes[2].y,
      halfAxes[2].z};
}

std::optional<BoundingRegion> TileBoundingVolumes::getBoundingRegion(
    const BoundingVolume& boundingVolume,
    const Ellipsoid& ellipsoid) {
  if (boundingVolume.region.size() < 6)
    return std::nullopt;

  const std::vector<double>& a = boundingVolume.region;
  return CesiumGeospatial::BoundingRegion(
      CesiumGeospatial::GlobeRectangle(a[0], a[1], a[2], a[3]),
      a[4],
      a[5],
      ellipsoid);
}

void TileBoundingVolumes::setBoundingRegion(
    Cesium3DTiles::BoundingVolume& boundingVolume,
    const CesiumGeospatial::BoundingRegion& boundingRegion) {
  const CesiumGeospatial::GlobeRectangle& rectangle =
      boundingRegion.getRectangle();
  boundingVolume.region = {
      rectangle.getWest(),
      rectangle.getSouth(),
      rectangle.getEast(),
      rectangle.getNorth(),
      boundingRegion.getMinimumHeight(),
      boundingRegion.getMaximumHeight()};
}

std::optional<BoundingSphere>
TileBoundingVolumes::getBoundingSphere(const BoundingVolume& boundingVolume) {
  if (boundingVolume.sphere.size() < 4)
    return std::nullopt;

  const std::vector<double>& a = boundingVolume.sphere;
  return CesiumGeometry::BoundingSphere(glm::dvec3(a[0], a[1], a[2]), a[3]);
}

void TileBoundingVolumes::setBoundingSphere(
    Cesium3DTiles::BoundingVolume& boundingVolume,
    const CesiumGeometry::BoundingSphere& boundingSphere) {
  const glm::dvec3& center = boundingSphere.getCenter();
  boundingVolume
      .sphere = {center.x, center.y, center.z, boundingSphere.getRadius()};
}

std::optional<S2CellBoundingVolume>
TileBoundingVolumes::getS2CellBoundingVolume(
    const BoundingVolume& boundingVolume,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  const Extension3dTilesBoundingVolumeS2* pExtension =
      boundingVolume.getExtension<Extension3dTilesBoundingVolumeS2>();
  if (!pExtension)
    return std::nullopt;

  return CesiumGeospatial::S2CellBoundingVolume(
      CesiumGeospatial::S2CellID::fromToken(pExtension->token),
      pExtension->minimumHeight,
      pExtension->maximumHeight,
      ellipsoid);
}

void TileBoundingVolumes::setS2CellBoundingVolume(
    Cesium3DTiles::BoundingVolume& boundingVolume,
    const CesiumGeospatial::S2CellBoundingVolume& s2BoundingVolume) {
  Extension3dTilesBoundingVolumeS2& extension =
      boundingVolume.addExtension<Extension3dTilesBoundingVolumeS2>();
  extension.token = s2BoundingVolume.getCellID().toToken();
  extension.minimumHeight = s2BoundingVolume.getMinimumHeight();
  extension.maximumHeight = s2BoundingVolume.getMaximumHeight();
}

std::optional<BoundingCylinderRegion>
TileBoundingVolumes::getBoundingCylinderRegion(
    const BoundingVolume& boundingVolume) {
  const Extension3dTilesBoundingVolumeCylinder* pExtension =
      boundingVolume.getExtension<Extension3dTilesBoundingVolumeCylinder>();
  if (!pExtension)
    return std::nullopt;

  glm::dvec3 translation(0.0);
  glm::dquat rotation(1.0, 0.0, 0.0, 0.0);

  if (pExtension->translation.size() == 3) {
    const std::vector<double>& t = pExtension->translation;
    translation = glm::dvec3(t[0], t[1], t[2]);
  }

  if (pExtension->rotation.size() == 4) {
    const std::vector<double>& r = pExtension->rotation;
    rotation = glm::dquat(r[3], r[0], r[1], r[2]);
  }

  return CesiumGeometry::BoundingCylinderRegion(
      translation,
      rotation,
      pExtension->height,
      glm::dvec2(pExtension->minRadius, pExtension->maxRadius),
      glm::dvec2(pExtension->minAngle, pExtension->maxAngle));
}

void TileBoundingVolumes::setBoundingCylinderRegion(
    Cesium3DTiles::BoundingVolume& boundingVolume,
    const CesiumGeometry::BoundingCylinderRegion& boundingCylinderRegion) {
  Extension3dTilesBoundingVolumeCylinder& extension =
      boundingVolume.addExtension<Extension3dTilesBoundingVolumeCylinder>();

  const glm::dvec3& translation = boundingCylinderRegion.getTranslation();
  const glm::dquat& rotation = boundingCylinderRegion.getRotation();
  double height = boundingCylinderRegion.getHeight();
  const glm::dvec2& radialBounds = boundingCylinderRegion.getRadialBounds();
  const glm::dvec2& angularBounds = boundingCylinderRegion.getAngularBounds();

  extension.translation = {translation.x, translation.y, translation.z};
  extension.rotation = {rotation.x, rotation.y, rotation.z, rotation.w};
  extension.height = height;
  extension.minRadius = radialBounds.x;
  extension.minRadius = radialBounds.y;
  extension.minAngle = angularBounds.x;
  extension.maxAngle = angularBounds.y;
}

} // namespace Cesium3DTilesContent
