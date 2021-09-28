#pragma once

#include "ITileExcluder.h"
#include "Library.h"

namespace Cesium3DTilesSelection {

class RasterizedPolygonsOverlay;

/**
 * @brief When provided to {@link TilesetOptions::excluders}, uses the polygons
 * owned by a {@link RasterizedPolygonsOverlay} to exclude tiles that are
 * entirely inside any of the polygon from loading. This is useful when the
 * polygons will be used for clipping.
 */
class CESIUM3DTILESSELECTION_API RasterizedPolygonsTileExcluder
    : public Cesium3DTilesSelection::ITileExcluder {
public:
  /**
   * @brief Constructs a new instance.
   *
   * @param overlay The overlay definining the polygons. Care must be taken to
   * ensure that the lifetime of this overlay is longer than the lifetime of the
   * newly-constructed `RasterizedPolygonsOverlay`.
   */
  RasterizedPolygonsTileExcluder(
      const RasterizedPolygonsOverlay& overlay) noexcept;

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
  const RasterizedPolygonsOverlay& getOverlay() const;

private:
  const RasterizedPolygonsOverlay* _pOverlay;
};

} // namespace Cesium3DTilesSelection
