#include <Cesium3DTiles/BoundingVolume.h>
#include <Cesium3DTiles/Extension3dTilesBoundingVolumeS2.h>
#include <Cesium3DTilesContent/TileBoundingVolumes.h>

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

std::optional<BoundingRegion>
TileBoundingVolumes::getBoundingRegion(const BoundingVolume& boundingVolume) {
  if (boundingVolume.region.size() < 6)
    return std::nullopt;

  const std::vector<double>& a = boundingVolume.region;
  return CesiumGeospatial::BoundingRegion(
      CesiumGeospatial::GlobeRectangle(a[0], a[1], a[2], a[3]),
      a[4],
      a[5]);
}

std::optional<BoundingSphere>
TileBoundingVolumes::getBoundingSphere(const BoundingVolume& boundingVolume) {
  if (boundingVolume.sphere.size() < 4)
    return std::nullopt;

  const std::vector<double>& a = boundingVolume.sphere;
  return CesiumGeometry::BoundingSphere(glm::dvec3(a[0], a[1], a[2]), a[3]);
}

std::optional<S2CellBoundingVolume>
TileBoundingVolumes::getS2CellBoundingVolume(
    const BoundingVolume& boundingVolume) {
  const Extension3dTilesBoundingVolumeS2* pExtension =
      boundingVolume.getExtension<Extension3dTilesBoundingVolumeS2>();
  if (!pExtension)
    return std::nullopt;

  return CesiumGeospatial::S2CellBoundingVolume(
      CesiumGeospatial::S2CellID::fromToken(pExtension->token),
      pExtension->minimumHeight,
      pExtension->maximumHeight);
}

} // namespace Cesium3DTilesContent
