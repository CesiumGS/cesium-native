#pragma once

#include "RasterOverlayUpsampler.h"

#include <Cesium3DTilesSelection/RasterOverlayCollection.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/IAssetAccessor.h>

#include <vector>

namespace Cesium3DTilesSelection {
class TilesetContentManager {
public:
  TilesetContentManager(
      const TilesetExternals& externals,
      std::vector<CesiumAsync::IAssetAccessor::THeader>&& requestHeaders,
      std::unique_ptr<TilesetContentLoader>&& pLoader,
      RasterOverlayCollection& overlayCollection);

  ~TilesetContentManager() noexcept;

  void loadTileContent(Tile& tile, const TilesetOptions& tilesetOptions);

  void updateTileContent(Tile& tile, const TilesetOptions& tilesetOptions);

  bool unloadTileContent(Tile& tile);

  void waitIdle();

  const std::vector<CesiumAsync::IAssetAccessor::THeader>&
  getRequestHeaders() const noexcept;

  std::vector<CesiumAsync::IAssetAccessor::THeader>&
  getRequestHeaders() noexcept;

  int32_t getNumOfTilesLoading() const noexcept;

  int64_t getTotalDataUsed() const noexcept;

  bool doesTileNeedLoading(const Tile& tile) const noexcept;

private:
  static void setTileContent(
      Tile& tile,
      TileLoadResult&& result,
      void* pWorkerRenderResources);

  void
  updateContentLoadedState(Tile& tile, const TilesetOptions& tilesetOptions);

  void updateDoneState(Tile& tile, const TilesetOptions& tilesetOptions);

  void unloadContentLoadedState(Tile& tile);

  void unloadDoneState(Tile& tile);

  void notifyTileStartLoading(Tile& tile) noexcept;

  void notifyTileDoneLoading(Tile& tile) noexcept;

  void notifyTileUnloading(Tile& tile) noexcept;

  TilesetExternals _externals;
  std::vector<CesiumAsync::IAssetAccessor::THeader> _requestHeaders;
  std::unique_ptr<TilesetContentLoader> _pLoader;
  RasterOverlayUpsampler _upsampler;
  RasterOverlayCollection* _pOverlayCollection;
  int32_t _tilesLoadOnProgress;
  int64_t _tilesDataUsed;
};
} // namespace Cesium3DTilesSelection
