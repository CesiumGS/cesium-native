#include <Cesium3DTiles/Tile.h>
#include <Cesium3DTilesContent/TileTransform.h>

#include <glm/ext/matrix_double4x4.hpp>

#include <optional>
#include <vector>

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

void TileTransform::setTransform(
    Cesium3DTiles::Tile& tile,
    const glm::dmat4& newTransform) {
  tile.transform.resize(16);
  tile.transform[0] = newTransform[0].x;
  tile.transform[1] = newTransform[0].y;
  tile.transform[2] = newTransform[0].z;
  tile.transform[3] = newTransform[0].w;
  tile.transform[4] = newTransform[1].x;
  tile.transform[5] = newTransform[1].y;
  tile.transform[6] = newTransform[1].z;
  tile.transform[7] = newTransform[1].w;
  tile.transform[8] = newTransform[2].x;
  tile.transform[9] = newTransform[2].y;
  tile.transform[10] = newTransform[2].z;
  tile.transform[11] = newTransform[2].w;
  tile.transform[12] = newTransform[3].x;
  tile.transform[13] = newTransform[3].y;
  tile.transform[14] = newTransform[3].z;
  tile.transform[15] = newTransform[3].w;
}

} // namespace Cesium3DTilesContent
