#pragma once

#include <Cesium3DTilesSelection/ITileExcluder.h>
#include <Cesium3DTilesSelection/Library.h>
#include <CesiumUtility/IntrusivePointer.h>

namespace CesiumRasterOverlays {
class RasterizedPolygonsOverlay;
}

namespace Cesium3DTilesSelection {

/**
 * @brief When provided to {@link TilesetOptions::excluders}, uses the polygons
 * owned by a {@link CesiumRasterOverlays::RasterizedPolygonsOverlay} to exclude tiles that are
 * entirely inside any of the polygon from loading. This is useful when the
 * polygons will be used for clipping.
 */
class CESIUM3DTILESSELECTION_API RasterizedPolygonsTileExcluder
    : public Cesium3DTilesSelection::ITileExcluder {
public:
  /**
   * @brief Constructs a new instance.
   *
   * @param pOverlay The overlay definining the polygons.
   */
  RasterizedPolygonsTileExcluder(
      const CesiumUtility::IntrusivePointer<
          const CesiumRasterOverlays::RasterizedPolygonsOverlay>&
          pOverlay) noexcept;

  /**
   * @brief Determines whether a given tile is entirely inside a polygon and
   * therefore should be excluded.
   *
   * @param tile The tile to check.
   * @return true if the tile should be excluded because it is entirely inside a
   * polygon.
   */
  virtual bool shouldExclude(const Tile& tile) const noexcept override;

  /**
   * @brief Gets the overlay defining the polygons.
   */
  const CesiumRasterOverlays::RasterizedPolygonsOverlay& getOverlay() const;

private:
  CesiumUtility::IntrusivePointer<
      const CesiumRasterOverlays::RasterizedPolygonsOverlay>
      _pOverlay;
};

} // namespace Cesium3DTilesSelection
