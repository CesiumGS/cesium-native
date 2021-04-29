#pragma once

#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterOverlayCollection.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/TileContext.h"
#include "Cesium3DTiles/TilesetContentOptions.h"
#include "Cesium3DTiles/TilesetExternals.h"
#include "Cesium3DTiles/TilesetStreamingOptions.h"
#include "Cesium3DTiles/ViewState.h"
#include "Cesium3DTiles/ViewUpdateResult.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetRequest.h"
#include "CesiumGeometry/Axis.h"
#include "CesiumGeometry/QuadtreeTileAvailability.h"
#include <atomic>
#include <memory>
#include <optional>
#include <rapidjson/fwd.h>
#include <string>
#include <vector>

namespace Cesium3DTiles {

/**
 * @brief A <a
 * href="https://github.com/CesiumGS/3d-tiles/tree/master/specification">3D
 * Tiles tileset</a>, used for streaming massive heterogeneous 3D geospatial
 * datasets.
 */
class CESIUM3DTILES_API Tileset final {
public:
  /**
   * @brief Constructs a new instance with a given `tileset.json` URL.
   * @param externals The external interfaces to use.
   * @param url The URL of the `tileset.json`.
   * @param options Additional options for the tileset.
   */
  Tileset(
      const TilesetExternals& externals,
      const std::string& url,
      const TilesetContentOptions& contentOptions = TilesetContentOptions(),
      const TilesetStreamingOptions& streamingOtions =
          TilesetStreamingOptions());

  /**
   * @brief Constructs a new instance with the given asset ID on <a
   * href="https://cesium.com/ion/">Cesium ion</a>.
   * @param externals The external interfaces to use.
   * @param ionAssetID The ID of the Cesium ion asset to use.
   * @param ionAccessToken The Cesium ion access token authorizing access to the
   * asset.
   * @param options Additional options for the tileset.
   */
  Tileset(
      const TilesetExternals& externals,
      uint32_t ionAssetID,
      const std::string& ionAccessToken,
      const TilesetContentOptions& contentOptions = TilesetContentOptions(),
      const TilesetStreamingOptions& streamingOptions =
          TilesetStreamingOptions());

  /**
   * @brief Destroys this tileset.
   * This may block the calling thread while waiting for pending asynchronous
   * tile loads to terminate.
   */
  ~Tileset();

  /**
   * @brief Gets the URL that was used to construct this tileset.
   * If the tileset references a Cesium ion asset,
   * this property will not have a value.
   */
  std::optional<std::string> getUrl() const noexcept { return this->_url; }

  /**
   * @brief Gets the Cesium ion asset ID of this tileset.
   * If the tileset references a URL, this property
   * will not have a value.
   */
  std::optional<uint32_t> getIonAssetID() const noexcept {
    return this->_ionAssetID;
  }

  /**
   * @brief Gets the Cesium ion access token to use to access this tileset.
   * If the tileset references a URL, this property will not have a value.
   */
  std::optional<std::string> getIonAccessToken() const noexcept {
    return this->_ionAccessToken;
  }

  /**
   * @brief Gets the {@link TilesetExternals} that summarize the external
   * interfaces used by this tileset.
   */
  TilesetExternals& getExternals() noexcept { return this->_externals; }

  /**
   * @brief Gets the {@link TilesetExternals} that summarize the external
   * interfaces used by this tileset.
   */
  const TilesetExternals& getExternals() const noexcept {
    return this->_externals;
  }

  /**
   * @brief Returns the {@link CesiumAsync::AsyncSystem} that is used for
   * dispatching asynchronous tasks.
   */
  CesiumAsync::AsyncSystem& getAsyncSystem() noexcept {
    return this->_asyncSystem;
  }

  /** @copydoc Tileset::getAsyncSystem() */
  const CesiumAsync::AsyncSystem& getAsyncSystem() const noexcept {
    return this->_asyncSystem;
  }

  /** @copydoc Tileset::getContentOptions() */
  const TilesetContentOptions& getContentOptions() const noexcept {
    return this->_contentOptions;
  }

  /**
   * @brief Gets the {@link TilesetContentOptions} of this tileset.
   */
  TilesetContentOptions& getContentOptions() noexcept {
    return this->_contentOptions;
  }

  /** @copydoc Tileset::getStreamingOptions() */
  const TilesetStreamingOptions& getStreamingOptions() const noexcept {
    return this->_streamingOptions;
  }

  /**
   * @brief Gets the {@link TilesetStreamingOptions} of this tileset.
   */
  TilesetStreamingOptions& getStreamingOptions() noexcept {
    return this->_streamingOptions;
  }

  /**
   * @brief Gets the root tile of this tileset.
   *
   * This may be `nullptr` if there is currently no root tile.
   */
  Tile* getRootTile() noexcept { return this->_pRootTile.get(); }

  /** @copydoc Tileset::getRootTile() */
  const Tile* getRootTile() const noexcept { return this->_pRootTile.get(); }

  /**
   * @brief Returns the {@link RasterOverlayCollection} of this tileset.
   */
  RasterOverlayCollection& getOverlays() noexcept { return this->_overlays; }

  /** @copydoc Tileset::getOverlays() */
  const RasterOverlayCollection& getOverlays() const noexcept {
    return this->_overlays;
  }

  /**
   * @brief Updates this view but waits for all tiles that meet sse to finish
   * loading and ready to be rendered before returning the function. This method
   * is significantly slower than {@link Tileset::updateView} and should only be
   * used for capturing movie or non-realtime situation.
   * @param viewState The {@link ViewState} that the view should be updated for
   * @returns The set of tiles to render in the updated view. This value is only
   * valid until the next call to `updateView` or until the tileset is
   * destroyed, whichever comes first.
   */
  const ViewUpdateResult& updateViewOffline(const ViewState& viewState);

  /**
   * @brief Updates this view, returning the set of tiles to render in this
   * view.
   * @param viewState The {@link ViewState} that the view should be updated for
   * @returns The set of tiles to render in the updated view. This value is only
   * valid until the next call to `updateView` or until the tileset is
   * destroyed, whichever comes first.
   */
  const ViewUpdateResult& updateView(const ViewState& viewState);

  /**
   * @brief Notifies the tileset that the given tile has started loading.
   * This method may be called from any thread.
   */
  void notifyTileStartLoading(Tile* pTile) noexcept;

  /**
   * @brief Notifies the tileset that the given tile has finished loading and is
   * ready to render. This method may be called from any thread.
   */
  void notifyTileDoneLoading(Tile* pTile) noexcept;

  /**
   * @brief Notifies the tileset that the given tile is about to be unloaded.
   */
  void notifyTileUnloading(Tile* pTile) noexcept;

  /**
   * @brief Loads a tile tree from a tileset.json file.
   *
   * This method is safe to call from any thread.
   *
   * @param rootTile A blank tile into which to load the root.
   * @param tilesetJson The parsed tileset.json.
   * @param parentTransform The new tile's parent transform.
   * @param parentRefine The default refinment to use if not specified
   * explicitly for this tile.
   * @param context The context of the new tiles.
   * @param pLogger The logger.
   */
  static void loadTilesFromJson(
      Tile& rootTile,
      const rapidjson::Value& tilesetJson,
      const glm::dmat4& parentTransform,
      TileRefine parentRefine,
      const TileContext& context,
      const std::shared_ptr<spdlog::logger>& pLogger);

  /**
   * @brief Request to load the content for the given tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @param tile The tile for which the content is requested.
   * @return A future that resolves when the content response is received, or
   * std::nullopt if this Tile has no content to load.
   */
  std::optional<
      CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>>
  requestTileContent(Tile& tile);

  /**
   * @brief Add the given {@link TileContext} to this tile set.
   *
   * This function is not supposed to be called by clients.
   *
   * @param pNewContext The new context. May not be `nullptr`.
   */
  void addContext(std::unique_ptr<TileContext>&& pNewContext);

  /**
   * @brief Invokes a function for each tile that is currently loaded.
   *
   * @param callback The function to invoke.
   */
  void forEachLoadedTile(const std::function<void(Tile& tile)>& callback);

  /**
   * @brief Gets the total number of bytes of tile and raster overlay data that
   * are currently loaded.
   */
  int64_t getTotalDataBytes() const noexcept;

  /**
   * @brief Determines if this tileset supports raster overlays.
   *
   * Currently, raster overlays can only be draped over quantized-mesh terrain
   * tilesets.
   */
  bool supportsRasterOverlays() const noexcept {
    return this->_supportsRasterOverlays;
  }

  /**
   * @brief Returns the value indicating the glTF up-axis.
   *
   * This function is not supposed to be called by clients.
   *
   * The value indicates the axis, via 0=X, 1=Y, 2=Z.
   *
   * @return The value representing the axis
   */
  CesiumGeometry::Axis getGltfUpAxis() const noexcept {
    return this->_gltfUpAxis;
  }

private:
  /**
   * @brief The result of traversing one branch of the tile hierarchy.
   *
   * Instances of this structure are created by the `_visit...` functions,
   * and summarize the information that was gathered during the traversal
   * of the respective branch, so that this information can be used by
   * the parent to decide on the further traversal process.
   */
  struct TraversalDetails {
    /**
     * @brief Whether all selected tiles in this tile's subtree are renderable.
     *
     * This is `true` if all selected (i.e. not culled or refined) tiles in this
     * tile's subtree are renderable. If the subtree is renderable, we'll render
     * it; no drama.
     */
    bool allAreRenderable = true;

    /**
     * @brief Whether any tile in this tile's subtree was rendered in the last
     * frame.
     *
     * This is `true` if any tiles in this tile's subtree were rendered last
     * frame. If any were, we must render the subtree rather than this tile,
     * because rendering this tile would cause detail to vanish that was visible
     * last frame, and that's no good.
     */
    bool anyWereRenderedLastFrame = false;

    /**
     * @brief The number of selected tiles in this tile's subtree that are not
     * yet renderable.
     *
     * Counts the number of selected tiles in this tile's subtree that are
     * not yet ready to be rendered because they need more loading. Note that
     * this value will _not_ necessarily be zero when
     * `allAreRenderable` is `true`, for subtle reasons.
     * When `allAreRenderable` and `anyWereRenderedLastFrame` are both `false`,
     * we will render this tile instead of any tiles in its subtree and the
     * `allAreRenderable` value for this tile will reflect only whether _this_
     * tile is renderable. The `notYetRenderableCount` value, however, will
     * still reflect the total number of tiles that we are waiting on, including
     * the ones that we're not rendering. `notYetRenderableCount` is only reset
     * when a subtree is removed from the render queue because the
     * `notYetRenderableCount` exceeds the
     * {@link TilesetStreamingOptions::loadingDescendantLimit}.
     */
    uint32_t notYetRenderableCount = 0;
  };

  /**
   * @brief Handles the response that was received for an asset request.
   *
   * This function is supposed to be called on the main thread.
   *
   * It the response for the given request consists of a valid JSON,
   * then {@link _loadTilesetJson} will be called. Otherwise, an error
   * message will be printed and {@link notifyTileDoneLoading} will be
   * called with a `nullptr`.
   *
   * @param pRequest The request for which the response was received.
   */
  void
  _handleAssetResponse(std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest);

  struct LoadResult {
    std::unique_ptr<TileContext> pContext;
    std::unique_ptr<Tile> pRootTile;
    bool supportsRasterOverlays;
  };

  /**
   * @brief Handles the response that was received for an tileset.json request.
   *
   * This function is supposed to be called on the main thread.
   *
   * It the response for the given request consists of a valid tileset JSON,
   * then {@link createTile} or {@link _createTerrainTile} will be called.
   * Otherwise, and error message will be printed and the root tile of the
   * return value will be `nullptr`.
   *
   * @param pRequest The request for which the response was received.
   * @return The LoadResult structure
   */
  static LoadResult _handleTilesetResponse(
      std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest,
      std::unique_ptr<TileContext>&& pContext,
      const std::shared_ptr<spdlog::logger>& pLogger);

  void _loadTilesetJson(
      const std::string& url,
      const std::vector<std::pair<std::string, std::string>>& headers =
          std::vector<std::pair<std::string, std::string>>(),
      std::unique_ptr<TileContext>&& pContext = nullptr);

  static void _createTile(
      Tile& tile,
      const rapidjson::Value& tileJson,
      const glm::dmat4& parentTransform,
      TileRefine parentRefine,
      const TileContext& context,
      const std::shared_ptr<spdlog::logger>& pLogger);
  static void _createTerrainTile(
      Tile& tile,
      const rapidjson::Value& layerJson,
      TileContext& context,
      const std::shared_ptr<spdlog::logger>& pLogger);
  FailedTileAction _onIonTileFailed(Tile& failedTile);

  /**
   * @brief Handles a Cesium ion response to refreshing a token, retrying tiles
   * that previously failed due to token expiration.
   *
   * If the token refresh request succeeded, tiles that are in the
   * `FailedTemporarily` {@link Tile::LoadState} with an `httpStatusCode` of 401
   * will be returned to the `Unloaded` state so that they can be retried with
   * the new token. If the token refresh request failed, these tiles will be
   * marked `Failed` permanently.
   *
   * @param pIonRequest The request.
   * @param pContext The context.
   */
  void _handleTokenRefreshResponse(
      std::shared_ptr<CesiumAsync::IAssetRequest>&& pIonRequest,
      TileContext* pContext,
      const std::shared_ptr<spdlog::logger>& pLogger);

  /**
   * @brief Input information that is constant throughout the traversal.
   *
   * An instance of this structure is created upon entry of the top-level
   * `_visitTile` function, for the traversal for a certain frame, and
   * passed on through the traversal.
   */
  struct FrameState {
    const ViewState& viewState;
    int32_t lastFrameNumber;
    int32_t currentFrameNumber;
    double fogDensity;
  };

  TraversalDetails _renderLeaf(
      const FrameState& frameState,
      Tile& tile,
      double distance,
      ViewUpdateResult& result);
  TraversalDetails _renderInnerTile(
      const FrameState& frameState,
      Tile& tile,
      ViewUpdateResult& result);
  TraversalDetails _refineToNothing(
      const FrameState& frameState,
      Tile& tile,
      ViewUpdateResult& result,
      bool areChildrenRenderable);
  bool _kickDescendantsAndRenderTile(
      const FrameState& frameState,
      Tile& tile,
      ViewUpdateResult& result,
      TraversalDetails& traversalDetails,
      size_t firstRenderedDescendantIndex,
      size_t loadIndexLow,
      size_t loadIndexMedium,
      size_t loadIndexHigh,
      bool queuedForLoad,
      double distance);

  TraversalDetails _visitTile(
      const FrameState& frameState,
      uint32_t depth,
      bool ancestorMeetsSse,
      Tile& tile,
      double distance,
      bool culled,
      ViewUpdateResult& result);
  TraversalDetails _visitTileIfNeeded(
      const FrameState& frameState,
      uint32_t depth,
      bool ancestorMeetsSse,
      Tile& tile,
      ViewUpdateResult& result);
  TraversalDetails _visitVisibleChildrenNearToFar(
      const FrameState& frameState,
      uint32_t depth,
      bool ancestorMeetsSse,
      Tile& tile,
      ViewUpdateResult& result);

  /**
   * @brief When called on an additive-refined tile, queues it for load and adds
   * it to the render list.
   *
   * For replacement-refined tiles, this method does nothing and returns false.
   *
   * @param frameState The state of the current frame.
   * @param tile The tile to potentially load and render.
   * @param result The current view update result.
   * @param distance The distance to this tile, used to compute the load
   * priority.
   * @return true The additive-refined tile was queued for load and added to the
   * render list.
   * @return false The non-additive-refined tile was ignored.
   */
  bool _loadAndRenderAdditiveRefinedTile(
      const FrameState& frameState,
      Tile& tile,
      ViewUpdateResult& result,
      double distance);

  /**
   * @brief Queues load of tiles that are _required_ to be loaded before the
   * given tile can be refined.
   *
   * If {@link TilesetStreamingOptions::forbidHoles} is false (the default), any
   * tile can be refined, regardless of whether its children are loaded or not.
   * So in that case, this method immediately returns `false`.
   *
   * When `forbidHoles` is true, however, and some of this tile's children are
   * not yet renderable, this method returns `true`. It also adds those
   * not-yet-renderable tiles to the load queue.
   *
   * @param frameState The state of the current frame.
   * @param tile The tile that is potentially being refined.
   * @param distance The distance to the tile.
   * @return true Some of the required children are not yet loaded, so this tile
   * _cannot_ yet be refined.
   * @return false All of the required children (if there are any) are loaded,
   * so this tile _can_ be refined.
   */
  bool _queueLoadOfChildrenRequiredForRefinement(
      const FrameState& frameState,
      Tile& tile,
      double distance);
  bool _meetsSse(
      const ViewState& viewState,
      const Tile& tile,
      double distance,
      bool culled) const;

  void _processLoadQueue();
  void _unloadCachedTiles();
  void _markTileVisited(Tile& tile);

  std::string getResolvedContentUrl(const Tile& tile) const;

  std::vector<std::unique_ptr<TileContext>> _contexts;
  TilesetExternals _externals;
  CesiumAsync::AsyncSystem _asyncSystem;

  //  credits provided with the tileset from Cesium Ion
  std::vector<Credit> _tilesetCredits;

  std::optional<std::string> _url;
  std::optional<uint32_t> _ionAssetID;
  std::optional<std::string> _ionAccessToken;
  bool _isRefreshingIonToken;

  TilesetStreamingOptions _streamingOptions;
  TilesetContentOptions _contentOptions;

  std::unique_ptr<Tile> _pRootTile;

  int32_t _previousFrameNumber;
  ViewUpdateResult _updateResult;

  struct LoadRecord {
    Tile* pTile;

    /**
     * @brief The relative priority of loading this tile.
     *
     * Lower priority values load sooner.
     */
    double priority;

    bool operator<(const LoadRecord& rhs) const {
      return this->priority < rhs.priority;
    }
  };

  std::vector<LoadRecord> _loadQueueHigh;
  std::vector<LoadRecord> _loadQueueMedium;
  std::vector<LoadRecord> _loadQueueLow;
  std::atomic<uint32_t> _loadsInProgress;

  Tile::LoadedLinkedList _loadedTiles;

  RasterOverlayCollection _overlays;

  int64_t _tileDataBytes;

  bool _supportsRasterOverlays;

  /**
   * @brief The axis that was declared as the "up-axis" for glTF content.
   *
   * The glTF specification mandates that the Y-axis is the "up"-axis, so the
   * default value is {@link Axis::Y}. Older tilesets may contain a string
   * property in the "assets" dictionary, named "gltfUpAxis", indicating a
   * different up-axis. Although the "gltfUpAxis" property is no longer part of
   * the 3D tiles specification, it is still considered for backward
   * compatibility.
   */
  CesiumGeometry::Axis _gltfUpAxis;

  static void addTileToLoadQueue(
      std::vector<LoadRecord>& loadQueue,
      const ViewState& viewState,
      Tile& tile,
      double distance);
  static void processQueue(
      std::vector<Tileset::LoadRecord>& queue,
      std::atomic<uint32_t>& loadsInProgress,
      uint32_t maximumLoadsInProgress);

  Tileset(const Tileset& rhs) = delete;
  Tileset& operator=(const Tileset& rhs) = delete;
};

} // namespace Cesium3DTiles
