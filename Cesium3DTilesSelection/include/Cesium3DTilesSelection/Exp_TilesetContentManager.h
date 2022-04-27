#pragma once

#include <Cesium3DTilesSelection/Exp_TilesetContentLoader.h>
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

  ~TilesetContentManager() noexcept;

  void loadTileContent(Tile& tile, const TilesetContentOptions& contentOptions);

  void updateTileContent(Tile& tile);

  bool unloadTileContent(Tile& tile);

  void updateRequestHeader(const std::string &header, const std::string &headerValue);

  int32_t getNumOfTilesLoading() const noexcept;

  int64_t getSizeOfTilesDataUsed() const noexcept;

private:
  static void setTileContent(
      TileContent& content,
      TileLoadResult&& result,
      void* pWorkerRenderResources);

  void updateContentLoadedState(Tile& tile);

  void unloadContentLoadedState(Tile& tile);

  void unloadDoneState(Tile& tile);

  void notifyTileStartLoading(Tile& tile) noexcept;

  void notifyTileDoneLoading(Tile& tile) noexcept;

  void notifyTileUnloading(Tile& tile) noexcept;

  TilesetExternals _externals;
  std::vector<CesiumAsync::IAssetAccessor::THeader> _requestHeaders;
  std::unique_ptr<TilesetContentLoader> _pLoader;
  int32_t _tilesLoadOnProgress;
  int64_t _tilesDataUsed;
};
} // namespace Cesium3DTilesSelection
