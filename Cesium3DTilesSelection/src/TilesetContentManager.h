#pragma once

#include "RasterOverlayUpsampler.h"
#include "TilesetContentLoaderResult.h"

#include <Cesium3DTilesSelection/RasterOverlayCollection.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetLoadFailureDetails.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/ReferenceCounted.h>

#include <vector>

namespace Cesium3DTilesSelection {

class TilesetSharedAssetSystem;

class TilesetContentManager
    : public CesiumUtility::ReferenceCountedNonThreadSafe<
          TilesetContentManager> {
public:
  TilesetContentManager(
      const TilesetExternals& externals,
      const TilesetOptions& tilesetOptions,
      RasterOverlayCollection&& overlayCollection,
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

  const CesiumUtility::IntrusivePointer<TilesetSharedAssetSystem>&
  getSharedAssetSystem() const noexcept;

  int32_t getNumberOfTilesLoading() const noexcept;

  int32_t getNumberOfTilesLoaded() const noexcept;

  int64_t getTotalDataUsed() const noexcept;

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

  // Stores assets that might be shared between tiles.
  CesiumUtility::IntrusivePointer<TilesetSharedAssetSystem> _pSharedAssetSystem;

  CesiumAsync::Promise<void> _destructionCompletePromise;
  CesiumAsync::SharedFuture<void> _destructionCompleteFuture;

  CesiumAsync::Promise<void> _rootTileAvailablePromise;
  CesiumAsync::SharedFuture<void> _rootTileAvailableFuture;
};
} // namespace Cesium3DTilesSelection
