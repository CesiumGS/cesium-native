#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <atomic>
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/TilesetExternals.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/IAssetRequest.h"
#include "Cesium3DTiles/ViewUpdateResult.h"
#include "Cesium3DTiles/Camera.h"
#include "Cesium3DTiles/RasterOverlayCollection.h"
#include "CesiumUtility/Json.h"
#include "CesiumUtility/DoublyLinkedList.h"

namespace Cesium3DTiles {

    typedef size_t TilePrimaryKey;
    typedef size_t TileContextPrimaryKey;
    typedef size_t VisitedTilePrimaryKey;
    typedef size_t UnvisitedTilePrimaryKey;

    struct TileRange {
        TilePrimaryKey first;
        uint32_t count;
    };

    struct TileData {
        std::vector<TileID> ids;
        std::vector<TilePrimaryKey> parents;
        std::vector<BoundingVolume> boundingVolumes;
        std::vector<double> geometricErrors;
        std::vector<TileContextPrimaryKey> contexts;

        // TODO: replace std::unordered_map with a hashmap that isn't terrible.
        // https://martin.ankerl.com/2019/04/01/hashmap-benchmarks-01-overview/
        // Or: A simple vector sorted on primary key that we can walk in step
        // with the vectors above, or binary search through if necessary.
        std::unordered_map<TilePrimaryKey, BoundingVolume> viewerRequestVolumes;
        std::unordered_map<TilePrimaryKey, TileRefine> refineModes;
        std::unordered_map<TilePrimaryKey, BoundingVolume> contentBoundingVolumeS;
        std::unordered_map<TilePrimaryKey, glm::dmat4> transforms;
        std::unordered_map<TilePrimaryKey, TileRange> children;

        std::unordered_map<TilePrimaryKey, VisitedTilePrimaryKey> visitedTileIndices;
        std::unordered_map<TilePrimaryKey, UnvisitedTilePrimaryKey> unvisitedTileIndices;
    };

    struct VisitedTile {
        TilePrimaryKey tilePrimaryKey;
        double minimumDistance;
        double maximumDistance;
        BoundingVolume boundingVolume;
        bool culled : 1;
        bool rendered : 1;
    };

    struct VisitedTileData {
        std::vector<VisitedTile> visitedTiles;
        std::vector<VisitedTile> nextVisitedTiles;
    
        // References to tiles that were not visited in the last frame and are therefore
        // candidates for unloading. Tiles that were visited less recently are closer to
        // the front of the vector.
        std::vector<TilePrimaryKey> unvistedTiles;
    };

    /**
     * Additional options for configuring a \ref Tileset.
     * 
     */
    struct CESIUM3DTILES_API TilesetOptions {
        /**
         * The maximum number of pixels of error when rendering this tileset.
         * This is used to select an appropriate level-of-detail.
         */
        double maximumScreenSpaceError = 16.0;

        /**
         * The maximum number of tiles that may simultaneously be in the process
         * of loading.
         */
        uint32_t maximumSimultaneousTileLoads = 20;

        /**
         * Indicates whether the ancestors of rendered tiles should be preloaded.
         * Setting this to true optimizes the zoom-out experience and provides more detail in
         * newly-exposed areas when panning. The down side is that it requires loading more tiles.
         */
        bool preloadAncestors = true;

        /**
         * Indicates whether the siblings of rendered tiles should be preloaded.
         * Setting this to true causes tiles with the same parent as a rendered tile to be loaded, even
         * if they are culled. Setting this to true may provide a better panning experience at the
         * cost of loading more tiles.
         */
        bool preloadSiblings = true;

        /**
         * The number of loading descendant tiles that is considered "too many".
         * If a tile has too many loading descendants, that tile will be loaded and rendered before any of
         * its descendants are loaded and rendered. This means more feedback for the user that something
         * is happening at the cost of a longer overall load time. Setting this to 0 will cause each
         * tile level to be loaded successively, significantly increasing load time. Setting it to a large
         * number (e.g. 1000) will minimize the number of tiles that are loaded but tend to make
         * detail appear all at once after a long wait.
         */
        uint32_t loadingDescendantLimit = 20;

        /**
         * When true, the tileset will guarantee that the tileset will never be rendered with holes in place
         * of tiles that are not yet loaded. It does this by refusing to refine a parent tile until all of its
         * child tiles are ready to render. Thus, when the camera moves, we will always have something - even
         * if it's low resolution - to render any part of the tileset that becomes visible. When false, overall
         * loading will be faster, but newly-visible parts of the tileset may initially be blank.
         */
        bool forbidHoles = false;

        /**
         * The maximum number of tiles that may be cached. Note that this value, even if 0, will never
         * cause tiles that are needed for rendering to be unloaded. However, if the total number of
         * loaded tiles is greater than this value, tiles will be unloaded until the total is under
         * this number or until only required tiles remain, whichever comes first.
         */
        uint32_t maximumCachedTiles = 400;
    };

    /**
     * A {@link https://github.com/CesiumGS/3d-tiles/tree/master/specification|3D Tiles tileset},
     * used for streaming massive heterogeneous 3D geospatial datasets.
     */
    class CESIUM3DTILES_API Tileset {
    public:
        /**
         * Constructs a new instance with a given `tileset.json` URL.
         * @param externals The external interfaces to use.
         * @param url The URL of the `tileset.json`.
         * @param options Additional options for the tileset.
         */
        Tileset(const TilesetExternals& externals, const std::string& url, const TilesetOptions& options = TilesetOptions());

        /**
         * Constructs a new instance with the given asset ID on <a href="https://cesium.com/ion/">Cesium ion</a>.
         * @param externals The external interfaces to use.
         * @param ionAssetID The ID of the Cesium ion asset to use.
         * @param ionAccessToken The Cesium ion access token authorizing access to the asset.
         * @param options Additional options for the tileset.
         */
        Tileset(const TilesetExternals& externals, uint32_t ionAssetID, const std::string& ionAccessToken, const TilesetOptions& options = TilesetOptions());

        /**
         * Destroys this tileset. This may block the calling thread while waiting for pending asynchronous
         * tile loads to terminate.
         */
        ~Tileset();

        /**
         * Gets the URL that was used to construct this tileset. If the tileset references a Cesium ion asset,
         * this property will not have a value.
         */
        std::optional<std::string> getUrl() const { return this->_url; }

        /**
         * Gets the Cesium ion asset ID of this tileset. If the tileset references a URL, this property
         * will not have a value.
         */
        std::optional<uint32_t> getIonAssetID() const { return this->_ionAssetID; }

        /**
         * Gets the Cesium ion access token to use to access this tileset. If the tileset references a URL, this
         * property will not have a value.
         */
        std::optional<std::string> getIonAccessToken() const { return this->_ionAccessToken; }

        /**
         * Gets the external interfaces used by this tileset.
         */
        TilesetExternals& getExternals() { return this->_externals; }
        const TilesetExternals& getExternals() const { return this->_externals; }

        /**
         * Gets this tileset's options.
         */
        const TilesetOptions& getOptions() const { return this->_options; }

        RasterOverlayCollection& getOverlays() { return this->_overlays; }
        const RasterOverlayCollection& getOverlays() const { return this->_overlays; }

        const gsl::span<const TileContext> getTileContexts() const { return this->_tileContexts; }

        /**
         * Updates this view, returning the set of tiles to render in this view.
         * @param camera The updated camera.
         * @returns The set of tiles to render in the updated camera view. This value is only valid until
         *          the next call to <see cref="Cesium3DTilesetView.update" /> or until the view is
         *          destroyed, whichever comes first.
         */
        const ViewUpdateResult& updateView(const Camera& camera);

        /**
         * Notifies the tileset that the given tile has finished loading and is ready to render.
         * This method may be called from any thread.
         */
        void notifyTileDoneLoading(Tile* pTile);

        /**
         * Loads a tile tree from a tileset.json file. This method is safe to call from any thread.
         * @param tilesetJson The parsed tileset.json.
         */
        static TileData loadTilesFromJson(const nlohmann::json& tilesetJson);

        std::unique_ptr<IAssetRequest> requestTileContent(Tile& tile);

    private:
        struct TraversalDetails {
            /**
             * True if all selected (i.e. not culled or refined) tiles in this tile's subtree
             * are renderable. If the subtree is renderable, we'll render it; no drama.
             */
            bool allAreRenderable = true;

            /**
             * True if any tiles in this tile's subtree were rendered last frame. If any
             * were, we must render the subtree rather than this tile, because rendering
             * this tile would cause detail to vanish that was visible last frame, and
             * that's no good.
             */
            bool anyWereRenderedLastFrame = false;

            /**
             * Counts the number of selected tiles in this tile's subtree that are
             * not yet ready to be rendered because they need more loading. Note that
             * this value will _not_ necessarily be zero when
             * /ref TraversalDetails::allAreRenderable is true, for subtle reasons.
             * When /ref TraversalDetails::allAreRenderable and
             * /ref TraversalDetails::anyWereRenderedLastFrame are both false, we
             * will render this tile instead of any tiles in its subtree and
             * the `allAreRenderable` value for this tile will reflect only whether _this_
             * tile is renderable. The `notYetRenderableCount` value, however, will still
             * reflect the total number of tiles that we are waiting on, including the
             * ones that we're not rendering. `notYetRenderableCount` is only reset
             * when a subtree is removed from the render queue because the
             * `notYetRenderableCount` exceeds the
             * /ref TilesetOptions::loadingDescendantLimit.
             */
            uint32_t notYetRenderableCount = 0;
        };

        void _ionResponseReceived(IAssetRequest* pRequest);
        void _tilesetJsonResponseReceived(IAssetRequest* pRequest);
        void _layerJsonResponseReceived(IAssetRequest* pRequest);
        static void _createTile(TileData& tiles, TileContextPrimaryKey contextID, TilePrimaryKey parentTile, const nlohmann::json& tileJson);
        void _createTerrain(TileData& tiles, TileContextPrimaryKey contextID, const nlohmann::json& layerJson);

        TraversalDetails _visitTile(uint32_t lastFrameNumber, uint32_t currentFrameNumber, const Camera& camera, bool ancestorMeetsSse, TilePrimaryKey tile, ViewUpdateResult& result);
        TraversalDetails _visitTileIfVisible(uint32_t lastFrameNumber, uint32_t currentFrameNumber, const Camera& camera, bool ancestorMeetsSse, TilePrimaryKey tile, ViewUpdateResult& result);
        TraversalDetails _visitVisibleChildrenNearToFar(uint32_t lastFrameNumber, uint32_t currentFrameNumber, const Camera& camera, bool ancestorMeetsSse, TilePrimaryKey tile, ViewUpdateResult& result);
        void _processLoadQueue();
        void _unloadCachedTiles();
        void _markTileVisited(TilePrimaryKey tile);

        bool isDoingInitialLoad() const;
        void markInitialLoadComplete();
        std::string getResolvedContentUrl(const Tile& tile) const;

        TilesetExternals _externals;

        std::optional<std::string> _url;
        std::optional<uint32_t> _ionAssetID;
        std::optional<std::string> _ionAccessToken;

        TilesetOptions _options;

        std::unique_ptr<IAssetRequest> _pIonRequest;
        std::unique_ptr<IAssetRequest> _pTilesetJsonRequest;
        std::atomic<bool> _isDoingInitialLoad;

        TileData _tiles;

        uint32_t _previousFrameNumber;
        ViewUpdateResult _updateResult;

        std::vector<TilePrimaryKey> _loadQueueHigh;
        std::vector<TilePrimaryKey> _loadQueueMedium;
        std::vector<TilePrimaryKey> _loadQueueLow;
        std::atomic<uint32_t> _loadsInProgress;

        CesiumUtility::DoublyLinkedList<Tile, &Tile::_loadedTilesLinks> _loadedTiles;

        RasterOverlayCollection _overlays;

        std::vector<TileContext> _tileContexts;

        Tileset(const Tileset& rhs) = delete;
        Tileset& operator=(const Tileset& rhs) = delete;
    };

} // namespace Cesium3DTiles