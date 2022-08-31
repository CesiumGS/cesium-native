#include "Cesium3DTilesSelection/RasterOverlayCollection.h"

#include <CesiumUtility/Tracing.h>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

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

// We use these to avoid a heap allocation just to return empty vectors.
const std::vector<CesiumUtility::IntrusivePointer<RasterOverlay>>
    emptyOverlays{};
const std::vector<CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>>
    emptyTileProviders{};

} // namespace

RasterOverlayCollection::RasterOverlayCollection(
    Tile::LoadedLinkedList& loadedTiles,
    const TilesetExternals& externals) noexcept
    : _pLoadedTiles(&loadedTiles), _externals{externals}, _pOverlays(nullptr) {}

RasterOverlayCollection::~RasterOverlayCollection() noexcept {
  if (this->_pOverlays) {
    OverlayList& list = *this->_pOverlays;
    if (!list.overlays.empty()) {
      for (int64_t i = static_cast<int64_t>(list.overlays.size() - 1); i >= 0;
           --i) {
        this->remove(list.overlays[static_cast<size_t>(i)].get());
      }
    }
  }
}

void RasterOverlayCollection::add(std::unique_ptr<RasterOverlay>&& pOverlay) {
  CESIUM_TRACE_USE_TRACK_SET(this->_loadingSlots);

  if (!this->_pOverlays)
    this->_pOverlays = new OverlayList();

  IntrusivePointer<OverlayList> pList = this->_pOverlays;

  IntrusivePointer<RasterOverlay> pOverlayRaw =
      pList->overlays.emplace_back(pOverlay.release());

  IntrusivePointer<RasterOverlayTileProvider> pPlaceholder =
      pOverlayRaw->createPlaceholder(
          this->_externals.asyncSystem,
          this->_externals.pAssetAccessor);

  pList->tileProviders.emplace_back(pPlaceholder);
  pList->placeholders.emplace_back(pPlaceholder);

  CESIUM_TRACE_BEGIN_IN_TRACK("createTileProvider");

  CesiumAsync::Future<IntrusivePointer<RasterOverlayTileProvider>> future =
      pOverlayRaw->createTileProvider(
          this->_externals.asyncSystem,
          this->_externals.pAssetAccessor,
          this->_externals.pCreditSystem,
          this->_externals.pPrepareRendererResources,
          this->_externals.pLogger,
          nullptr);

  // Add a placeholder for this overlay to existing geometry tiles.
  forEachTile(*this->_pLoadedTiles, [&](Tile& tile) {
    // The tile rectangle and geometric error don't matter for a placeholder.
    if (tile.getState() != TileLoadState::Unloaded &&
        tile.getState() != TileLoadState::Unloading) {
      tile.getMappedRasterTiles().push_back(RasterMappedTo3DTile(
          pPlaceholder->getTile(Rectangle(), glm::dvec2(0.0)),
          -1));
    }
  });

  // This continuation, by capturing pList, keeps the OverlayList from being
  // destroyed. But it does not keep the RasterOverlayCollection itself alive.
  std::move(future)
      .thenInMainThread(
          [pOverlayRaw,
           pList](IntrusivePointer<RasterOverlayTileProvider>&& pProvider) {
            // Find the overlay's current location in the list.
            // It's possible it has been removed completely.
            auto it = std::find(
                pList->overlays.begin(),
                pList->overlays.end(),
                pOverlayRaw);
            if (it != pList->overlays.end()) {
              std::int64_t index = it - pList->overlays.begin();
              pList->tileProviders[index] = pProvider;
            }
            CESIUM_TRACE_END_IN_TRACK("createTileProvider");
          })
      .catchInMainThread(
          [pLogger = this->_externals.pLogger](const std::exception& e) {
            SPDLOG_LOGGER_ERROR(
                pLogger,
                "Error while creating tile provider: {0}",
                e.what());
            CESIUM_TRACE_END_IN_TRACK("createTileProvider");
          });
}

void RasterOverlayCollection::remove(RasterOverlay* pOverlay) noexcept {
  if (!this->_pOverlays)
    return;

  // Remove all mappings of this overlay to geometry tiles.
  auto removeCondition = [pOverlay](
                             const RasterMappedTo3DTile& mapped) noexcept {
    return (
        (mapped.getLoadingTile() &&
         &mapped.getLoadingTile()->getTileProvider().getOwner() == pOverlay) ||
        (mapped.getReadyTile() &&
         &mapped.getReadyTile()->getTileProvider().getOwner() == pOverlay));
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

  OverlayList& list = *this->_pOverlays;

  auto it = std::find_if(
      list.overlays.begin(),
      list.overlays.end(),
      [pOverlay](const IntrusivePointer<RasterOverlay>& pCheck) noexcept {
        return pCheck.get() == pOverlay;
      });
  if (it == list.overlays.end()) {
    return;
  }

  list.overlays.erase(it);
}

const std::vector<CesiumUtility::IntrusivePointer<RasterOverlay>>&
RasterOverlayCollection::getOverlays() const {
  if (!this->_pOverlays)
    return emptyOverlays;

  return this->_pOverlays->overlays;
}

/**
 * @brief Gets the tile providers in this collection. Each tile provider
 * corresponds with the overlay at the same position in the collection
 * returned by {@link getOverlays}.
 */
const std::vector<CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>>&
RasterOverlayCollection::getTileProviders() const {
  if (!this->_pOverlays)
    return emptyTileProviders;

  return this->_pOverlays->tileProviders;
}

RasterOverlayTileProvider* RasterOverlayCollection::findTileProviderForOverlay(
    RasterOverlay& overlay) noexcept {
  // Call the const version
  const RasterOverlayTileProvider* pResult = this->findTileProviderForOverlay(
      const_cast<const RasterOverlay&>(overlay));
  return const_cast<RasterOverlayTileProvider*>(pResult);
}

const RasterOverlayTileProvider*
RasterOverlayCollection::findTileProviderForOverlay(
    const RasterOverlay& overlay) const noexcept {
  if (!this->_pOverlays)
    return nullptr;

  const auto& overlays = this->_pOverlays->overlays;
  const auto& tileProviders = this->_pOverlays->tileProviders;

  assert(overlays.size() == tileProviders.size());

  for (size_t i = 0; i < overlays.size() && i < tileProviders.size(); ++i) {
    if (overlays[i].get() == &overlay)
      return tileProviders[i].get();
  }

  return nullptr;
}

RasterOverlayCollection::const_iterator
RasterOverlayCollection::begin() const noexcept {
  if (!this->_pOverlays)
    return emptyOverlays.begin();

  return this->_pOverlays->overlays.begin();
}

RasterOverlayCollection::const_iterator
RasterOverlayCollection::end() const noexcept {
  if (!this->_pOverlays)
    return emptyOverlays.end();

  return this->_pOverlays->overlays.end();
}

size_t RasterOverlayCollection::size() const noexcept {
  if (!this->_pOverlays)
    return 0;

  return this->_pOverlays->overlays.size();
}

void RasterOverlayCollection::OverlayList::addReference() noexcept {
  ++this->_referenceCount;
}

void RasterOverlayCollection::OverlayList::releaseReference() noexcept {
  assert(this->_referenceCount > 0);
  --this->_referenceCount;
  if (this->_referenceCount == 0) {
    delete this;
  }
}

} // namespace Cesium3DTilesSelection
