#pragma once

#include "RasterOverlayUpsampler.h"
#include "TilesetContentLoaderResult.h"

#include <Cesium3DTilesSelection/RasterOverlayCollection.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileWorkManager.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetLoadFailureDetails.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/ReferenceCounted.h>

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

  /**
   * @brief A future that resolves when the details of the root tile of this
   * tileset are available. The root tile's content (e.g., 3D model), however,
   * will not necessarily be loaded yet.
   */
  CesiumAsync::SharedFuture<void>& getRootTileAvailableEvent();

  ~TilesetContentManager() noexcept;

  struct TileWorkChain {
    Tile* pTile;
    CesiumAsync::RequestData requestData;
    TileLoaderCallback tileCallback;

    // Must have a valid tile and some work
    bool isValid() {
      return pTile && (!requestData.url.empty() || tileCallback);
    }
  };

  struct RasterWorkChain {
    RasterMappedTo3DTile* pRasterTile;
    CesiumAsync::RequestData requestData;
    CesiumRasterOverlays::RasterProcessingCallback rasterCallback;

    // Must have a valid tile and some work
    bool isValid() {
      return pRasterTile && (!requestData.url.empty() || rasterCallback);
    }
  };

  struct ParsedTileWork {
    size_t depthIndex = 0;

    TileWorkChain tileWorkChain = {};

    std::vector<RasterWorkChain> rasterWorkChains = {};

    std::vector<CesiumGeospatial::Projection> projections = {};

    bool operator<(const ParsedTileWork& rhs) const noexcept {
      return this->depthIndex > rhs.depthIndex;
    }
  };

  void processLoadRequests(
      std::vector<TileLoadRequest>& requests,
      TilesetOptions& options);

  void parseTileWork(
      Tile* pTile,
      size_t depthIndex,
      double maximumScreenSpaceError,
      std::vector<ParsedTileWork>& outWork);

  void updateTileContent(Tile& tile, const TilesetOptions& tilesetOptions);

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

  const CesiumUtility::Credit* getUserCredit() const noexcept;

  const std::vector<CesiumUtility::Credit>& getTilesetCredits() const noexcept;

  int32_t getNumberOfTilesLoading() const noexcept;

  int32_t getNumberOfTilesLoaded() const noexcept;

  int64_t getTotalDataUsed() const noexcept;

  int32_t getNumberOfRastersLoading() const noexcept;

  int32_t getNumberOfRastersLoaded() const noexcept;

  size_t getActiveWorkCount();

  void getLoadingWorkStats(
      size_t& requestCount,
      size_t& inFlightCount,
      size_t& processingCount,
      size_t& failedCount);

  bool tileNeedsWorkerThreadLoading(const Tile& tile) const noexcept;
  bool tileNeedsMainThreadLoading(const Tile& tile) const noexcept;

  // Transition the tile from the ContentLoaded to the Done state.
  void finishLoading(Tile& tile, const TilesetOptions& tilesetOptions);

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

  void notifyTileStartLoading(const Tile* pTile) noexcept;

  void notifyTileDoneLoading(const Tile* pTile) noexcept;

  void notifyTileUnloading(const Tile* pTile) noexcept;

  void notifyRasterStartLoading() noexcept;

  void notifyRasterDoneLoading() noexcept;

  template <class TilesetContentLoaderType>
  void propagateTilesetContentLoaderResult(
      TilesetLoadType type,
      const std::function<void(const TilesetLoadFailureDetails&)>&
          loadErrorCallback,
      TilesetContentLoaderResult<TilesetContentLoaderType>&& result);

  void createWorkManager(const TilesetExternals& externals);

  void discoverLoadWork(
      const std::vector<TileLoadRequest>& requests,
      double maximumScreenSpaceError,
      const TilesetOptions& tilesetOptions,
      std::vector<TileWorkManager::Order>& outOrders);

  void markWorkTilesAsLoading(
      const std::vector<const TileWorkManager::Work*>& workVector);

  void handleCompletedWork();

  void dispatchTileWork(
      TileProcessingData& processingData,
      const CesiumAsync::UrlResponseDataMap& responseDataMap,
      TileWorkManager::Work* work);

  void dispatchRasterWork(
      RasterProcessingData& processingData,
      const CesiumAsync::UrlResponseDataMap& responseDataMap,
      TileWorkManager::Work* work);

  TilesetExternals _externals;
  std::vector<CesiumAsync::IAssetAccessor::THeader> _requestHeaders;
  std::unique_ptr<TilesetContentLoader> _pLoader;
  std::unique_ptr<Tile> _pRootTile;
  std::optional<CesiumUtility::Credit> _userCredit;
  std::vector<CesiumUtility::Credit> _tilesetCredits;
  RasterOverlayUpsampler _upsampler;
  RasterOverlayCollection _overlayCollection;

  // Thread safe shared pointer
  std::shared_ptr<TileWorkManager> _pTileWorkManager;

  int32_t _tileLoadsInProgress;
  int32_t _loadedTilesCount;
  int64_t _tilesDataUsed;

  int32_t _rasterLoadsInProgress;
  int32_t _loadedRastersCount;

  CesiumAsync::Promise<void> _destructionCompletePromise;
  CesiumAsync::SharedFuture<void> _destructionCompleteFuture;

  CesiumAsync::Promise<void> _rootTileAvailablePromise;
  CesiumAsync::SharedFuture<void> _rootTileAvailableFuture;
};
} // namespace Cesium3DTilesSelection
