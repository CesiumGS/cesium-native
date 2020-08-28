#pragma once

#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterOverlay.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include <vector>
#include <memory>
#include <gsl/span>

namespace Cesium3DTiles {

    class CESIUM3DTILES_API RasterOverlayCollection {
    public:
        RasterOverlayCollection();

        void push_back(std::unique_ptr<RasterOverlay>&& pOverlay);

        void createTileProviders(IAssetAccessor& assetAccessor);

        gsl::span<RasterOverlayTileProvider*> getTileProviders() { return this->_quickTileProviders; }

    private:
        void overlayCreated(std::unique_ptr<RasterOverlayTileProvider>&& pOverlay);

        std::vector<std::unique_ptr<RasterOverlay>> _overlays;
        std::vector<std::unique_ptr<RasterOverlayTileProvider>> _tileProviders;
        std::vector<RasterOverlayTileProvider*> _quickTileProviders;

    };

}
