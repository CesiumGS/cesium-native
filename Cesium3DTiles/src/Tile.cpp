#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/IAssetAccessor.h"
#include "Cesium3DTiles/IAssetResponse.h"
#include "Cesium3DTiles/TileContentFactory.h"
#include <chrono>
#include <algorithm>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace std::string_literals;

namespace Cesium3DTiles {

    Tile::Tile() :
        _loadedTilesLinks(),
        _pTileset(nullptr),
        _pParent(nullptr),
        _children(),
        _boundingVolume(OrientedBoundingBox(glm::dvec3(), glm::dmat4())),
        _viewerRequestVolume(),
        _geometricError(0.0),
        _refine(),
        _transform(1.0),
        _id(""s),
        _contentBoundingVolume(),
        _state(LoadState::Unloaded),
        _pContentRequest(nullptr),
        _pContent(nullptr),
        _pRendererResources(nullptr),
        _lastSelectionState()
    {
    }

    Tile::~Tile() {
        this->prepareToDestroy();

        // Wait for this tile to exit the "Destroying" state, indicating that
        // work happening in the loading thread has concluded.
        if (this->getState() == LoadState::Destroying) {
            const auto timeoutSeconds = std::chrono::seconds(5LL);

            auto start = std::chrono::steady_clock::now();
            while (this->getState() == LoadState::Destroying) {
                auto duration = std::chrono::steady_clock::now() - start;
                if (duration > timeoutSeconds) {
                    // TODO: report timeout, because it may be followed by a crash.
                    return;
                }
                this->_pTileset->getExternals().pAssetAccessor->tick();
            }
        }

        this->unloadContent();
    }

    Tile::Tile(Tile&& rhs) noexcept :
        _loadedTilesLinks(),
        _pTileset(rhs._pTileset),
        _pParent(rhs._pParent),
        _children(std::move(rhs._children)),
        _boundingVolume(rhs._boundingVolume),
        _viewerRequestVolume(rhs._viewerRequestVolume),
        _geometricError(rhs._geometricError),
        _refine(rhs._refine),
        _transform(rhs._transform),
        _id(std::move(rhs._id)),
        _contentBoundingVolume(rhs._contentBoundingVolume),
        _state(rhs.getState()),
        _pContentRequest(std::move(rhs._pContentRequest)),
        _pContent(std::move(rhs._pContent)),
        _pRendererResources(rhs._pRendererResources),
        _lastSelectionState(rhs._lastSelectionState)
    {
    }

    Tile& Tile::operator=(Tile&& rhs) noexcept {
        if (this != &rhs) {
            this->_loadedTilesLinks = std::move(rhs._loadedTilesLinks);
            this->_pTileset = rhs._pTileset;
            this->_pParent = rhs._pParent;
            this->_children = std::move(rhs._children);
            this->_boundingVolume = rhs._boundingVolume;
            this->_viewerRequestVolume = rhs._viewerRequestVolume;
            this->_geometricError = rhs._geometricError;
            this->_refine = rhs._refine;
            this->_transform = rhs._transform;
            this->_id = std::move(rhs._id);
            this->_contentBoundingVolume = rhs._contentBoundingVolume;
            this->setState(rhs.getState());
            this->_pContentRequest = std::move(rhs._pContentRequest);
            this->_pContent = std::move(rhs._pContent);
            this->_pRendererResources = rhs._pRendererResources;
            this->_lastSelectionState = rhs._lastSelectionState;
        }

        return *this;
    }

    void Tile::prepareToDestroy() {
        if (this->_pContentRequest) {
            this->_pContentRequest->cancel();
        }

        // Atomically change any tile in the ContentLoading state to the Destroying state.
        // Tiles in other states remain in those states.
        LoadState stateToChange = LoadState::ContentLoading;
        this->_state.compare_exchange_strong(stateToChange, LoadState::Destroying);
    }

    void Tile::createChildTiles(size_t count) {
        if (this->_children.size() > 0) {
            throw std::runtime_error("Children already created.");
        }
        this->_children.resize(count);
    }

    void Tile::createChildTiles(std::vector<Tile>&& children) {
        if (this->_children.size() > 0) {
            // TODO
            return;
            //throw std::runtime_error("Children already created.");
        }
        this->_children = std::move(children);
    }

    void Tile::setTileID(const TileID& id) {
        this->_id = id;
    }

    bool Tile::isRenderable() const {
        // A tile whose content is an external tileset has no renderable content. If
        // we select such a tile for rendering, we'll end up rendering nothing even though
        // the tile's parent and its children may both have content. End result: when the
        // tile's parent refines, we get a hole in the content until the children load.
        
        // So, we explicitly treat external tilesets as non-renderable.

        return
            this->getState() >= LoadState::ContentLoaded &&
            (!this->_pContent || this->_pContent->getType() != ExternalTilesetContent::TYPE);
    }

    void Tile::loadContent() {
        if (this->getState() != LoadState::Unloaded) {
            return;
        }

        this->setState(LoadState::ContentLoading);

        Tileset& tileset = *this->getTileset();

        CesiumGeospatial::BoundingRegion* pRegion = std::get_if<CesiumGeospatial::BoundingRegion>(&this->_boundingVolume);
        CesiumGeospatial::BoundingRegionWithLooseFittingHeights* pLooseRegion = std::get_if<CesiumGeospatial::BoundingRegionWithLooseFittingHeights>(&this->_boundingVolume);
        
        const CesiumGeospatial::GlobeRectangle* pRectangle = nullptr;
        if (pRegion) {
            pRectangle = &pRegion->getRectangle();
        } else if (pLooseRegion) {
            pRectangle = &pLooseRegion->getBoundingRegion().getRectangle();
        }

        // TODO: support overlay mapping for tiles that aren't region-based.
        if (pRectangle) {
            // Map overlays to this tile.
            
            RasterOverlayCollection& overlays = tileset.getOverlays();
            gsl::span<RasterOverlayTileProvider*> providers = overlays.getTileProviders();
            
            for (RasterOverlayTileProvider* pProvider : providers) {
                this->loadOverlays(*pProvider, *pRectangle);
            }
        }
        
        this->_pContentRequest = tileset.requestTileContent(*this);
        this->_pContentRequest->bind(std::bind(&Tile::contentResponseReceived, this, std::placeholders::_1));
    }

    void Tile::loadReadyContent(std::unique_ptr<TileContent> pReadyContent) {
        if (this->getState() != LoadState::Unloaded) {
            return;
        }

        this->_pContent = std::move(pReadyContent);

        const TilesetExternals& externals = this->_pTileset->getExternals();
        if (externals.pPrepareRendererResources) {
            this->_pRendererResources = externals.pPrepareRendererResources->prepareInLoadThread(*this);
        }
        else {
            this->_pRendererResources = nullptr;
        }

        this->setState(LoadState::ContentLoaded);
    }

    bool Tile::unloadContent() {
        // Cannot unload while an async operation is in progress.
        // Also, don't unload tiles with external tileset content at all, because reloading
        // currently won't work correctly.
        if (
            this->getState() == Tile::LoadState::ContentLoading ||
            (this->getContent() != nullptr && this->getContent()->getType() == ExternalTilesetContent::TYPE)
        ) {
            return false;
        }

        const TilesetExternals& externals = this->_pTileset->getExternals();
        if (externals.pPrepareRendererResources) {
            if (this->getState() == LoadState::ContentLoaded) {
                externals.pPrepareRendererResources->free(*this, this->_pRendererResources, nullptr);
            } else {
                externals.pPrepareRendererResources->free(*this, nullptr, this->_pRendererResources);
            }
        }

        this->_pRendererResources = nullptr;
        this->_pContentRequest.reset();
        this->_pContent.reset();
        this->setState(LoadState::Unloaded);

        return true;
    }

    void Tile::cancelLoadContent() {
        if (this->_pContentRequest) {
            this->_pContentRequest->cancel();
            this->_pContentRequest.release();

            if (this->getState() == LoadState::ContentLoading) {
                this->setState(LoadState::Unloaded);
            }
        }
    }

    void Tile::update(uint32_t /*previousFrameNumber*/, uint32_t /*currentFrameNumber*/) {
        const TilesetExternals& externals = this->_pTileset->getExternals();

        if (this->getState() == LoadState::ContentLoaded) {
            if (externals.pPrepareRendererResources) {
                this->_pRendererResources = externals.pPrepareRendererResources->prepareInMainThread(*this, this->getRendererResources());
            }

            TileContent* pContent = this->getContent();
            if (pContent) {
                pContent->finalizeLoad(*this);
            }

            // Free the request now that it is complete.
            this->_pContentRequest.reset();

            this->setState(LoadState::Done);
        }

        if (this->getState() == LoadState::Done) {
            for (RasterMappedTo3DTile& mappedRasterTile : this->_rasterTiles) {
                if (mappedRasterTile.getState() == RasterMappedTo3DTile::AttachmentState::Unattached) {
                    RasterOverlayTile& rasterTile = mappedRasterTile.getRasterTile();
                    rasterTile.loadInMainThread();
                    mappedRasterTile.attachToTile(*this);
                }
            }
        }
    }

    void Tile::setState(LoadState value) {
        this->_state.store(value, std::memory_order::memory_order_release);
    }

    void Tile::contentResponseReceived(IAssetRequest* pRequest) {
        if (this->getState() == LoadState::Destroying) {
            this->_pTileset->notifyTileDoneLoading(this);
            this->setState(LoadState::Failed);
            return;
        }

        if (this->getState() > LoadState::ContentLoading) {
            // This is a duplicate response, ignore it.
            return;
        }

        IAssetResponse* pResponse = pRequest->response();
        if (!pResponse) {
            // TODO: report the lack of response. Network error? Can this even happen?
            this->_pTileset->notifyTileDoneLoading(this);
            this->setState(LoadState::Failed);
            return;
        }

        if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
            // TODO: report error response.
            this->_pTileset->notifyTileDoneLoading(this);
            this->setState(LoadState::Failed);
            return;
        }

        const TilesetExternals& externals = this->_pTileset->getExternals();

        externals.pTaskProcessor->startTask([pResponse, &externals, this]() {
            if (this->getState() == LoadState::Destroying) {
                this->_pTileset->notifyTileDoneLoading(this);
                this->setState(LoadState::Failed);
                return;
            }

            std::unique_ptr<TileContent> pContent = TileContentFactory::createContent(*this, pResponse->data(), this->_pContentRequest->url(), pResponse->contentType());
            if (pContent) {
                this->_pContent = std::move(pContent);

                if (this->getState() == LoadState::Destroying) {
                    this->_pTileset->notifyTileDoneLoading(this);
                    this->setState(LoadState::Failed);
                    return;
                }

                if (externals.pPrepareRendererResources) {
                    this->_pRendererResources = externals.pPrepareRendererResources->prepareInLoadThread(*this);
                }
                else {
                    this->_pRendererResources = nullptr;
                }
            }

            this->_pTileset->notifyTileDoneLoading(this);
            this->setState(LoadState::ContentLoaded);
        });
    }

    // These functions assume the rectangle is still a rectangle after projecting, which is true for Web Mercator -> geographic,
    // but not necessarily true for others.

    static Rectangle projectRectangleSimple(const WebMercatorProjection& projection, const GlobeRectangle& rectangle) {
        glm::dvec3 sw = projection.project(rectangle.getSouthwest());
        glm::dvec3 ne = projection.project(rectangle.getNortheast());

        return Rectangle(sw.x, sw.y, ne.x, ne.y);
    }

    static GlobeRectangle unprojectRectangleSimple(const WebMercatorProjection& projection, const Rectangle& rectangle) {
        Cartographic sw = projection.unproject(rectangle.getLowerLeft());
        Cartographic ne = projection.unproject(rectangle.getUpperRight());

        return GlobeRectangle(sw.longitude, sw.latitude, ne.longitude, ne.latitude);
    }

    void Tile::loadOverlays(RasterOverlayTileProvider& tileProvider, const CesiumGeospatial::GlobeRectangle& tileRectangle) {
        const QuadtreeTilingScheme& imageryTilingScheme = tileProvider.getTilingScheme();
        const WebMercatorProjection* pWebMercatorProjection = std::get_if<WebMercatorProjection>(&tileProvider.getProjection());
        if (!pWebMercatorProjection) {
            // TODO: Currently only Web Mercator is supported
            return;
        }

        const WebMercatorProjection& projection = *pWebMercatorProjection;

        // Use Web Mercator for our texture coordinate computations if this imagery layer uses
        // that projection and the terrain tile falls entirely inside the valid bounds of the
        // projection.
        // bool useWebMercatorT =
        //     pWebMercatorProjection &&
        //     tileRectangle.getNorth() <= WebMercatorProjection::MAXIMUM_LATITUDE &&
        //     tileRectangle.getSouth() >= -WebMercatorProjection::MAXIMUM_LATITUDE;

        const Rectangle& providerRectangleProjected = imageryTilingScheme.getRectangle();

        // TODO: don't assume we can just unproject the corners; it may be more complicated than that with more exotic projections.
        // The ultimate solution is to unproject each vertex, but that requires loading geometry before we can even start loading imagery.
        // We could use this simple approach for lon/lat aligned projections like web mercator, but project each vertex for
        // more complicated projections.
        GlobeRectangle providerRectangle = unprojectRectangleSimple(projection, providerRectangleProjected);

        // Compute the rectangle of the imagery from this imageryProvider that overlaps
        // the geometry tile.  The ImageryProvider and ImageryLayer both have the
        // opportunity to constrain the rectangle.  The imagery TilingScheme's rectangle
        // always fully contains the ImageryProvider's rectangle.
        // TODO: intersect with the imagery layer's bounds, when we have an imagery layer with bounds.

        GlobeRectangle imageryBoundsGeodetic = providerRectangle;
        std::optional<GlobeRectangle> maybeRectangle = tileRectangle.intersect(imageryBoundsGeodetic);
        GlobeRectangle rectangle(0.0, 0.0, 0.0, 0.0);

        if (maybeRectangle) {
            rectangle = maybeRectangle.value();
        } else {
            // There is no overlap between this terrain tile and this imagery
            // provider.  Unless this is the base layer, no skeletons need to be created.
            // We stretch texels at the edge of the base layer over the entire globe.

            // TODO: base layers
            // if (!this.isBaseLayer()) {
            //     return false;
            // }

            GlobeRectangle baseImageryRectangle = imageryBoundsGeodetic;
            GlobeRectangle baseTerrainRectangle = tileRectangle;

            double north, south;
            if (baseTerrainRectangle.getSouth() >= baseImageryRectangle.getNorth()) {
                north = south = baseImageryRectangle.getNorth();
            } else if (baseTerrainRectangle.getNorth() <= baseImageryRectangle.getSouth()) {
                north = south = baseImageryRectangle.getSouth();
            } else {
                south = std::max(
                    baseTerrainRectangle.getSouth(),
                    baseImageryRectangle.getSouth()
                );
                north = std::min(
                    baseTerrainRectangle.getNorth(),
                    baseImageryRectangle.getNorth()
                );
            }

            double east, west;
            if (baseTerrainRectangle.getWest() >= baseImageryRectangle.getEast()) {
                west = east = baseImageryRectangle.getEast();
            } else if (baseTerrainRectangle.getEast() <= baseImageryRectangle.getWest()) {
                west = east = baseImageryRectangle.getWest();
            } else {
                west = std::max(
                    baseTerrainRectangle.getWest(),
                    baseImageryRectangle.getWest()
                );
                east = std::min(
                    baseTerrainRectangle.getEast(),
                    baseImageryRectangle.getEast()
                );
            }

            rectangle = GlobeRectangle(west, south, east, north);
        }

        double latitudeClosestToEquator = 0.0;
        if (rectangle.getSouth() > 0.0) {
            latitudeClosestToEquator = rectangle.getSouth();
        } else if (rectangle.getNorth() < 0.0) {
            latitudeClosestToEquator = rectangle.getNorth();
        }

        // Compute the required level in the imagery tiling scheme.
        // The errorRatio should really be imagerySSE / terrainSSE rather than this hard-coded value.
        // But first we need configurable imagery SSE and we need the rendering to be able to handle more
        // images attached to a terrain tile than there are available texture units.  So that's for the future.
        const double errorRatio = 1.0;
        double targetGeometricError = errorRatio * this->getGeometricError();
        // TODO: dividing by 8 to change the default 3D Tiles SSE (16) back to the terrain SSE (2)
        uint32_t imageryLevel = tileProvider.getLevelWithMaximumTexelSpacing(targetGeometricError / 8.0, latitudeClosestToEquator);
        imageryLevel = std::max(0U, imageryLevel);

        uint32_t maximumLevel = tileProvider.getMaximumLevel();
        if (imageryLevel > maximumLevel) {
            imageryLevel = maximumLevel;
        }

        uint32_t minimumLevel = tileProvider.getMinimumLevel();
        if (imageryLevel < minimumLevel) {
            imageryLevel = minimumLevel;
        }

        std::optional<QuadtreeTileID> southwestTileCoordinatesOpt = imageryTilingScheme.positionToTile(
            projection.project(rectangle.getSouthwest()),
            imageryLevel
        );
        std::optional<QuadtreeTileID> northeastTileCoordinatesOpt = imageryTilingScheme.positionToTile(
            projection.project(rectangle.getNortheast()),
            imageryLevel
        );

        // Because of the intersection, we should always have valid tile coordinates. But give up if we don't.
        if (!southwestTileCoordinatesOpt || !northeastTileCoordinatesOpt) {
            return;
        }

        QuadtreeTileID southwestTileCoordinates = southwestTileCoordinatesOpt.value();
        QuadtreeTileID northeastTileCoordinates = northeastTileCoordinatesOpt.value();

        // If the northeast corner of the rectangle lies very close to the south or west side
        // of the northeast tile, we don't actually need the northernmost or easternmost
        // tiles.
        // Similarly, if the southwest corner of the rectangle lies very close to the north or east side
        // of the southwest tile, we don't actually need the southernmost or westernmost tiles.

        // We define "very close" as being within 1/512 of the width of the tile.
        double veryCloseX = tileRectangle.computeWidth() / 512.0;
        double veryCloseY = tileRectangle.computeHeight() / 512.0;

        GlobeRectangle southwestTileRectangle = unprojectRectangleSimple(projection, imageryTilingScheme.tileToRectangle(southwestTileCoordinates));
        
        if (
            std::abs(southwestTileRectangle.getNorth() - tileRectangle.getSouth()) < veryCloseY &&
            southwestTileCoordinates.y < northeastTileCoordinates.y
        ) {
            ++southwestTileCoordinates.y;
        }

        if (
            std::abs(southwestTileRectangle.getEast() - tileRectangle.getWest()) < veryCloseX &&
            southwestTileCoordinates.x < northeastTileCoordinates.x
        ) {
            ++southwestTileCoordinates.x;
        }

        GlobeRectangle northeastTileRectangle = unprojectRectangleSimple(projection, imageryTilingScheme.tileToRectangle(northeastTileCoordinates));

        if (
            std::abs(northeastTileRectangle.getNorth() - tileRectangle.getSouth()) < veryCloseY &&
            northeastTileCoordinates.y > southwestTileCoordinates.y
        ) {
            --northeastTileCoordinates.y;
        }

        if (
            std::abs(northeastTileRectangle.getWest() - tileRectangle.getEast()) < veryCloseX &&
            northeastTileCoordinates.x > southwestTileCoordinates.x
        ) {
            --northeastTileCoordinates.x;
        }

        // Create TileImagery instances for each imagery tile overlapping this terrain tile.
        // We need to do all texture coordinate computations in the imagery tile's tiling scheme.

        Rectangle terrainRectangle = projectRectangleSimple(projection, tileRectangle);
        double terrainWidth = terrainRectangle.computeWidth();
        double terrainHeight = terrainRectangle.computeHeight();
        Rectangle imageryRectangle = imageryTilingScheme.tileToRectangle(southwestTileCoordinates);
        Rectangle imageryBounds = providerRectangleProjected.intersect(terrainRectangle).value();
        std::optional<Rectangle> clippedImageryRectangle = imageryRectangle.intersect(imageryBounds).value();

        veryCloseX = terrainRectangle.computeWidth() / 512.0;
        veryCloseY = terrainRectangle.computeHeight() / 512.0;

        double minU;
        double maxU = 0.0;

        double minV;
        double maxV = 0.0;

        // If this is the northern-most or western-most tile in the imagery tiling scheme,
        // it may not start at the northern or western edge of the terrain tile.
        // Calculate where it does start.
        // TODO
        // if (
        //     /*!this.isBaseLayer()*/ false &&
        //     std::abs(clippedImageryRectangle.value().getWest() - terrainRectangle.getWest()) >= veryCloseX
        // ) {
        //     maxU = std::min(
        //         1.0,
        //         (clippedImageryRectangle.value().getWest() - terrainRectangle.getWest()) / terrainRectangle.computeWidth()
        //     );
        // }

        // if (
        //     /*!this.isBaseLayer()*/ false &&
        //     std::abs(clippedImageryRectangle.value().getNorth() - terrainRectangle.getNorth()) >= veryCloseY
        // ) {
        //     minV = std::max(
        //         0.0,
        //         (clippedImageryRectangle.value().getNorth() - terrainRectangle.getSouth()) / terrainRectangle.computeHeight()
        //     );
        // }

        double initialMaxV = maxV;

        for (
            uint32_t i = southwestTileCoordinates.x;
            i <= northeastTileCoordinates.x;
            ++i
        ) {
            minU = maxU;

            imageryRectangle = imageryTilingScheme.tileToRectangle(QuadtreeTileID(imageryLevel, i, southwestTileCoordinates.y));
            clippedImageryRectangle = imageryRectangle.intersect(imageryBounds);

            if (!clippedImageryRectangle) {
                continue;
            }

            maxU = std::min(1.0, (clippedImageryRectangle.value().maximumX - terrainRectangle.minimumX) / terrainRectangle.computeWidth());

            // If this is the eastern-most imagery tile mapped to this terrain tile,
            // and there are more imagery tiles to the east of this one, the maxU
            // should be 1.0 to make sure rounding errors don't make the last
            // image fall shy of the edge of the terrain tile.
            if (
                i == northeastTileCoordinates.x &&
                (/*this.isBaseLayer()*/ true || std::abs(clippedImageryRectangle.value().maximumX - terrainRectangle.maximumX) < veryCloseX)
            ) {
                maxU = 1.0;
            }

            maxV = initialMaxV;

            for (
                uint32_t j = southwestTileCoordinates.y;
                j <= northeastTileCoordinates.y;
                ++j
            ) {
                minV = maxV;

                imageryRectangle = imageryTilingScheme.tileToRectangle(QuadtreeTileID(imageryLevel, i, j));
                clippedImageryRectangle = imageryRectangle.intersect(imageryBounds);

                if (!clippedImageryRectangle) {
                    continue;
                }

                maxV = std::min(
                    1.0,
                    (clippedImageryRectangle.value().maximumY - terrainRectangle.minimumY) / terrainRectangle.computeHeight()
                );

                // If this is the northern-most imagery tile mapped to this terrain tile,
                // and there are more imagery tiles to the north of this one, the maxV
                // should be 1.0 to make sure rounding errors don't make the last
                // image fall shy of the edge of the terrain tile.
                if (
                    j == northeastTileCoordinates.y &&
                    (/*this.isBaseLayer()*/ true || std::abs(clippedImageryRectangle.value().maximumY - terrainRectangle.maximumY) < veryCloseY)
                ) {
                    maxV = 1.0;
                }

                Rectangle texCoordsRectangle(minU, minV, maxU, maxV);

                double scaleX = terrainWidth / imageryRectangle.computeWidth();
                double scaleY = terrainHeight / imageryRectangle.computeHeight();
                glm::dvec2 translation(
                    (scaleX * (terrainRectangle.minimumX - imageryRectangle.minimumX)) / terrainWidth,
                    (scaleY * (terrainRectangle.minimumY - imageryRectangle.minimumY)) / terrainHeight
                );
                glm::dvec2 scale(scaleX, scaleY);

                std::shared_ptr<RasterOverlayTile> pTile = tileProvider.getTile(QuadtreeTileID(imageryLevel, i, j));
                this->_rasterTiles.emplace_back(pTile, texCoordsRectangle, translation, scale);

                // var imagery = this.getImageryFromCache(i, j, imageryLevel);
                // surfaceTile.imagery.splice(
                //     insertionPoint,
                //     0,
                //     new TileImagery(imagery, texCoordsRectangle, useWebMercatorT)
                // );
                // ++insertionPoint;
            }
        }
    }

    // void Tile::RasterTile::loadComplete(tinygltf::Image&& loadedImage) {
    //     this->image = std::move(loadedImage);
    //     this->pImageRequest.reset();
    // }

}
