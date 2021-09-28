#include "CesiumGeometry/OctreeTilingScheme.h"

namespace CesiumGeometry {

OctreeTilingScheme::OctreeTilingScheme(
    const CesiumGeometry::OrientedBoundingBox& box,
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

std::optional<CesiumGeometry::OctreeTileID>
OctreeTilingScheme::positionToTile(const glm::dvec3& position, uint32_t level)
    const noexcept {

  glm::dvec3 localPosition = this->getBox().getInverseHalfAxes() * position;

  if (glm::abs(localPosition.x) > 1.0 ||
      glm::abs(localPosition.y) > 1.0 ||
      glm::abs(localPosition.z) > 1.0) {
    // outside the bounds of the tiling scheme
    return std::nullopt;
  }    

  uint32_t xTiles = this->getNumberOfXTilesAtLevel(level);
  uint32_t yTiles = this->getNumberOfYTilesAtLevel(level);
  uint32_t zTiles = this->getNumberOfZTilesAtLevel(level);

  const glm::dvec3& boxDimensions = this->getBox().getLengths();
  double xTileSize = boxDimensions.x / xTiles;
  double yTileSize = boxDimensions.y / yTiles;
  double zTileSize = boxDimensions.z / zTiles;

  uint32_t xTileCoordinate =
      static_cast<uint32_t>((localPosition.x - 1.0) / xTileSize);
  if (xTileCoordinate >= xTiles) {
    xTileCoordinate = xTiles - 1;
  }
  uint32_t yTileCoordinate =
      static_cast<uint32_t>((localPosition.y - 1.0) / yTileSize);
  if (yTileCoordinate >= yTiles) {
    yTileCoordinate = yTiles - 1;
  }
  uint32_t zTileCoordinate =
      static_cast<uint32_t>((localPosition.z - 1.0) / zTileSize);
  if (zTileCoordinate >= zTiles) {
    zTileCoordinate = zTiles - 1;
  }

  return CesiumGeometry::OctreeTileID(
      level,
      xTileCoordinate,
      yTileCoordinate,
      zTileCoordinate);
}

CesiumGeometry::OrientedBoundingBox OctreeTilingScheme::tileToBoundingBox(
    const CesiumGeometry::OctreeTileID& tileID) const noexcept {
  double xTiles = this->getNumberOfXTilesAtLevel(tileID.level);
  double yTiles = this->getNumberOfYTilesAtLevel(tileID.level);
  double zTiles = this->getNumberOfZTilesAtLevel(tileID.level);

  const glm::dmat3& boxHalfAxes = this->getBox().getHalfAxes();
  glm::dmat3 tileHalfAxes(
      boxHalfAxes[0] / xTiles,
      boxHalfAxes[1] / yTiles,
      boxHalfAxes[2] / zTiles);
    
  const glm::dvec3& boxDimensions = this->getBox().getLengths();
  glm::dvec3 tileDimensions(
      boxDimensions.x / xTiles,
      boxDimensions.y / yTiles,
      boxDimensions.z / zTiles);

  glm::dvec3 tileCenter = 
      this->getBox().getCenter() 
      - boxHalfAxes[0] 
      - boxHalfAxes[1]
      - boxHalfAxes[2] 
      + tileDimensions * glm::dvec3(tileID.x, tileID.y, tileID.z)
      + tileHalfAxes[0]
      + tileHalfAxes[1]
      + tileHalfAxes[2];

  return OrientedBoundingBox(tileCenter, tileHalfAxes);
}

} // namespace CesiumGeometry
