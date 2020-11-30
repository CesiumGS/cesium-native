#pragma once

#include "Cesium3DTiles/AsyncSystem.h"
#include "Cesium3DTiles/Camera.h"
#include "Cesium3DTiles/IAssetRequest.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterOverlayCollection.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/TileContext.h"
#include "Cesium3DTiles/TilesetExternals.h"
#include "Cesium3DTiles/ViewUpdateResult.h"
#include "CesiumGeometry/QuadtreeTileAvailability.h"
#include "CesiumUtility/Json.h"
#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Cesium3DTiles {

    /**
     * @brief Defines the fog density at a certain height.
     * 
     * @see TilesetOptions::fogDensityTable
     */
    struct FogDensityAtHeight {

        /** @brief The height. */
        double cameraHeight;

        /** @brief The fog density. */
        double fogDensity;
    };

    /**
     * @brief Additional options for configuring a {@link Tileset}.
     */
    struct CESIUM3DTILES_API TilesetOptions {
        /**
         * @brief The maximum number of pixels of error when rendering this tileset.
         * This is used to select an appropriate level-of-detail.
         */
        double maximumScreenSpaceError = 16.0;

        /**
         * @brief The maximum number of tiles that may simultaneously be in the process
         * of loading.
         */
        uint32_t maximumSimultaneousTileLoads = 20;

        /**
         * @brief Indicates whether the ancestors of rendered tiles should be preloaded.
         * Setting this to true optimizes the zoom-out experience and provides more detail in
         * newly-exposed areas when panning. The down side is that it requires loading more tiles.
         */
        bool preloadAncestors = true;

        /**
         * @brief Indicates whether the siblings of rendered tiles should be preloaded.
         * Setting this to true causes tiles with the same parent as a rendered tile to be loaded, even
         * if they are culled. Setting this to true may provide a better panning experience at the
         * cost of loading more tiles.
         */
        bool preloadSiblings = true;

        /**
         * @brief The number of loading descendant tiles that is considered "too many".
         * If a tile has too many loading descendants, that tile will be loaded and rendered before any of
         * its descendants are loaded and rendered. This means more feedback for the user that something
         * is happening at the cost of a longer overall load time. Setting this to 0 will cause each
         * tile level to be loaded successively, significantly increasing load time. Setting it to a large
         * number (e.g. 1000) will minimize the number of tiles that are loaded but tend to make
         * detail appear all at once after a long wait.
         */
        uint32_t loadingDescendantLimit = 20;

        /**
         * @brief Never render a tileset with missing tiles.
         *
         * When true, the tileset will guarantee that the tileset will never be rendered with holes in place
         * of tiles that are not yet loaded. It does this by refusing to refine a parent tile until all of its
         * child tiles are ready to render. Thus, when the camera moves, we will always have something - even
         * if it's low resolution - to render any part of the tileset that becomes visible. When false, overall
         * loading will be faster, but newly-visible parts of the tileset may initially be blank.
         */
        bool forbidHoles = false;

        /**
         * @brief The maximum number of bytes that may be cached.
         * 
         * Note that this value, even if 0, will never
         * cause tiles that are needed for rendering to be unloaded. However, if the total number of
         * loaded bytes is greater than this value, tiles will be unloaded until the total is under
         * this number or until only required tiles remain, whichever comes first.
         */
        size_t maximumCachedBytes = 512 * 1024 * 1024;

        /**
         * @brief A table that maps the camera height above the ellipsoid to a fog density. Tiles that are in full fog are culled.
         * The density of the fog increases as this number approaches 1.0 and becomes less dense as it approaches zero.
         * The more dense the fog is, the more aggressively the tiles are culled. For example, if the camera is a height of
         * 1000.0m above the ellipsoid, increasing the value to 3.0e-3 will cause many tiles close to the viewer be culled.
         * Decreasing the value will push the fog further from the viewer, but decrease performance as more of the tiles are rendered.
         * Tiles are culled when `1.0 - glm::exp(-(distance * distance * fogDensity * fogDensity))` is >= 1.0.
         */
        std::vector<FogDensityAtHeight> fogDensityTable = {
            { 359.393, 2.0e-5 },
            { 800.749, 2.0e-4 },
            { 1275.6501, 1.0e-4 },
            { 2151.1192, 7.0e-5 },
            { 3141.7763, 5.0e-5 },
            { 4777.5198, 4.0e-5 },
            { 6281.2493, 3.0e-5 },
            { 12364.307, 1.9e-5 },
            { 15900.765, 1.0e-5 },
            { 49889.0549, 8.5e-6 },
            { 78026.8259, 6.2e-6,},
            { 99260.7344, 5.8e-6 },
            { 120036.3873, 5.3e-6 },
            { 151011.0158, 5.2e-6 },
            { 156091.1953, 5.1e-6 },
            { 203849.3112, 4.2e-6 },
            { 274866.9803, 4.0e-6 },
            { 319916.3149, 3.4e-6 },
            { 493552.0528, 2.6e-6 },
            { 628733.5874, 2.2e-6 },
            { 1000000.0, 0.0 }
        };

        /**
         * @brief Whether to render tiles directly under the camera, even if they're not in the view frustum.
         * 
         * This is useful for detecting the camera's collision with terrain and other models.
         * NOTE: This option currently only works with tiles that use a `region` as their bounding volume.
         * It is ignored for other bounding volume types.
         */
        bool renderTilesUnderCamera = true;
    };

    /**
     * @brief A <a href="https://github.com/CesiumGS/3d-tiles/tree/master/specification">3D Tiles tileset</a>,
     * used for streaming massive heterogeneous 3D geospatial datasets.
     */
    class CESIUM3DTILES_API Tileset {
    public:
        /**
         * @brief Constructs a new instance with a given `tileset.json` URL.
         * @param externals The external interfaces to use.
         * @param url The URL of the `tileset.json`.
         * @param options Additional options for the tileset.
         */
        Tileset(const TilesetExternals& externals, const std::string& url, const TilesetOptions& options = TilesetOptions());

        /**
         * @brief Constructs a new instance with the given asset ID on <a href="https://cesium.com/ion/">Cesium ion</a>.
         * @param externals The external interfaces to use.
         * @param ionAssetID The ID of the Cesium ion asset to use.
         * @param ionAccessToken The Cesium ion access token authorizing access to the asset.
         * @param options Additional options for the tileset.
         */
        Tileset(const TilesetExternals& externals, uint32_t ionAssetID, const std::string& ionAccessToken, const TilesetOptions& options = TilesetOptions());

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
        std::optional<std::string> getUrl() const { return this->_url; }

        /**
         * @brief Gets the Cesium ion asset ID of this tileset. 
         * If the tileset references a URL, this property
         * will not have a value.
         */
        std::optional<uint32_t> getIonAssetID() const { return this->_ionAssetID; }

        /**
         * @brief Gets the Cesium ion access token to use to access this tileset. 
         * If the tileset references a URL, this property will not have a value.
         */
        std::optional<std::string> getIonAccessToken() const { return this->_ionAccessToken; }

        /**
         * @brief Gets the {@link TilesetExternals} that summarize the external interfaces used by this tileset.
         */
        TilesetExternals& getExternals() noexcept { return this->_externals; }

        /**
         * @brief Gets the {@link TilesetExternals} that summarize the external interfaces used by this tileset.
         */
        const TilesetExternals& getExternals() const { return this->_externals; }

        AsyncSystem& getAsyncSystem() { return this->_asyncSystem; }
        const AsyncSystem& getAsyncSystem() const { return this->_asyncSystem; }

        /** @copydoc Tileset::getOptions() */
        const TilesetOptions& getOptions() const { return this->_options; }

        /**
         * @brief Gets the {@link TilesetOptions} of this tileset.
         */
        TilesetOptions& getOptions() { return this->_options; }

        /**
         * @brief Gets the root tile of this tileset.
         *
         * This may be `nullptr` if there is currently no root tile.
         */
        Tile* getRootTile() { return this->_pRootTile.get(); }

        /** @copydoc Tileset::getRootTile() */
        const Tile* getRootTile() const { return this->_pRootTile.get(); }

        /**
         * @brief Returns the {@link RasterOverlayCollection} of this tileset.
         */
        RasterOverlayCollection& getOverlays() { return this->_overlays; }

        /** @copydoc Tileset::getOverlays() */
        const RasterOverlayCollection& getOverlays() const { return this->_overlays; }

        /**
         * @brief Updates this view, returning the set of tiles to render in this view.
         * @param camera The updated camera.
         * @returns The set of tiles to render in the updated camera view. This value is only valid until
         *          the next call to `updateView` or until the tileset is destroyed, whichever comes first.
         */
        const ViewUpdateResult& updateView(const Camera& camera);

        /**
         * @brief Notifies the tileset that the given tile has started loading.
         * This method may be called from any thread.
         */
        void notifyTileStartLoading(Tile* pTile);

        /**
         * @brief Notifies the tileset that the given tile has finished loading and is ready to render.
         * This method may be called from any thread.
         */
        void notifyTileDoneLoading(Tile* pTile);

        /**
         * @brief Notifies the tileset that the given tile is about to be unloaded.
         */
        void notifyTileUnloading(Tile* pTile);

        /**
         * @brief Loads a tile tree from a tileset.json file. 
         * 
         * This method is safe to call from any thread.
         * 
         * @param rootTile A blank tile into which to load the root.
         * @param tilesetJson The parsed tileset.json.
         * @param parentTransform The new tile's parent transform.
         * @param parentRefine The default refinment to use if not specified explicitly for this tile.
         * @param context The context of the new tiles.
         */
        void loadTilesFromJson(Tile& rootTile, const nlohmann::json& tilesetJson, const glm::dmat4& parentTransform, TileRefine parentRefine, const TileContext& context) const;

        /**
         * @brief Request to load the content for the given tile.
         * 
         * This function is not supposed to be called by clients.
         * 
         * @param tile The tile for which the content is requested.
         * @return A future that resolves when the content response is received, or std::nullopt if this Tile has no content to load.
         */
        std::optional<Future<std::unique_ptr<IAssetRequest>>> requestTileContent(Tile& tile);

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
        void forEachLoadedTile(const std::function<void (Tile& tile)>& callback);

        /**
         * @brief Gets the total number of bytes of tile and raster overlay data that are currently loaded.
         */
        size_t getTotalDataBytes() const;

    private:
        struct TraversalDetails {
            /**
             * @brief Whether all selected tiles in this tile's subtree are renderable.
             * 
             * This is `true` if all selected (i.e. not culled or refined) tiles in this tile's subtree
             * are renderable. If the subtree is renderable, we'll render it; no drama.
             */
            bool allAreRenderable = true;

            /**
             * @brief Whether any tile in this tile's subtree was rendered in the last frame.
             * 
             * This is `true` if any tiles in this tile's subtree were rendered last frame. If any
             * were, we must render the subtree rather than this tile, because rendering
             * this tile would cause detail to vanish that was visible last frame, and
             * that's no good.
             */
            bool anyWereRenderedLastFrame = false;

            /**
             * @brief The number of selected tiles in this tile's subtree that are not yet renderable.
             * 
             * Counts the number of selected tiles in this tile's subtree that are
             * not yet ready to be rendered because they need more loading. Note that
             * this value will _not_ necessarily be zero when
             * `allAreRenderable` is `true`, for subtle reasons.
             * When `allAreRenderable` and `anyWereRenderedLastFrame` are both `false`, we
             * will render this tile instead of any tiles in its subtree and
             * the `allAreRenderable` value for this tile will reflect only whether _this_
             * tile is renderable. The `notYetRenderableCount` value, however, will still
             * reflect the total number of tiles that we are waiting on, including the
             * ones that we're not rendering. `notYetRenderableCount` is only reset
             * when a subtree is removed from the render queue because the
             * `notYetRenderableCount` exceeds the
             * {@link TilesetOptions::loadingDescendantLimit}.
             */
            uint32_t notYetRenderableCount = 0;
        };

        void _ionResponseReceived(IAssetRequest* pRequest);
        void _tilesetJsonResponseReceived(IAssetRequest* pRequest);
        void _createTile(Tile& tile, const nlohmann::json& tileJson, const glm::dmat4& parentTransform, TileRefine parentRefine, const TileContext& context) const;
        void _createTerrainTile(Tile& tile, const nlohmann::json& layerJson, TileContext& context);
        FailedTileAction _onIonTileFailed(Tile& failedTile);

        struct FrameState {
            const Camera& camera;
            uint32_t lastFrameNumber;
            uint32_t currentFrameNumber;
            double fogDensity;
        };

        TraversalDetails _visitTile(const FrameState& frameState, uint32_t depth, bool ancestorMeetsSse, Tile& tile, double distance, ViewUpdateResult& result);
        TraversalDetails _visitTileIfVisible(const FrameState& frameState, uint32_t depth, bool ancestorMeetsSse, Tile& tile, ViewUpdateResult& result);
        TraversalDetails _visitVisibleChildrenNearToFar(const FrameState& frameState, uint32_t depth, bool ancestorMeetsSse, Tile& tile, ViewUpdateResult& result);

        void _processLoadQueue();
        void _unloadCachedTiles();
        void _markTileVisited(Tile& tile);

        std::string getResolvedContentUrl(const Tile& tile) const;

        std::vector<std::unique_ptr<TileContext>> _contexts;
        TilesetExternals _externals;
        AsyncSystem _asyncSystem;

        std::optional<std::string> _url;
        std::optional<uint32_t> _ionAssetID;
        std::optional<std::string> _ionAccessToken;

        TilesetOptions _options;

        std::unique_ptr<IAssetRequest> _pIonRequest;
        std::unique_ptr<IAssetRequest> _pTilesetJsonRequest;

        std::unique_ptr<Tile> _pRootTile;

        uint32_t _previousFrameNumber;
        ViewUpdateResult _updateResult;

        struct LoadRecord {
            Tile* pTile;

            /**
             * @brief The relative priority of loading this tile.
             * 
             * Lower priority values load sooner.
             */
            double priority;

            bool operator<(const LoadRecord& rhs) {
                return this->priority < rhs.priority;
            }
        };

        std::vector<LoadRecord> _loadQueueHigh;
        std::vector<LoadRecord> _loadQueueMedium;
        std::vector<LoadRecord> _loadQueueLow;
        std::atomic<uint32_t> _loadsInProgress;

        Tile::LoadedLinkedList _loadedTiles;

        RasterOverlayCollection _overlays;

        std::atomic<size_t> _tileDataBytes;

        static void addTileToLoadQueue(std::vector<LoadRecord>& loadQueue, const FrameState& frameState, Tile& tile, double distance);
        static void processQueue(std::vector<Tileset::LoadRecord>& queue, std::atomic<uint32_t>& loadsInProgress, uint32_t maximumLoadsInProgress);

        Tileset(const Tileset& rhs) = delete;
        Tileset& operator=(const Tileset& rhs) = delete;
    };

} // namespace Cesium3DTiles
