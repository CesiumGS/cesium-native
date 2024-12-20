#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/SharedFuture.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <memory>
#include <string>
#include <utility>

using namespace CesiumAsync;
using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace {
class PlaceholderTileProvider : public RasterOverlayTileProvider {
public:
  PlaceholderTileProvider(
      const IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
      const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept
      : RasterOverlayTileProvider(
            pOwner,
            asyncSystem,
            pAssetAccessor,
            ellipsoid) {}

  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(RasterOverlayTile& /* overlayTile */) override {
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

CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>
RasterOverlay::createPlaceholder(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const CesiumGeospatial::Ellipsoid& ellipsoid) const {
  return new PlaceholderTileProvider(
      this,
      asyncSystem,
      pAssetAccessor,
      ellipsoid);
}
