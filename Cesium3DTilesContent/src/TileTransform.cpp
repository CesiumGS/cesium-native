#include <Cesium3DTiles/Tile.h>
#include <Cesium3DTilesContent/TileTransform.h>

namespace Cesium3DTilesContent {

std::optional<glm::dmat4>
TileTransform::getTransform(const Cesium3DTiles::Tile& tile) {
  if (tile.transform.size() < 16)
    return std::nullopt;

  const std::vector<double>& a = tile.transform;
  return glm::dmat4(
      glm::dvec4(a[0], a[1], a[2], a[3]),
      glm::dvec4(a[4], a[5], a[6], a[7]),
      glm::dvec4(a[8], a[9], a[10], a[11]),
      glm::dvec4(a[12], a[13], a[14], a[15]));
}

} // namespace Cesium3DTilesContent
