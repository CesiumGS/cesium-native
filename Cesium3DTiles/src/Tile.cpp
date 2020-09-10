#include "Cesium3DTiles/IAssetAccessor.h"
#include "Cesium3DTiles/IAssetResponse.h"
#include "Cesium3DTiles/IPrepareRendererResources.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/TileContentFactory.h"
#include "Cesium3DTiles/Tileset.h"
#include <algorithm>
#include <chrono>

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
            (!this->_pContent || this->_pContent->getType() != ExternalTilesetContent::TYPE) &&
            !std::any_of(this->_rasterTiles.begin(), this->_rasterTiles.end(), [](const RasterMappedTo3DTile& rasterTile) {
                return rasterTile.getRasterTile().getState() == RasterOverlayTile::LoadState::Loading;
            });
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
        // Probably be creating a placeholder for each raster overlay and resolving it to actual raster tiles once
        // we have real geometry. This will also be necessary for raster overlays with a projection that isn't
        // nicely lon/lat aligned like geographic or web mercator, because we won't know our raster rectangle
        // until we can project each vertex.

        if (pRectangle) {
            // Map overlays to this tile.
            RasterOverlayCollection& overlays = tileset.getOverlays();
            gsl::span<RasterOverlayTileProvider*> providers = overlays.getTileProviders();
            
            // Map raster tiles to a new vector first, and then replace the old one.
            // Doing it in this order ensures that tiles that are already loaded and that we
            // still need are not freed too soon.
            std::vector<RasterMappedTo3DTile> newRasterTiles;

            for (RasterOverlayTileProvider* pProvider : providers) {
                pProvider->mapRasterTilesToGeometryTile(*pRectangle, this->getGeometricError(), newRasterTiles);
            }

            this->_rasterTiles = std::move(newRasterTiles);
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

        // TODO: unload raster tiles

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

                // Generate texture coordinates for each projection.
                if (!this->_rasterTiles.empty()) {
                    CesiumGeospatial::BoundingRegion* pRegion = std::get_if<CesiumGeospatial::BoundingRegion>(&this->_boundingVolume);
                    CesiumGeospatial::BoundingRegionWithLooseFittingHeights* pLooseRegion = std::get_if<CesiumGeospatial::BoundingRegionWithLooseFittingHeights>(&this->_boundingVolume);
                    
                    const CesiumGeospatial::GlobeRectangle* pRectangle = nullptr;
                    if (pRegion) {
                        pRectangle = &pRegion->getRectangle();
                    } else if (pLooseRegion) {
                        pRectangle = &pLooseRegion->getBoundingRegion().getRectangle();
                    }

                    if (pRectangle) {
                        std::vector<Projection> projections;
                        uint32_t projectionID = 0;

                        for (RasterMappedTo3DTile& mappedTile : this->_rasterTiles) {
                            const CesiumGeospatial::Projection& projection = mappedTile.getRasterTile().getTileProvider().getProjection();

                            auto existingCoordinatesIt = std::find(projections.begin(), projections.end(), projection);
                            if (existingCoordinatesIt == projections.end()) {
                                // Create new texture coordinates for this not-previously-seen projection
                                CesiumGeometry::Rectangle rectangle = projectRectangleSimple(projection, *pRectangle);
                                this->getContent()->createRasterOverlayTextureCoordinates(projectionID, projection, rectangle);
                                projections.push_back(projection);

                                mappedTile.setTextureCoordinateID(projectionID);
                                ++projectionID;
                            } else {
                                // Use previously-added texture coordinates.
                                mappedTile.setTextureCoordinateID(static_cast<uint32_t>(existingCoordinatesIt - projections.begin()));
                            }
                        }
                    }
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

}
