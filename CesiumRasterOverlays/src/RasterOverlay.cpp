#include "EmptyRasterOverlayTileProvider.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/SharedFuture.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumRasterOverlays/ActivatedRasterOverlay.h>
#include <CesiumRasterOverlays/CreateRasterOverlayTileProviderOptions.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayExternals.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <fmt/format.h>
#include <nonstd/expected.hpp>
#include <spdlog/spdlog.h>

#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <utility>

using namespace CesiumAsync;
using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace {
class PlaceholderTileProvider : public RasterOverlayTileProvider {
public:
  PlaceholderTileProvider(
      const IntrusivePointer<const RasterOverlay>& pCreator,
      const CreateRasterOverlayTileProviderOptions& options,
      const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept
      : RasterOverlayTileProvider(
            pCreator,
            options,
            CesiumGeospatial::GeographicProjection(ellipsoid),
            CesiumGeometry::Rectangle()) {}

  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(const RasterOverlayTile& /* overlayTile */) override {
    return this->getAsyncSystem()
        .createResolvedFuture<LoadedRasterOverlayImage>({});
  }
};
} // namespace

RasterOverlay::RasterOverlay(
    const std::string& name,
    const RasterOverlayOptions& options)
    : _name(name), _options(options), _destructionCompleteDetails{} {}

RasterOverlay::~RasterOverlay() noexcept {
  if (this->_destructionCompleteDetails.has_value()) {
    this->_destructionCompleteDetails->promise.resolve();
  }
}

CesiumAsync::SharedFuture<void>&
RasterOverlay::getAsyncDestructionCompleteEvent(
    const CesiumAsync::AsyncSystem& asyncSystem) {
  if (!this->_destructionCompleteDetails.has_value()) {
    auto promise = asyncSystem.createPromise<void>();
    auto sharedFuture = promise.getFuture().share();
    this->_destructionCompleteDetails.emplace(DestructionCompleteDetails{
        asyncSystem,
        std::move(promise),
        std::move(sharedFuture)});
  } else {
    // All invocations of getAsyncDestructionCompleteEvent on a particular
    // RasterOverlay must pass equivalent AsyncSystems.
    CESIUM_ASSERT(
        this->_destructionCompleteDetails->asyncSystem == asyncSystem);
  }

  return this->_destructionCompleteDetails->future;
}

CesiumUtility::IntrusivePointer<ActivatedRasterOverlay> RasterOverlay::activate(
    const RasterOverlayExternals& externals,
    const CesiumGeospatial::Ellipsoid& ellipsoid) const {
  IntrusivePointer<ActivatedRasterOverlay> pResult =
      new ActivatedRasterOverlay(externals, this, ellipsoid);

  CreateRasterOverlayTileProviderOptions options{
      .externals = externals,
      .pOwner = nullptr,
      .pCreditSource = nullptr};

  CesiumAsync::Future<RasterOverlay::CreateTileProviderResult> future =
      this->createTileProvider(options);

  // This continuation, by capturing pResult, keeps the instance from being
  // destroyed. But it does not keep the RasterOverlayCollection itself alive.
  std::move(future)
      .catchInMainThread(
          [](const std::exception& e)
              -> RasterOverlay::CreateTileProviderResult {
            return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                RasterOverlayLoadType::Unknown,
                nullptr,
                fmt::format(
                    "Error while creating tile provider: {0}",
                    e.what())});
          })
      .thenInMainThread(
          [pResult, options](RasterOverlay::CreateTileProviderResult&& result) {
            IntrusivePointer<RasterOverlayTileProvider> pProvider = nullptr;
            if (result) {
              pProvider = *result;
            } else {
              // Report error creating the tile provider.
              const RasterOverlayLoadFailureDetails& failureDetails =
                  result.error();
              SPDLOG_LOGGER_ERROR(
                  options.externals.pLogger,
                  failureDetails.message);
              if (pResult->getOverlay().getOptions().loadErrorCallback) {
                pResult->getOverlay().getOptions().loadErrorCallback(
                    failureDetails);
              }

              // Create a tile provider that does not provide any tiles at
              // all.
              pProvider = new EmptyRasterOverlayTileProvider(
                  &pResult->getOverlay(),
                  options);
            }

            pResult->setTileProvider(pProvider);
          });

  return pResult;
}

CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>
RasterOverlay::createPlaceholder(
    const RasterOverlayExternals& externals,
    const CesiumGeospatial::Ellipsoid& ellipsoid) const {
  return new PlaceholderTileProvider(
      this,
      CreateRasterOverlayTileProviderOptions{
          .externals = externals,
          .pOwner = nullptr,
          .pCreditSource = nullptr},
      ellipsoid);
}