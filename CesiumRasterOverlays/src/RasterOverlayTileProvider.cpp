#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGltfReader/ImageDecoder.h>
#include <CesiumRasterOverlays/IPrepareRasterOverlayRendererResources.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayExternals.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/CreditReferencer.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Tracing.h>

#include <fmt/format.h>
#include <spdlog/fwd.h>

#include <cstddef>
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
    const CesiumGeospatial::Projection& projection,
    const CesiumGeometry::Rectangle& coverageRectangle) noexcept
    : _pOwner(const_intrusive_cast<RasterOverlay>(pOwner)),
      _externals(externals),
      _credit(),
      _projection(projection),
      _coverageRectangle(coverageRectangle),
      _destructionCompleteDetails() {}

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
          projection,
          coverageRectangle) {
  this->_credit = credit;
}

RasterOverlayTileProvider::~RasterOverlayTileProvider() noexcept {
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

const RasterOverlayExternals&
RasterOverlayTileProvider::getExternals() const noexcept {
  return this->_externals;
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

const std::optional<CesiumUtility::Credit>&
RasterOverlayTileProvider::getCredit() const noexcept {
  return _credit;
}

void RasterOverlayTileProvider::addCredits(
    CreditReferencer& creditReferencer) noexcept {
  if (this->_credit) {
    creditReferencer.addCreditReference(*this->_credit);
  }
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

} // namespace CesiumRasterOverlays
