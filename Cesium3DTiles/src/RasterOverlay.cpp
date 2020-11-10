#include "Cesium3DTiles/RasterOverlay.h"
#include "Cesium3DTiles/RasterOverlayCollection.h"

namespace Cesium3DTiles {

    RasterOverlay::RasterOverlay() :
        _pPlaceholder(),
        _pTileProvider(),
        _cutouts()
    {
    }

    RasterOverlay::~RasterOverlay() {
    }

    RasterOverlayTileProvider* RasterOverlay::getTileProvider() {
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

}
