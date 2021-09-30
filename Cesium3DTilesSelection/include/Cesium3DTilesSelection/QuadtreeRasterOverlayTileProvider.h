#pragma once

#include "CreditSystem.h"
#include "IPrepareRendererResources.h"
#include "Library.h"
#include "RasterOverlayTileProvider.h"
#include "TileID.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>

#include <list>
#include <memory>
#include <optional>

namespace Cesium3DTilesSelection {

class CESIUM3DTILESSELECTION_API QuadtreeRasterOverlayTileProvider
    : public RasterOverlayTileProvider {

public:
  /**
   * @brief Creates a new instance.
   *
   * @param owner The raster overlay that owns this tile provider.
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
      RasterOverlay& owner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      std::optional<Credit> credit,
      const std::shared_ptr<IPrepareRendererResources>&
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
   * @brief Returns the coverage {@link CesiumGeometry::Rectangle} of this
   * instance.
   */
  const CesiumGeometry::Rectangle& getCoverageRectangle() const noexcept {
    return this->_coverageRectangle;
  }

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
   * Computes the appropriate tile level of detail (zoom level) for a given
   * geometric error near a given projected position. The position is required
   * because coordinates in many projections will map to real-world meters
   * differently in different parts of the globe.
   *
   * @param geometricError The geometric error for which to compute a level.
   * @param position The projected position defining the area of interest.
   * @return The level that is closest to the desired geometric error.
   */
  uint32_t computeLevelFromGeometricError(
      double geometricError,
      const glm::dvec2& position) const noexcept;

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
    std::shared_ptr<LoadedRasterOverlayImage> pLoaded;
    std::optional<CesiumGeometry::Rectangle> subset;
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
      double targetGeometricError);

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

  CesiumGeometry::Rectangle _coverageRectangle;
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
} // namespace Cesium3DTilesSelection
