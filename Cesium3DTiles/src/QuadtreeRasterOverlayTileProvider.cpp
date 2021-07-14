
#include "Cesium3DTiles/QuadtreeRasterOverlayTileProvider.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace Cesium3DTiles {

QuadtreeRasterOverlayTileProvider::QuadtreeRasterOverlayTileProvider(
    RasterOverlay& owner,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor) noexcept
    : RasterOverlayTileProvider(owner, asyncSystem, pAssetAccessor),
      _coverageRectangle(CesiumGeometry::Rectangle(0.0, 0.0, 0.0, 0.0)),
      _minimumLevel(0),
      _maximumLevel(0),
      _imageWidth(1),
      _imageHeight(1),
      _tilingScheme(CesiumGeometry::QuadtreeTilingScheme(
          CesiumGeometry::Rectangle(0.0, 0.0, 0.0, 0.0),
          1,
          1)) {}

QuadtreeRasterOverlayTileProvider::QuadtreeRasterOverlayTileProvider(
    RasterOverlay& owner,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    std::optional<Credit> credit,
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const CesiumGeospatial::Projection& projection,
    const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
    const CesiumGeometry::Rectangle& coverageRectangle,
    uint32_t minimumLevel,
    uint32_t maximumLevel,
    uint32_t imageWidth,
    uint32_t imageHeight) noexcept
    : RasterOverlayTileProvider(
          owner,
          asyncSystem,
          pAssetAccessor,
          credit,
          pPrepareRendererResources,
          pLogger,
          projection),
      _coverageRectangle(coverageRectangle),
      _minimumLevel(minimumLevel),
      _maximumLevel(maximumLevel),
      _imageWidth(imageWidth),
      _imageHeight(imageHeight),
      _tilingScheme(tilingScheme) {}

std::vector<CesiumUtility::IntrusivePointer<RasterOverlayTile>>
QuadtreeRasterOverlayTileProvider::mapRasterTilesToGeometryTile(
    const CesiumGeospatial::GlobeRectangle& geometryRectangle,
    double targetGeometricError) {
  return this->mapRasterTilesToGeometryTile(
      projectRectangleSimple(this->getProjection(), geometryRectangle),
      targetGeometricError);
}

std::vector<CesiumUtility::IntrusivePointer<RasterOverlayTile>>
QuadtreeRasterOverlayTileProvider::mapRasterTilesToGeometryTile(
    const CesiumGeometry::Rectangle& geometryRectangle,
    double targetGeometricError) {
  if (this->_pPlaceholder) {
    return {this->_pPlaceholder.get()};
  }

  std::vector<CesiumUtility::IntrusivePointer<RasterOverlayTile>> result;

  const QuadtreeTilingScheme& imageryTilingScheme = this->getTilingScheme();

  // Use Web Mercator for our texture coordinate computations if this imagery
  // layer uses that projection and the terrain tile falls entirely inside the
  // valid bounds of the projection. bool useWebMercatorT =
  //     pWebMercatorProjection &&
  //     tileRectangle.getNorth() <= WebMercatorProjection::MAXIMUM_LATITUDE &&
  //     tileRectangle.getSouth() >= -WebMercatorProjection::MAXIMUM_LATITUDE;

  const CesiumGeometry::Rectangle& providerRectangle =
      this->getCoverageRectangle();
  const CesiumGeometry::Rectangle& tilingSchemeRectangle =
      imageryTilingScheme.getRectangle();

  // Compute the rectangle of the imagery from this raster tile provider that
  // overlaps the geometry tile.  The RasterOverlayTileProvider and its tiling
  // scheme both have the opportunity to constrain the rectangle.
  CesiumGeometry::Rectangle imageryRectangle =
      tilingSchemeRectangle.intersect(providerRectangle)
          .value_or(tilingSchemeRectangle);

  CesiumGeometry::Rectangle intersection(0.0, 0.0, 0.0, 0.0);
  std::optional<CesiumGeometry::Rectangle> maybeIntersection =
      geometryRectangle.intersect(imageryRectangle);
  if (maybeIntersection) {
    intersection = maybeIntersection.value();
  } else {
    // There is no overlap between this terrain tile and this imagery
    // provider.  Unless this is the base layer, no skeletons need to be
    // created. We stretch texels at the edge of the base layer over the entire
    // globe.

    // TODO: base layers
    // if (!this.isBaseLayer()) {
    //     return false;
    // }
    if (geometryRectangle.minimumY >= imageryRectangle.maximumY) {
      intersection.minimumY = intersection.maximumY = imageryRectangle.maximumY;
    } else if (geometryRectangle.maximumY <= imageryRectangle.minimumY) {
      intersection.minimumY = intersection.maximumY = imageryRectangle.minimumY;
    } else {
      intersection.minimumY =
          glm::max(geometryRectangle.minimumY, imageryRectangle.minimumY);

      intersection.maximumY =
          glm::min(geometryRectangle.maximumY, imageryRectangle.maximumY);
    }

    if (geometryRectangle.minimumX >= imageryRectangle.maximumX) {
      intersection.minimumX = intersection.maximumX = imageryRectangle.maximumX;
    } else if (geometryRectangle.maximumX <= imageryRectangle.minimumX) {
      intersection.minimumX = intersection.maximumX = imageryRectangle.minimumX;
    } else {
      intersection.minimumX =
          glm::max(geometryRectangle.minimumX, imageryRectangle.minimumX);

      intersection.maximumX =
          glm::min(geometryRectangle.maximumX, imageryRectangle.maximumX);
    }
  }

  // Compute the required level in the imagery tiling scheme.
  // TODO: dividing by 8 to change the default 3D Tiles SSE (16) back to the
  // terrain SSE (2)
  // TODO: Correct latitude factor, which is really a property of the
  // projection.
  uint32_t imageryLevel = this->computeLevelFromGeometricError(
      targetGeometricError / 8.0,
      intersection.getCenter());
  imageryLevel = glm::max(0U, imageryLevel);

  uint32_t maximumLevel = this->getMaximumLevel();
  if (imageryLevel > maximumLevel) {
    imageryLevel = maximumLevel;
  }

  uint32_t minimumLevel = this->getMinimumLevel();
  if (imageryLevel < minimumLevel) {
    imageryLevel = minimumLevel;
  }

  std::optional<QuadtreeTileID> southwestTileCoordinatesOpt =
      imageryTilingScheme.positionToTile(
          intersection.getLowerLeft(),
          imageryLevel);
  std::optional<QuadtreeTileID> northeastTileCoordinatesOpt =
      imageryTilingScheme.positionToTile(
          intersection.getUpperRight(),
          imageryLevel);

  // Because of the intersection, we should always have valid tile coordinates.
  // But give up if we don't.
  if (!southwestTileCoordinatesOpt || !northeastTileCoordinatesOpt) {
    return result;
  }

  QuadtreeTileID southwestTileCoordinates = southwestTileCoordinatesOpt.value();
  QuadtreeTileID northeastTileCoordinates = northeastTileCoordinatesOpt.value();

  // If the northeast corner of the rectangle lies very close to the south or
  // west side of the northeast tile, we don't actually need the northernmost or
  // easternmost tiles. Similarly, if the southwest corner of the rectangle lies
  // very close to the north or east side of the southwest tile, we don't
  // actually need the southernmost or westernmost tiles.

  // We define "very close" as being within 1/512 of the width of the tile.
  double veryCloseX = geometryRectangle.computeWidth() / 512.0;
  double veryCloseY = geometryRectangle.computeHeight() / 512.0;

  CesiumGeometry::Rectangle southwestTileRectangle =
      imageryTilingScheme.tileToRectangle(southwestTileCoordinates);

  if (glm::abs(southwestTileRectangle.maximumY - geometryRectangle.minimumY) <
          veryCloseY &&
      southwestTileCoordinates.y < northeastTileCoordinates.y) {
    ++southwestTileCoordinates.y;
  }

  if (glm::abs(southwestTileRectangle.maximumX - geometryRectangle.minimumX) <
          veryCloseX &&
      southwestTileCoordinates.x < northeastTileCoordinates.x) {
    ++southwestTileCoordinates.x;
  }

  CesiumGeometry::Rectangle northeastTileRectangle =
      imageryTilingScheme.tileToRectangle(northeastTileCoordinates);

  if (glm::abs(northeastTileRectangle.maximumY - geometryRectangle.minimumY) <
          veryCloseY &&
      northeastTileCoordinates.y > southwestTileCoordinates.y) {
    --northeastTileCoordinates.y;
  }

  if (glm::abs(northeastTileRectangle.minimumX - geometryRectangle.maximumX) <
          veryCloseX &&
      northeastTileCoordinates.x > southwestTileCoordinates.x) {
    --northeastTileCoordinates.x;
  }

  // Create TileImagery instances for each imagery tile overlapping this terrain
  // tile. We need to do all texture coordinate computations in the imagery
  // provider's projection.

  imageryRectangle =
      imageryTilingScheme.tileToRectangle(southwestTileCoordinates);
  CesiumGeometry::Rectangle imageryBounds = intersection;
  std::optional<CesiumGeometry::Rectangle> clippedImageryRectangle =
      std::nullopt;

  // size_t realOutputIndex =
  //    outputIndex ? outputIndex.value() : outputRasterTiles.size();

  double minU;
  double maxU = 0.0;

  double minV;
  double maxV = 0.0;

  // If this is the northern-most or western-most tile in the imagery tiling
  // scheme, it may not start at the northern or western edge of the terrain
  // tile. Calculate where it does start.
  // TODO
  // if (
  //     /*!this.isBaseLayer()*/ false &&
  //     glm::abs(clippedImageryRectangle.value().getWest() -
  //     terrainRectangle.getWest()) >= veryCloseX
  // ) {
  //     maxU = glm::min(
  //         1.0,
  //         (clippedImageryRectangle.value().getWest() -
  //         terrainRectangle.getWest()) / terrainRectangle.computeWidth()
  //     );
  // }

  // if (
  //     /*!this.isBaseLayer()*/ false &&
  //     glm::abs(clippedImageryRectangle.value().getNorth() -
  //     terrainRectangle.getNorth()) >= veryCloseY
  // ) {
  //     minV = glm::max(
  //         0.0,
  //         (clippedImageryRectangle.value().getNorth() -
  //         terrainRectangle.getSouth()) / terrainRectangle.computeHeight()
  //     );
  // }

  std::shared_ptr<std::vector<RasterToCombine>> pRastersToCombine =
      std::make_shared<std::vector<RasterToCombine>>();

  double initialMaxV = maxV;

  for (uint32_t i = southwestTileCoordinates.x; i <= northeastTileCoordinates.x;
       ++i) {
    minU = maxU;

    imageryRectangle = imageryTilingScheme.tileToRectangle(
        QuadtreeTileID(imageryLevel, i, southwestTileCoordinates.y));
    clippedImageryRectangle = imageryRectangle.intersect(imageryBounds);

    if (!clippedImageryRectangle) {
      continue;
    }

    maxU = glm::min(
        1.0,
        (clippedImageryRectangle.value().maximumX -
         geometryRectangle.minimumX) /
            geometryRectangle.computeWidth());

    // If this is the eastern-most imagery tile mapped to this terrain tile,
    // and there are more imagery tiles to the east of this one, the maxU
    // should be 1.0 to make sure rounding errors don't make the last
    // image fall shy of the edge of the terrain tile.
    if (i == northeastTileCoordinates.x &&
        (/*this.isBaseLayer()*/ true ||
         glm::abs(
             clippedImageryRectangle.value().maximumX -
             geometryRectangle.maximumX) < veryCloseX)) {
      maxU = 1.0;
    }

    maxV = initialMaxV;

    for (uint32_t j = southwestTileCoordinates.y;
         j <= northeastTileCoordinates.y;
         ++j) {
      minV = maxV;

      imageryRectangle = imageryTilingScheme.tileToRectangle(
          QuadtreeTileID(imageryLevel, i, j));
      clippedImageryRectangle = imageryRectangle.intersect(imageryBounds);

      if (!clippedImageryRectangle) {
        continue;
      }

      maxV = glm::min(
          1.0,
          (clippedImageryRectangle.value().maximumY -
           geometryRectangle.minimumY) /
              geometryRectangle.computeHeight());

      // If this is the northern-most imagery tile mapped to this terrain tile,
      // and there are more imagery tiles to the north of this one, the maxV
      // should be 1.0 to make sure rounding errors don't make the last
      // image fall shy of the edge of the terrain tile.
      if (j == northeastTileCoordinates.y &&
          (/*this.isBaseLayer()*/ true ||
           glm::abs(
               clippedImageryRectangle.value().maximumY -
               geometryRectangle.maximumY) < veryCloseY)) {
        maxV = 1.0;
      }

      CesiumGeometry::Rectangle texCoordsRectangle(minU, minV, maxU, maxV);

      CesiumUtility::IntrusivePointer<RasterOverlayTile> pTile =
          this->getQuadtreeTile(QuadtreeTileID(imageryLevel, i, j));

      if (pTile->getState() != RasterOverlayTile::LoadState::Placeholder) {
        this->loadTileThrottled(*pTile);
      }

      result.emplace_back(pTile);
    }
  }

  return result;
}

bool QuadtreeRasterOverlayTileProvider::hasMoreDetailsAvailable(
    const TileID& tileID) const {
  const CesiumGeometry::QuadtreeTileID* quadtreeTileID =
      std::get_if<CesiumGeometry::QuadtreeTileID>(&tileID);
  return quadtreeTileID != nullptr &&
         quadtreeTileID->level < this->_maximumLevel;
}

uint32_t QuadtreeRasterOverlayTileProvider::computeLevelFromGeometricError(
    double geometricError,
    const glm::dvec2& position) const noexcept {
  // PERFORMANCE_IDEA: factor out the stuff that doesn't change.
  const QuadtreeTilingScheme& tilingScheme = this->getTilingScheme();
  const CesiumGeometry::Rectangle& tilingSchemeRectangle =
      tilingScheme.getRectangle();

  double toMeters = computeApproximateConversionFactorToMetersNearPosition(
      this->getProjection(),
      position);

  double levelZeroMaximumTexelSpacingMeters =
      (tilingSchemeRectangle.computeWidth() * toMeters) /
      (this->_imageWidth * tilingScheme.getNumberOfXTilesAtLevel(0));

  double twoToTheLevelPower =
      levelZeroMaximumTexelSpacingMeters / geometricError;
  double level = glm::log2(twoToTheLevelPower);
  double rounded = glm::max(glm::round(level), 0.0);
  return static_cast<uint32_t>(rounded);
}

CesiumAsync::Future<LoadedRasterOverlayImage>
QuadtreeRasterOverlayTileProvider::loadTileImage(
    const RasterOverlayTile& overlayTile) {
  // Figure out which quadtree level we need, and which tiles from that level.
  // Load each needed tile (or pull it from cache).
  // If any tiles fail to load, use a parent (or ancestor) instead.
  // If _all_ tiles fail to load, we probably don't need this tile at all.
  //  exception: the parent geometry tile doesn't have the most detailed
  //  available overlay tile. But we can't tell that here. Here we just fail.
  std::vector<IntrusivePointer<RasterOverlayTile>> tiles =
      this->mapRasterTilesToGeometryTile(
          overlayTile.getRectangle(),
          overlayTile.getTargetGeometricError());

  
  LoadedRasterOverlayImage result;
  result.errors.push_back("Error: `loadTileImage(TileID tileId)` called on "
                          "QuadtreeRasterOverlayTileProvider");
  return this->getAsyncSystem().createResolvedFuture(std::move(result));
}
} // namespace Cesium3DTiles