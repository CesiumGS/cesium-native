#pragma once

#include <Cesium3DTilesSelection/TilesetContentLoader.h>

namespace Cesium3DTilesSelection {
class RasterOverlayUpsampler : public TilesetContentLoader {
public:
  CesiumAsync::Future<TileLoadResult>
  loadTileContent(const TileLoadInput& loadInput) override;

  void getLoadWork(
      Tile* pTile,
      RequestData& outRequest,
      TileProcessingCallback& outCallback) override;

  TileChildrenResult createTileChildren(const Tile& tile) override;

private:
  static CesiumAsync::Future<TileLoadResult>
  doProcessing(const TileLoadInput& loadInput, TilesetContentLoader* loader);
};
} // namespace Cesium3DTilesSelection
