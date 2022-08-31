#include "Cesium3DTilesSelection/RasterOverlay.h"

#include "Cesium3DTilesSelection/RasterOverlayCollection.h"
#include "Cesium3DTilesSelection/RasterOverlayLoadFailureDetails.h"
#include "Cesium3DTilesSelection/RasterOverlayTileProvider.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"

using namespace CesiumAsync;
using namespace Cesium3DTilesSelection;
using namespace CesiumUtility;

namespace {
class PlaceholderTileProvider : public RasterOverlayTileProvider {
public:
  PlaceholderTileProvider(
      const IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor) noexcept
      : RasterOverlayTileProvider(pOwner, asyncSystem, pAssetAccessor) {}

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
    : _name(name), _options(options) {}

RasterOverlay::~RasterOverlay() noexcept {}

CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>
RasterOverlay::createPlaceholder(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor) const {
  return new PlaceholderTileProvider(this, asyncSystem, pAssetAccessor);
}

void RasterOverlay::reportError(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<spdlog::logger>& pLogger,
    RasterOverlayLoadFailureDetails&& errorDetails) const {
  SPDLOG_LOGGER_ERROR(pLogger, errorDetails.message);
  if (this->getOptions().loadErrorCallback) {
    asyncSystem.runInMainThread(
        [this, errorDetails = std::move(errorDetails)]() mutable {
          this->getOptions().loadErrorCallback(std::move(errorDetails));
        });
  }
}
