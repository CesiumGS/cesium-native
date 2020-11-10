#pragma once

#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterOverlay.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include <vector>
#include <memory>
#include <gsl/span>

namespace Cesium3DTiles {

    class Tileset;

    class CESIUM3DTILES_API RasterOverlayCollection {
    public:
        RasterOverlayCollection(Tileset& tileset);

        void add(std::unique_ptr<RasterOverlay>&& pOverlay);
        void remove(RasterOverlay* pOverlay);

        typedef std::vector<std::unique_ptr<RasterOverlay>>::const_iterator const_iterator;
        const_iterator begin() const { return this->_overlays.begin(); }
        const_iterator end() const { return this->_overlays.end(); }

        gsl::span<RasterOverlayTileProvider*> getTileProviders() { return this->_quickTileProviders; }

        RasterOverlayTileProvider* findProviderForPlaceholder(RasterOverlayTileProvider* pPlaceholder);

    private:
        void overlayCreated(std::unique_ptr<RasterOverlayTileProvider>&& pOverlay);

        Tileset* _pTileset;
        std::vector<std::unique_ptr<RasterOverlay>> _overlays;
        std::vector<std::unique_ptr<RasterOverlayTileProvider>> _placeholders;
        std::vector<std::unique_ptr<RasterOverlayTileProvider>> _tileProviders;
        std::vector<RasterOverlayTileProvider*> _quickTileProviders;
    };

}
