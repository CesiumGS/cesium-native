#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGltfReader/ImageDecoder.h>
#include <CesiumRasterOverlays/IPrepareRasterOverlayRendererResources.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayExternals.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Tracing.h>

#include <fmt/format.h>
#include <glm/ext/vector_double2.hpp>
#include <spdlog/fwd.h>

#include <any>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumGltfReader;
using namespace CesiumUtility;

namespace CesiumRasterOverlays {

RasterOverlayTileProvider::RasterOverlayTileProvider(
    const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner,
    const RasterOverlayExternals& externals,
    const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept
    : _pOwner(const_intrusive_cast<RasterOverlay>(pOwner)),
      _externals(externals),
      _credit(std::nullopt),
      _projection(CesiumGeospatial::GeographicProjection(ellipsoid)),
      _coverageRectangle(CesiumGeospatial::GeographicProjection::
                             computeMaximumProjectedRectangle(ellipsoid)),
      _pPlaceholder(new RasterOverlayTile(*this)),
      _tileDataBytes(0),
      _totalTilesCurrentlyLoading(0),
      _throttledTilesCurrentlyLoading(0),
      _destructionCompleteDetails() {}

RasterOverlayTileProvider::RasterOverlayTileProvider(
    const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner,
    const RasterOverlayExternals& externals,
    std::optional<CesiumUtility::Credit> credit,
    const CesiumGeospatial::Projection& projection,
    const CesiumGeometry::Rectangle& coverageRectangle) noexcept
    : _pOwner(const_intrusive_cast<RasterOverlay>(pOwner)),
      _externals(externals),
      _credit(credit),
      _projection(projection),
      _coverageRectangle(coverageRectangle),
      _pPlaceholder(nullptr),
      _tileDataBytes(0),
      _totalTilesCurrentlyLoading(0),
      _throttledTilesCurrentlyLoading(0),
      _destructionCompleteDetails() {}

RasterOverlayTileProvider::RasterOverlayTileProvider(
    const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
    const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept
    : RasterOverlayTileProvider(
          pOwner,
          RasterOverlayExternals{
              .pAssetAccessor = pAssetAccessor,
              .pPrepareRendererResources = nullptr,
              .asyncSystem = asyncSystem,
              .pCreditSystem = pCreditSystem,
              .pLogger = nullptr},
          ellipsoid) {}

RasterOverlayTileProvider::RasterOverlayTileProvider(
    const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
    std::optional<Credit> credit,
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const CesiumGeospatial::Projection& projection,
    const Rectangle& coverageRectangle) noexcept
    : RasterOverlayTileProvider(
          pOwner,
          RasterOverlayExternals{
              .pAssetAccessor = pAssetAccessor,
              .pPrepareRendererResources = pPrepareRendererResources,
              .asyncSystem = asyncSystem,
              .pCreditSystem = pCreditSystem,
              .pLogger = pLogger},
          credit,
          projection,
          coverageRectangle) {}

RasterOverlayTileProvider::~RasterOverlayTileProvider() noexcept {
  // Explicitly release the placeholder first, because RasterOverlayTiles must
  // be destroyed before the tile provider that created them.
  if (this->_pPlaceholder) {
    CESIUM_ASSERT(this->_pPlaceholder->getReferenceCount() == 1);
    this->_pPlaceholder = nullptr;
  }

  if (this->_destructionCompleteDetails) {
    this->_destructionCompleteDetails->promise.resolve();
  }
}

CesiumAsync::SharedFuture<void>&
RasterOverlayTileProvider::getAsyncDestructionCompleteEvent() {
  if (!this->_destructionCompleteDetails) {
    auto promise = this->_externals.asyncSystem.createPromise<void>();
    auto sharedFuture = promise.getFuture().share();
    this->_destructionCompleteDetails.emplace(DestructionCompleteDetails{
        std::move(promise),
        std::move(sharedFuture)});
  }

  return this->_destructionCompleteDetails->future;
}

RasterOverlay& RasterOverlayTileProvider::getOwner() noexcept {
  return *this->_pOwner;
}

const RasterOverlay& RasterOverlayTileProvider::getOwner() const noexcept {
  return *this->_pOwner;
}

const std::shared_ptr<CesiumAsync::IAssetAccessor>&
RasterOverlayTileProvider::getAssetAccessor() const noexcept {
  return this->_externals.pAssetAccessor;
}

const std::shared_ptr<CesiumUtility::CreditSystem>&
RasterOverlayTileProvider::getCreditSystem() const noexcept {
  return this->_externals.pCreditSystem;
}

const CesiumAsync::AsyncSystem&
RasterOverlayTileProvider::getAsyncSystem() const noexcept {
  return this->_externals.asyncSystem;
}

const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
RasterOverlayTileProvider::getPrepareRendererResources() const noexcept {
  return this->_externals.pPrepareRendererResources;
}

const std::shared_ptr<spdlog::logger>&
RasterOverlayTileProvider::getLogger() const noexcept {
  return this->_externals.pLogger;
}

const CesiumGeospatial::Projection&
RasterOverlayTileProvider::getProjection() const noexcept {
  return this->_projection;
}

const CesiumGeometry::Rectangle&
RasterOverlayTileProvider::getCoverageRectangle() const noexcept {
  return this->_coverageRectangle;
}

int64_t RasterOverlayTileProvider::getTileDataBytes() const noexcept {
  return this->_tileDataBytes;
}

uint32_t RasterOverlayTileProvider::getNumberOfTilesLoading() const noexcept {
  CESIUM_ASSERT(this->_totalTilesCurrentlyLoading > -1);
  return static_cast<uint32_t>(this->_totalTilesCurrentlyLoading);
}

void RasterOverlayTileProvider::removeTile(RasterOverlayTile* pTile) noexcept {
  CESIUM_ASSERT(pTile->getReferenceCount() == 0);
  if (pTile->getImage()) {
    this->_tileDataBytes -= pTile->getImage()->sizeBytes;
  }
}

const std::optional<CesiumUtility::Credit>&
RasterOverlayTileProvider::getCredit() const noexcept {
  return _credit;
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

CesiumAsync::Future<LoadedRasterOverlayImage>
RasterOverlayTileProvider::loadTileImageFromUrl(
    const std::string& url,
    const std::vector<IAssetAccessor::THeader>& headers,
    LoadTileImageFromUrlOptions&& options) const {

  return this->getAssetAccessor()
      ->get(this->getAsyncSystem(), url, headers)
      .thenInWorkerThread(
          [options = std::move(options),
           Ktx2TranscodeTargets =
               this->getOwner().getOptions().ktx2TranscodeTargets](
              std::shared_ptr<IAssetRequest>&& pRequest) mutable {
            CESIUM_TRACE("load image");
            const IAssetResponse* pResponse = pRequest->response();
            if (pResponse == nullptr) {
              ErrorList errors;
              errors.emplaceError(
                  fmt::format("Image request for {} failed.", pRequest->url()));
              return LoadedRasterOverlayImage{
                  nullptr,
                  options.rectangle,
                  std::move(options.credits),
                  std::move(errors),
                  options.moreDetailAvailable};
            }

            if (pResponse->statusCode() != 0 &&
                (pResponse->statusCode() < 200 ||
                 pResponse->statusCode() >= 300)) {
              ErrorList errors;
              errors.emplaceError(fmt::format(
                  "Received response code {} for image {}.",
                  pResponse->statusCode(),
                  pRequest->url()));
              return LoadedRasterOverlayImage{
                  nullptr,
                  options.rectangle,
                  std::move(options.credits),
                  std::move(errors),
                  options.moreDetailAvailable};
            }

            if (pResponse->data().empty()) {
              if (options.allowEmptyImages) {
                return LoadedRasterOverlayImage{
                    new CesiumGltf::ImageAsset(),
                    options.rectangle,
                    std::move(options.credits),
                    {},
                    options.moreDetailAvailable};
              }

              ErrorList errors;
              errors.emplaceError(fmt::format(
                  "Image response for {} is empty.",
                  pRequest->url()));
              return LoadedRasterOverlayImage{
                  nullptr,
                  options.rectangle,
                  std::move(options.credits),
                  std::move(errors),
                  options.moreDetailAvailable};
            }

            const std::span<const std::byte> data = pResponse->data();

            CesiumGltfReader::ImageReaderResult loadedImage =
                ImageDecoder::readImage(data, Ktx2TranscodeTargets);

            if (!loadedImage.errors.empty()) {
              loadedImage.errors.push_back("Image url: " + pRequest->url());
            }
            if (!loadedImage.warnings.empty()) {
              loadedImage.warnings.push_back("Image url: " + pRequest->url());
            }

            return LoadedRasterOverlayImage{
                loadedImage.pImage,
                options.rectangle,
                std::move(options.credits),
                ErrorList{
                    std::move(loadedImage.errors),
                    std::move(loadedImage.warnings)},
                options.moreDetailAvailable};
          });
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
