
#include "Cesium3DTiles/QuadtreeRasterOverlayTileProvider.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumGltf;
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

std::vector<CesiumAsync::SharedFuture<LoadedRasterOverlayImage>>
QuadtreeRasterOverlayTileProvider::mapRasterTilesToGeometryTile(
    const CesiumGeospatial::GlobeRectangle& geometryRectangle,
    double targetGeometricError) {
  return this->mapRasterTilesToGeometryTile(
      projectRectangleSimple(this->getProjection(), geometryRectangle),
      targetGeometricError);
}

std::vector<CesiumAsync::SharedFuture<LoadedRasterOverlayImage>>
QuadtreeRasterOverlayTileProvider::mapRasterTilesToGeometryTile(
    const CesiumGeometry::Rectangle& geometryRectangle,
    double targetGeometricError) {
  std::vector<CesiumAsync::SharedFuture<LoadedRasterOverlayImage>> result;

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

      CesiumAsync::SharedFuture<LoadedRasterOverlayImage> pTile =
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

CesiumAsync::SharedFuture<LoadedRasterOverlayImage>
QuadtreeRasterOverlayTileProvider::getQuadtreeTile(
    const CesiumGeometry::QuadtreeTileID& tileID) {
  auto it = this->_tileCache.find(tileID);
  if (it != this->_tileCache.end()) {
    return it->second;
  }

  Future<LoadedRasterOverlayImage> future = this->loadQuadtreeTileImage(tileID);
  return this->_tileCache.emplace(tileID, std::move(future).share())
      .first->second;
}

namespace {

// Copies a rectangle of a source image into another image.
void blitImage(
    ImageCesium& target,
    int32_t targetX,
    int32_t targetY,
    const ImageCesium& source,
    int32_t sourceX,
    int32_t sourceY,
    int32_t sourceWidth,
    int32_t sourceHeight) {

  if (sourceX < 0 || sourceY < 0 || sourceWidth < 0 || sourceHeight < 0 ||
      (sourceX + sourceWidth) > source.width ||
      (sourceY + sourceHeight) > source.height) {
    // Attempting to blit from outside the bounds of the source image.
    assert(false);
    return;
  }

  if (targetX < 0 || targetY < 0 || (targetX + sourceWidth) > target.width ||
      (targetY + sourceHeight) > target.height) {
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
  size_t bytesToCopyPerRow = bytesPerPixel * size_t(sourceWidth);

  size_t bytesPerSourceRow = bytesPerPixel * size_t(source.width);
  size_t bytesPerTargetRow = bytesPerPixel * size_t(target.width);

  // Position both pointers at the start of the first row.
  std::byte* pTarget = target.pixelData.data();
  const std::byte* pSource = source.pixelData.data();
  pTarget +=
      size_t(targetY) * bytesPerTargetRow + size_t(targetX) * bytesPerPixel;
  pSource +=
      size_t(sourceY) * bytesPerSourceRow + size_t(sourceX) * bytesPerPixel;

  // Copy each row
  for (size_t j = 0; j < size_t(sourceHeight); ++j) {
    std::memcpy(pTarget, pSource, bytesToCopyPerRow);
    pTarget += bytesPerTargetRow;
    pSource += bytesPerSourceRow;
  }
}

// Copy part of a source image to part of a target image.
// The two rectangles are the extents of each image, and the part of the source
// image where the source rectangle that overlaps the target rectangle is copied
// to the target image.
void blitImage(
    ImageCesium& target,
    const Rectangle& targetRectangle,
    const ImageCesium& source,
    const Rectangle& sourceRectangle) {
  std::optional<Rectangle> overlap = targetRectangle.intersect(sourceRectangle);
  if (!overlap) {
    // No overlap, nothing to do.
    return;
  }

  // Pixel coordinates are measured from the top left.
  // Projected rectangles are measured from the bottom left.

  int32_t targetX = static_cast<int32_t>(glm::floor(
      target.width * (overlap->minimumX - targetRectangle.minimumX) /
      targetRectangle.computeWidth()));
  int32_t targetY = static_cast<int32_t>(glm::floor(
      target.height * (targetRectangle.maximumY - overlap->maximumY) /
      targetRectangle.computeHeight()));

  int32_t sourceX = static_cast<int32_t>(glm::floor(
      source.width * (overlap->minimumX - sourceRectangle.minimumX) /
      sourceRectangle.computeWidth()));
  int32_t sourceY = static_cast<int32_t>(glm::floor(
      source.height * (sourceRectangle.maximumY - overlap->maximumY) /
      sourceRectangle.computeHeight()));

  int32_t sourceMaxX = static_cast<int32_t>(glm::ceil(
      source.width * (overlap->maximumX - sourceRectangle.minimumX) /
      sourceRectangle.computeWidth()));
  int32_t sourceMaxY = static_cast<int32_t>(glm::ceil(
      source.height * (sourceRectangle.maximumY - overlap->minimumY) /
      sourceRectangle.computeHeight()));
  int32_t sourceWidth = sourceMaxX - sourceX;
  int32_t sourceHeight = sourceMaxY - sourceY;

  blitImage(
      target,
      targetX,
      targetY,
      source,
      sourceX,
      sourceY,
      sourceWidth,
      sourceHeight);
}

// TODO: find a way to do this type of thing on the GPU
// Will need a well thought out interface to let cesium-native use GPU
// resources while being engine-agnostic.
// TODO: probably can simplify dramatically by ignoring cases where there is
// discrepancy between channels count or bytesPerChannel between the rasters
// std::optional<CesiumGltf::ImageCesium> blitRasters(
//     const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
//     const CesiumGeospatial::Projection& projection,
//     const CesiumGeometry::Rectangle& targetRectangle,
//     std::vector<LoadedQuadtreeImage>& rastersToCombine) {

//   double targetWidth = targetRectangle.computeWidth();
//   double targetHeight = targetRectangle.computeHeight();

//   int32_t pixelsWidth = 0;
//   int32_t pixelsHeight = 0;
//   int32_t bytesPerChannel = 1;
//   int32_t channels = 1;

//   for (const LoadedQuadtreeImage& rasterToCombine : rastersToCombine) {
//     if (!rasterToCombine.image) {
//       continue;
//     }

//     Rectangle imageRectangle =
//     tilingScheme.tileToRectangle(rasterToCombine.id);

//     std::optional<Rectangle> overlap =
//         targetRectangle.intersect(imageRectangle);

//     // There should always be an overlap, otherwise why is this image here at
//     // all?
//     assert(overlap.has_value());

//     // Find the number of pixels of this source image that overlap the target
//     // rectangle. Round up.
//     const CesiumGltf::ImageCesium& rasterImage = *rasterToCombine.image;
//     int32_t width = static_cast<int32_t>(glm::ceil(
//         (overlap->computeWidth() / imageRectangle.computeWidth()) *
//         rasterImage.width));
//     int32_t height = static_cast<int32_t>(glm::ceil(
//         (overlap->computeHeight() / imageRectangle.computeHeight()) *
//         rasterImage.height));

//     pixelsWidth += width;
//     pixelsHeight += height;

//     if (rasterImage.bytesPerChannel > bytesPerChannel) {
//       bytesPerChannel = rasterImage.bytesPerChannel;
//     }
//     if (rasterImage.channels > channels) {
//       channels = rasterImage.channels;
//     }
//   }

//   CesiumGltf::ImageCesium image;
//   image.bytesPerChannel = bytesPerChannel;
//   image.channels = channels;
//   image.width = static_cast<int32_t>(glm::ceil(pixelsWidth));
//   image.height = static_cast<int32_t>(glm::ceil(pixelsHeight));
//   image.pixelData.resize(static_cast<size_t>(
//       image.width * image.height * image.channels * image.bytesPerChannel));

//   // Texture coordinates range from South (0.0) to North (1.0).
//   // But pixels in images are stored in North (0) to South (imageHeight - 1)
//   // order.

//   for (int32_t j = 0; j < image.height; ++j) {
//     // Use the texture coordinate for the _center_ of each pixel.
//     // And adjust for the flipped direction of texture coordinates and
//     pixels. double v = 1.0 - ((double(j) + 0.5) / double(image.height));

//     for (int32_t i = 0; i < image.width; ++i) {
//       glm::dvec2 uv((double(i) + 0.5) / double(image.width), v);

//       for (const RasterToCombine& rasterToCombine : *pRastersToCombine) {

//         if (rasterToCombine._textureCoordinateRectangle.contains(uv)) {
//           const CesiumGltf::ImageCesium& srcImage =
//               rasterToCombine._pReadyTile->getImage();

//           glm::dvec2 srcUv =
//               uv * rasterToCombine._scale + rasterToCombine._translation;

//           // TODO: remove?
//           if (srcUv.x < 0.0 || srcUv.x > 1.0 || srcUv.y < 0.0 ||
//               srcUv.y > 1.0) {
//             continue;
//           }

//           glm::dvec2 srcPixel(
//               srcUv.x * srcImage.width,
//               (1.0 - srcUv.y) * srcImage.height);

//           int32_t srcPixelX = static_cast<int32_t>(
//               glm::clamp(glm::floor(srcPixel.x), 0.0,
//               double(srcImage.width)));
//           int32_t srcPixelY = static_cast<int32_t>(
//               glm::clamp(glm::floor(srcPixel.y), 0.0,
//               double(srcImage.height)));

//           const std::byte* pSrcPixelValue =
//               srcImage.pixelData.data() +
//               static_cast<size_t>(
//                   srcImage.channels * srcImage.bytesPerChannel *
//                   (srcImage.width * srcPixelY + srcPixelX));

//           std::byte* pTargetPixel =
//               image.pixelData.data() +
//               channels * bytesPerChannel * (image.width * j + i);

//           for (int32_t channel = 0; channel < srcImage.channels; ++channel) {
//             std::memcpy(
//                 pTargetPixel +
//                     static_cast<size_t>(
//                         channel * bytesPerChannel +
//                         (bytesPerChannel - srcImage.bytesPerChannel)),
//                 pSrcPixelValue +
//                     static_cast<size_t>(channel * srcImage.bytesPerChannel),
//                 static_cast<size_t>(srcImage.bytesPerChannel));
//           }
//         }
//       }
//     }
//   }

//   return image;
// }

struct CombinedImageMeasurements {
  Rectangle rectangle;
  int32_t widthPixels;
  int32_t heightPixels;
  int32_t channels;
  int32_t bytesPerChannel;
};

CombinedImageMeasurements
measureCombinedImage(const std::vector<LoadedRasterOverlayImage>& images) {
  auto it = std::find_if(
      images.begin(),
      images.end(),
      [](const LoadedRasterOverlayImage& loaded) {
        return loaded.image && loaded.image->width > 0 &&
               loaded.image->height > 0;
      });
  if (it == images.end()) {
    // There are no images to combine.
    return CombinedImageMeasurements{Rectangle(), 0, 0, 0, 0};
  }

  const LoadedRasterOverlayImage& first = *it;
  const ImageCesium& firstImage = first.image.value();

  double projectedWidthPerPixel =
      first.rectangle.computeWidth() / firstImage.width;
  double projectedHeightPerPixel =
      first.rectangle.computeHeight() / firstImage.height;

  Rectangle combinedRectangle = first.rectangle;

  // Assumption: all images have pixels with the same width and height in
  // projected units. This might prove false with esoteric projections that,
  // for example, change tiling scheme or projection parameters as latitude
  // increases. But we're not currently handling that kind of scenario.
  for (; it != images.end(); ++it) {
    const LoadedRasterOverlayImage& loaded = *it;
    if (!loaded.image || loaded.image->width <= 0 ||
        loaded.image->height <= 0) {
      continue;
    }

    // Make sure the assumption above holds to within 1/1000 of a pixel.
    assert(Math::equalsEpsilon(
        loaded.rectangle.computeWidth() / loaded.image->width,
        projectedWidthPerPixel,
        Math::EPSILON3 / loaded.image->width));
    assert(Math::equalsEpsilon(
        loaded.rectangle.computeHeight() / loaded.image->height,
        projectedHeightPerPixel,
        Math::EPSILON3 / loaded.image->height));

    // Also make sure all images have the same format.
    assert(loaded.image->channels == firstImage.channels);
    assert(loaded.image->bytesPerChannel == firstImage.bytesPerChannel);

    // Find the bounds of the combined image.
    combinedRectangle = combinedRectangle.computeUnion(loaded.rectangle);
  }

  // Compute the pixel dimensions needed for the combined image.
  int32_t combinedWidthPixels = static_cast<int32_t>(
      glm::ceil(combinedRectangle.computeWidth() / projectedWidthPerPixel));
  int32_t combinedHeightPixels = static_cast<int32_t>(
      glm::ceil(combinedRectangle.computeHeight() / projectedHeightPerPixel));

  return CombinedImageMeasurements{
      combinedRectangle,
      combinedWidthPixels,
      combinedHeightPixels,
      firstImage.channels,
      firstImage.bytesPerChannel};
}

LoadedRasterOverlayImage combineImages(
    const Projection& /* projection */,
    std::vector<LoadedRasterOverlayImage>&& images) {
  CombinedImageMeasurements measurements = measureCombinedImage(images);

  LoadedRasterOverlayImage result;
  result.rectangle = measurements.rectangle;
  result.moreDetailAvailable = false;

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
    blitImage(target, result.rectangle, *it->image, it->rectangle);
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
  std::vector<CesiumAsync::SharedFuture<LoadedRasterOverlayImage>> tiles =
      this->mapRasterTilesToGeometryTile(
          overlayTile.getRectangle(),
          overlayTile.getTargetGeometricError());

  return this->_asyncSystem.all(std::move(tiles))
      .thenInWorkerThread([projection = this->getProjection()](
                              std::vector<LoadedRasterOverlayImage>&& images) {
        return combineImages(projection, std::move(images));
      })
      .catchImmediately(
          [](std::exception&& /* e */) { return LoadedRasterOverlayImage(); });
}

} // namespace Cesium3DTiles
