#include <CesiumGeometry/AxisAlignedBox.h>
#include <CesiumGeometry/OctreeTileID.h>
#include <CesiumGeometry/OctreeTilingScheme.h>

#include <glm/ext/vector_double3.hpp>

#include <cstdint>
#include <optional>

namespace CesiumGeometry {

OctreeTilingScheme::OctreeTilingScheme(
    const AxisAlignedBox& box,
    uint32_t rootTilesX,
    uint32_t rootTilesY,
    uint32_t rootTilesZ) noexcept
    : _box(box),
      _rootTilesX(rootTilesX),
      _rootTilesY(rootTilesY),
      _rootTilesZ(rootTilesZ) {}

uint32_t
OctreeTilingScheme::getNumberOfXTilesAtLevel(uint32_t level) const noexcept {
  return this->_rootTilesX << level;
}

uint32_t
OctreeTilingScheme::getNumberOfYTilesAtLevel(uint32_t level) const noexcept {
  return this->_rootTilesY << level;
}

uint32_t
OctreeTilingScheme::getNumberOfZTilesAtLevel(uint32_t level) const noexcept {
  return this->_rootTilesZ << level;
}

std::optional<OctreeTileID> OctreeTilingScheme::positionToTile(
    const glm::dvec3& position,
    uint32_t level) const noexcept {

  if (!this->_box.contains(position)) {
    return std::nullopt;
  }

  uint32_t xTiles = this->getNumberOfXTilesAtLevel(level);
  uint32_t yTiles = this->getNumberOfYTilesAtLevel(level);
  uint32_t zTiles = this->getNumberOfZTilesAtLevel(level);

  double xTileSize = this->_box.lengthX / xTiles;
  double yTileSize = this->_box.lengthY / yTiles;
  double zTileSize = this->_box.lengthZ / zTiles;

  uint32_t xTileCoordinate =
      static_cast<uint32_t>((position.x - this->_box.minimumX) / xTileSize);
  if (xTileCoordinate >= xTiles) {
    xTileCoordinate = xTiles - 1;
  }
  uint32_t yTileCoordinate =
      static_cast<uint32_t>((position.y - this->_box.minimumY) / yTileSize);
  if (yTileCoordinate >= yTiles) {
    yTileCoordinate = yTiles - 1;
  }
  uint32_t zTileCoordinate =
      static_cast<uint32_t>((position.z - this->_box.minimumZ) / zTileSize);
  if (zTileCoordinate >= zTiles) {
    zTileCoordinate = zTiles - 1;
  }

  return OctreeTileID(level, xTileCoordinate, yTileCoordinate, zTileCoordinate);
}

AxisAlignedBox
OctreeTilingScheme::tileToBox(const OctreeTileID& tileID) const noexcept {
  uint32_t xTiles = this->getNumberOfXTilesAtLevel(tileID.level);
  uint32_t yTiles = this->getNumberOfYTilesAtLevel(tileID.level);
  uint32_t zTiles = this->getNumberOfZTilesAtLevel(tileID.level);

  double xTileSize = this->_box.lengthX / xTiles;
  double yTileSize = this->_box.lengthY / yTiles;
  double zTileSize = this->_box.lengthZ / zTiles;

  double minimumX = xTileSize * tileID.x;
  double minimumY = yTileSize * tileID.y;
  double minimumZ = zTileSize * tileID.z;

  return AxisAlignedBox(
      minimumX,
      minimumY,
      minimumZ,
      minimumX + xTileSize,
      minimumY + yTileSize,
      minimumZ + zTileSize);
}
} // namespace CesiumGeometry
