#pragma once

#include "Cesium3DTiles/RasterMappedTo3DTile.h"
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "Cesium3DTiles/Tile.h"
#include "CesiumGeometry/Rectangle.h"
#include "CesiumUtility/IntrusivePointer.h"

namespace Cesium3DTiles {

/**
 * @brief The result of applying a {@link RasterOverlayTile} from a quadtree
 * to geometry.
 *
 *
 */
class QuadtreeRasterMappedTo3DTile final : public RasterMappedTo3DTile {
public:
  QuadtreeRasterMappedTo3DTile(
      const CesiumUtility::IntrusivePointer<RasterOverlayTile>& pRasterTile,
      const CesiumGeometry::Rectangle& textureCoordinateRectangle);

  virtual ~QuadtreeRasterMappedTo3DTile(){};

  virtual MoreDetailAvailable update(Tile& tile) noexcept override;
};

} // namespace Cesium3DTiles