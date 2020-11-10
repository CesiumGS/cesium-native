#include "Cesium3DTiles/RasterOverlayCollection.h"
#include "Cesium3DTiles/Tileset.h"

namespace Cesium3DTiles {

    RasterOverlayCollection::RasterOverlayCollection(Tileset& tileset) :
        _pTileset(&tileset),
        _overlays(),
        _placeholders(),
        _tileProviders(),
        _quickTileProviders()
    {
    }

    void RasterOverlayCollection::add(std::unique_ptr<RasterOverlay>&& pOverlay) {
        RasterOverlay* pOverlayRaw = pOverlay.get();

        this->_overlays.push_back(std::move(pOverlay));
        this->_placeholders.push_back(std::make_unique<RasterOverlayTileProvider>(
            *pOverlayRaw,
            this->_pTileset->getExternals()
        ));
        this->_tileProviders.push_back(nullptr);
        this->_quickTileProviders.push_back(this->_placeholders.back().get());

        pOverlayRaw->createTileProvider(this->_pTileset->getExternals());
    }

    void RasterOverlayCollection::remove(RasterOverlay* pOverlay) {
        // Remove all mappings of this overlay to geometry tiles.
        auto removeCondition = [pOverlay](RasterMappedTo3DTile& mapped) {
            return (
                mapped.getLoadingTile() && &mapped.getLoadingTile()->getOverlay() == pOverlay ||
                mapped.getReadyTile() && &mapped.getReadyTile()->getOverlay() == pOverlay
            );
        };

        this->_pTileset->forEachLoadedTile([&removeCondition](Tile& tile) {
            std::vector<RasterMappedTo3DTile>& mapped = tile.getMappedRasterTiles();
            mapped.erase(std::remove_if(mapped.begin(), mapped.end(), removeCondition), mapped.end());
        });

        auto it = std::find_if(this->_tileProviders.begin(), this->_tileProviders.end(), [pOverlay](std::unique_ptr<RasterOverlayTileProvider>& pCheck) {
            return &pCheck->getOwner() == pOverlay;
        });
        if (it == this->_tileProviders.end()) {
            return;
        }

        // Tell the overlay provider to destroy itself, which effectively transfers ownership
        // _of_ itself, _to_ itself. It will delete itself when all in-progress loads
        // are complete, which may be immediately.
        size_t index = it - this->_tileProviders.begin();
        assert(this->_overlays[index].get() == pOverlay);

        (*it)->destroySafely(std::move(*it), std::move(this->_overlays[index]));

        this->_tileProviders.erase(it);
        this->_overlays.erase(this->_overlays.begin() + index);
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
        // TODO: We're assuming that this callback is called in the main thread.
        // This is true in Unreal Engine, but not guaranteed to be true in general!

        if (!pOverlayProvider) {
            return;
        }

        RasterOverlay* pOverlay = &pOverlayProvider->getOwner();

        auto it = std::find_if(this->_placeholders.begin(), this->_placeholders.end(), [pOverlay](const std::unique_ptr<RasterOverlayTileProvider>& pPlaceholder) {
            if (&pPlaceholder->getOwner() == pOverlay) {
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
