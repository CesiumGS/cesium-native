#pragma once

#include "Cesium3DTiles/CreditSystem.h"
#include "Cesium3DTiles/IPrepareRendererResources.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/TileID.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumGeometry/QuadtreeTileID.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include <memory>
#include <optional>

namespace Cesium3DTiles {

class CESIUM3DTILES_API QuadtreeRasterOverlayTileProvider
    : public RasterOverlayTileProvider {

public:
  QuadtreeRasterOverlayTileProvider(
      RasterOverlay& owner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>&
          pAssetAccessor) noexcept;

  /**
   * @brief Creates a new instance.
   *
   * @param owner The {@link RasterOverlay}. May not be `nullptr`.
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
   * @brief Whether the given raster tile has more detail.
   *
   * If so its children may be subdivided to use the more detailed raster
   * tiles.
   */
  virtual bool hasMoreDetailsAvailable(const TileID& tileID) const override;

  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(const RasterOverlayTile& overlayTile) override;

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
   * @brief Returns the image width of this instance.
   */
  uint32_t getWidth() const noexcept { return this->_imageWidth; }

  /**
   * @brief Returns the image height of this instance.
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
  virtual CesiumAsync::SharedFuture<LoadedRasterOverlayImage>
  loadQuadtreeTileImage(const CesiumGeometry::QuadtreeTileID& tileID) const = 0;

private:
  CesiumAsync::SharedFuture<LoadedRasterOverlayImage>
  getQuadtreeTile(const CesiumGeometry::QuadtreeTileID& tileID);

  /**
   * @brief Map raster tiles to geometry tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @param geometryRectangle The rectangle.
   * @param targetGeometricError The geometric error.
   * @return A single raster tile combining the given rasters into the
   * geometry tile's rectangle.
   */
  std::vector<CesiumAsync::SharedFuture<LoadedRasterOverlayImage>>
  mapRasterTilesToGeometryTile(
      const CesiumGeospatial::GlobeRectangle& geometryRectangle,
      double targetGeometricError);

  /** @copydoc mapRasterTilesToGeometryTile */
  std::vector<CesiumAsync::SharedFuture<LoadedRasterOverlayImage>>
  mapRasterTilesToGeometryTile(
      const CesiumGeometry::Rectangle& geometryRectangle,
      double targetGeometricError);

  CesiumGeometry::Rectangle _coverageRectangle;
  uint32_t _minimumLevel;
  uint32_t _maximumLevel;
  uint32_t _imageWidth;
  uint32_t _imageHeight;
  CesiumGeometry::QuadtreeTilingScheme _tilingScheme;
};
} // namespace Cesium3DTiles
