#pragma once

#include "RasterOverlayUpsampler.h"
#include "TilesetContentLoader.h"

#include <Cesium3DTilesSelection/RasterOverlayCollection.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
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

  void updateTileContent(
      Tile& tile,
      double priority,
      const TilesetOptions& tilesetOptions);

  bool unloadTileContent(Tile& tile);

  const std::vector<CesiumAsync::IAssetAccessor::THeader>&
  getRequestHeaders() const noexcept;

  std::vector<CesiumAsync::IAssetAccessor::THeader>&
  getRequestHeaders() noexcept;

  int32_t getNumOfTilesLoading() const noexcept;

  int64_t getTotalDataUsed() const noexcept;

  bool doesTileNeedLoading(const Tile& tile) const noexcept;

  void tickResourceCreation(double timeBudget);

private:
  static void setTileContent(
      Tile& tile,
      TileLoadResult&& result,
      std::optional<RasterOverlayDetails>&& rasterOverlayDetails,
      void* pWorkerRenderResources);

  void updateContentLoadedState(
      Tile& tile,
      double priority,
      const TilesetOptions& tilesetOptions);

  void updateCreatingResourcesState(Tile& tile, double priority);

  void updateDoneState(Tile& tile, const TilesetOptions& tilesetOptions);

  void createRenderResources(Tile& tile);

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

  struct ResourceCreationTask {
    Tile* pTile;

    /**
     * @brief The relative priority of creating the resources for this tile.
     *
     * Lower priority values load sooner.
     */
    double priority;

    bool operator<(const ResourceCreationTask& rhs) const noexcept {
      return this->priority < rhs.priority;
    }
  };

  std::vector<ResourceCreationTask> _resourceCreationQueue;
};
} // namespace Cesium3DTilesSelection
