#pragma once

#include <Cesium3DTilesSelection/Exp_TileUserDataStorage.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/IAssetAccessor.h>

#include <functional>
#include <vector>

namespace Cesium3DTilesSelection {
class TilesetContentManager {
public:
  TilesetContentManager(
      const TilesetExternals& externals,
      std::vector<CesiumAsync::IAssetAccessor::THeader>&& requestHeaders,
      std::unique_ptr<TilesetContentLoader> pLoader);

  void loadTileContent(Tile& tile, const TilesetContentOptions& contentOptions);

  void updateTileContent(Tile& tile);

  bool unloadTileContent(Tile& tile);

private:
  static void setTileContent(
      TileContent& content,
      TileContentKind&& contentKind,
      std::function<void(Tile&)>&& tileInitializer,
      TileLoadState state,
      void* pRenderResources);

  void updateContentLoadedState(Tile& tile);

  void unloadContentLoadedState(Tile& tile);

  void unloadDoneState(Tile& tile);

  TilesetExternals _externals;
  std::vector<CesiumAsync::IAssetAccessor::THeader> _requestHeaders;
  std::unique_ptr<TilesetContentLoader> _pLoader;
  TileUserDataStorage _customDataStorage;
};
} // namespace Cesium3DTilesSelection
