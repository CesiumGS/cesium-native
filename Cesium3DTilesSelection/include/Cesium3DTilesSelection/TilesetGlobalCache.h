#pragma once

#include <Cesium3DTilesSelection/Tileset.h>
#include <CesiumUtility/DoublyLinkedList.h>

namespace Cesium3DTilesSelection {
class Tileset;

class TilesetGlobalCache {
public:
  TilesetGlobalCache();

  void setCacheSize(size_t size) noexcept;

  void markTilesetRecentlyVisible(Tileset& tileset);

  void removeTileset(Tileset& tileset);

  void updateCache();

private:
  void purgeCache(Tileset* begin, Tileset* end);

  size_t _maxSize;
  Tileset::VisibleTilesetList _visibleList;
};
} // namespace Cesium3DTilesSelection
