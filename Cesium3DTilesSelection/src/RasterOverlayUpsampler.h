#pragma once

#include "TilesetContentLoader.h"

namespace Cesium3DTilesSelection {
class RasterOverlayUpsampler : public TilesetContentLoader {
public:
  CesiumAsync::Future<TileLoadResult>
  loadTileContent(const TileLoadInput& loadInput) override;

  bool updateTileContent(Tile& tile) override;
};
} // namespace Cesium3DTilesSelection
