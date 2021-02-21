#include "Cesium3DTiles/RasterOverlayCollection.h"
#include "Cesium3DTiles/Tileset.h"

namespace Cesium3DTiles {

    RasterOverlayCollection::RasterOverlayCollection(Tileset& tileset) noexcept :
        _pTileset(&tileset),
        _overlays()
    {
    }

    RasterOverlayCollection::~RasterOverlayCollection() {
        if (this->_overlays.size() > 0) {
            for (int64_t i = static_cast<int64_t>(this->_overlays.size() - 1); i >= 0; --i) {
                this->remove(this->_overlays[static_cast<size_t>(i)].get());
            }
        }
    }

    void RasterOverlayCollection::add(std::unique_ptr<RasterOverlay>&& pOverlay) {
        RasterOverlay* pOverlayRaw = pOverlay.get();
        this->_overlays.push_back(std::move(pOverlay));
        pOverlayRaw->createTileProvider(
            this->_pTileset->getAsyncSystem(),
            this->_pTileset->getExternals().pCreditSystem,
            this->_pTileset->getExternals().pPrepareRendererResources,
            this->_pTileset->getExternals().pLogger
        );

        // Add this overlay to existing geometry tiles.
        this->_pTileset->forEachLoadedTile([pOverlayRaw](Tile& tile) {
            // The tile rectangle doesn't matter for a placeholder.
            pOverlayRaw->getPlaceholder()->mapRasterTilesToGeometryTile(
                CesiumGeospatial::GlobeRectangle(0.0, 0.0, 0.0, 0.0),
                tile.getGeometricError(),
                tile.getMappedRasterTiles()
            );
        });
    }

    void RasterOverlayCollection::remove(RasterOverlay* pOverlay) noexcept {
        // Remove all mappings of this overlay to geometry tiles.
        auto removeCondition = [pOverlay](RasterMappedTo3DTile& mapped) {
            return (
                (mapped.getLoadingTile() && &mapped.getLoadingTile()->getOverlay() == pOverlay) ||
                (mapped.getReadyTile() && &mapped.getReadyTile()->getOverlay() == pOverlay)
            );
        };

        this->_pTileset->forEachLoadedTile([&removeCondition](Tile& tile) {
            std::vector<RasterMappedTo3DTile>& mapped = tile.getMappedRasterTiles();
            auto firstToRemove = std::remove_if(mapped.begin(), mapped.end(), removeCondition);

            for (auto it = firstToRemove; it != mapped.end(); ++it) {
                it->detachFromTile(tile);
            }

            mapped.erase(firstToRemove, mapped.end());
        });

        auto it = std::find_if(this->_overlays.begin(), this->_overlays.end(), [pOverlay](std::unique_ptr<RasterOverlay>& pCheck) {
            return pCheck.get() == pOverlay;
        });
        if (it == this->_overlays.end()) {
            return;
        }

        // Tell the overlay provider to destroy itself, which effectively transfers ownership
        // _of_ itself, _to_ itself. It will delete itself when all in-progress loads
        // are complete, which may be immediately.
        (*it)->destroySafely(std::move(*it));
        this->_overlays.erase(it);
    }

}
