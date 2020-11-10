#include "Cesium3DTiles/RasterOverlayCollection.h"
#include "Cesium3DTiles/Tileset.h"

namespace Cesium3DTiles {

    RasterOverlayCollection::RasterOverlayCollection(Tileset& tileset) :
        _pTileset(&tileset),
        _overlays(),
        _placeholders(),
        _tileProviders(),
        _quickTileProviders(),
        _removedOverlays()
    {
    }

    void RasterOverlayCollection::add(std::unique_ptr<RasterOverlay>&& pOverlay) {
        RasterOverlay* pOverlayRaw = pOverlay.get();

        this->_overlays.push_back(std::move(pOverlay));
        this->_placeholders.push_back(std::make_unique<RasterOverlayTileProvider>(
            pOverlayRaw,
            this->_pTileset->getExternals()
        ));
        this->_tileProviders.push_back(nullptr);
        this->_quickTileProviders.push_back(this->_placeholders.back().get());

        pOverlayRaw->createTileProvider(this->_pTileset->getExternals(), std::bind(&RasterOverlayCollection::overlayCreated, this, std::placeholders::_1));
    }

    void RasterOverlayCollection::remove(RasterOverlay* pOverlay) {
        auto it = std::find_if(this->_overlays.begin(), this->_overlays.end(), [pOverlay](std::unique_ptr<RasterOverlay>& pCheck) {
            return pCheck.get() == pOverlay;
        });
        if (it != this->_overlays.end()) {
            this->_removedOverlays.emplace_back(std::move(*it));
            this->_overlays.erase(it);
        }
    }

    RasterOverlayTileProvider* RasterOverlayCollection::findProviderForPlaceholder(RasterOverlayTileProvider* pPlaceholder) {
        for (size_t i = 0; i < this->_placeholders.size(); ++i) {
            if (this->_placeholders[i].get() == pPlaceholder) {
                if (this->_quickTileProviders[i] != pPlaceholder) {
                    return this->_quickTileProviders[i];
                }
                break;
            }
        }

        return nullptr;
    }

    void RasterOverlayCollection::overlayCreated(std::unique_ptr<RasterOverlayTileProvider>&& pOverlayProvider) {
        if (!pOverlayProvider) {
            return;
        }

        RasterOverlay* pOverlay = pOverlayProvider->getOverlay();

        auto it = std::find_if(this->_placeholders.begin(), this->_placeholders.end(), [pOverlay](const std::unique_ptr<RasterOverlayTileProvider>& pPlaceholder) {
            if (pPlaceholder->getOverlay() == pOverlay) {
                return true;
            }
            return false;
        });

        assert(it != this->_placeholders.end());

        size_t index = it - this->_placeholders.begin();
        this->_quickTileProviders[index] = pOverlayProvider.get();
        this->_tileProviders[index] = std::move(pOverlayProvider);
    }

}
