#include <Cesium3DTilesSelection/TilesetGlobalCache.h>

namespace Cesium3DTilesSelection {
TilesetGlobalCache::TilesetGlobalCache()
  : _maxSize{1} 
{}

void TilesetGlobalCache::setCacheSize(size_t size) noexcept { _maxSize = size; }

void TilesetGlobalCache::markTilesetRecentlyVisible(Tileset& tileset) {
  _visibleList.insertAtTail(tileset);
}

void TilesetGlobalCache::removeTileset(Tileset& tileset) {
  _visibleList.remove(tileset);
}

void TilesetGlobalCache::updateCache() {
  auto it = _visibleList.tail();
  Tileset* prev = nullptr;
  size_t currentUsage = 0;
  while (it) {
    auto back_it = _visibleList.previous(it);
    size_t tilesetUsage = it->getTotalDataBytes();

    if (currentUsage + tilesetUsage > _maxSize) {
      purgeCache(_visibleList.head(), prev);
      break;
    } else {
      currentUsage += tilesetUsage;
    }

    prev = it;
    it = back_it;
  }
}

void TilesetGlobalCache::purgeCache(Tileset* begin, Tileset* end) {
  std::vector<int> v;
  v.push_back(1);
  auto it = begin;
  while (it != end &&
         it->getViewUpdateResult().tilesToRenderThisFrame.empty()) {

    // maybe use timeslice, so to prevent spike in frame
    it->unloadCachedTiles(0, true);
    auto next = _visibleList.next(it);
    it = next;
  }
}
} // namespace Cesium3DTilesSelection
