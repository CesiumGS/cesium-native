#pragma once

#include <Cesium3DTilesSelection/TilesetContentLoader.h>

namespace Cesium3DTilesSelection {
class RasterOverlayUpsampler : public TilesetContentLoader {
public:
  CesiumAsync::Future<TileLoadResult>
  loadTileContent(const TileLoadInput& loadInput) override;

  void getLoadWork(
      const Tile* pTile,
      CesiumAsync::RequestData& outRequest,
      TileLoaderCallback& outCallback) override;

  TileChildrenResult createTileChildren(const Tile& tile) override;
};
} // namespace Cesium3DTilesSelection
