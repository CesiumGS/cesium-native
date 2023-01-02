#pragma once

#include "RasterOverlayUpsampler.h"
#include "TilesetContentLoaderResult.h"

#include <Cesium3DTilesSelection/CreditSystem.h>
#include <Cesium3DTilesSelection/RasterOverlayCollection.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetLoadFailureDetails.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumUtility/ReferenceCountedNonThreadSafe.h>

#include <vector>

namespace Cesium3DTilesSelection {
class TilesetContentManager
    : public CesiumUtility::ReferenceCountedNonThreadSafe<
          TilesetContentManager> {
public:
  TilesetContentManager(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      RasterOverlayCollection&& overlayCollection,
      std::vector<CesiumAsync::IAssetAccessor::THeader>&& requestHeaders,
      std::unique_ptr<TilesetContentLoader>&& pLoader,
      std::unique_ptr<Tile>&& pRootTile);

  TilesetContentManager(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      RasterOverlayCollection&& overlayCollection,
      const std::string& url);

  TilesetContentManager(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      RasterOverlayCollection&& overlayCollection,
      int64_t ionAssetID,
      const std::string& ionAccessToken,
      const std::string& ionAssetEndpointUrl = "https://api.cesium.com/");

  /**
   * @brief A future that resolves after all async operations initiated by this
   * content manager have completed and all tiles are unloaded, but before the
   * content manager itself is destroyed.
   */
  CesiumAsync::SharedFuture<void>& getAsyncDestructionCompleteEvent();

  ~TilesetContentManager() noexcept;

  void loadTileContent(Tile& tile, const TilesetOptions& tilesetOptions);

  void updateTileContent(
      Tile& tile,
      double priority,
      const TilesetOptions& tilesetOptions);

  bool unloadTileContent(Tile& tile);

  void waitUntilIdle();

  /**
   * @brief Unload every tile that is safe to unload.
   *
   * Tiles that are currently loading asynchronously will not be unloaded. If
   * {@link isIdle} returns true, all tiles will be unloaded.
   */
  void unloadAll();

  const Tile* getRootTile() const noexcept;

  Tile* getRootTile() noexcept;

  const std::vector<CesiumAsync::IAssetAccessor::THeader>&
  getRequestHeaders() const noexcept;

  std::vector<CesiumAsync::IAssetAccessor::THeader>&
  getRequestHeaders() noexcept;

  const RasterOverlayCollection& getRasterOverlayCollection() const noexcept;

  RasterOverlayCollection& getRasterOverlayCollection() noexcept;

  const Credit* getUserCredit() const noexcept;

  const std::vector<Credit>& getTilesetCredits() const noexcept;

  int32_t getNumberOfTilesLoading() const noexcept;

  int32_t getNumberOfTilesLoaded() const noexcept;

  int64_t getTotalDataUsed() const noexcept;

  bool tileNeedsLoading(const Tile& tile) const noexcept;

  void tickMainThreadLoading(
      double timeBudget,
      const TilesetOptions& tilesetOptions);

private:
  static void setTileContent(
      Tile& tile,
      TileLoadResult&& result,
      void* pWorkerRenderResources);

  void updateContentLoadedState(
      Tile& tile,
      double priority,
      const TilesetOptions& tilesetOptions);

  void updateDoneState(Tile& tile, const TilesetOptions& tilesetOptions);

  void finishLoading(Tile& tile, const TilesetOptions& tilesetOptions);

  void unloadContentLoadedState(Tile& tile);

  void unloadDoneState(Tile& tile);

  void notifyTileStartLoading(const Tile* pTile) noexcept;

  void notifyTileDoneLoading(const Tile* pTile) noexcept;

  void notifyTileUnloading(const Tile* pTile) noexcept;

  template <class TilesetContentLoaderType>
  void propagateTilesetContentLoaderResult(
      TilesetLoadType type,
      const std::function<void(const TilesetLoadFailureDetails&)>&
          loadErrorCallback,
      TilesetContentLoaderResult<TilesetContentLoaderType>&& result);

  TilesetExternals _externals;
  std::vector<CesiumAsync::IAssetAccessor::THeader> _requestHeaders;
  std::unique_ptr<TilesetContentLoader> _pLoader;
  std::unique_ptr<Tile> _pRootTile;
  std::optional<Credit> _userCredit;
  std::vector<Credit> _tilesetCredits;
  RasterOverlayUpsampler _upsampler;
  RasterOverlayCollection _overlayCollection;
  int32_t _tilesLoadOnProgress;
  int32_t _loadedTilesCount;
  int64_t _tilesDataUsed;

  struct MainThreadLoadTask {
    Tile* pTile;

    /**
     * @brief The relative priority of loading for this tile.
     *
     * Lower priority values load sooner.
     */
    double priority;

    bool operator<(const MainThreadLoadTask& rhs) const noexcept {
      return this->priority < rhs.priority;
    }
  };

  std::vector<MainThreadLoadTask> _finishLoadingQueue;

  CesiumAsync::Promise<void> _destructionCompletePromise;
  CesiumAsync::SharedFuture<void> _destructionCompleteFuture;
};
} // namespace Cesium3DTilesSelection
