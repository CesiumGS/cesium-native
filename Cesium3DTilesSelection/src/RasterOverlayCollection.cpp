#include "Cesium3DTilesSelection/RasterOverlayCollection.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "CesiumUtility/Tracing.h"

using namespace CesiumGeometry;

namespace Cesium3DTilesSelection {

RasterOverlayCollection::RasterOverlayCollection(Tileset& tileset) noexcept
    : _pTileset(&tileset) {}

RasterOverlayCollection::~RasterOverlayCollection() {
  if (!this->_overlays.empty()) {
    for (int64_t i = static_cast<int64_t>(this->_overlays.size() - 1); i >= 0;
         --i) {
      this->remove(this->_overlays[static_cast<size_t>(i)].get());
    }
  }
}

void RasterOverlayCollection::add(std::unique_ptr<RasterOverlay>&& pOverlay) {
  CESIUM_TRACE_USE_TRACK_SET(this->_loadingSlots);

  RasterOverlay* pOverlayRaw = pOverlay.get();
  this->_overlays.push_back(std::move(pOverlay));
  pOverlayRaw->loadTileProvider(
      this->_pTileset->getAsyncSystem(),
      this->_pTileset->getExternals().pAssetAccessor,
      this->_pTileset->getExternals().pCreditSystem,
      this->_pTileset->getExternals().pPrepareRendererResources,
      this->_pTileset->getExternals().pLogger);

  // Add this overlay to existing geometry tiles.
  this->_pTileset->forEachLoadedTile([pOverlayRaw](Tile& tile) {
    // The tile rectangle and geometric error don't matter for a placeholder.
    if (tile.getState() != Tile::LoadState::Unloaded) {
      tile.getMappedRasterTiles().push_back(RasterMappedTo3DTile(
          pOverlayRaw->getPlaceholder()->getTile(Rectangle(), 0.0)));
    }
  });
}

void RasterOverlayCollection::remove(RasterOverlay* pOverlay) noexcept {
  // Remove all mappings of this overlay to geometry tiles.
  auto removeCondition =
      [pOverlay](const RasterMappedTo3DTile& mapped) noexcept {
        return (
            (mapped.getLoadingTile() &&
             &mapped.getLoadingTile()->getOverlay() == pOverlay) ||
            (mapped.getReadyTile() &&
             &mapped.getReadyTile()->getOverlay() == pOverlay));
      };

  this->_pTileset->forEachLoadedTile([&removeCondition](Tile& tile) {
    std::vector<RasterMappedTo3DTile>& mapped = tile.getMappedRasterTiles();

    for (RasterMappedTo3DTile& rasterTile : mapped) {
      if (removeCondition(rasterTile)) {
        rasterTile.detachFromTile(tile);
      }
    }

    auto firstToRemove =
        std::remove_if(mapped.begin(), mapped.end(), removeCondition);
    mapped.erase(firstToRemove, mapped.end());
  });

  auto it = std::find_if(
      this->_overlays.begin(),
      this->_overlays.end(),
      [pOverlay](std::unique_ptr<RasterOverlay>& pCheck) noexcept {
        return pCheck.get() == pOverlay;
      });
  if (it == this->_overlays.end()) {
    return;
  }

  // Tell the overlay provider to destroy itself, which effectively transfers
  // ownership _of_ itself, _to_ itself. It will delete itself when all
  // in-progress loads are complete, which may be immediately.
  (*it)->destroySafely(std::move(*it));
  this->_overlays.erase(it);
}

} // namespace Cesium3DTilesSelection
