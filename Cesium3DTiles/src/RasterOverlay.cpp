#include "Cesium3DTiles/RasterOverlay.h"
#include "Cesium3DTiles/RasterOverlayCollection.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/spdlog-cesium.h"

using namespace CesiumAsync;

namespace {
    class PlaceholderTileProvider : public Cesium3DTiles::RasterOverlayTileProvider {
    public:
        PlaceholderTileProvider(
            Cesium3DTiles::RasterOverlay& owner,
            const CesiumAsync::AsyncSystem& asyncSystem
        ) noexcept :
            Cesium3DTiles::RasterOverlayTileProvider(owner, asyncSystem)
        {
        }

        virtual CesiumAsync::Future<Cesium3DTiles::LoadedRasterOverlayImage> loadTileImage(const CesiumGeometry::QuadtreeTileID& /* tileID */) const override {
            return this->getAsyncSystem().createResolvedFuture<Cesium3DTiles::LoadedRasterOverlayImage>({});
        }
    };
}

namespace Cesium3DTiles {

    RasterOverlay::RasterOverlay(const RasterOverlayOptions& options) :
        _pPlaceholder(),
        _pTileProvider(),
        _cutouts(),
        _pSelf(),
        _isLoadingTileProvider(false),
        _options(options)
    {
    }

    RasterOverlay::~RasterOverlay() {
    }

    RasterOverlayTileProvider* RasterOverlay::getTileProvider() noexcept {
        return this->_pTileProvider ? this->_pTileProvider.get() : this->_pPlaceholder.get();
    }

    const RasterOverlayTileProvider* RasterOverlay::getTileProvider() const noexcept {
        return this->_pTileProvider ? this->_pTileProvider.get() : this->_pPlaceholder.get();
    }

    void RasterOverlay::createTileProvider(
        const AsyncSystem& asyncSystem,
        const std::shared_ptr<CreditSystem>& pCreditSystem,
        std::shared_ptr<IPrepareRendererResources> pPrepareRendererResources,
        std::shared_ptr<spdlog::logger> pLogger
    ) {
        if (this->_pPlaceholder) {
            return;
        }

        this->_pPlaceholder = std::make_unique<PlaceholderTileProvider>(
            *this,
            asyncSystem
        );

        this->_isLoadingTileProvider = true;

        this->createTileProvider(
            asyncSystem,
            pCreditSystem,
            pPrepareRendererResources,
            pLogger,
            this
        ).thenInMainThread([this](std::unique_ptr<RasterOverlayTileProvider> pProvider) {
            this->_pTileProvider = std::move(pProvider);
            this->_isLoadingTileProvider = false;
        }).catchInMainThread([this, pLogger](const std::exception& e) {
            SPDLOG_LOGGER_ERROR(pLogger, "Exception while creating tile provider: {0}", e.what());
            this->_pTileProvider.reset();
            this->_isLoadingTileProvider = false;
        });
    }

    void RasterOverlay::destroySafely(std::unique_ptr<RasterOverlay>&& pOverlay) noexcept {
        if (pOverlay) {
            this->_pSelf = std::move(pOverlay);
        } else if (!this->isBeingDestroyed()) {
            // Ownership was not transferred and we're not already being destroyed,
            // so nothing more to do.
            return;
        }

        // Check if it's safe to delete this object yet.
        if (this->_isLoadingTileProvider) {
            // Loading, so it's not safe to unload yet.
            return;
        }

        RasterOverlayTileProvider* pTileProvider = this->getTileProvider();
        if (!pTileProvider || pTileProvider->getNumberOfTilesLoading() == 0) {
            // No tile provider or no tiles loading, so it's safe to delete!
            this->_pSelf.reset();
        }
    }

}
