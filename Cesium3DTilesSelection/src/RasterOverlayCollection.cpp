#include "Cesium3DTilesSelection/RasterOverlayCollection.h"

#include <CesiumUtility/Tracing.h>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace Cesium3DTilesSelection {
namespace {
template <class Function>
void forEachTile(Tile::LoadedLinkedList& list, Function callback) {
  Tile* pCurrent = list.head();
  while (pCurrent) {
    Tile* pNext = list.next(pCurrent);
    callback(*pCurrent);
    pCurrent = pNext;
  }
}
} // namespace

RasterOverlayCollection::RasterOverlayCollection(
    Tile::LoadedLinkedList& loadedTiles,
    const TilesetExternals& externals) noexcept
    : _pLoadedTiles(&loadedTiles), _externals{externals} {}

RasterOverlayCollection::~RasterOverlayCollection() noexcept {
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
      this->_externals.asyncSystem,
      this->_externals.pAssetAccessor,
      this->_externals.pCreditSystem,
      this->_externals.pPrepareRendererResources,
      this->_externals.pLogger);

  // Add this overlay to existing geometry tiles.
  forEachTile(*this->_pLoadedTiles, [pOverlayRaw](Tile& tile) {
    // The tile rectangle and geometric error don't matter for a placeholder.
    if (tile.getState() != TileLoadState::Unloaded) {
      tile.getMappedRasterTiles().push_back(RasterMappedTo3DTile(
          pOverlayRaw->getPlaceholder()->getTile(Rectangle(), glm::dvec2(0.0)),
          -1));
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

  auto pPrepareRenderResources =
      this->_externals.pPrepareRendererResources.get();
  forEachTile(
      *this->_pLoadedTiles,
      [&removeCondition, pPrepareRenderResources](Tile& tile) {
        std::vector<RasterMappedTo3DTile>& mapped = tile.getMappedRasterTiles();

        for (RasterMappedTo3DTile& rasterTile : mapped) {
          if (removeCondition(rasterTile)) {
            rasterTile.detachFromTile(*pPrepareRenderResources, tile);
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
