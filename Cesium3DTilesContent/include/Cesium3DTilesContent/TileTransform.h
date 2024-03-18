#pragma once

#include <glm/mat4x4.hpp>

#include <optional>

namespace Cesium3DTiles {
struct Tile;
}

namespace Cesium3DTilesContent {

/**
 * @brief Convenience functions for getting and setting
 * {@link Cesium3DTiles::Tile::transform} as a `glm::dmat4`.
 */
class TileTransform {
public:
  /**
   * @brief Gets the tile's transform as a `glm::dmat4`.
   *
   * @param tile The tile from which to get the transform.
   * @return The transform, or `std::nullopt` if the
   * {@link Cesium3DTiles::Tile::transform} does not have 16 elements.
   */
  static std::optional<glm::dmat4>
  getTransform(const Cesium3DTiles::Tile& tile);

  /**
   * @brief Sets the tile's transform as a `glm::dmat4`.
   *
   * @param tile The tile on which to set the transform.
   * @param newTransform The new transform.
   */
  static void
  setTransform(Cesium3DTiles::Tile& tile, const glm::dmat4& newTransform);
};

} // namespace Cesium3DTilesContent
