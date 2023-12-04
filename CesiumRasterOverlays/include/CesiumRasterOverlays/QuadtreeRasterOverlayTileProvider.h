#pragma once

#include "IPrepareRasterOverlayRendererResources.h"
#include "Library.h"
#include "RasterOverlayTileProvider.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumUtility/CreditSystem.h>

#include <list>
#include <memory>
#include <optional>

namespace CesiumRasterOverlays {

class CESIUMRASTEROVERLAYS_API QuadtreeRasterOverlayTileProvider
    : public RasterOverlayTileProvider {

public:
  /**
   * @brief Creates a new instance.
   *
   * @param pOwner The raster overlay that created this tile provider.
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The interface used to obtain assets (tiles, etc.) for
   * this raster overlay.
   * @param credit The {@link Credit} for this tile provider, if it exists.
   * @param pPrepareRendererResources The interface used to prepare raster
   * images for rendering.
   * @param pLogger The logger to which to send messages about the tile provider
   * and tiles.
   * @param projection The {@link CesiumGeospatial::Projection}.
   * @param coverageRectangle The {@link CesiumGeometry::Rectangle}.
   * @param minimumLevel The minimum quadtree tile level.
   * @param maximumLevel The maximum quadtree tile level.
   * @param imageWidth The image width.
   * @param imageHeight The image height.
   */
  QuadtreeRasterOverlayTileProvider(
      const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      std::optional<CesiumUtility::Credit> credit,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
      const CesiumGeometry::Rectangle& coverageRectangle,
      uint32_t minimumLevel,
      uint32_t maximumLevel,
      uint32_t imageWidth,
      uint32_t imageHeight) noexcept;

  /**
   * @brief Returns the minimum tile level of this instance.
   */
  uint32_t getMinimumLevel() const noexcept { return this->_minimumLevel; }

  /**
   * @brief Returns the maximum tile level of this instance.
   */
  uint32_t getMaximumLevel() const noexcept { return this->_maximumLevel; }

  /**
   * @brief Returns the image width of this instance, in pixels.
   */
  uint32_t getWidth() const noexcept { return this->_imageWidth; }

  /**
   * @brief Returns the image height of this instance, in pixels.
   */
  uint32_t getHeight() const noexcept { return this->_imageHeight; }

  /**
   * @brief Returns the {@link CesiumGeometry::QuadtreeTilingScheme} of this
   * instance.
   */
  const CesiumGeometry::QuadtreeTilingScheme& getTilingScheme() const noexcept {
    return this->_tilingScheme;
  }

  /**
   * @brief Computes the best quadtree level to use for an image intended to
   * cover a given projected rectangle when it is a given size on the screen.
   *
   * @param rectangle The range of projected coordinates to cover.
   * @param screenPixels The number of screen pixels to be covered by the
   * rectangle.
   * @return The level.
   */
  uint32_t computeLevelFromTargetScreenPixels(
      const CesiumGeometry::Rectangle& rectangle,
      const glm::dvec2& screenPixels);

protected:
  /**
   * @brief Asynchronously loads a tile in the quadtree.
   *
   * @param tileID The ID of the quadtree tile to load.
   * @return A Future that resolves to the loaded image data or error
   * information.
   */
  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadQuadtreeTileImage(const CesiumGeometry::QuadtreeTileID& tileID) const = 0;

private:
  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(RasterOverlayTile& overlayTile) override final;

  struct LoadedQuadtreeImage {
    std::shared_ptr<LoadedRasterOverlayImage> pLoaded = nullptr;
    std::optional<CesiumGeometry::Rectangle> subset = std::nullopt;
  };

  CesiumAsync::SharedFuture<LoadedQuadtreeImage>
  getQuadtreeTile(const CesiumGeometry::QuadtreeTileID& tileID);

  /**
   * @brief Map raster tiles to geometry tile.
   *
   * @param geometryRectangle The rectangle for which to load tiles.
   * @param targetGeometricError The geometric error controlling which quadtree
   * level to use to cover the rectangle.
   * @return A vector of shared futures, each of which will resolve to image
   * data that is required to cover the rectangle with the given geometric
   * error.
   */
  std::vector<CesiumAsync::SharedFuture<LoadedQuadtreeImage>>
  mapRasterTilesToGeometryTile(
      const CesiumGeometry::Rectangle& geometryRectangle,
      const glm::dvec2 targetScreenPixels);

  void unloadCachedTiles();

  struct CombinedImageMeasurements {
    CesiumGeometry::Rectangle rectangle;
    int32_t widthPixels;
    int32_t heightPixels;
    int32_t channels;
    int32_t bytesPerChannel;
  };

  static CombinedImageMeasurements measureCombinedImage(
      const CesiumGeometry::Rectangle& targetRectangle,
      const std::vector<LoadedQuadtreeImage>& images);

  static LoadedRasterOverlayImage combineImages(
      const CesiumGeometry::Rectangle& targetRectangle,
      const CesiumGeospatial::Projection& projection,
      std::vector<LoadedQuadtreeImage>&& images);

  uint32_t _minimumLevel;
  uint32_t _maximumLevel;
  uint32_t _imageWidth;
  uint32_t _imageHeight;
  CesiumGeometry::QuadtreeTilingScheme _tilingScheme;

  struct CacheEntry {
    CesiumGeometry::QuadtreeTileID tileID;
    CesiumAsync::SharedFuture<LoadedQuadtreeImage> future;
  };

  // Tiles at the beginning of this list are the least recently used (oldest),
  // while the tiles at the end are most recently used (newest).
  using TileLeastRecentlyUsedList = std::list<CacheEntry>;
  TileLeastRecentlyUsedList _tilesOldToRecent;

  // Allows a Future to be looked up by quadtree tile ID.
  std::unordered_map<
      CesiumGeometry::QuadtreeTileID,
      TileLeastRecentlyUsedList::iterator>
      _tileLookup;

  std::atomic<int64_t> _cachedBytes;
};
} // namespace CesiumRasterOverlays
