#include "Cesium3DTiles/RasterOverlay.h"
#include "Cesium3DTiles/RasterOverlayCollection.h"

namespace Cesium3DTiles {

    RasterOverlay::RasterOverlay() :
        _pPlaceholder(),
        _pTileProvider(),
        _cutouts(),
        _pSelf()
    {
    }

    RasterOverlay::~RasterOverlay() {
    }

    RasterOverlayTileProvider* RasterOverlay::getTileProvider() noexcept {
        return this->_pTileProvider ? this->_pTileProvider.get() : this->_pPlaceholder.get();
    }

    const RasterOverlayTileProvider* RasterOverlay::getTileProvider() const {
        return this->_pTileProvider ? this->_pTileProvider.get() : this->_pPlaceholder.get();
    }

    void RasterOverlay::createTileProvider(const TilesetExternals& externals) {
        if (this->_pPlaceholder) {
            return;
        }

        this->_pPlaceholder = std::make_unique<RasterOverlayTileProvider>(
            *this,
            externals
        );

        this->createTileProvider(externals, this, [this](std::unique_ptr<RasterOverlayTileProvider>&& pTileProvider) {
            this->_pTileProvider = std::move(pTileProvider);
        });
    }

    void RasterOverlay::destroySafely(std::unique_ptr<RasterOverlay>&& pOverlay) {
        if (pOverlay) {
            this->_pSelf = std::move(pOverlay);
        } else if (!this->isBeingDestroyed()) {
            // Ownership was not transferred and we're not already being destroyed,
            // so nothing more to do.
            return;
        }

        // Check if it's safe to delete this object yet.
        RasterOverlayTileProvider* pTileProvider = this->getTileProvider();
        if (!pTileProvider || pTileProvider->getNumberOfTilesLoading() == 0) {
            // No tile provider or no tiles loading, so it's safe to delete!
            this->_pSelf.reset();
        }
    }

}
