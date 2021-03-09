#include "CesiumGeometry/QuadtreeTilingScheme.h"

namespace CesiumGeometry {

QuadtreeTilingScheme::QuadtreeTilingScheme(
    const CesiumGeometry::Rectangle& rectangle,
    uint32_t rootTilesX,
    uint32_t rootTilesY) noexcept
    : _rectangle(rectangle), _rootTilesX(rootTilesX), _rootTilesY(rootTilesY) {}

uint32_t
QuadtreeTilingScheme::getNumberOfXTilesAtLevel(uint32_t level) const noexcept {
  return this->_rootTilesX << level;
}

uint32_t
QuadtreeTilingScheme::getNumberOfYTilesAtLevel(uint32_t level) const noexcept {
  return this->_rootTilesY << level;
}

std::optional<CesiumGeometry::QuadtreeTileID>
QuadtreeTilingScheme::positionToTile(const glm::dvec2& position, uint32_t level)
    const noexcept {
  if (!this->getRectangle().contains(position)) {
    // outside the bounds of the tiling scheme
    return std::nullopt;
  }

  uint32_t xTiles = this->getNumberOfXTilesAtLevel(level);
  uint32_t yTiles = this->getNumberOfYTilesAtLevel(level);

  double overallWidth = this->getRectangle().computeWidth();
  double xTileWidth = overallWidth / xTiles;
  double overallHeight = this->getRectangle().computeHeight();
  double yTileHeight = overallHeight / yTiles;

  double distanceFromWest = position.x - this->getRectangle().minimumX;
  double distanceFromSouth = position.y - this->getRectangle().minimumY;

  uint32_t xTileCoordinate =
      static_cast<uint32_t>(distanceFromWest / xTileWidth);
  if (xTileCoordinate >= xTiles) {
    xTileCoordinate = xTiles - 1;
  }
  uint32_t yTileCoordinate =
      static_cast<uint32_t>(distanceFromSouth / yTileHeight);
  if (yTileCoordinate >= yTiles) {
    yTileCoordinate = yTiles - 1;
  }

  return CesiumGeometry::QuadtreeTileID(
      level,
      xTileCoordinate,
      yTileCoordinate);
}

CesiumGeometry::Rectangle QuadtreeTilingScheme::tileToRectangle(
    const CesiumGeometry::QuadtreeTileID& tileID) const noexcept {
  uint32_t xTiles = this->getNumberOfXTilesAtLevel(tileID.level);
  uint32_t yTiles = this->getNumberOfYTilesAtLevel(tileID.level);

  double xTileWidth = this->_rectangle.computeWidth() / xTiles;
  double left = this->_rectangle.minimumX + tileID.x * xTileWidth;
  double right = this->_rectangle.minimumX + (tileID.x + 1) * xTileWidth;

  double yTileHeight = this->_rectangle.computeHeight() / yTiles;
  double bottom = this->_rectangle.minimumY + tileID.y * yTileHeight;
  double top = this->_rectangle.minimumY + (tileID.y + 1) * yTileHeight;

  return Rectangle(left, bottom, right, top);
}

} // namespace CesiumGeometry
