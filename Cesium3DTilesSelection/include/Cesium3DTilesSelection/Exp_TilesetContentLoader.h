#pragma once

#include <Cesium3DTilesSelection/Exp_TileContent.h>
#include <Cesium3DTilesSelection/Exp_TileContentLoadInfo.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/Future.h>

#include <memory>

namespace Cesium3DTilesSelection {
class Tile;

class TilesetContentLoader {
public:
  TilesetContentLoader(const TilesetExternals& externals);

  virtual ~TilesetContentLoader() = default;

  void loadTileContent(Tile& tile, const TilesetContentOptions& contentOptions);

  void updateTileContent(Tile& tile);

  bool unloadTileContent(Tile& tile);

private:
  static void setTileContentState(
      TileContent& content,
      TileContentKind&& contentKind,
      TileLoadState state);

  static void resetTileContent(TileContent& content);

  void updateFailedTemporarilyState(Tile& tile);

  void updateContentLoadedState(Tile& tile);

  void updateDoneState(Tile& tile);

  bool unloadContentLoadedState(Tile& tile);

  bool unloadDoneState(Tile& tile);

  virtual CesiumAsync::Future<TileContentKind>
  doLoadTileContent(const TileContentLoadInfo& loadInfo) = 0;

  virtual void doProcessLoadedContent(Tile& tile) = 0;

  virtual void doUpdateTileContent(Tile& tile) = 0;

  virtual bool doUnloadTileContent(Tile& tile) = 0;

  TilesetExternals _externals;
};
} // namespace Cesium3DTilesSelection
