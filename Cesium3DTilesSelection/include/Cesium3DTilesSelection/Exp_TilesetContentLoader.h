#pragma once

#include <Cesium3DTilesSelection/Exp_TileContent.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>

namespace Cesium3DTilesSelection {
class Tile;

class TilesetContentLoader {
public:
  virtual ~TilesetContentLoader() = default;

  virtual CesiumAsync::Future<void> requestTileContent(
      Tile& tile,
      CesiumAsync::IAssetAccessor& assetAccessor,
      CesiumAsync::AsyncSystem& asyncSystem) = 0;

  virtual TileLoadState processLoadedContentSome(Tile& tile) = 0;

  virtual TileLoadState processLoadedContentTillDone(Tile& tile) = 0;

  virtual bool unloadTileContent(Tile& tile) = 0;
};
}
