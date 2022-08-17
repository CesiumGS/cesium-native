#pragma once

#include "ImplicitTraversal.h"
#include "Library.h"
#include "RasterOverlayCollection.h"
#include "Tile.h"
#include "TileContext.h"
#include "TilesetExternals.h"
#include "TilesetOptions.h"
#include "ViewState.h"
#include "ViewUpdateResult.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/QuadtreeRectangleAvailability.h>
#include <CesiumGeometry/TileAvailabilityFlags.h>

#include <rapidjson/fwd.h>

#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Cesium3DTilesSelection {

/**
 * @brief A <a
 * href="https://github.com/CesiumGS/3d-tiles/tree/master/specification">3D
 * Tiles tileset</a>, used for streaming massive heterogeneous 3D geospatial
 * datasets.
 */
class CESIUM3DTILESSELECTION_API Tileset final {
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
      const TilesetOptions& options = TilesetOptions());

  /**
   * @brief Constructs a new instance with the given asset ID on <a
   * href="https://cesium.com/ion/">Cesium ion</a>.
   * @param externals The external interfaces to use.
   * @param ionAssetID The ID of the Cesium ion asset to use.
   * @param ionAccessToken The Cesium ion access token authorizing access to the
   * asset.
   * @param options Additional options for the tileset.
   * @param ionAssetEndpointUrl The URL of the ion asset endpoint. Defaults
   * to Cesium ion but a custom endpoint can be specified.
   */
  Tileset(
      const TilesetExternals& externals,
      int64_t ionAssetID,
      const std::string& ionAccessToken,
      const TilesetOptions& options = TilesetOptions(),
      const std::string& ionAssetEndpointUrl = "https://api.cesium.com/");

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
  std::optional<int64_t> getIonAssetID() const noexcept {
    return this->_ionAssetID;
  }

  /**
   * @brief Gets the Cesium ion access token to use to access this tileset.
   * If the tileset references a URL, this property will not have a value.
   */
  std::optional<std::string> getIonAccessToken() const noexcept {
    return this->_ionAccessToken;
  }

  const std::vector<Credit> getTilesetCredits() const noexcept {
    return this->_tilesetCredits;
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

  /** @copydoc Tileset::getOptions() */
  const TilesetOptions& getOptions() const noexcept { return this->_options; }

  /**
   * @brief Gets the {@link TilesetOptions} of this tileset.
   */
  TilesetOptions& getOptions() noexcept { return this->_options; }

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
   * @param frustums The {@link ViewState}s that the view should be updated for
   * @returns The set of tiles to render in the updated view. This value is only
   * valid until the next call to `updateView` or until the tileset is
   * destroyed, whichever comes first.
   */
  const ViewUpdateResult&
  updateViewOffline(const std::vector<ViewState>& frustums);

  /**
   * @brief Updates this view, returning the set of tiles to render in this
   * view.
   * @param frustums The {@link ViewState}s that the view should be updated for
   * @returns The set of tiles to render in the updated view. This value is only
   * valid until the next call to `updateView` or until the tileset is
   * destroyed, whichever comes first.
   */
  const ViewUpdateResult&
  updateView(const std::vector<ViewState>& frustums, float deltaTime);

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
   * @param newContexts The new contexts that are generated from recursively
   * parsing the tiles.
   * @param tilesetJson The parsed tileset.json.
   * @param parentTransform The new tile's parent transform.
   * @param parentRefine The default refinment to use if not specified
   * explicitly for this tile.
   * @param context The context of the new tiles.
   * @param pLogger The logger.
   */
  static void loadTilesFromJson(
      Tile& rootTile,
      std::vector<std::unique_ptr<TileContext>>& newContexts,
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
   * Do not call this function if the tile has no content to load.
   *
   * @param tile The tile for which the content is requested.
   * @return A future that resolves when the content response is received.
   */
  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  requestTileContent(Tile& tile);

  /**
   * @brief Request to load the availability subtree for the given tile.
   *
   * This function is not supposed to be called by client.
   *
   * @param tile The tile for which the subtree is requested.
   * @return A future that resolves when the subtree response is received.
   */
  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  requestAvailabilitySubtree(Tile& tile);

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
     * {@link TilesetOptions::loadingDescendantLimit}.
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
  CesiumAsync::Future<void>
  _handleAssetResponse(std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest);

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
    const std::vector<ViewState>& frustums;
    std::vector<double> fogDensities;
    int32_t lastFrameNumber;
    int32_t currentFrameNumber;
  };

  TraversalDetails _renderLeaf(
      const FrameState& frameState,
      const ImplicitTraversalInfo& implicitInfo,
      Tile& tile,
      const std::vector<double>& distances,
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
      const ImplicitTraversalInfo& implicitInfo,
      ViewUpdateResult& result,
      TraversalDetails& traversalDetails,
      size_t firstRenderedDescendantIndex,
      size_t loadIndexLow,
      size_t loadIndexMedium,
      size_t loadIndexHigh,
      bool queuedForLoad,
      const std::vector<double>& distances);

  TraversalDetails _visitTile(
      const FrameState& frameState,
      const ImplicitTraversalInfo& implicitInfo,
      uint32_t depth,
      bool ancestorMeetsSse,
      Tile& tile,
      const std::vector<double>& distances,
      bool culled,
      ViewUpdateResult& result);
  TraversalDetails _visitTileIfNeeded(
      const FrameState& frameState,
      const ImplicitTraversalInfo implicitInfo,
      uint32_t depth,
      bool ancestorMeetsSse,
      Tile& tile,
      ViewUpdateResult& result);
  TraversalDetails _visitVisibleChildrenNearToFar(
      const FrameState& frameState,
      const ImplicitTraversalInfo& implicitInfo,
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
   * @param implicitInfo The implicit traversal information.
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
      const ImplicitTraversalInfo& implicitInfo,
      ViewUpdateResult& result,
      const std::vector<double>& distances);

  /**
   * @brief Queues load of tiles that are _required_ to be loaded before the
   * given tile can be refined in "Forbid Holes" mode.
   *
   * The queued tiles may include descedents, too, if any children are set to
   * Unconditionally Refine ({@link Tile::getUnconditionallyRefine}).
   *
   * This method should only be called if {@link TilesetOptions::forbidHoles} is enabled.
   *
   * @param frameState The state of the current frame.
   * @param tile The tile that is potentially being refined.
   * @param implicitInfo The implicit traversal info.
   * @param distance The distance to the tile.
   * @return true Some of the required descendents are not yet loaded, so this
   * tile _cannot_ yet be refined.
   * @return false All of the required descendents (if there are any) are
   * loaded, so this tile _can_ be refined.
   */
  bool _queueLoadOfChildrenRequiredForForbidHoles(
      const FrameState& frameState,
      Tile& tile,
      const ImplicitTraversalInfo& implicitInfo,
      const std::vector<double>& distances);
  bool _meetsSse(
      const std::vector<ViewState>& frustums,
      const Tile& tile,
      const std::vector<double>& distances,
      bool culled) const noexcept;

  void _processLoadQueue();
  void _unloadCachedTiles() noexcept;
  void _markTileVisited(Tile& tile) noexcept;

  std::string
  getResolvedContentUrl(const TileContext& context, const TileID& tileID) const;

  std::vector<std::unique_ptr<TileContext>> _contexts;
  TilesetExternals _externals;
  CesiumAsync::AsyncSystem _asyncSystem;

  // per-tileset credit passed in explicitly by the user through
  // `TilesetOptions`
  std::optional<Credit> _userCredit;
  //  credits provided with the tileset from Cesium Ion
  std::vector<Credit> _tilesetCredits;

  std::optional<std::string> _url;
  std::optional<int64_t> _ionAssetID;
  std::optional<std::string> _ionAccessToken;
  bool _isRefreshingIonToken;
  std::optional<std::string> _ionAssetEndpointUrl;

  TilesetOptions _options;

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

    bool operator<(const LoadRecord& rhs) const noexcept {
      return this->priority < rhs.priority;
    }
  };

  struct SubtreeLoadRecord {
    /**
     * @brief The root tile of the subtree to load.
     */
    Tile* pTile;

    /**
     * @brief The implicit traversal information which will tell us what subtree
     * to load and which parent to attach it to.
     */
    ImplicitTraversalInfo implicitInfo;

    /**
     * @brief The relative priority of loading this tile.
     *
     * Lower priority values load sooner.
     */
    double priority;

    bool operator<(const SubtreeLoadRecord& rhs) const noexcept {
      return this->priority < rhs.priority;
    }
  };

  std::vector<LoadRecord> _loadQueueHigh;
  std::vector<LoadRecord> _loadQueueMedium;
  std::vector<LoadRecord> _loadQueueLow;
  std::atomic<uint32_t> _loadsInProgress; // TODO: does this need to be atomic?

  std::vector<SubtreeLoadRecord> _subtreeLoadQueue;
  std::atomic<uint32_t>
      _subtreeLoadsInProgress; // TODO: does this need to be atomic?

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

  // Holds computed distances, to avoid allocating them on the heap during tile
  // selection.
  std::vector<std::unique_ptr<std::vector<double>>> _distancesStack;
  size_t _nextDistancesVector;

  CESIUM_TRACE_DECLARE_TRACK_SET(_loadingSlots, "Tileset Loading Slot");

  static double addTileToLoadQueue(
      std::vector<LoadRecord>& loadQueue,
      const ImplicitTraversalInfo& implicitInfo,
      const std::vector<ViewState>& frustums,
      Tile& tile,
      const std::vector<double>& distances);
  void processQueue(
      std::vector<Tileset::LoadRecord>& queue,
      const std::atomic<uint32_t>& loadsInProgress,
      uint32_t maximumLoadsInProgress);

  void addSubtreeToLoadQueue(
      Tile& tile,
      const ImplicitTraversalInfo& implicitInfo,
      double loadPriority);
  void processSubtreeQueue();

  void reportError(TilesetLoadFailureDetails&& errorDetails);

  CesiumAsync::Future<int> _requestQuantizedMeshAvailabilityTile(
      const CesiumGeometry::QuadtreeTileID& availabilityTileID,
      TileContext* pAvailabilityContext);

  Tileset(const Tileset& rhs) = delete;
  Tileset& operator=(const Tileset& rhs) = delete;

  class LoadIonAssetEndpoint;
  class LoadTilesetDotJson;
  class LoadTileFromJson;
  class LoadSubtree;
};

} // namespace Cesium3DTilesSelection
