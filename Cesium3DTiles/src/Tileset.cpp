#include "Cesium3DTiles/ExternalTilesetContent.h"
#include "Cesium3DTiles/IAssetAccessor.h"
#include "Cesium3DTiles/IAssetResponse.h"
#include "Cesium3DTiles/TerrainLayerJsonContent.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/Tileset.h"
#include "CesiumGeospatial/GeographicProjection.h"
#include "CesiumGeometry/QuadtreeTileAvailability.h"
#include "CesiumUtility/Math.h"
#include "TilesetJson.h"
#include "Uri.h"

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace Cesium3DTiles {
    Tileset::Tileset(
        const TilesetExternals& externals,
        const std::string& url,
        const TilesetOptions& options
    ) :
        _contexts(),
        _externals(externals),
        _url(url),
        _ionAssetID(),
        _ionAccessToken(),
        _options(options),
        _pIonRequest(),
        _pTilesetJsonRequest(),
        _pRootTile(),
        _previousFrameNumber(0),
        _updateResult(),
        _loadQueueHigh(),
        _loadQueueMedium(),
        _loadQueueLow(),
        _loadsInProgress(0),
        _loadedTiles(),
        _overlays()
    {
        ++this->_loadsInProgress;
        this->_pTilesetJsonRequest = this->_externals.pAssetAccessor->requestAsset(url);
        this->_pTilesetJsonRequest->bind(std::bind(&Tileset::_tilesetJsonResponseReceived, this, std::placeholders::_1));
    }

    Tileset::Tileset(
        const TilesetExternals& externals,
        uint32_t ionAssetID,
        const std::string& ionAccessToken,
        const TilesetOptions& options
    ) :
        _contexts(),
        _externals(externals),
        _url(),
        _ionAssetID(ionAssetID),
        _ionAccessToken(ionAccessToken),
        _options(options),
        _pIonRequest(),
        _pTilesetJsonRequest(),
        _pRootTile(),
        _previousFrameNumber(0),
        _updateResult(),
        _loadQueueHigh(),
        _loadQueueMedium(),
        _loadQueueLow(),
        _loadsInProgress(0),
        _loadedTiles(),
        _overlays()
    {
        std::string url = "https://api.cesium.com/v1/assets/" + std::to_string(ionAssetID) + "/endpoint";
        if (ionAccessToken.size() > 0)
        {
            url += "?access_token=" + ionAccessToken;
        }

        ++this->_loadsInProgress;
        this->_pIonRequest = this->_externals.pAssetAccessor->requestAsset(url);
        this->_pIonRequest->bind(std::bind(&Tileset::_ionResponseReceived, this, std::placeholders::_1));
    }

    Tileset::~Tileset() {
        // Tell all async loads to wrap it up.
        if (this->_pIonRequest) {
            this->_pIonRequest->cancel();
        }

        if (this->_pTilesetJsonRequest) {
            this->_pTilesetJsonRequest->cancel();
        }

        // Tell any ContentLoading tiles that we're destroying.
        Tile* pCurrent = this->_loadedTiles.head();
        while (pCurrent) {
            Tile* pNext = this->_loadedTiles.next(pCurrent);
            pCurrent->prepareToDestroy();
            pCurrent = pNext;
        }

        // Wait for all asynchronous loading to terminate.
        // If you're hanging here, it's most likely caused by _loadsInProgress not being
        // decremented correctly when an async load ends.
        while (this->_loadsInProgress.load(std::memory_order::memory_order_acquire) > 0) {
            this->_externals.pAssetAccessor->tick();
        }
    }

    const ViewUpdateResult& Tileset::updateView(const Camera& camera) {
        uint32_t previousFrameNumber = this->_previousFrameNumber; 
        uint32_t currentFrameNumber = previousFrameNumber + 1;

        ViewUpdateResult& result = this->_updateResult;
        // result.tilesLoading = 0;
        result.tilesToRenderThisFrame.clear();
        // result.newTilesToRenderThisFrame.clear();
        result.tilesToNoLongerRenderThisFrame.clear();

        Tile* pRootTile = this->getRootTile();
        if (!pRootTile) {
            return result;
        }

        this->_loadQueueHigh.clear();
        this->_loadQueueMedium.clear();
        this->_loadQueueLow.clear();

        this->_visitTileIfVisible(previousFrameNumber, currentFrameNumber, camera, false, *pRootTile, result);
        this->_unloadCachedTiles();
        this->_processLoadQueue();

        this->_previousFrameNumber = currentFrameNumber;

        return result;
    }

    void Tileset::notifyTileDoneLoading(Tile* /*pTile*/) {
        --this->_loadsInProgress;
    }

    void Tileset::loadTilesFromJson(Tile& rootTile, const nlohmann::json& tilesetJson, const glm::dmat4& parentTransform, TileRefine parentRefine, const TileContext& context) const {
        this->_createTile(rootTile, tilesetJson["root"], parentTransform, parentRefine, context);
    }

    std::unique_ptr<IAssetRequest> Tileset::requestTileContent(Tile& tile) {
        std::string url = this->getResolvedContentUrl(tile);
        if (url.empty()) {
            return nullptr;
        }

        IAssetAccessor* pAssetAccessor = this->getExternals().pAssetAccessor;
        return pAssetAccessor->requestAsset(url, tile.getContext()->requestHeaders);
    }

    void Tileset::addContext(std::unique_ptr<TileContext>&& pNewContext) {
        this->_contexts.push_back(std::move(pNewContext));
    }

    void Tileset::_ionResponseReceived(IAssetRequest* pRequest) {
        IAssetResponse* pResponse = pRequest->response();
        if (!pResponse) {
            // TODO: report the lack of response. Network error? Can this even happen?
            this->notifyTileDoneLoading(nullptr);
            return;
        }

        if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
            // TODO: report error response.
            this->notifyTileDoneLoading(nullptr);
            return;
        }

        gsl::span<const uint8_t> data = pResponse->data();

        using nlohmann::json;
        json ionResponse = json::parse(data.begin(), data.end());

        std::string url = ionResponse.value<std::string>("url", "");
        std::string accessToken = ionResponse.value<std::string>("accessToken", "");
        
        std::string type = ionResponse.value<std::string>("type", "");
        if (type == "TERRAIN") {
            // For terrain resources, we need to append `/layer.json` to the end of the URL.
            url = Uri::resolve(url, "layer.json", true);
        } else if (type != "3DTILES") {
            // TODO: report unsupported type.
            this->notifyTileDoneLoading(nullptr);
            return;
        }

        this->_contexts.push_back(std::move(std::make_unique<TileContext>()));
        TileContext& context = *this->_contexts[0];

        context.pTileset = this;
        context.baseUrl = url;
        context.requestHeaders.push_back(std::make_pair("Authorization", "Bearer " + accessToken));

        // When we assign _pTilesetRequest, the previous request and response
        // that we're currently handling may immediately be deleted.
        pRequest = nullptr;
        pResponse = nullptr;
        this->_pIonRequest.reset();

        this->_pTilesetJsonRequest = this->_externals.pAssetAccessor->requestAsset(context.baseUrl, context.requestHeaders);
        this->_pTilesetJsonRequest->bind(std::bind(&Tileset::_tilesetJsonResponseReceived, this, std::placeholders::_1));
    }

    void Tileset::_tilesetJsonResponseReceived(IAssetRequest* pRequest) {
        IAssetResponse* pResponse = pRequest->response();
        if (!pResponse) {
            // TODO: report the lack of response. Network error? Can this even happen?
            this->_pTilesetJsonRequest.reset();
            this->notifyTileDoneLoading(nullptr);
            return;
        }

        if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
            // TODO: report error response.
            this->_pTilesetJsonRequest.reset();
            this->notifyTileDoneLoading(nullptr);
            return;
        }

        if (this->_contexts.empty()) {
            this->_contexts.push_back(std::move(std::make_unique<TileContext>()));
        }

        TileContext& context = *this->_contexts[0];

        context.pTileset = this;
        context.baseUrl = this->_pTilesetJsonRequest->url();

        gsl::span<const uint8_t> data = pResponse->data();

        const TilesetExternals& externals = this->getExternals();
        externals.pTaskProcessor->startTask([data, &externals, this, &context]() {
            using nlohmann::json;
            json tileset = json::parse(data.begin(), data.end());

            std::unique_ptr<Tile> pRootTile = std::make_unique<Tile>();
            pRootTile->setContext(&context);

            json::iterator rootIt = tileset.find("root");
            if (rootIt != tileset.end()) {
                json& rootJson = *rootIt;
                this->_createTile(*pRootTile, rootJson, glm::dmat4(1.0), TileRefine::Replace, context);
            } else if (tileset.value("format", "") == "quantized-mesh-1.0") {
                this->_createTerrainTile(*pRootTile, tileset, context);
            }

            this->_pRootTile = std::move(pRootTile);

            this->getOverlays().createTileProviders(this->_externals);

            this->_pTilesetJsonRequest.reset();
            this->notifyTileDoneLoading(nullptr);
        });
    }

    void Tileset::_createTile(Tile& tile, const nlohmann::json& tileJson, const glm::dmat4& parentTransform, TileRefine parentRefine, const TileContext& context) const {
        using nlohmann::json;

        if (!tileJson.is_object())
        {
            return;
        }

        tile.setContext(const_cast<TileContext*>(&context));

        std::optional<glm::dmat4x4> tileTransform = TilesetJson::getTransformProperty(tileJson, "transform");
        glm::dmat4x4 transform = parentTransform * tileTransform.value_or(glm::dmat4x4(1.0));
        tile.setTransform(transform);

        json::const_iterator contentIt = tileJson.find("content");
        json::const_iterator childrenIt = tileJson.find("children");

        if (contentIt != tileJson.end())
        {
            json::const_iterator uriIt = contentIt->find("uri");
            if (uriIt == contentIt->end()) {
                uriIt = contentIt->find("url");
            }

            if (uriIt != contentIt->end()) {
                tile.setTileID(Uri::resolve(context.baseUrl, *uriIt));
            }

            std::optional<BoundingVolume> contentBoundingVolume = TilesetJson::getBoundingVolumeProperty(*contentIt, "boundingVolume");
            if (contentBoundingVolume) {
                tile.setContentBoundingVolume(transformBoundingVolume(transform, contentBoundingVolume.value()));
            }
        }

        std::optional<BoundingVolume> boundingVolume = TilesetJson::getBoundingVolumeProperty(tileJson, "boundingVolume");
        if (!boundingVolume) {
            // TODO: report missing required property
            return;
        }

        std::optional<double> geometricError = TilesetJson::getScalarProperty(tileJson, "geometricError");
        if (!geometricError) {
            // TODO: report missing required property
            return;
        }

        tile.setBoundingVolume(transformBoundingVolume(transform, boundingVolume.value()));
        //tile->setBoundingVolume(boundingVolume.value());
        tile.setGeometricError(geometricError.value());

        std::optional<BoundingVolume> viewerRequestVolume = TilesetJson::getBoundingVolumeProperty(tileJson, "viewerRequestVolume");
        if (viewerRequestVolume) {
            tile.setViewerRequestVolume(transformBoundingVolume(transform, viewerRequestVolume.value()));
        }
        
        json::const_iterator refineIt = tileJson.find("refine");
        if (refineIt != tileJson.end()) {
            const std::string& refine = *refineIt;
            if (refine == "REPLACE") {
                tile.setRefine(TileRefine::Replace);
            } else if (refine == "ADD") {
                tile.setRefine(TileRefine::Add);
            } else {
                // TODO: report invalid value
            }
        } else {
            tile.setRefine(parentRefine);
        }

        if (childrenIt != tileJson.end())
        {
            const json& childrenJson = *childrenIt;
            if (!childrenJson.is_array())
            {
                return;
            }

            tile.createChildTiles(childrenJson.size());
            gsl::span<Tile> childTiles = tile.getChildren();

            for (size_t i = 0; i < childrenJson.size(); ++i) {
                const json& childJson = childrenJson[i];
                Tile& child = childTiles[i];
                child.setParent(&tile);
                this->_createTile(child, childJson, transform, tile.getRefine(), context);
            }
        }
    }

    void Tileset::_createTerrainTile(Tile& tile, const nlohmann::json& layerJson, TileContext& context) {
        context.requestHeaders.push_back(std::make_pair("Accept", "application/vnd.quantized-mesh,application/octet-stream;q=0.9,*/*;q=0.01"));
        context.version = layerJson.value("version", "");

        std::vector<double> bounds = layerJson.value<std::vector<double>>("bounds", std::vector<double>());

        std::string projectionString = layerJson.value<std::string>("projection", "EPSG:4326");
        CesiumGeospatial::Projection projection;
        CesiumGeospatial::GlobeRectangle quadtreeRectangleGlobe(0.0, 0.0, 0.0, 0.0);
        CesiumGeometry::Rectangle quadtreeRectangleProjected(0.0, 0.0, 0.0, 0.0);
        uint32_t quadtreeXTiles;

        if (projectionString == "EPSG:4326") {
            CesiumGeospatial::GeographicProjection geographic;
            projection = geographic;
            quadtreeRectangleGlobe = bounds.size() >= 4
                ? CesiumGeospatial::GlobeRectangle::fromDegrees(bounds[0], bounds[1], bounds[2], bounds[3])
                : GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
            quadtreeRectangleProjected = geographic.project(quadtreeRectangleGlobe);
            quadtreeXTiles = 2;
        } else if (projectionString == "EPSG:3857") {
            CesiumGeospatial::WebMercatorProjection webMercator;
            projection = webMercator;
            quadtreeRectangleGlobe = bounds.size() >= 4
                ? CesiumGeospatial::GlobeRectangle::fromDegrees(bounds[0], bounds[1], bounds[2], bounds[3])
                : WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;
            quadtreeRectangleProjected = webMercator.project(quadtreeRectangleGlobe);
            quadtreeXTiles = 1;
        } else {
            // Unsupported projection
            // TODO: report error
            return;
        }

        CesiumGeometry::QuadtreeTilingScheme tilingScheme(quadtreeRectangleProjected, quadtreeXTiles, 1);

        context.implicitContext = {
            layerJson.value<std::vector<std::string>>("tiles", std::vector<std::string>()),
            tilingScheme,
            projection,
            CesiumGeometry::QuadtreeTileAvailability(tilingScheme, layerJson.value<uint32_t>("maxzoom", 30))
        };

        std::vector<std::string> extensions = layerJson.value<std::vector<std::string>>("extensions", std::vector<std::string>());

        // Request normals and metadata if they're available
        std::string extensionsToRequest;

        if (std::find(extensions.begin(), extensions.end(), "octvertexnormals") != extensions.end()) {
            if (!extensionsToRequest.empty()) {
                extensionsToRequest += "-";
            }
            extensionsToRequest += "octvertexnormals";
        }

        if (std::find(extensions.begin(), extensions.end(), "metadata") != extensions.end()) {
            if (!extensionsToRequest.empty()) {
                extensionsToRequest += "-";
            }
            extensionsToRequest += "metadata";
        }

        if (!extensionsToRequest.empty()) {
            for (std::string& url : context.implicitContext.value().tileTemplateUrls) {
                url = Uri::addQuery(url, "extensions", extensionsToRequest);
            }
        }

        tile.setContext(&context);
        tile.setBoundingVolume(BoundingRegionWithLooseFittingHeights(BoundingRegion(
            quadtreeRectangleGlobe,
            -1000.0,
            -9000.0
        )));
        tile.setGeometricError(999999999.0);
        tile.createChildTiles(quadtreeXTiles);

        for (uint32_t i = 0; i < quadtreeXTiles; ++i) {
            Tile& childTile = tile.getChildren()[i];
            QuadtreeTileID id(0, i, 0);

            childTile.setContext(&context);
            childTile.setParent(&tile);
            childTile.setTileID(id);
            childTile.setBoundingVolume(BoundingRegionWithLooseFittingHeights(BoundingRegion(
                unprojectRectangleSimple(projection, tilingScheme.tileToRectangle(id)),
                -1000.0,
                -9000.0
            )));
            childTile.setGeometricError(
                8.0 * (
                    Ellipsoid::WGS84.getRadii().x *
                    2.0 *
                    CesiumUtility::Math::ONE_PI *
                    0.25
                ) / (
                    65 * quadtreeXTiles
                )
            );
        }
    }

    static void markTileNonRendered(TileSelectionState::Result lastResult, Tile& tile, ViewUpdateResult& result) {
        if (lastResult == TileSelectionState::Result::Rendered) {
            result.tilesToNoLongerRenderThisFrame.push_back(&tile);
        }
    }

    static void markTileNonRendered(uint32_t lastFrameNumber, Tile& tile, ViewUpdateResult& result) {
        TileSelectionState::Result lastResult = tile.getLastSelectionState().getResult(lastFrameNumber);
        markTileNonRendered(lastResult, tile, result);
    }

    static void markChildrenNonRendered(int32_t lastFrameNumber, TileSelectionState::Result lastResult, Tile& tile, ViewUpdateResult& result) {
        if (lastResult == TileSelectionState::Result::Refined) {
            for (Tile& child : tile.getChildren()) {
                TileSelectionState::Result childLastResult = child.getLastSelectionState().getResult(lastFrameNumber);
                markTileNonRendered(childLastResult, child, result);
                markChildrenNonRendered(lastFrameNumber, childLastResult, child, result);
            }
        }
    }

    static void markChildrenNonRendered(uint32_t lastFrameNumber, Tile& tile, ViewUpdateResult& result) {
        TileSelectionState::Result lastResult = tile.getLastSelectionState().getResult(lastFrameNumber);
        markChildrenNonRendered(lastFrameNumber, lastResult, tile, result);
    }

    static void markTileAndChildrenNonRendered(int32_t lastFrameNumber, Tile& tile, ViewUpdateResult& result) {
        TileSelectionState::Result lastResult = tile.getLastSelectionState().getResult(lastFrameNumber);
        markTileNonRendered(lastResult, tile, result);
        markChildrenNonRendered(lastFrameNumber, lastResult, tile, result);
    }

    // Visits a tile for possible rendering. When we call this function with a tile:
    //   * It is not yet known whether the tile is visible.
    //   * Its parent tile does _not_ meet the SSE (unless ancestorMeetsSse=true, see comments below).
    //   * The tile may or may not be renderable.
    //   * The tile has not yet been added to a load queue.
    Tileset::TraversalDetails Tileset::_visitTileIfVisible(
        uint32_t lastFrameNumber,
        uint32_t currentFrameNumber,
        const Camera& camera,
        bool ancestorMeetsSse,
        Tile& tile,
        ViewUpdateResult& result
    ) {
        tile.update(lastFrameNumber, currentFrameNumber);
        this->_markTileVisited(tile);

        const BoundingVolume& boundingVolume = tile.getBoundingVolume();
        if (!camera.isBoundingVolumeVisible(boundingVolume)) {
            markTileAndChildrenNonRendered(lastFrameNumber, tile, result);
            tile.setLastSelectionState(TileSelectionState(currentFrameNumber, TileSelectionState::Result::Culled));

            // Preload this culled sibling if requested.
            if (this->_options.preloadSiblings) {
                this->_loadQueueLow.push_back(&tile);
            }

            return TraversalDetails();
        }

        return this->_visitTile(lastFrameNumber, currentFrameNumber, camera, ancestorMeetsSse, tile, result);
    }

    // Visits a tile for possible rendering. When we call this function with a tile:
    //   * The tile has previously been determined to be visible.
    //   * Its parent tile does _not_ meet the SSE (unless ancestorMeetsSse=true, see comments below).
    //   * The tile may or may not be renderable.
    //   * The tile has not yet been added to a load queue.
    Tileset::TraversalDetails Tileset::_visitTile(
        uint32_t lastFrameNumber,
        uint32_t currentFrameNumber,
        const Camera& camera,
        bool ancestorMeetsSse,
        Tile& tile,
        ViewUpdateResult& result
    ) {
        TileSelectionState lastFrameSelectionState = tile.getLastSelectionState();

        // If this is a leaf tile, just render it (it's already been deemed visible).
        gsl::span<Tile> children = tile.getChildren();
        if (children.size() == 0) {
            // Render this leaf tile.
            tile.setLastSelectionState(TileSelectionState(currentFrameNumber, TileSelectionState::Result::Rendered));
            result.tilesToRenderThisFrame.push_back(&tile);
            this->_loadQueueMedium.push_back(&tile);

            TraversalDetails traversalDetails;
            traversalDetails.allAreRenderable = tile.isRenderable();
            traversalDetails.anyWereRenderedLastFrame = lastFrameSelectionState.getResult(lastFrameNumber) == TileSelectionState::Result::Rendered;
            traversalDetails.notYetRenderableCount = traversalDetails.allAreRenderable ? 0 : 1;
            return traversalDetails;
        }

        const BoundingVolume& boundingVolume = tile.getBoundingVolume();
        double distanceSquared = camera.computeDistanceSquaredToBoundingVolume(boundingVolume);
        double distance = sqrt(distanceSquared);

        // Does this tile meet the screen-space error?
        double sse = camera.computeScreenSpaceError(tile.getGeometricError(), distance);
        bool meetsSse = sse < this->_options.maximumScreenSpaceError;

        // If we're forbidding holes, don't refine if any children are still loading.
        bool waitingForChildren = false;
        if (this->_options.forbidHoles) {
            for (Tile& child : children) {
                if (!child.isRenderable()) {
                    waitingForChildren = true;
                    this->_loadQueueMedium.push_back(&child);
                }
            }
        }

        if (meetsSse || ancestorMeetsSse || waitingForChildren) {
            // This tile (or an ancestor) is the one we want to render this frame, but we'll do different things depending
            // on the state of this tile and on what we did _last_ frame.

            // We can render it if _any_ of the following are true:
            // 1. We rendered it (or kicked it) last frame.
            // 2. This tile was culled last frame, or it wasn't even visited because an ancestor was culled.
            // 3. The tile is done loading and ready to render.
            //
            // Note that even if we decide to render a tile here, it may later get "kicked" in favor of an ancestor.
            TileSelectionState::Result originalResult = lastFrameSelectionState.getOriginalResult(lastFrameNumber);
            bool oneRenderedLastFrame = originalResult == TileSelectionState::Result::Rendered;
            bool twoCulledOrNotVisited = originalResult == TileSelectionState::Result::Culled || originalResult == TileSelectionState::Result::None;
            bool threeCompletelyLoaded = tile.isRenderable();

            bool renderThisTile = oneRenderedLastFrame || twoCulledOrNotVisited || threeCompletelyLoaded;
            if (renderThisTile) {
                // Only load this tile if it (not just an ancestor) meets the SSE.
                if (meetsSse) {
                    this->_loadQueueMedium.push_back(&tile);
                }

                markChildrenNonRendered(lastFrameNumber, tile, result);
                tile.setLastSelectionState(TileSelectionState(currentFrameNumber, TileSelectionState::Result::Rendered));
                result.tilesToRenderThisFrame.push_back(&tile);

                TraversalDetails traversalDetails;
                traversalDetails.allAreRenderable = tile.isRenderable();
                traversalDetails.anyWereRenderedLastFrame = lastFrameSelectionState.getResult(lastFrameNumber) == TileSelectionState::Result::Rendered;
                traversalDetails.notYetRenderableCount = traversalDetails.allAreRenderable ? 0 : 1;

                return traversalDetails;
            }

            // Otherwise, we can't render this tile (or blank space where it would be) because doing so would cause detail to disappear
            // that was visible last frame. Instead, keep rendering any still-visible descendants that were rendered
            // last frame and render nothing for newly-visible descendants. E.g. if we were rendering level 15 last
            // frame but this frame we want level 14 and the closest renderable level <= 14 is 0, rendering level
            // zero would be pretty jarring so instead we keep rendering level 15 even though its SSE is better
            // than required. So fall through to continue traversal...
            ancestorMeetsSse = true;

            // Load this blocker tile with high priority, but only if this tile (not just an ancestor) meets the SSE.
            if (meetsSse) {
                this->_loadQueueHigh.push_back(&tile);
            }
        }

        // Refine!

        bool queuedForLoad = false;

        // If this tile uses additive refinement, we need to render this tile in addition to its children.
        if (tile.getRefine() == TileRefine::Add) {
            result.tilesToRenderThisFrame.push_back(&tile);
            this->_loadQueueMedium.push_back(&tile);
            queuedForLoad = true;
        }

        size_t firstRenderedDescendantIndex = result.tilesToRenderThisFrame.size();
        size_t loadIndexLow = this->_loadQueueLow.size();
        size_t loadIndexMedium = this->_loadQueueMedium.size();
        size_t loadIndexHigh = this->_loadQueueHigh.size();

        TraversalDetails traversalDetails = this->_visitVisibleChildrenNearToFar(lastFrameNumber, currentFrameNumber, camera, ancestorMeetsSse, tile, result);

        if (firstRenderedDescendantIndex == result.tilesToRenderThisFrame.size()) {
            // No descendant tiles were added to the render list by the function above, meaning they were all
            // culled even though this tile was deemed visible. That's pretty common.
            // Nothing else to do except mark this tile refined and return.
            TraversalDetails noChildrenTraversalDetails;

            if (tile.getRefine() == TileRefine::Add) {
                noChildrenTraversalDetails.allAreRenderable = tile.isRenderable();
                noChildrenTraversalDetails.anyWereRenderedLastFrame = lastFrameSelectionState.getResult(lastFrameNumber) == TileSelectionState::Result::Rendered;
                noChildrenTraversalDetails.notYetRenderableCount = traversalDetails.allAreRenderable ? 0 : 1;
            } else {
                markTileNonRendered(lastFrameNumber, tile, result);
            }

            tile.setLastSelectionState(TileSelectionState(currentFrameNumber, TileSelectionState::Result::Refined));
            return noChildrenTraversalDetails;
        }

        // At least one descendant tile was added to the render list.
        // The traversalDetails tell us what happened while visiting the children.
        if (!traversalDetails.allAreRenderable && !traversalDetails.anyWereRenderedLastFrame) {
            // Some of our descendants aren't ready to render yet, and none were rendered last frame,
            // so kick them all out of the render list and render this tile instead. Continue to load them though!

            std::vector<Tile*>& renderList = result.tilesToRenderThisFrame;
            
            // Mark the rendered descendants and their ancestors - up to this tile - as kicked.
            for (size_t i = firstRenderedDescendantIndex; i < renderList.size(); ++i) {
                Tile* pWorkTile = renderList[i];
                while (
                    pWorkTile != nullptr &&
                    !pWorkTile->getLastSelectionState().wasKicked(currentFrameNumber) &&
                    pWorkTile != &tile
                ) {
                    pWorkTile->getLastSelectionState().kick();
                    pWorkTile = pWorkTile->getParent();
                }
            }

            // Remove all descendants from the render list and add this tile.
            renderList.erase(renderList.begin() + firstRenderedDescendantIndex, renderList.end());
            renderList.push_back(&tile);
            tile.setLastSelectionState(TileSelectionState(currentFrameNumber, TileSelectionState::Result::Rendered));

            // If we're waiting on heaps of descendants, the above will take too long. So in that case,
            // load this tile INSTEAD of loading any of the descendants, and tell the up-level we're only waiting
            // on this tile. Keep doing this until we actually manage to render this tile.
            bool wasRenderedLastFrame = lastFrameSelectionState.getResult(lastFrameNumber) == TileSelectionState::Result::Rendered;
            bool wasReallyRenderedLastFrame = wasRenderedLastFrame && tile.isRenderable();

            if (!wasReallyRenderedLastFrame && traversalDetails.notYetRenderableCount > this->_options.loadingDescendantLimit) {
                // Remove all descendants from the load queues.
                this->_loadQueueLow.erase(this->_loadQueueLow.begin() + loadIndexLow, this->_loadQueueLow.end());
                this->_loadQueueMedium.erase(this->_loadQueueMedium.begin() + loadIndexMedium, this->_loadQueueMedium.end());
                this->_loadQueueHigh.erase(this->_loadQueueHigh.begin() + loadIndexHigh, this->_loadQueueHigh.end());

                if (!queuedForLoad) {
                    this->_loadQueueMedium.push_back(&tile);
                }

                traversalDetails.notYetRenderableCount = tile.isRenderable() ? 0 : 1;
                queuedForLoad = true;
            }

            traversalDetails.allAreRenderable = tile.isRenderable();
            traversalDetails.anyWereRenderedLastFrame = wasRenderedLastFrame;
        } else {
            if (tile.getRefine() != TileRefine::Add) {
                markTileNonRendered(lastFrameNumber, tile, result);
            }
            tile.setLastSelectionState(TileSelectionState(currentFrameNumber, TileSelectionState::Result::Refined));
        }

        if (this->_options.preloadAncestors && !queuedForLoad) {
            this->_loadQueueLow.push_back(&tile);
        }

        return traversalDetails;
    }

    Tileset::TraversalDetails Tileset::_visitVisibleChildrenNearToFar(
        uint32_t lastFrameNumber,
        uint32_t currentFrameNumber,
        const Camera& camera,
        bool ancestorMeetsSse,
        Tile& tile,
        ViewUpdateResult& result
    ) {
        TraversalDetails traversalDetails;

        // TODO: actually visit near-to-far, rather than in order of occurrence.
        gsl::span<Tile> children = tile.getChildren();
        for (Tile& child : children) {
            TraversalDetails childTraversal = this->_visitTileIfVisible(
                lastFrameNumber,
                currentFrameNumber,
                camera,
                ancestorMeetsSse,
                child,
                result
            );

            traversalDetails.allAreRenderable &= childTraversal.allAreRenderable;
            traversalDetails.anyWereRenderedLastFrame |= childTraversal.anyWereRenderedLastFrame;
            traversalDetails.notYetRenderableCount += childTraversal.notYetRenderableCount;
        }

        return traversalDetails;
    }

    static void processQueue(const std::vector<Tile*>& queue, std::atomic<uint32_t>& loadsInProgress, uint32_t maximumLoadsInProgress) {
        if (loadsInProgress >= maximumLoadsInProgress) {
            return;
        }

        for (Tile* pTile : queue) {
            if (pTile->getState() == Tile::LoadState::Unloaded) {
                ++loadsInProgress;
                pTile->loadContent();

                if (loadsInProgress >= maximumLoadsInProgress) {
                    break;
                }
            }
        }
    }

    void Tileset::_processLoadQueue() {
        processQueue(this->_loadQueueHigh, this->_loadsInProgress, this->_options.maximumSimultaneousTileLoads);
        processQueue(this->_loadQueueMedium, this->_loadsInProgress, this->_options.maximumSimultaneousTileLoads);
        processQueue(this->_loadQueueLow, this->_loadsInProgress, this->_options.maximumSimultaneousTileLoads);
    }

    void Tileset::_unloadCachedTiles() {
        Tile* pTile = this->_loadedTiles.head();

        while (this->_loadedTiles.size() > this->_options.maximumCachedTiles) {
            if (pTile == nullptr || pTile == this->_pRootTile.get()) {
                // We've either removed all tiles or the next tile is the root.
                // The root tile marks the beginning of the tiles that were used
                // for rendering last frame.
                break;
            }

            Tile* pNext = this->_loadedTiles.next(*pTile);

            bool removed = pTile->unloadContent();
            if (removed) {
                this->_loadedTiles.remove(*pTile);
            }

            pTile = pNext;
        }
    }

    void Tileset::_markTileVisited(Tile& tile) {
        this->_loadedTiles.insertAtTail(tile);
    }

    std::string Tileset::getResolvedContentUrl(const Tile& tile) const {
        struct Operation {
            const TileContext& context;

            std::string operator()(const std::string& url) {
                return url;
            }

            std::string operator()(const QuadtreeTileID& quadtreeID) {
                if (!this->context.implicitContext) {
                    return std::string();
                }

                return Uri::substituteTemplateParameters(context.implicitContext.value().tileTemplateUrls[0], [this, &quadtreeID](const std::string& placeholder) -> std::string {
                    if (placeholder == "level" || placeholder == "z") {
                        return std::to_string(quadtreeID.level);
                    } else if (placeholder == "x") {
                        return std::to_string(quadtreeID.x);
                    } else if (placeholder == "y") {
                        return std::to_string(quadtreeID.y);
                    } else if (placeholder == "version") {
                        return this->context.version.value_or(std::string());
                    }

                    return placeholder;
                });
            }

            std::string operator()(const OctreeTileID& octreeID) {
                if (!this->context.implicitContext) {
                    return std::string();
                }

                return Uri::substituteTemplateParameters(context.implicitContext.value().tileTemplateUrls[0], [this, &octreeID](const std::string& placeholder) -> std::string {
                    if (placeholder == "level") {
                        return std::to_string(octreeID.level);
                    } else if (placeholder == "x") {
                        return std::to_string(octreeID.x);
                    } else if (placeholder == "y") {
                        return std::to_string(octreeID.y);
                    } else if (placeholder == "z") {
                        return std::to_string(octreeID.z);
                    } else if (placeholder == "version") {
                        return this->context.version.value_or(std::string());
                    }

                    return placeholder;
                });
            }
        };

        std::string url = std::visit(Operation { *tile.getContext() }, tile.getTileID());
        if (url.empty()) {
            return url;
        }

        return Uri::resolve(tile.getContext()->baseUrl, url, true);
    }

}
