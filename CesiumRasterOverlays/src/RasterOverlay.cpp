#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>

#include <spdlog/fwd.h>

using namespace CesiumAsync;
using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace {
class PlaceholderTileProvider : public RasterOverlayTileProvider {
public:
  PlaceholderTileProvider(
      const IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor) noexcept
      : RasterOverlayTileProvider(pOwner, asyncSystem, pAssetAccessor) {}

  virtual CesiumAsync::Future<RasterLoadResult>
  loadTileImage(const RasterOverlayTile&, const UrlResponseDataMap&) override {
    return this->getAsyncSystem().createResolvedFuture<RasterLoadResult>({});
  }

  virtual void getLoadTileImageWork(
      const RasterOverlayTile&,
      RequestData&,
      RasterProcessingCallback&) override {
    // There is no actual work to be done
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
    assert(this->_destructionCompleteDetails->asyncSystem == asyncSystem);
  }

  return this->_destructionCompleteDetails->future;
}

CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>
RasterOverlay::createPlaceholder(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor) const {
  return new PlaceholderTileProvider(this, asyncSystem, pAssetAccessor);
}
