
#include "Cesium3DTiles/QuadtreeRasterOverlayTileProvider.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumUtility/Math.h"
#include <stb_image_resize.h>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumUtility;

namespace {

// One hundredth of a pixel. If we compute a rectangle that goes less than "this
// much" into the next pixel, we'll ignore the extra.
const double pixelTolerance = 0.01;

} // namespace

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

std::vector<CesiumAsync::SharedFuture<LoadedQuadtreeImage>>
QuadtreeRasterOverlayTileProvider::mapRasterTilesToGeometryTile(
    const CesiumGeospatial::GlobeRectangle& geometryRectangle,
    double targetGeometricError) {
  return this->mapRasterTilesToGeometryTile(
      projectRectangleSimple(this->getProjection(), geometryRectangle),
      targetGeometricError);
}

std::vector<CesiumAsync::SharedFuture<LoadedQuadtreeImage>>
QuadtreeRasterOverlayTileProvider::mapRasterTilesToGeometryTile(
    const CesiumGeometry::Rectangle& geometryRectangle,
    double targetGeometricError) {
  std::vector<CesiumAsync::SharedFuture<LoadedQuadtreeImage>> result;

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

      CesiumAsync::SharedFuture<LoadedQuadtreeImage> pTile =
          this->getQuadtreeTile(QuadtreeTileID(imageryLevel, i, j));

      // if (pTile->getState() != RasterOverlayTile::LoadState::Placeholder) {
      //   this->loadTileThrottled(*pTile);
      // }

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

CesiumAsync::SharedFuture<LoadedQuadtreeImage>
QuadtreeRasterOverlayTileProvider::getQuadtreeTile(
    const CesiumGeometry::QuadtreeTileID& tileID) {
  auto it = this->_tileCache.find(tileID);
  if (it != this->_tileCache.end()) {
    return it->second;
  }

  Future<LoadedQuadtreeImage> future =
      this->loadQuadtreeTileImage(tileID)
          .catchImmediately([](std::exception&& e) {
            // Turn an exception into an error.
            LoadedRasterOverlayImage result;
            result.errors.emplace_back(e.what());
            return result;
          })
          .thenInMainThread([tileID, this](LoadedRasterOverlayImage&& loaded) {
            if (loaded.image && loaded.errors.empty() &&
                loaded.image->width > 0 && loaded.image->height > 0) {
              // Successfully loaded, continue.
              return this->getAsyncSystem().createResolvedFuture(
                  LoadedQuadtreeImage(std::move(loaded)));
            }

            // Tile failed to load, try loading the parent tile instead.
            if (tileID.level > this->getMinimumLevel()) {
              QuadtreeTileID parentID(
                  tileID.level - 1,
                  tileID.x >> 1,
                  tileID.y >> 1);
              return this->getQuadtreeTile(parentID).thenImmediately(
                  [rectangle =
                       loaded.rectangle](const LoadedQuadtreeImage& image) {
                    // TODO: can we avoid copying the image data?
                    LoadedQuadtreeImage newImage(image);
                    newImage.subset = rectangle;
                    return newImage;
                  });
            }

            // No parent available, so return the original failed result.
            return this->getAsyncSystem().createResolvedFuture(
                LoadedQuadtreeImage(std::move(loaded)));
          });
  return this->_tileCache.emplace(tileID, std::move(future).share())
      .first->second;
}

namespace {

struct PixelRectangle {
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;
};

// Copies a rectangle of a source image into another image.
void blitImage(
    ImageCesium& target,
    const PixelRectangle& targetPixels,
    const ImageCesium& source,
    const PixelRectangle& sourcePixels) {

  if (sourcePixels.x < 0 || sourcePixels.y < 0 || sourcePixels.width < 0 ||
      sourcePixels.height < 0 ||
      (sourcePixels.x + sourcePixels.width) > source.width ||
      (sourcePixels.y + sourcePixels.height) > source.height) {
    // Attempting to blit from outside the bounds of the source image.
    assert(false);
    return;
  }

  if (targetPixels.x < 0 || targetPixels.y < 0 ||
      (targetPixels.x + targetPixels.width) > target.width ||
      (targetPixels.y + targetPixels.height) > target.height) {
    // Attempting to blit outside the bounds of the target image.
    assert(false);
    return;
  }

  if (target.channels != source.channels ||
      target.bytesPerChannel != source.bytesPerChannel) {
    // Source and target image formats don't match; currently not supported.
    assert(false);
    return;
  }

  size_t bytesPerPixel = size_t(target.bytesPerChannel * target.channels);
  size_t bytesToCopyPerRow = bytesPerPixel * size_t(sourcePixels.width);

  size_t bytesPerSourceRow = bytesPerPixel * size_t(source.width);
  size_t bytesPerTargetRow = bytesPerPixel * size_t(target.width);

  // Position both pointers at the start of the first row.
  std::byte* pTarget = target.pixelData.data();
  const std::byte* pSource = source.pixelData.data();
  pTarget += size_t(targetPixels.y) * bytesPerTargetRow +
             size_t(targetPixels.x) * bytesPerPixel;
  pSource += size_t(sourcePixels.y) * bytesPerSourceRow +
             size_t(sourcePixels.x) * bytesPerPixel;

  if (sourcePixels.width == targetPixels.width &&
      sourcePixels.height == targetPixels.height) {
    // Simple, unscaled, byte-for-byte image copy.
    for (size_t j = 0; j < size_t(sourcePixels.height); ++j) {
      assert(pTarget < target.pixelData.data() + target.pixelData.size());
      assert(pSource < source.pixelData.data() + source.pixelData.size());
      assert(
          pTarget + bytesToCopyPerRow <=
          target.pixelData.data() + target.pixelData.size());
      assert(
          pSource + bytesToCopyPerRow <=
          source.pixelData.data() + source.pixelData.size());
      std::memcpy(pTarget, pSource, bytesToCopyPerRow);
      pTarget += bytesPerTargetRow;
      pSource += bytesPerSourceRow;
    }
  } else {
    // Use STB to do the copy / scale
    stbir_resize_uint8(
        reinterpret_cast<const unsigned char*>(pSource),
        sourcePixels.width,
        sourcePixels.height,
        int(bytesPerSourceRow),
        reinterpret_cast<unsigned char*>(pTarget),
        targetPixels.width,
        targetPixels.height,
        int(bytesPerTargetRow),
        target.channels);
  }
}

PixelRectangle computePixelRectangle(
    const ImageCesium& image,
    const Rectangle& totalRectangle,
    const Rectangle& partRectangle) {
  // Pixel coordinates are measured from the top left.
  // Projected rectangles are measured from the bottom left.

  int32_t x = static_cast<int32_t>(Math::roundDown(
      image.width * (partRectangle.minimumX - totalRectangle.minimumX) /
          totalRectangle.computeWidth(),
      pixelTolerance));
  x = glm::max(0, x);
  int32_t y = static_cast<int32_t>(Math::roundDown(
      image.height * (totalRectangle.maximumY - partRectangle.maximumY) /
          totalRectangle.computeHeight(),
      pixelTolerance));
  y = glm::max(0, y);

  int32_t maxX = static_cast<int32_t>(Math::roundUp(
      image.width * (partRectangle.maximumX - totalRectangle.minimumX) /
          totalRectangle.computeWidth(),
      pixelTolerance));
  maxX = glm::min(maxX, image.width);
  int32_t maxY = static_cast<int32_t>(Math::roundUp(
      image.height * (totalRectangle.maximumY - partRectangle.minimumY) /
          totalRectangle.computeHeight(),
      pixelTolerance));
  maxY = glm::min(maxY, image.height);

  return PixelRectangle{x, y, maxX - x, maxY - y};
}

// Copy part of a source image to part of a target image.
// The two rectangles are the extents of each image, and the part of the source
// image where the source subset rectangle overlaps the target rectangle is
// copied to the target image.
void blitImage(
    ImageCesium& target,
    const Rectangle& targetRectangle,
    const ImageCesium& source,
    const Rectangle& sourceRectangle,
    const std::optional<Rectangle>& sourceSubset) {
  Rectangle sourceToCopy = sourceSubset.value_or(sourceRectangle);
  std::optional<Rectangle> overlap = targetRectangle.intersect(sourceToCopy);
  if (!overlap) {
    // No overlap, nothing to do.
    return;
  }

  // Pixel coordinates are measured from the top left.
  // Projected rectangles are measured from the bottom left.

  PixelRectangle targetPixels =
      computePixelRectangle(target, targetRectangle, *overlap);
  PixelRectangle sourcePixels =
      computePixelRectangle(source, sourceRectangle, *overlap);

  blitImage(target, targetPixels, source, sourcePixels);
}

struct CombinedImageMeasurements {
  Rectangle rectangle;
  int32_t widthPixels;
  int32_t heightPixels;
  int32_t channels;
  int32_t bytesPerChannel;
};

CombinedImageMeasurements measureCombinedImage(
    const Rectangle& targetRectangle,
    const std::vector<LoadedQuadtreeImage>& images) {
  // Find the image with the densest pixels, and use that to select the
  // resolution of the target image.

  // In a quadtree, all tiles within a single zoom level should have pixels with
  // the same projected dimensions. However, some of our images may be from
  // different levels. For example, if a child tile from a particular zoom level
  // is not available, an ancestor tile with a lower resolution (larger pixel
  // size) may be used instead. These ancestor tiles should have a pixel spacing
  // that is an even multiple of the finest tiles.
  double projectedWidthPerPixel = std::numeric_limits<double>::max();
  double projectedHeightPerPixel = std::numeric_limits<double>::max();
  int32_t channels = -1;
  int32_t bytesPerChannel = -1;
  for (const LoadedQuadtreeImage& loaded : images) {
    if (!loaded.image || loaded.image->width <= 0 ||
        loaded.image->height <= 0) {
      continue;
    }

    projectedWidthPerPixel = glm::min(
        projectedWidthPerPixel,
        loaded.rectangle.computeWidth() / loaded.image->width);
    projectedHeightPerPixel = glm::min(
        projectedHeightPerPixel,
        loaded.rectangle.computeHeight() / loaded.image->height);

    channels = glm::max(channels, loaded.image->channels);
    bytesPerChannel = glm::max(bytesPerChannel, loaded.image->bytesPerChannel);
  }

  std::optional<Rectangle> combinedRectangle;

  for (const LoadedQuadtreeImage& loaded : images) {
    if (!loaded.image || loaded.image->width <= 0 ||
        loaded.image->height <= 0) {
      continue;
    }

    // The portion of the source that we actually need to copy.
    Rectangle sourceSubset = loaded.subset.value_or(loaded.rectangle);

    // Find the bounds of the combined image.
    // Intersect the loaded image's rectangle with the target rectangle.
    std::optional<Rectangle> maybeIntersection =
        targetRectangle.intersect(sourceSubset);
    if (!maybeIntersection) {
      // We really shouldn't have an image that doesn't overlap the target.
      assert(false);
      continue;
    }

    Rectangle& intersection = *maybeIntersection;

    // Expand this slightly so we don't wind up with partial pixels in the
    // target
    intersection.minimumX = Math::roundDown(
                                intersection.minimumX / projectedWidthPerPixel,
                                pixelTolerance) *
                            projectedWidthPerPixel;
    intersection.minimumY = Math::roundDown(
                                intersection.minimumY / projectedHeightPerPixel,
                                pixelTolerance) *
                            projectedHeightPerPixel;
    intersection.maximumX = Math::roundUp(
                                intersection.maximumX / projectedWidthPerPixel,
                                pixelTolerance) *
                            projectedWidthPerPixel;
    intersection.maximumY = Math::roundUp(
                                intersection.maximumY / projectedHeightPerPixel,
                                pixelTolerance) *
                            projectedHeightPerPixel;

    if (combinedRectangle) {
      combinedRectangle = combinedRectangle->computeUnion(intersection);
    } else {
      combinedRectangle = intersection;
    }
  }

  if (!combinedRectangle) {
    return CombinedImageMeasurements{targetRectangle, 0, 0, 0, 0};
  }

  // Compute the pixel dimensions needed for the combined image.
  int32_t combinedWidthPixels = static_cast<int32_t>(Math::roundUp(
      combinedRectangle->computeWidth() / projectedWidthPerPixel,
      pixelTolerance));
  int32_t combinedHeightPixels = static_cast<int32_t>(Math::roundUp(
      combinedRectangle->computeHeight() / projectedHeightPerPixel,
      pixelTolerance));

  return CombinedImageMeasurements{
      *combinedRectangle,
      combinedWidthPixels,
      combinedHeightPixels,
      channels,
      bytesPerChannel};
}

LoadedRasterOverlayImage combineImages(
    const Rectangle& targetRectangle,
    const Projection& /* projection */,
    std::vector<LoadedQuadtreeImage>&& images) {

  CombinedImageMeasurements measurements =
      measureCombinedImage(targetRectangle, images);

  int32_t targetImageBytes = measurements.widthPixels *
                             measurements.heightPixels * measurements.channels *
                             measurements.bytesPerChannel;
  if (targetImageBytes <= 0) {
    // Target image has no pixels, so our work here is done.
    return LoadedRasterOverlayImage{
        std::nullopt,
        targetRectangle,
        {},
        {},
        {},
        true // TODO
    };
  }

  LoadedRasterOverlayImage result;
  result.rectangle = measurements.rectangle;
  result.moreDetailAvailable = true; // TODO

  ImageCesium& target = result.image.emplace();
  target.bytesPerChannel = measurements.bytesPerChannel;
  target.channels = measurements.channels;
  target.width = measurements.widthPixels;
  target.height = measurements.heightPixels;
  target.pixelData.resize(size_t(
      target.width * target.height * target.channels * target.bytesPerChannel));

  for (auto it = images.begin(); it != images.end(); ++it) {
    if (!it->image) {
      continue;
    }

    blitImage(target, result.rectangle, *it->image, it->rectangle, it->subset);
  }

  // TODO: detect when _all_ images are from an ancestor, because then
  // we can discard this image.
  return result;
}

} // namespace

CesiumAsync::Future<LoadedRasterOverlayImage>
QuadtreeRasterOverlayTileProvider::loadTileImage(
    RasterOverlayTile& overlayTile) {

  // Figure out which quadtree level we need, and which tiles from that level.
  // Load each needed tile (or pull it from cache).
  // If any tiles fail to load, use a parent (or ancestor) instead.
  // If _all_ tiles fail to load, we probably don't need this tile at all.
  //  exception: the parent geometry tile doesn't have the most detailed
  //  available overlay tile. But we can't tell that here. Here we just fail.
  //  TODO: handle this exception somewhere
  std::vector<CesiumAsync::SharedFuture<LoadedQuadtreeImage>> tiles =
      this->mapRasterTilesToGeometryTile(
          overlayTile.getRectangle(),
          overlayTile.getTargetGeometricError());

  return this->getAsyncSystem()
      .all(std::move(tiles))
      .thenInWorkerThread([projection = this->getProjection(),
                           rectangle = overlayTile.getRectangle()](
                              std::vector<LoadedQuadtreeImage>&& images) {
        // This set of images is only "useful" if at least one actually has
        // image data, and that image data is _not_ from an ancestor. We can
        // identify ancestor images because they have a `subset`.
        bool haveAnyUsefulImageData = std::any_of(
            images.begin(),
            images.end(),
            [](const LoadedQuadtreeImage& image) {
              return image.image.has_value() && !image.subset.has_value();
            });

        if (!haveAnyUsefulImageData) {
          // For non-useful sets of images, just return an empty image,
          // signalling that the parent tile should be used instead.
          return LoadedRasterOverlayImage{
              ImageCesium(),
              Rectangle(),
              {},
              {},
              {},
              false};
        }

        return combineImages(rectangle, projection, std::move(images));
      })
      .catchImmediately(
          [](std::exception&& /* e */) { return LoadedRasterOverlayImage(); });
}

} // namespace Cesium3DTiles
