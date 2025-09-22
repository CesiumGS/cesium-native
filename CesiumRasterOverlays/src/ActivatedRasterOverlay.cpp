#include <CesiumAsync/Future.h>
#include <CesiumAsync/SharedFuture.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumRasterOverlays/ActivatedRasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayExternals.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Tracing.h>

#include <glm/ext/vector_double2.hpp>
#include <spdlog/logger.h>

#include <any>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <utility>
#include <vector>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumUtility;

namespace CesiumRasterOverlays {

ActivatedRasterOverlay::ActivatedRasterOverlay(
    const RasterOverlayExternals& externals,
    const IntrusivePointer<const RasterOverlay>& pOverlay,
    const Ellipsoid& ellipsoid) noexcept
    : _pOverlay(pOverlay),
      _pPlaceholderTileProvider(
          pOverlay->createPlaceholder(externals, ellipsoid)),
      _pPlaceholderTile(new RasterOverlayTile(*this)),
      _pTileProvider(nullptr),
      _tileDataBytes(0),
      _totalTilesCurrentlyLoading(0),
      _throttledTilesCurrentlyLoading(0),
      _readyPromise(externals.asyncSystem.createPromise<void>()),
      _readyEvent(this->_readyPromise.getFuture().share()) {}

ActivatedRasterOverlay::~ActivatedRasterOverlay() noexcept {
  // Explicitly release the placeholder first, because RasterOverlayTiles must
  // be destroyed before the tile provider that created them.
  if (this->_pPlaceholderTile) {
    CESIUM_ASSERT(this->_pPlaceholderTile->getReferenceCount() == 1);
    this->_pPlaceholderTile.reset();
  }
}

CesiumAsync::SharedFuture<void>& ActivatedRasterOverlay::getReadyEvent() {
  return this->_readyEvent;
}

const RasterOverlay& ActivatedRasterOverlay::getOverlay() const noexcept {
  return *this->_pOverlay;
}

const CesiumRasterOverlays::RasterOverlayTileProvider*
ActivatedRasterOverlay::getTileProvider() const noexcept {
  return this->_pTileProvider;
}

CesiumRasterOverlays::RasterOverlayTileProvider*
ActivatedRasterOverlay::getTileProvider() noexcept {
  return this->_pTileProvider;
}

void ActivatedRasterOverlay::setTileProvider(
    const IntrusivePointer<RasterOverlayTileProvider>& pTileProvider) {
  CESIUM_ASSERT(pTileProvider != nullptr);

  bool hadValue = this->_pTileProvider != nullptr;
  this->_pTileProvider = pTileProvider;
  if (!hadValue && this->_pTileProvider != nullptr) {
    this->_readyPromise.resolve();
  }
}

const CesiumRasterOverlays::RasterOverlayTileProvider*
ActivatedRasterOverlay::getPlaceholderTileProvider() const noexcept {
  return this->_pPlaceholderTileProvider;
}

CesiumRasterOverlays::RasterOverlayTileProvider*
ActivatedRasterOverlay::getPlaceholderTileProvider() noexcept {
  return this->_pPlaceholderTileProvider;
}

const CesiumRasterOverlays::RasterOverlayTile*
ActivatedRasterOverlay::getPlaceholderTile() const noexcept {
  return this->_pPlaceholderTile;
}

CesiumRasterOverlays::RasterOverlayTile*
ActivatedRasterOverlay::getPlaceholderTile() noexcept {
  return this->_pPlaceholderTile;
}

IntrusivePointer<RasterOverlayTile> ActivatedRasterOverlay::getTile(
    const CesiumGeometry::Rectangle& rectangle,
    const glm::dvec2& targetScreenPixels) {
  if (this->_pTileProvider == nullptr) {
    return this->_pPlaceholderTile;
  }

  if (!rectangle.overlaps(this->_pTileProvider->getCoverageRectangle())) {
    return nullptr;
  }

  return new RasterOverlayTile(*this, targetScreenPixels, rectangle);
}

int64_t ActivatedRasterOverlay::getTileDataBytes() const noexcept {
  return this->_tileDataBytes;
}

uint32_t ActivatedRasterOverlay::getNumberOfTilesLoading() const noexcept {
  CESIUM_ASSERT(this->_totalTilesCurrentlyLoading > -1);
  return static_cast<uint32_t>(this->_totalTilesCurrentlyLoading);
}

void ActivatedRasterOverlay::removeTile(RasterOverlayTile* pTile) noexcept {
  CESIUM_ASSERT(pTile->getReferenceCount() == 0);
  if (pTile->getImage()) {
    this->_tileDataBytes -= pTile->getImage()->sizeBytes;
  }
}

CesiumAsync::Future<TileProviderAndTile>
ActivatedRasterOverlay::loadTile(RasterOverlayTile& tile) {
  if (!this->_pTileProvider) {
    // Refuse to load if the tile provider isn't ready yet.
    return this->_pPlaceholderTileProvider->getAsyncSystem()
        .createResolvedFuture(
            TileProviderAndTile{this->_pPlaceholderTileProvider, nullptr});
  }

  return this->doLoad(tile, false);
}

bool ActivatedRasterOverlay::loadTileThrottled(RasterOverlayTile& tile) {
  if (tile.getState() != RasterOverlayTile::LoadState::Unloaded) {
    return true;
  }

  if (this->_throttledTilesCurrentlyLoading >=
      this->_pOverlay->getOptions().maximumSimultaneousTileLoads) {
    return false;
  }

  this->doLoad(tile, true);
  return true;
}

namespace {
struct LoadResult {
  RasterOverlayTile::LoadState state = RasterOverlayTile::LoadState::Unloaded;
  CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> pImage = nullptr;
  CesiumGeometry::Rectangle rectangle = {};
  std::vector<Credit> credits = {};
  void* pRendererResources = nullptr;
  bool moreDetailAvailable = true;
};

/**
 * @brief Processes the given `LoadedRasterOverlayImage`, producing a
 * `LoadResult`.
 *
 * This function is intended to be called on the worker thread.
 *
 * If the given `loadedImage` contains no valid image data, then a
 * `LoadResult` with the state `RasterOverlayTile::LoadState::Failed` will be
 * returned.
 *
 * Otherwise, the image data will be passed to
 * `IPrepareRasterOverlayRendererResources::prepareRasterInLoadThread`, and the
 * function will return a `LoadResult` with the image, the prepared renderer
 * resources, and the state `RasterOverlayTile::LoadState::Loaded`.
 *
 * @param tileId The @ref TileID - only used for logging
 * @param pPrepareRendererResources The `IPrepareRasterOverlayRendererResources`
 * @param pLogger The logger
 * @param loadedImage The `LoadedRasterOverlayImage`
 * @param rendererOptions Renderer options
 * @return The `LoadResult`
 */
LoadResult createLoadResultFromLoadedImage(
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    LoadedRasterOverlayImage&& loadedImage,
    const std::any& rendererOptions) {
  if (!loadedImage.pImage) {
    loadedImage.errorList.logError(pLogger, "Failed to load image for tile");
    LoadResult result;
    result.state = RasterOverlayTile::LoadState::Failed;
    return result;
  }

  if (loadedImage.errorList.hasErrors()) {
    loadedImage.errorList.logError(
        pLogger,
        "Errors while loading image for tile");
  }

  if (!loadedImage.errorList.warnings.empty()) {
    loadedImage.errorList.logWarning(
        pLogger,
        "Warnings while loading image for tile");
  }

  CesiumGltf::ImageAsset& image = *loadedImage.pImage;

  const int32_t bytesPerPixel = image.channels * image.bytesPerChannel;
  const int64_t requiredBytes =
      static_cast<int64_t>(image.width) * image.height * bytesPerPixel;
  if (image.width > 0 && image.height > 0 &&
      image.pixelData.size() >= static_cast<size_t>(requiredBytes)) {
    CESIUM_TRACE(
        "Prepare Raster " + std::to_string(image.width) + "x" +
        std::to_string(image.height) + "x" + std::to_string(image.channels) +
        "x" + std::to_string(image.bytesPerChannel));

    void* pRendererResources = nullptr;
    if (pPrepareRendererResources) {
      pRendererResources = pPrepareRendererResources->prepareRasterInLoadThread(
          image,
          rendererOptions);
    }

    LoadResult result;
    result.state = RasterOverlayTile::LoadState::Loaded;
    result.pImage = loadedImage.pImage;
    result.rectangle = loadedImage.rectangle;
    result.credits = std::move(loadedImage.credits);
    result.pRendererResources = pRendererResources;
    result.moreDetailAvailable = loadedImage.moreDetailAvailable;
    return result;
  }

  LoadResult result;
  result.pRendererResources = nullptr;
  result.state = RasterOverlayTile::LoadState::Failed;
  result.moreDetailAvailable = false;
  return result;
}

} // namespace

CesiumAsync::Future<TileProviderAndTile>
ActivatedRasterOverlay::doLoad(RasterOverlayTile& tile, bool isThrottledLoad) {
  if (tile.getState() != RasterOverlayTile::LoadState::Unloaded) {
    // Already loading or loaded, do nothing.
    return this->_pTileProvider->getAsyncSystem().createResolvedFuture(
        TileProviderAndTile{this->_pTileProvider, nullptr});
  }

  // Don't let this tile be destroyed while it's loading.
  tile.setState(RasterOverlayTile::LoadState::Loading);

  this->beginTileLoad(isThrottledLoad);

  // Keep the tile and tile provider alive while the async operation is in
  // progress.
  IntrusivePointer<RasterOverlayTile> pTile = &tile;
  IntrusivePointer<ActivatedRasterOverlay> thiz = this;

  return this->_pTileProvider->loadTileImage(tile)
      .thenInWorkerThread(
          [pPrepareRendererResources =
               this->_pTileProvider->getPrepareRendererResources(),
           pLogger = this->_pTileProvider->getLogger(),
           rendererOptions = this->_pOverlay->getOptions().rendererOptions](
              LoadedRasterOverlayImage&& loadedImage) {
            return createLoadResultFromLoadedImage(
                pPrepareRendererResources,
                pLogger,
                std::move(loadedImage),
                rendererOptions);
          })
      .thenInMainThread(
          [thiz, pTile, isThrottledLoad](LoadResult&& result) noexcept {
            pTile->_rectangle = result.rectangle;
            pTile->_pRendererResources = result.pRendererResources;
            pTile->_pImage = std::move(result.pImage);
            pTile->_tileCredits = std::move(result.credits);
            pTile->_moreDetailAvailable =
                result.moreDetailAvailable
                    ? RasterOverlayTile::MoreDetailAvailable::Yes
                    : RasterOverlayTile::MoreDetailAvailable::No;
            pTile->setState(result.state);

            if (pTile->getImage() != nullptr) {
              ImageAsset& imageCesium = *pTile->getImage();

              // If the image size hasn't been overridden, store the pixelData
              // size now. We'll add this number to our total memory usage now,
              // and remove it when the tile is later unloaded, and we must use
              // the same size in each case.
              if (imageCesium.sizeBytes < 0) {
                imageCesium.sizeBytes = int64_t(imageCesium.pixelData.size());
              }

              thiz->_tileDataBytes += imageCesium.sizeBytes;
            }

            thiz->finalizeTileLoad(isThrottledLoad);

            return TileProviderAndTile{thiz->getTileProvider(), pTile};
          })
      .catchInMainThread(
          [thiz, pTile, isThrottledLoad](const std::exception& /*e*/) {
            pTile->_pRendererResources = nullptr;
            pTile->_pImage = nullptr;
            pTile->_tileCredits = {};
            pTile->_moreDetailAvailable =
                RasterOverlayTile::MoreDetailAvailable::No;
            pTile->setState(RasterOverlayTile::LoadState::Failed);

            thiz->finalizeTileLoad(isThrottledLoad);

            return TileProviderAndTile{thiz->getTileProvider(), pTile};
          });
}

void ActivatedRasterOverlay::beginTileLoad(bool isThrottledLoad) noexcept {
  ++this->_totalTilesCurrentlyLoading;
  if (isThrottledLoad) {
    ++this->_throttledTilesCurrentlyLoading;
  }
}

void ActivatedRasterOverlay::finalizeTileLoad(bool isThrottledLoad) noexcept {
  --this->_totalTilesCurrentlyLoading;
  if (isThrottledLoad) {
    --this->_throttledTilesCurrentlyLoading;
  }
}

TileProviderAndTile::TileProviderAndTile(
    const CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>&
        pTileProvider_,
    const CesiumUtility::IntrusivePointer<RasterOverlayTile>& pTile_) noexcept
    : pTileProvider(pTileProvider_), pTile(pTile_) {}

TileProviderAndTile::~TileProviderAndTile() noexcept {
  // Ensure the tile is released before the tile provider.
  pTile = nullptr;
  pTileProvider = nullptr;
}

TileProviderAndTile::TileProviderAndTile(const TileProviderAndTile&) noexcept =
    default;

TileProviderAndTile&
TileProviderAndTile::operator=(const TileProviderAndTile&) noexcept = default;

TileProviderAndTile::TileProviderAndTile(TileProviderAndTile&&) noexcept =
    default;

TileProviderAndTile&
TileProviderAndTile::operator=(TileProviderAndTile&&) noexcept = default;

} // namespace CesiumRasterOverlays
