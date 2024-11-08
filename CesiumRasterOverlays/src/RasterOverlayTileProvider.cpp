#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGltfReader/ImageDecoder.h>
#include <CesiumRasterOverlays/IPrepareRasterOverlayRendererResources.h>
#include <CesiumRasterOverlays/NetworkRasterOverlayImageAssetDescriptor.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/Tracing.h>
#include <CesiumUtility/joinToString.h>

#include <spdlog/fwd.h>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumGltfReader;
using namespace CesiumUtility;

namespace {

Future<ResultPointer<CesiumRasterOverlays::LoadedRasterOverlayImage>>
rasterOverlayImageFactory(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const CesiumRasterOverlays::NetworkRasterOverlayImageAssetDescriptor& key) {
  return key.load(asyncSystem, pAssetAccessor);
}

} // namespace

namespace CesiumRasterOverlays {

RasterOverlayTileProvider::RasterOverlayTileProvider(
    const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept
    : _pOwner(const_intrusive_cast<RasterOverlay>(pOwner)),
      _asyncSystem(asyncSystem),
      _pAssetAccessor(pAssetAccessor),
      _credit(std::nullopt),
      _pPrepareRendererResources(nullptr),
      _pLogger(nullptr),
      _projection(CesiumGeospatial::GeographicProjection(ellipsoid)),
      _coverageRectangle(CesiumGeospatial::GeographicProjection::
                             computeMaximumProjectedRectangle(ellipsoid)),
      _pPlaceholder(),
      _tileDataBytes(0),
      _totalTilesCurrentlyLoading(0),
      _throttledTilesCurrentlyLoading(0) {
  this->_pPlaceholder = new RasterOverlayTile(*this);
  this->_pRasterOverlayImageDepot.emplace(rasterOverlayImageFactory);
}

RasterOverlayTileProvider::RasterOverlayTileProvider(
    const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    std::optional<Credit> credit,
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const CesiumGeospatial::Projection& projection,
    const Rectangle& coverageRectangle) noexcept
    : _pOwner(const_intrusive_cast<RasterOverlay>(pOwner)),
      _asyncSystem(asyncSystem),
      _pAssetAccessor(pAssetAccessor),
      _credit(credit),
      _pPrepareRendererResources(pPrepareRendererResources),
      _pLogger(pLogger),
      _projection(projection),
      _coverageRectangle(coverageRectangle),
      _pPlaceholder(nullptr),
      _tileDataBytes(0),
      _totalTilesCurrentlyLoading(0),
      _throttledTilesCurrentlyLoading(0) {
  this->_pRasterOverlayImageDepot.emplace(rasterOverlayImageFactory);
}

RasterOverlayTileProvider::~RasterOverlayTileProvider() noexcept {
  // Explicitly release the placeholder first, because RasterOverlayTiles must
  // be destroyed before the tile provider that created them.
  if (this->_pPlaceholder) {
    CESIUM_ASSERT(this->_pPlaceholder->getReferenceCount() == 1);
    this->_pPlaceholder = nullptr;
  }
}

CesiumUtility::IntrusivePointer<RasterOverlayTile>
RasterOverlayTileProvider::getTile(
    const CesiumGeometry::Rectangle& rectangle,
    const glm::dvec2& targetScreenPixels) {
  if (this->_pPlaceholder) {
    return this->_pPlaceholder;
  }

  if (!rectangle.overlaps(this->_coverageRectangle)) {
    return nullptr;
  }

  return new RasterOverlayTile(*this, targetScreenPixels, rectangle);
}

void RasterOverlayTileProvider::removeTile(RasterOverlayTile* pTile) noexcept {
  CESIUM_ASSERT(pTile->getReferenceCount() == 0);
  if (pTile->getImage()) {
    this->_tileDataBytes -= pTile->getImage()->sizeBytes;
  }
}

CesiumAsync::Future<TileProviderAndTile>
RasterOverlayTileProvider::loadTile(RasterOverlayTile& tile) {
  if (this->_pPlaceholder) {
    // Refuse to load placeholders.
    return this->getAsyncSystem().createResolvedFuture(
        TileProviderAndTile{this, nullptr});
  }

  return this->doLoad(tile, false);
}

bool RasterOverlayTileProvider::loadTileThrottled(RasterOverlayTile& tile) {
  if (tile.getState() != RasterOverlayTile::LoadState::Unloaded) {
    return true;
  }

  if (this->_throttledTilesCurrentlyLoading >=
      this->getOwner().getOptions().maximumSimultaneousTileLoads) {
    return false;
  }

  this->doLoad(tile, true);
  return true;
}

CesiumAsync::SharedFuture<ResultPointer<LoadedRasterOverlayImage>>
RasterOverlayTileProvider::loadTileImageFromUrl(
    const std::string& url,
    const std::vector<IAssetAccessor::THeader>& headers,
    LoadTileImageFromUrlOptions&& options) const {

  NetworkRasterOverlayImageAssetDescriptor descriptor{url, headers};
  descriptor.loadTileOptions = std::move(options);
  descriptor.ktx2TranscodeTargets =
      this->getOwner().getOptions().ktx2TranscodeTargets;

  return this->_pRasterOverlayImageDepot->getOrCreate(
      this->_asyncSystem,
      this->_pAssetAccessor,
      descriptor);
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
 * @param tileId The {@link TileID} - only used for logging
 * @param pPrepareRendererResources The `IPrepareRasterOverlayRendererResources`
 * @param pLogger The logger
 * @param loadedImage The `LoadedRasterOverlayImage`
 * @param rendererOptions Renderer options
 * @return The `LoadResult`
 */
static LoadResult createLoadResultFromLoadedImage(
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    ResultPointer<LoadedRasterOverlayImage>&& loadedImage,
    const std::any& rendererOptions) {
  if (!loadedImage.pValue || !loadedImage.pValue->pImage) {
    loadedImage.errors.logError(pLogger, "Failed to load image for tile");
    LoadResult result;
    result.state = RasterOverlayTile::LoadState::Failed;
    return result;
  }

  if (loadedImage.errors.hasErrors()) {
    loadedImage.errors.logError(pLogger, "Errors while loading image for tile");
  }

  if (!loadedImage.errors.warnings.empty()) {
    loadedImage.errors.logWarning(
        pLogger,
        "Warnings while loading image for tile");
  }

  CesiumGltf::ImageAsset& image = *loadedImage.pValue->pImage;

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
    result.pImage = loadedImage.pValue->pImage;
    result.rectangle = loadedImage.pValue->rectangle;
    result.credits = std::move(loadedImage.pValue->credits);
    result.pRendererResources = pRendererResources;
    result.moreDetailAvailable = loadedImage.pValue->moreDetailAvailable;
    return result;
  }
  LoadResult result;
  result.pRendererResources = nullptr;
  result.state = RasterOverlayTile::LoadState::Failed;
  result.moreDetailAvailable = false;
  return result;
}

} // namespace

CesiumAsync::Future<TileProviderAndTile> RasterOverlayTileProvider::doLoad(
    RasterOverlayTile& tile,
    bool isThrottledLoad) {
  if (tile.getState() != RasterOverlayTile::LoadState::Unloaded) {
    // Already loading or loaded, do nothing.
    return this->getAsyncSystem().createResolvedFuture(
        TileProviderAndTile{this, nullptr});
  }

  // CESIUM_TRACE_USE_TRACK_SET(this->_loadingSlots);

  // Don't let this tile be destroyed while it's loading.
  tile.setState(RasterOverlayTile::LoadState::Loading);

  this->beginTileLoad(isThrottledLoad);

  // Keep the tile and tile provider alive while the async operation is in
  // progress.
  IntrusivePointer<RasterOverlayTile> pTile = &tile;
  IntrusivePointer<RasterOverlayTileProvider> thiz = this;

  return this->loadTileImage(tile)
      .thenInWorkerThread(
          [pPrepareRendererResources = this->getPrepareRendererResources(),
           pLogger = this->getLogger(),
           rendererOptions = this->_pOwner->getOptions().rendererOptions](
              ResultPointer<LoadedRasterOverlayImage>&& loadedImage) {
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

            return TileProviderAndTile{thiz, pTile};
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

            return TileProviderAndTile{thiz, pTile};
          });
}

void RasterOverlayTileProvider::beginTileLoad(bool isThrottledLoad) noexcept {
  ++this->_totalTilesCurrentlyLoading;
  if (isThrottledLoad) {
    ++this->_throttledTilesCurrentlyLoading;
  }
}

void RasterOverlayTileProvider::finalizeTileLoad(
    bool isThrottledLoad) noexcept {
  --this->_totalTilesCurrentlyLoading;
  if (isThrottledLoad) {
    --this->_throttledTilesCurrentlyLoading;
  }
}

TileProviderAndTile::~TileProviderAndTile() noexcept {
  // Ensure the tile is released before the tile provider.
  pTile = nullptr;
  pTileProvider = nullptr;
}

} // namespace CesiumRasterOverlays
