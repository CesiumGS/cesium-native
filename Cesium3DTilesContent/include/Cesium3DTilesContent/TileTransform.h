#pragma once

#include <glm/mat4x4.hpp>

#include <optional>

namespace Cesium3DTiles {
struct Tile;
}

namespace Cesium3DTilesContent {

class TileTransform {
public:
  static std::optional<glm::dmat4>
  getTransform(const Cesium3DTiles::Tile& tile);
};

} // namespace Cesium3DTilesContent
