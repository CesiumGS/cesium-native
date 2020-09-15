#include "Cesium3DTiles/RasterOverlayCollection.h"

namespace Cesium3DTiles {

    RasterOverlayCollection::RasterOverlayCollection() {

    }

    void RasterOverlayCollection::push_back(std::unique_ptr<RasterOverlay>&& pOverlay) {
        this->_overlays.push_back(std::move(pOverlay));
    }

    void RasterOverlayCollection::createTileProviders(TilesetExternals& tilesetExternals) {
        for (std::unique_ptr<RasterOverlay>& pOverlay : this->_overlays) {
            pOverlay->createTileProvider(tilesetExternals, std::bind(&RasterOverlayCollection::overlayCreated, this, std::placeholders::_1));
        }
    }

    void RasterOverlayCollection::overlayCreated(std::unique_ptr<RasterOverlayTileProvider>&& pOverlay) {
        // TODO: preserve order of overlays
        this->_quickTileProviders.push_back(pOverlay.get());
        this->_tileProviders.push_back(std::move(pOverlay));
    }

}
