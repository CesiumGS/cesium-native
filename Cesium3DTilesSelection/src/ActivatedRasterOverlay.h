#pragma once

#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCounted.h>

#include <glm/vec2.hpp>

namespace CesiumRasterOverlays {

class RasterOverlay;
class RasterOverlayExternals;
class RasterOverlayTile;
class RasterOverlayTileProvider;

} // namespace CesiumRasterOverlays

namespace Cesium3DTilesSelection {

class LoadedTileEnumerator;

class ActivatedRasterOverlay
    : public CesiumUtility::ReferenceCountedNonThreadSafe<
          ActivatedRasterOverlay> {
public:
  ActivatedRasterOverlay(
      const CesiumRasterOverlays::RasterOverlayExternals& externals,
      const CesiumUtility::IntrusivePointer<
          CesiumRasterOverlays::RasterOverlay>& pOverlay,
      const LoadedTileEnumerator& loadedTiles,
      const CesiumGeospatial::Ellipsoid& ellipsoid);
  ~ActivatedRasterOverlay();

  const CesiumRasterOverlays::RasterOverlay* getOverlay() const noexcept;

  CesiumRasterOverlays::RasterOverlay* getOverlay() noexcept;

  const CesiumRasterOverlays::RasterOverlayTileProvider*
  getTileProvider() const noexcept;

  CesiumRasterOverlays::RasterOverlayTileProvider* getTileProvider() noexcept;

  const CesiumRasterOverlays::RasterOverlayTile*
  getPlaceholderTile() const noexcept;

  CesiumRasterOverlays::RasterOverlayTile* getPlaceholderTile() noexcept;

  /**
   * @brief Returns a new {@link RasterOverlayTile} with the given
   * specifications.
   *
   * The returned tile will not start loading immediately. To start loading,
   * call {@link RasterOverlayTileProvider::loadTile} or
   * {@link RasterOverlayTileProvider::loadTileThrottled}.
   *
   * @param rectangle The rectangle that the returned image must cover. It is
   * allowed to cover a slightly larger rectangle in order to maintain pixel
   * alignment. It may also cover a smaller rectangle when the overlay itself
   * does not cover the entire rectangle.
   * @param targetScreenPixels The maximum number of pixels on the screen that
   * this tile is meant to cover. The overlay image should be approximately this
   * many pixels divided by the
   * {@link RasterOverlayOptions::maximumScreenSpaceError} in order to achieve
   * the desired level-of-detail, but it does not need to be exactly this size.
   * @return The tile.
   */
  CesiumUtility::IntrusivePointer<CesiumRasterOverlays::RasterOverlayTile>
  getTile(
      const CesiumGeometry::Rectangle& rectangle,
      const glm::dvec2& targetScreenPixels);

private:
  CesiumUtility::IntrusivePointer<CesiumRasterOverlays::RasterOverlay>
      _pOverlay;
  CesiumUtility::IntrusivePointer<
      CesiumRasterOverlays::RasterOverlayTileProvider>
      _pPlaceholderTileProvider;
  CesiumUtility::IntrusivePointer<CesiumRasterOverlays::RasterOverlayTile>
      _pPlaceholderTile;
  CesiumUtility::IntrusivePointer<
      CesiumRasterOverlays::RasterOverlayTileProvider>
      _pTileProvider;

  int64_t _tileDataBytes;
  int32_t _totalTilesCurrentlyLoading;
  int32_t _throttledTilesCurrentlyLoading;
};

} // namespace Cesium3DTilesSelection
