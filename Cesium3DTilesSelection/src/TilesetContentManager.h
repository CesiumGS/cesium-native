#pragma once

#include "RasterOverlayUpsampler.h"

#include <Cesium3DTilesSelection/CesiumIonTilesetContentLoaderFactory.h>
#include <Cesium3DTilesSelection/RasterOverlayCollection.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderFactory.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetLoadFailureDetails.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/ReferenceCounted.h>

#include <vector>

namespace Cesium3DTilesSelection {

/**
 * @brief Represents the result of calling \ref
 * TilesetContentManager::unloadTileContent.
 */
enum class UnloadTileContentResult : uint8_t {
  /**
   * @brief The tile should remain in the loaded tiles list.
   */
  Keep = 0,
  /**
   * @brief The tile should be removed from the loaded tiles list.
   */
  Remove = 1,
  /**
   * @brief The tile should be removed from the loaded tiles list and have its
   * children cleared.
   */
  RemoveAndClearChildren = 3
};

class TilesetSharedAssetSystem;
class TileLoadRequester;

class TilesetContentManager
    : public CesiumUtility::ReferenceCountedNonThreadSafe<
          TilesetContentManager> {
public:
  TilesetContentManager(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      std::unique_ptr<TilesetContentLoader>&& pLoader,
      std::unique_ptr<Tile>&& pRootTile);

  TilesetContentManager(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      const std::string& url);

  TilesetContentManager(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      TilesetContentLoaderFactory&& loaderFactory);

  TilesetContentManager(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
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

  void loadTileContent(Tile& tile, const TilesetOptions& tilesetOptions);

  void updateTileContent(Tile& tile, const TilesetOptions& tilesetOptions);

  /**
   * @brief Creates explicit Tile instances for a tile's latent children, if
   * it is necessary and possible to do so.
   *
   * Latent children are child tiles that can be created by
   * {@link TilesetContentLoader::createChildTiles} but that are not yet
   * reflected in {@link Tile::getChildren}. For example, in implicit tiling,
   * we save memory by only creating explicit Tile instances from implicit
   * availability as those instances are needed. Calling this method will create
   * the explicit tile instances for the given tile's children.
   *
   * This method does nothing if the given tile already has children, or if
   * {@link Tile::getMightHaveLatentChildren} returns false.
   *
   * @param tile The tile for which to create latent children.
   * @param tilesetOptions The tileset's options.
   */
  void createLatentChildrenIfNecessary(
      Tile& tile,
      const TilesetOptions& tilesetOptions);

  UnloadTileContentResult unloadTileContent(Tile& tile);

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

  const CesiumUtility::IntrusivePointer<TilesetSharedAssetSystem>&
  getSharedAssetSystem() const noexcept;

  int32_t getNumberOfTilesLoading() const noexcept;

  int32_t getNumberOfTilesLoaded() const noexcept;

  int64_t getTotalDataUsed() const noexcept;

  // Transition the tile from the ContentLoaded to the Done state.
  void finishLoading(Tile& tile, const TilesetOptions& tilesetOptions);

  void markTileIneligibleForContentUnloading(Tile& tile);
  void markTileEligibleForContentUnloading(Tile& tile);

  /**
   * @brief Unloads unused tiles until the total memory usage by all loaded
   * tiles is less than `maximumCachedBytes`.
   *
   * Tiles that are in use will not be unloaded even if the total exceeds the
   * specified `maximumCachedBytes`.
   *
   * @param maximumCachedBytes The maximum bytes to keep cached.
   * @param timeBudgetMilliseconds The maximum time, in milliseconds, to spend
   * unloading tiles. If 0.0, there is no limit.
   */
  void
  unloadCachedBytes(int64_t maximumCachedBytes, double timeBudgetMilliseconds);
  void clearChildrenRecursively(Tile* pTile) noexcept;

  void registerTileRequester(TileLoadRequester& requester);
  void unregisterTileRequester(TileLoadRequester& requester);

  void processWorkerThreadLoadRequests(const TilesetOptions& options);
  void processMainThreadLoadRequests(const TilesetOptions& options);

  void markTilesetDestroyed() noexcept;
  void releaseReference() const;

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
  std::optional<CesiumUtility::Credit> _userCredit;
  std::vector<CesiumUtility::Credit> _tilesetCredits;
  RasterOverlayUpsampler _upsampler;
  RasterOverlayCollection _overlayCollection;
  int32_t _tileLoadsInProgress;
  int32_t _loadedTilesCount;
  int64_t _tilesDataUsed;
  bool _tilesetDestroyed;

  // Stores assets that might be shared between tiles.
  CesiumUtility::IntrusivePointer<TilesetSharedAssetSystem> _pSharedAssetSystem;

  CesiumAsync::Promise<void> _destructionCompletePromise;
  CesiumAsync::SharedFuture<void> _destructionCompleteFuture;

  CesiumAsync::Promise<void> _rootTileAvailablePromise;
  CesiumAsync::SharedFuture<void> _rootTileAvailableFuture;

  // These tiles are not currently used, so their content may be unloaded. The
  // tiles at the head of the list are the least recently used, and the ones at
  // the tail are the most recently used.
  Tile::UnusedLinkedList _tilesEligibleForContentUnloading;

  std::vector<TileLoadRequester*> _requesters;
  double _roundRobinValueWorker;
  double _roundRobinValueMain;

  // These are scratch space, stored here to avoid heap allocations.
  std::vector<double> _requesterFractions;
  std::vector<TileLoadRequester*> _requestersWithRequests;
};
} // namespace Cesium3DTilesSelection
