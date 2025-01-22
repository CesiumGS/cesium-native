#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/SharedFuture.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGltfContent/ImageManipulation.h>
#include <CesiumRasterOverlays/QuadtreeRasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Math.h>
#include <CesiumUtility/Result.h>

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/ext/vector_double2.hpp>
#include <spdlog/logger.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumGltfContent;
using namespace CesiumGltfReader;
using namespace CesiumUtility;

namespace {

// One hundredth of a pixel. If we compute a rectangle that goes less than "this
// much" into the next pixel, we'll ignore the extra.
constexpr double pixelTolerance = 0.01;

} // namespace

namespace CesiumRasterOverlays {

QuadtreeRasterOverlayTileProvider::QuadtreeRasterOverlayTileProvider(
    const IntrusivePointer<const RasterOverlay>& pOwner,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    std::optional<Credit> credit,
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const CesiumGeospatial::Projection& projection,
    const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
    const CesiumGeometry::Rectangle& coverageRectangle,
    uint32_t minimumLevel,
    uint32_t maximumLevel,
    uint32_t imageWidth,
    uint32_t imageHeight) noexcept
    : RasterOverlayTileProvider(
          pOwner,
          asyncSystem,
          pAssetAccessor,
          credit,
          pPrepareRendererResources,
          pLogger,
          projection,
          coverageRectangle),
      _minimumLevel(minimumLevel),
      _maximumLevel(maximumLevel),
      _imageWidth(imageWidth),
      _imageHeight(imageHeight),
      _tilingScheme(tilingScheme) {
  auto loadParentTile = [this](const QuadtreeTileID& key)
      -> Future<ResultPointer<LoadedQuadtreeImage>> {
    const Rectangle rectangle = this->getTilingScheme().tileToRectangle(key);
    const QuadtreeTileID parentID(key.level - 1, key.x >> 1, key.y >> 1);
    return this->getQuadtreeTile(parentID).thenImmediately(
        [rectangle](const ResultPointer<LoadedQuadtreeImage>& loaded) {
          ResultPointer<LoadedQuadtreeImage> result(loaded.errors);
          if (loaded.pValue) {
            result.pValue.emplace(*loaded.pValue);
            result.pValue->subset = rectangle;
          }
          return result;
        });
  };

  this->_pTileDepot.emplace(std::function(
      [pThis = this, loadParentTile](
          const AsyncSystem& asyncSystem,
          [[maybe_unused]] const std::shared_ptr<IAssetAccessor>&
              pAssetAccessor,
          const QuadtreeTileID& key)
          -> Future<ResultPointer<LoadedQuadtreeImage>> {
        return pThis->loadQuadtreeTileImage(key)
            .catchImmediately([](std::exception&& e) {
              // Turn an exception into an error.
              LoadedRasterOverlayImage result;
              result.errorList.emplaceError(e.what());
              return result;
            })
            .thenImmediately(
                [loadParentTile,
                 key,
                 currentLevel = key.level,
                 minimumLevel = pThis->getMinimumLevel(),
                 asyncSystem](LoadedRasterOverlayImage&& loaded)
                    -> Future<ResultPointer<LoadedQuadtreeImage>> {
                  if (loaded.pImage && !loaded.errorList.hasErrors() &&
                      loaded.pImage->width > 0 && loaded.pImage->height > 0) {
#if SHOW_TILE_BOUNDARIES
                    // Highlight the edges in red to show tile boundaries.
                    std::span<uint32_t> pixels =
                        reintepretCastSpan<uint32_t, std::byte>(
                            loaded.image->pixelData);
                    for (int32_t j = 0; j < loaded.pImage->height; ++j) {
                      for (int32_t i = 0; i < loaded.pImage->width; ++i) {
                        if (i == 0 || j == 0 || i == loaded.pImage->width - 1 ||
                            j == loaded.pImage->height - 1) {
                          pixels[j * loaded.pImage->width + i] = 0xFF0000FF;
                        }
                      }
                    }
#endif

                    IntrusivePointer<LoadedQuadtreeImage> pLoadedImage;
                    pLoadedImage.emplace(
                        std::make_shared<LoadedRasterOverlayImage>(
                            std::move(loaded)),
                        std::nullopt);
                    return asyncSystem.createResolvedFuture(
                        ResultPointer<LoadedQuadtreeImage>(pLoadedImage));
                  }

                  // Tile failed to load, try loading the parent tile instead.
                  // We can only initiate a new tile request from the main
                  // thread, though.
                  if (currentLevel > minimumLevel) {
                    return asyncSystem.runInMainThread([key, loadParentTile]() {
                      return loadParentTile(key);
                    });
                  } else {
                    // No parent available, so return the original failed
                    // result.
                    IntrusivePointer<LoadedQuadtreeImage> pLoadedImage;
                    pLoadedImage.emplace(
                        std::make_shared<LoadedRasterOverlayImage>(
                            std::move(loaded)),
                        std::nullopt);
                    return asyncSystem.createResolvedFuture(
                        ResultPointer<LoadedQuadtreeImage>(pLoadedImage));
                  }
                });
      }));
}

uint32_t QuadtreeRasterOverlayTileProvider::computeLevelFromTargetScreenPixels(
    const CesiumGeometry::Rectangle& rectangle,
    const glm::dvec2& screenPixels) {
  const double rasterScreenSpaceError =
      this->getOwner().getOptions().maximumScreenSpaceError;

  const glm::dvec2 rasterPixels = screenPixels / rasterScreenSpaceError;
  const glm::dvec2 rasterTiles =
      rasterPixels / glm::dvec2(this->getWidth(), this->getHeight());
  const glm::dvec2 targetTileDimensions =
      glm::dvec2(rectangle.computeWidth(), rectangle.computeHeight()) /
      rasterTiles;
  const glm::dvec2 totalDimensions = glm::dvec2(
      this->getTilingScheme().getRectangle().computeWidth(),
      this->getTilingScheme().getRectangle().computeHeight());
  const glm::dvec2 totalTileDimensions =
      totalDimensions / glm::dvec2(
                            this->getTilingScheme().getRootTilesX(),
                            this->getTilingScheme().getRootTilesY());
  const glm::dvec2 twoToTheLevelPower =
      totalTileDimensions / targetTileDimensions;
  const glm::dvec2 level = glm::log2(twoToTheLevelPower);
  const glm::dvec2 rounded = glm::max(glm::round(level), 0.0);

  uint32_t imageryLevel = uint32_t(glm::max(rounded.x, rounded.y));

  const uint32_t maximumLevel = this->getMaximumLevel();
  if (imageryLevel > maximumLevel) {
    imageryLevel = maximumLevel;
  }

  const uint32_t minimumLevel = this->getMinimumLevel();
  if (imageryLevel < minimumLevel) {
    imageryLevel = minimumLevel;
  }

  return imageryLevel;
}

std::vector<CesiumAsync::SharedFuture<
    ResultPointer<QuadtreeRasterOverlayTileProvider::LoadedQuadtreeImage>>>
QuadtreeRasterOverlayTileProvider::mapRasterTilesToGeometryTile(
    const CesiumGeometry::Rectangle& geometryRectangle,
    const glm::dvec2 targetScreenPixels) {
  std::vector<CesiumAsync::SharedFuture<ResultPointer<LoadedQuadtreeImage>>>
      result;

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
      tilingSchemeRectangle.computeIntersection(providerRectangle)
          .value_or(tilingSchemeRectangle);

  CesiumGeometry::Rectangle intersection(0.0, 0.0, 0.0, 0.0);
  std::optional<CesiumGeometry::Rectangle> maybeIntersection =
      geometryRectangle.computeIntersection(imageryRectangle);
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
  uint32_t level = this->computeLevelFromTargetScreenPixels(
      geometryRectangle,
      targetScreenPixels);

  std::optional<QuadtreeTileID> southwestTileCoordinatesOpt =
      imageryTilingScheme.positionToTile(intersection.getLowerLeft(), level);
  std::optional<QuadtreeTileID> northeastTileCoordinatesOpt =
      imageryTilingScheme.positionToTile(intersection.getUpperRight(), level);

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
  const double veryCloseX = geometryRectangle.computeWidth() / 512.0;
  const double veryCloseY = geometryRectangle.computeHeight() / 512.0;

  const CesiumGeometry::Rectangle southwestTileRectangle =
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

  const CesiumGeometry::Rectangle northeastTileRectangle =
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

  // If we're mapping too many tiles, reduce the level until it's sane.
  uint32_t maxTextureSize =
      uint32_t(this->getOwner().getOptions().maximumTextureSize);

  uint32_t tilesX = northeastTileCoordinates.x - southwestTileCoordinates.x + 1;
  uint32_t tilesY = northeastTileCoordinates.y - southwestTileCoordinates.y + 1;

  while (level > 0U && (tilesX * this->getWidth() > maxTextureSize ||
                        tilesY * this->getHeight() > maxTextureSize)) {
    --level;
    northeastTileCoordinates = northeastTileCoordinates.getParent();
    southwestTileCoordinates = southwestTileCoordinates.getParent();
    tilesX = northeastTileCoordinates.x - southwestTileCoordinates.x + 1;
    tilesY = northeastTileCoordinates.y - southwestTileCoordinates.y + 1;
  }

  // Create TileImagery instances for each imagery tile overlapping this terrain
  // tile. We need to do all texture coordinate computations in the imagery
  // provider's projection.
  const CesiumGeometry::Rectangle imageryBounds = intersection;
  std::optional<CesiumGeometry::Rectangle> clippedImageryRectangle =
      std::nullopt;

  for (uint32_t i = southwestTileCoordinates.x; i <= northeastTileCoordinates.x;
       ++i) {

    imageryRectangle = imageryTilingScheme.tileToRectangle(
        QuadtreeTileID(level, i, southwestTileCoordinates.y));
    clippedImageryRectangle =
        imageryRectangle.computeIntersection(imageryBounds);

    if (!clippedImageryRectangle) {
      continue;
    }

    for (uint32_t j = southwestTileCoordinates.y;
         j <= northeastTileCoordinates.y;
         ++j) {

      imageryRectangle =
          imageryTilingScheme.tileToRectangle(QuadtreeTileID(level, i, j));
      clippedImageryRectangle =
          imageryRectangle.computeIntersection(imageryBounds);

      if (!clippedImageryRectangle) {
        continue;
      }

      CesiumAsync::SharedFuture<ResultPointer<LoadedQuadtreeImage>> pTile =
          this->getQuadtreeTile(QuadtreeTileID(level, i, j));
      result.emplace_back(std::move(pTile));
    }
  }

  return result;
}

CesiumAsync::SharedFuture<
    ResultPointer<QuadtreeRasterOverlayTileProvider::LoadedQuadtreeImage>>
QuadtreeRasterOverlayTileProvider::getQuadtreeTile(
    const CesiumGeometry::QuadtreeTileID& tileID) {
  return this->_pTileDepot->getOrCreate(
      this->getAsyncSystem(),
      this->getAssetAccessor(),
      tileID);
}

namespace {

PixelRectangle computePixelRectangle(
    const ImageAsset& image,
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
// The two rectangles are the extents of each image, and the part of the
// source image where the source subset rectangle overlaps the target
// rectangle is copied to the target image.
void blitImage(
    ImageAsset& target,
    const Rectangle& targetRectangle,
    const ImageAsset& source,
    const Rectangle& sourceRectangle,
    const std::optional<Rectangle>& sourceSubset) {
  const Rectangle sourceToCopy = sourceSubset.value_or(sourceRectangle);
  std::optional<Rectangle> overlap =
      targetRectangle.computeIntersection(sourceToCopy);
  if (!overlap) {
    // No overlap, nothing to do.
    return;
  }

  const PixelRectangle targetPixels =
      computePixelRectangle(target, targetRectangle, *overlap);
  const PixelRectangle sourcePixels =
      computePixelRectangle(source, sourceRectangle, *overlap);

  ImageManipulation::blitImage(target, targetPixels, source, sourcePixels);
}

} // namespace

CesiumAsync::Future<LoadedRasterOverlayImage>
QuadtreeRasterOverlayTileProvider::loadTileImage(
    RasterOverlayTile& overlayTile) {
  // Figure out which quadtree level we need, and which tiles from that level.
  // Load each needed tile (or pull it from cache).
  std::vector<CesiumAsync::SharedFuture<ResultPointer<LoadedQuadtreeImage>>>
      tiles = this->mapRasterTilesToGeometryTile(
          overlayTile.getRectangle(),
          overlayTile.getTargetScreenPixels());

  return this->getAsyncSystem()
      .all(std::move(tiles))
      .thenInWorkerThread(
          [projection = this->getProjection(),
           rectangle = overlayTile.getRectangle()](
              std::vector<ResultPointer<LoadedQuadtreeImage>>&& images) {
            // This set of images is only "useful" if at least one actually has
            // image data, and that image data is _not_ from an ancestor. We can
            // identify ancestor images because they have a `subset`.
            const bool haveAnyUsefulImageData = std::any_of(
                images.begin(),
                images.end(),
                [](const ResultPointer<LoadedQuadtreeImage>& image) {
                  return image.pValue && image.pValue->pLoaded->pImage &&
                         !image.pValue->subset.has_value();
                });

            if (!haveAnyUsefulImageData) {
              // For non-useful sets of images, just return an empty image,
              // signalling that the parent tile should be used instead.
              // See https://github.com/CesiumGS/cesium-native/issues/316 for an
              // edge case that is not yet handled. Be sure to pass through any
              // errors and warnings.
              ErrorList errors;
              for (ResultPointer<LoadedQuadtreeImage>& image : images) {
                if (image.pValue->pLoaded) {
                  errors.merge(image.pValue->pLoaded->errorList);
                }
                errors.merge(image.errors);
              }
              return LoadedRasterOverlayImage{
                  new ImageAsset(),
                  Rectangle(),
                  {},
                  std::move(errors),
                  false};
            }

            return QuadtreeRasterOverlayTileProvider::combineImages(
                rectangle,
                projection,
                std::move(images));
          });
}

/*static*/ QuadtreeRasterOverlayTileProvider::CombinedImageMeasurements
QuadtreeRasterOverlayTileProvider::measureCombinedImage(
    const Rectangle& targetRectangle,
    const std::vector<ResultPointer<LoadedQuadtreeImage>>& images) {
  // Find the image with the densest pixels, and use that to select the
  // resolution of the target image.

  // In a quadtree, all tiles within a single zoom level should have pixels
  // with the same projected dimensions. However, some of our images may be
  // from different levels. For example, if a child tile from a particular
  // zoom level is not available, an ancestor tile with a lower resolution
  // (larger pixel size) may be used instead. These ancestor tiles should
  // have a pixel spacing that is an even multiple of the finest tiles.
  double projectedWidthPerPixel = std::numeric_limits<double>::max();
  double projectedHeightPerPixel = std::numeric_limits<double>::max();
  int32_t channels = -1;
  int32_t bytesPerChannel = -1;
  for (const ResultPointer<LoadedQuadtreeImage>& image : images) {
    if (!image.pValue) {
      continue;
    }
    const LoadedRasterOverlayImage& loaded = *image.pValue->pLoaded;
    if (!loaded.pImage || loaded.pImage->width <= 0 ||
        loaded.pImage->height <= 0) {
      continue;
    }

    projectedWidthPerPixel = glm::min(
        projectedWidthPerPixel,
        loaded.rectangle.computeWidth() / loaded.pImage->width);
    projectedHeightPerPixel = glm::min(
        projectedHeightPerPixel,
        loaded.rectangle.computeHeight() / loaded.pImage->height);

    channels = glm::max(channels, loaded.pImage->channels);
    bytesPerChannel = glm::max(bytesPerChannel, loaded.pImage->bytesPerChannel);
  }

  std::optional<Rectangle> combinedRectangle;

  for (const ResultPointer<LoadedQuadtreeImage>& image : images) {
    if (!image.pValue) {
      continue;
    }
    const LoadedRasterOverlayImage& loaded = *image.pValue->pLoaded;
    if (!loaded.pImage || loaded.pImage->width <= 0 ||
        loaded.pImage->height <= 0) {
      continue;
    }

    // The portion of the source that we actually need to copy.
    const Rectangle sourceSubset =
        image.pValue->subset.value_or(loaded.rectangle);

    // Find the bounds of the combined image.
    // Intersect the loaded image's rectangle with the target rectangle.
    std::optional<Rectangle> maybeIntersection =
        targetRectangle.computeIntersection(sourceSubset);
    if (!maybeIntersection) {
      // We really shouldn't have an image that doesn't overlap the target.
      CESIUM_ASSERT(false);
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

    // We always need at least a 1x1 image, even if the target uses a tiny
    // fraction of that pixel. e.g. if a level zero quadtree tile is mapped
    // to a very tiny geometry tile.
    if (intersection.minimumX == intersection.maximumX) {
      intersection.maximumX += projectedWidthPerPixel;
    }
    if (intersection.minimumY == intersection.maximumY) {
      intersection.maximumY += projectedHeightPerPixel;
    }

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
  const int32_t combinedWidthPixels = static_cast<int32_t>(Math::roundUp(
      combinedRectangle->computeWidth() / projectedWidthPerPixel,
      pixelTolerance));
  const int32_t combinedHeightPixels = static_cast<int32_t>(Math::roundUp(
      combinedRectangle->computeHeight() / projectedHeightPerPixel,
      pixelTolerance));

  return CombinedImageMeasurements{
      *combinedRectangle,
      combinedWidthPixels,
      combinedHeightPixels,
      channels,
      bytesPerChannel};
}

/*static*/ LoadedRasterOverlayImage
QuadtreeRasterOverlayTileProvider::combineImages(
    const Rectangle& targetRectangle,
    const Projection& /* projection */,
    std::vector<ResultPointer<LoadedQuadtreeImage>>&& images) {
  ErrorList errors;
  for (ResultPointer<LoadedQuadtreeImage>& image : images) {
    if (image.pValue && image.pValue->pLoaded) {
      errors.merge(std::move(image.pValue->pLoaded->errorList));
    }
    errors.merge(image.errors);
  }

  const CombinedImageMeasurements measurements =
      QuadtreeRasterOverlayTileProvider::measureCombinedImage(
          targetRectangle,
          images);

  const int32_t targetImageBytes =
      measurements.widthPixels * measurements.heightPixels *
      measurements.channels * measurements.bytesPerChannel;
  if (targetImageBytes <= 0) {
    // Target image has no pixels, so our work here is done.
    return LoadedRasterOverlayImage{
        nullptr,
        targetRectangle,
        {},
        std::move(errors),
        true // TODO
    };
  }

  LoadedRasterOverlayImage result;
  result.rectangle = measurements.rectangle;
  result.moreDetailAvailable = false;
  result.errorList = std::move(errors);

  ImageAsset& target = result.pImage.emplace();
  target.bytesPerChannel = measurements.bytesPerChannel;
  target.channels = measurements.channels;
  target.width = measurements.widthPixels;
  target.height = measurements.heightPixels;
  target.pixelData.resize(size_t(
      target.width * target.height * target.channels * target.bytesPerChannel));

  for (const auto& image : images) {
    if (!image.pValue) {
      continue;
    }
    const LoadedRasterOverlayImage& loaded = *image.pValue->pLoaded;
    if (!loaded.pImage) {
      continue;
    }

    // Tiles with a subset inherently have no more detail available (otherwise
    // we wouldn't need the subset).
    if (!image.pValue->subset) {
      result.moreDetailAvailable |= loaded.moreDetailAvailable;
    }

    blitImage(
        target,
        result.rectangle,
        *loaded.pImage,
        loaded.rectangle,
        image.pValue->subset);
  }

  size_t combinedCreditsCount = 0;
  for (const auto& image : images) {
    if (!image.pValue) {
      continue;
    }
    const LoadedRasterOverlayImage& loaded = *image.pValue->pLoaded;
    if (!loaded.pImage) {
      continue;
    }

    combinedCreditsCount += loaded.credits.size();
  }

  result.credits.reserve(combinedCreditsCount);
  for (const auto& image : images) {
    if (!image.pValue) {
      continue;
    }
    const LoadedRasterOverlayImage& loaded = *image.pValue->pLoaded;
    if (!loaded.pImage) {
      continue;
    }

    for (const Credit& credit : loaded.credits) {
      result.credits.push_back(credit);
    }
  }

  // Highlight the edges in yellow to show tile boundaries.
#if SHOW_TILE_BOUNDARIES
  std::span<uint32_t> pixels =
      reintepretCastSpan<uint32_t, std::byte>(result.pImage->pixelData);
  for (int32_t j = 0; j < result.pImage->height; ++j) {
    for (int32_t i = 0; i < result.pImage->width; ++i) {
      if (i == 0 || j == 0 || i == result.pImage->width - 1 ||
          j == result.pImage->height - 1) {
        pixels[j * result.pImage->width + i] = 0xFF00FFFF;
      }
    }
  }
#endif

  return result;
}

} // namespace CesiumRasterOverlays
