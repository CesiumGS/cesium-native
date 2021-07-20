#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/GltfContent.h"
#include "Cesium3DTiles/IPrepareRendererResources.h"
#include "Cesium3DTiles/TileContentFactory.h"
#include "Cesium3DTiles/Tileset.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumAsync/ITaskProcessor.h"
#include "CesiumGeometry/Axis.h"
#include "CesiumUtility/Tracing.h"
#include "TileUtilities.h"
#include "upsampleGltfForRasterOverlays.h"
#include <cstddef>
#include <functional>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;
using namespace std::string_literals;

namespace Cesium3DTiles {

Tile::Tile() noexcept
    : _pContext(nullptr),
      _pParent(nullptr),
      _children(),
      _boundingVolume(OrientedBoundingBox(glm::dvec3(), glm::dmat4())),
      _viewerRequestVolume(),
      _geometricError(0.0),
      _refine(TileRefine::Replace),
      _transform(1.0),
      _id(""s),
      _contentBoundingVolume(),
      _state(LoadState::Unloaded),
      _pContent(nullptr),
      _pRendererResources(nullptr),
      _lastSelectionState(),
      _loadedTilesLinks() {}

Tile::~Tile() { this->unloadContent(); }

Tile::Tile(Tile&& rhs) noexcept
    : _pContext(rhs._pContext),
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
      _pContent(std::move(rhs._pContent)),
      _pRendererResources(rhs._pRendererResources),
      _lastSelectionState(rhs._lastSelectionState),
      _loadedTilesLinks() {}

Tile& Tile::operator=(Tile&& rhs) noexcept {
  if (this != &rhs) {
    this->_loadedTilesLinks = rhs._loadedTilesLinks;
    this->_pContext = rhs._pContext;
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
    this->_pContent = std::move(rhs._pContent);
    this->_pRendererResources = rhs._pRendererResources;
    this->_lastSelectionState = rhs._lastSelectionState;
  }

  return *this;
}

void Tile::createChildTiles(size_t count) {
  if (!this->_children.empty()) {
    throw std::runtime_error("Children already created.");
  }
  this->_children.resize(count);
}

void Tile::createChildTiles(std::vector<Tile>&& children) {
  if (!this->_children.empty()) {
    throw std::runtime_error("Children already created.");
  }
  this->_children = std::move(children);
}

void Tile::setTileID(const TileID& id) noexcept { this->_id = id; }

bool Tile::isRenderable() const noexcept {
  // A tile whose content is an external tileset has no renderable content. If
  // we select such a tile for rendering, we'll end up rendering nothing even
  // though the tile's parent and its children may both have content. End
  // result: when the tile's parent refines, we get a hole in the content until
  // the children load.

  // So, we explicitly treat external tilesets as non-renderable.
  if (this->getState() >= LoadState::ContentLoaded) {
    if (!this->_pContent || this->_pContent->model.has_value()) {
      return std::all_of(
          this->_rasterTiles.begin(),
          this->_rasterTiles.end(),
          [](const RasterMappedTo3DTile& rasterTile) {
            return rasterTile.getReadyTile() != nullptr;
          });
    }
  }

  return false;
}

void Tile::loadContent(const CesiumAsync::AsyncSystem& asyncSystem) {
  if (this->getState() != LoadState::Unloaded) {
    // No need to load geometry, but give previously-throttled
    // raster overlay tiles a chance to load.
    for (RasterMappedTo3DTile& mapped : this->getMappedRasterTiles()) {
      RasterOverlayTile* pLoading = mapped.getLoadingTile();
      if (pLoading &&
          pLoading->getState() == RasterOverlayTile::LoadState::Unloaded) {
        RasterOverlayTileProvider* pProvider =
            pLoading->getOverlay().getTileProvider();
        if (pProvider) {
          pProvider->loadTileThrottled(*pLoading);
        }
      }
    }
    return;
  }

  this->setState(LoadState::ContentLoading);

  Tileset& tileset = *this->getTileset();

  // TODO: support overlay mapping for tiles that aren't region-based.
  // Probably by creating a placeholder for each raster overlay and resolving it
  // to actual raster tiles once we have real geometry. This will also be
  // necessary for raster overlays with a projection that isn't nicely lon/lat
  // aligned like geographic or web mercator, because we won't know our raster
  // rectangle until we can project each vertex.

  std::vector<Projection> projections;

  const CesiumGeospatial::GlobeRectangle* pRectangle =
      Cesium3DTiles::Impl::obtainGlobeRectangle(&this->getBoundingVolume());
  if (pRectangle && tileset.supportsRasterOverlays()) {
    // Map overlays to this tile.
    RasterOverlayCollection& overlays = tileset.getOverlays();
    // gsl::span<RasterOverlayTileProvider*> providers =
    // overlays.getTileProviders();

    // Map raster tiles to a new vector first, and then replace the old one.
    // Doing it in this order ensures that tiles that are already loaded and
    // that we still need are not freed too soon.
    std::vector<RasterMappedTo3DTile> newRasterTiles;

    for (auto& overlay : overlays) {
      overlay->getTileProvider()->mapRasterTilesToGeometryTile(
          *pRectangle,
          this->getGeometricError(),
          newRasterTiles);
    }

    this->_rasterTiles = std::move(newRasterTiles);

    // Find the unique projections; we'll create texture coordinates for each
    // later.
    // TODO: actually find the unique projections, instead of assuming there's
    // only one projection and it's Web Mercator. The code below mostly works,
    // except that we don't currently have a way to add texture coordinates to
    // already-loaded tiles that didn't have them originally.

    // uint32_t projectionID = 0;

    // for (RasterMappedTo3DTile& mappedTile : this->_rasterTiles) {
    //     std::shared_ptr<RasterOverlayTile> pTile =
    //     mappedTile.getLoadingTile(); if (!pTile) {
    //         pTile = mappedTile.getReadyTile();
    //         if (!pTile) {
    //             continue;
    //         }
    //     }

    //     const CesiumGeospatial::Projection& projection =
    //     pTile->getOverlay().getTileProvider()->getProjection();

    //     auto existingCoordinatesIt = std::find(projections.begin(),
    //     projections.end(), projection); if (existingCoordinatesIt ==
    //     projections.end()) {
    //         projections.push_back(projection);

    //         mappedTile.setTextureCoordinateID(projectionID);
    //         ++projectionID;
    //     } else {
    //         // Use previously-added texture coordinates.
    //         mappedTile.setTextureCoordinateID(static_cast<uint32_t>(existingCoordinatesIt
    //         - projections.begin()));
    //     }
    // }

    projections.push_back(WebMercatorProjection());

    // Only add geographic coordinates for the watermask, currently.
    if (this->getTileset()->getOptions().contentOptions.enableWaterMask) {
      projections.push_back(GeographicProjection());
    }
  }

  std::optional<Future<std::shared_ptr<IAssetRequest>>> maybeRequestFuture =
      tileset.requestTileContent(*this);

  if (!maybeRequestFuture) {
    // There is no content to load. But we may need to upsample.

    const UpsampledQuadtreeNode* pSubdivided =
        std::get_if<UpsampledQuadtreeNode>(&this->getTileID());
    if (pSubdivided) {
      // We can't upsample this tile until its parent tile is done loading.
      if (this->getParent() &&
          this->getParent()->getState() == LoadState::Done) {
        this->upsampleParent(std::move(projections));
      } else {
        // Try again later. Push the parent tile loading along if we can.
        if (this->getParent()) {
          this->getParent()->loadContent(asyncSystem);
        }
        this->setState(LoadState::Unloaded);
      }
    } else {
      this->setState(LoadState::ContentLoaded);
    }

    return;
  }

  struct LoadResult {
    LoadState state;
    std::unique_ptr<TileContentLoadResult> pContent;
    void* pRendererResources;
  };
  TileContentLoadInput loadInput(tileset.getExternals().pLogger, *this);

  std::function<void(LoadResult&&)> handleLoadResult = [this](LoadResult&& loadResult) {
    // if not already loaded, update the tile with the loading result
    if (this->getState() != LoadState::ContentLoaded) {
      this->_pContent = std::move(loadResult.pContent);
      this->_pRendererResources = loadResult.pRendererResources;
      if (loadResult.state != LoadState::ContentLoading) {
        this->getTileset()->notifyTileDoneLoading(this);
      }
      this->setState(loadResult.state);
    }
  };

  std::function<void(const std::exception& e)> handleLoadError = [this](const std::exception& e) {
      this->_pContent.reset();
      this->_pRendererResources = nullptr;
      this->getTileset()->notifyTileDoneLoading(this);
      this->setState(LoadState::Failed);

      SPDLOG_LOGGER_ERROR(
          this->getTileset()->getExternals().pLogger,
          "An exception occurred while loading tile: {}",
          e.what());
    };

  const CesiumGeometry::Axis gltfUpAxis = tileset.getGltfUpAxis();
  std::move(maybeRequestFuture.value())
      .thenInWorkerThread(
          [this,
           asyncSystem,
           loadInput = std::move(loadInput),
           handleLoadResult,
           handleLoadError,
           projections = std::move(projections),
           gltfUpAxis,
           pPrepareRendererResources =
               tileset.getExternals().pPrepareRendererResources,
           pLogger = tileset.getExternals().pLogger](
              std::shared_ptr<IAssetRequest>&& pRequest) mutable {
            CESIUM_TRACE("loadContent worker thread");
            const IAssetResponse* pResponse = pRequest->response();
            if (!pResponse) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Did not receive a valid response for tile content {}",
                  pRequest->url());
              auto pLoadResult = std::make_unique<TileContentLoadResult>();
              pLoadResult->httpStatusCode = 0;
              return LoadResult{
                  LoadState::FailedTemporarily,
                  std::move(pLoadResult),
                  nullptr};
            }

            if (pResponse->statusCode() != 0 &&
                (pResponse->statusCode() < 200 ||
                 pResponse->statusCode() >= 300)) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Received status code {} for tile content {}",
                  pResponse->statusCode(),
                  pRequest->url());
              auto pLoadResult = std::make_unique<TileContentLoadResult>();
              pLoadResult->httpStatusCode = pResponse->statusCode();
              return LoadResult{
                  LoadState::FailedTemporarily,
                  std::move(pLoadResult),
                  nullptr};
            }

            loadInput.data = pResponse->data();
            loadInput.contentType = pResponse->contentType();
            loadInput.url = pRequest->url();
            //std::unique_ptr<TileContentLoadResult> pContent =
            TileContentFactory::createContent(asyncSystem, loadInput).thenInWorkerThread(
                [httpStatusCode = pResponse->statusCode(),
                 gltfUpAxis = std::move(gltfUpAxis),
                 loadInput = std::move(loadInput),
                 projections = std::move(projections),
                 pPrepareRendererResources = std::move(pPrepareRendererResources)
                ] (std::unique_ptr<TileContentLoadResult>&& pContent) {

              void* pRendererResources = nullptr;
              if (pContent) {
                pContent->httpStatusCode = httpStatusCode;

                if (pContent->model) {

                  // TODO The `extras` are currently the only way to pass
                  // arbitrary information to the consumer, so the up-axis
                  // is stored here:
                  pContent->model.value().extras["gltfUpAxis"] = gltfUpAxis;

                  const BoundingVolume& boundingVolume =
                      loadInput.tileBoundingVolume;
                  Tile::generateTextureCoordinates(
                      pContent->model.value(),
                      boundingVolume,
                      projections);

                  if (pPrepareRendererResources) {
                    CESIUM_TRACE("prepareInLoadThread");
                    const glm::dmat4& transform = loadInput.tileTransform;
                    pRendererResources =
                        pPrepareRendererResources->prepareInLoadThread(
                            pContent->model.value(),
                            transform);
                  }
                }
              }

              LoadResult result;
              result.state = LoadState::ContentLoaded;
              result.pContent = std::move(pContent);
              result.pRendererResources = pRendererResources;

              return result;
            })
            .thenInMainThread(handleLoadResult)
            .catchInMainThread(handleLoadError);
          
            return LoadResult{
              LoadState::ContentLoading,
              std::make_unique<TileContentLoadResult>(),
              nullptr
            };
          })
          .thenInMainThread(handleLoadResult)
          .catchInMainThread(handleLoadError);
}

bool Tile::unloadContent() noexcept {
  if (this->getState() != Tile::LoadState::Unloaded) {
    // Cannot unload while an async operation is in progress.
    if (this->getState() == Tile::LoadState::ContentLoading) {
      return false;
    }

    // If a child tile is being upsampled from this one, we can't unload this
    // one yet.
    if (this->getState() == Tile::LoadState::Done &&
        !this->getChildren().empty()) {
      for (const Tile& child : this->getChildren()) {
        if (child.getState() == Tile::LoadState::ContentLoading &&
            std::get_if<CesiumGeometry::UpsampledQuadtreeNode>(
                &child.getTileID()) != nullptr) {
          return false;
        }
      }
    }

    this->getTileset()->notifyTileUnloading(this);

    const TilesetExternals& externals = this->getTileset()->getExternals();
    if (externals.pPrepareRendererResources) {
      if (this->getState() == LoadState::ContentLoaded) {
        externals.pPrepareRendererResources->free(
            *this,
            this->_pRendererResources,
            nullptr);
      } else {
        externals.pPrepareRendererResources->free(
            *this,
            nullptr,
            this->_pRendererResources);
      }
    }

    this->setState(LoadState::Unloaded);
  }

  this->_pRendererResources = nullptr;
  this->_pContent.reset();
  this->_rasterTiles.clear();

  return true;
}

static void createImplicitTile(
    const ImplicitTilingContext& implicitContext,
    Tile& parent,
    Tile& child,
    const QuadtreeTileID& childID,
    bool available) {
  child.setContext(parent.getContext());
  child.setParent(&parent);

  if (available) {
    child.setTileID(childID);
  } else {
    child.setTileID(UpsampledQuadtreeNode{childID});
  }

  child.setGeometricError(parent.getGeometricError() * 0.5);

  double minimumHeight = -1000.0;
  double maximumHeight = 9000.0;

  const BoundingRegion* pRegion =
      std::get_if<BoundingRegion>(&parent.getBoundingVolume());
  if (!pRegion) {
    const BoundingRegionWithLooseFittingHeights* pLooseRegion =
        std::get_if<BoundingRegionWithLooseFittingHeights>(
            &parent.getBoundingVolume());
    if (pLooseRegion) {
      pRegion = &pLooseRegion->getBoundingRegion();
    }
  }

  if (pRegion) {
    minimumHeight = pRegion->getMinimumHeight();
    maximumHeight = pRegion->getMaximumHeight();
  }

  child.setBoundingVolume(BoundingRegionWithLooseFittingHeights(BoundingRegion(
      unprojectRectangleSimple(
          implicitContext.projection,
          implicitContext.tilingScheme.tileToRectangle(childID)),
      minimumHeight,
      maximumHeight)));
}

static void createQuadtreeSubdividedChildren(Tile& parent) {
  // TODO: support non-BoundingRegions.
  const BoundingRegion* pRegion =
      std::get_if<BoundingRegion>(&parent.getBoundingVolume());
  if (!pRegion) {
    const BoundingRegionWithLooseFittingHeights* pLooseRegion =
        std::get_if<BoundingRegionWithLooseFittingHeights>(
            &parent.getBoundingVolume());
    if (pLooseRegion) {
      pRegion = &pLooseRegion->getBoundingRegion();
    }
  }

  if (!pRegion) {
    return;
  }

  // TODO: support upsampling non-quadtrees.
  const QuadtreeTileID* pParentTileID =
      std::get_if<QuadtreeTileID>(&parent.getTileID());
  if (!pParentTileID) {
    const UpsampledQuadtreeNode* pUpsampledID =
        std::get_if<UpsampledQuadtreeNode>(&parent.getTileID());
    if (pUpsampledID) {
      pParentTileID = &pUpsampledID->tileID;
    }
  }

  if (!pParentTileID) {
    return;
  }

  // TODO: support upsampling non-implicit tiles.
  if (!parent.getContext()->implicitContext) {
    return;
  }

  QuadtreeTileID swID(
      pParentTileID->level + 1,
      pParentTileID->x * 2,
      pParentTileID->y * 2);
  QuadtreeTileID seID(swID.level, swID.x + 1, swID.y);
  QuadtreeTileID nwID(swID.level, swID.x, swID.y + 1);
  QuadtreeTileID neID(swID.level, swID.x + 1, swID.y + 1);

  QuadtreeTilingScheme& tilingScheme =
      parent.getContext()->implicitContext.value().tilingScheme;
  Projection& projection =
      parent.getContext()->implicitContext.value().projection;

  parent.createChildTiles(4);

  gsl::span<Tile> children = parent.getChildren();
  Tile& sw = children[0];
  Tile& se = children[1];
  Tile& nw = children[2];
  Tile& ne = children[3];

  sw.setContext(parent.getContext());
  se.setContext(parent.getContext());
  nw.setContext(parent.getContext());
  ne.setContext(parent.getContext());

  double geometricError = parent.getGeometricError() * 0.5;
  sw.setGeometricError(geometricError);
  se.setGeometricError(geometricError);
  nw.setGeometricError(geometricError);
  ne.setGeometricError(geometricError);

  sw.setParent(&parent);
  se.setParent(&parent);
  nw.setParent(&parent);
  ne.setParent(&parent);

  sw.setTileID(UpsampledQuadtreeNode{swID});
  se.setTileID(UpsampledQuadtreeNode{seID});
  nw.setTileID(UpsampledQuadtreeNode{nwID});
  ne.setTileID(UpsampledQuadtreeNode{neID});

  double minimumHeight = pRegion->getMinimumHeight();
  double maximumHeight = pRegion->getMaximumHeight();

  sw.setBoundingVolume(BoundingRegionWithLooseFittingHeights(BoundingRegion(
      unprojectRectangleSimple(projection, tilingScheme.tileToRectangle(swID)),
      minimumHeight,
      maximumHeight)));
  se.setBoundingVolume(BoundingRegionWithLooseFittingHeights(BoundingRegion(
      unprojectRectangleSimple(projection, tilingScheme.tileToRectangle(seID)),
      minimumHeight,
      maximumHeight)));
  nw.setBoundingVolume(BoundingRegionWithLooseFittingHeights(BoundingRegion(
      unprojectRectangleSimple(projection, tilingScheme.tileToRectangle(nwID)),
      minimumHeight,
      maximumHeight)));
  ne.setBoundingVolume(BoundingRegionWithLooseFittingHeights(BoundingRegion(
      unprojectRectangleSimple(projection, tilingScheme.tileToRectangle(neID)),
      minimumHeight,
      maximumHeight)));
}

void Tile::update(
    int32_t /*previousFrameNumber*/,
    int32_t /*currentFrameNumber*/) {
  const TilesetExternals& externals = this->getTileset()->getExternals();

  if (this->getState() == LoadState::FailedTemporarily) {
    // Check with the TileContext to see if we should retry.
    if (this->_pContext->failedTileCallback) {
      FailedTileAction action = this->_pContext->failedTileCallback(*this);
      switch (action) {
      case FailedTileAction::GiveUp:
        this->setState(LoadState::Failed);
        break;
      case FailedTileAction::Retry:
        this->setState(LoadState::Unloaded);
        break;
      case FailedTileAction::Wait:
        // Do nothing for now.
        break;
      }
    } else {
      this->setState(LoadState::Failed);
    }
  }

  if (this->getState() == LoadState::ContentLoaded) {
    if (externals.pPrepareRendererResources) {
      this->_pRendererResources =
          externals.pPrepareRendererResources->prepareInMainThread(
              *this,
              this->getRendererResources());
    }

    if (this->_pContent) {
      // Apply children from content, but only if we don't already have
      // children.
      if (this->_pContent->childTiles && this->getChildren().empty()) {
        for (Tile& childTile : this->_pContent->childTiles.value()) {
          childTile.setParent(this);
        }

        this->createChildTiles(std::move(this->_pContent->childTiles.value()));

        // Initialize the new context, if there is one.
        if (this->_pContent->pNewTileContext &&
            this->_pContent->pNewTileContext->contextInitializerCallback) {
          this->_pContent->pNewTileContext->contextInitializerCallback(
              *this->getContext(),
              *this->_pContent->pNewTileContext);
        }

        this->getTileset()->addContext(
            std::move(this->_pContent->pNewTileContext));
      }

      // If this tile has no model, set its geometric error very high so we
      // refine past it. Note that "no" model is different from having a model,
      // but it is blank. In the latter case, we'll happily render nothing in
      // the space of this tile, which is sometimes useful.
      if (!this->_pContent->model) {
        this->setGeometricError(999999999.0);
      }

      // A new and improved bounding volume.
      if (this->_pContent->updatedBoundingVolume) {
        this->setBoundingVolume(this->_pContent->updatedBoundingVolume.value());
      }

      if (!this->_pContent->availableTileRectangles.empty() &&
          this->getContext()->implicitContext) {
        ImplicitTilingContext& context =
            this->getContext()->implicitContext.value();
        for (const QuadtreeTileRectangularRange& range :
             this->_pContent->availableTileRectangles) {
          context.availability.addAvailableTileRange(range);
        }
      }
    }

    this->setState(LoadState::Done);
  }

  if (this->getContext()->implicitContext && this->getChildren().empty() &&
      std::get_if<QuadtreeTileID>(&this->_id)) {
    // Check if any child tiles are known to be available, and create them if
    // they are.
    const ImplicitTilingContext& implicitContext =
        this->getContext()->implicitContext.value();
    const CesiumGeometry::QuadtreeTileAvailability& availability =
        implicitContext.availability;

    QuadtreeTileID id = std::get<QuadtreeTileID>(this->_id);

    QuadtreeTileID swID(id.level + 1, id.x * 2, id.y * 2);
    uint32_t sw = availability.isTileAvailable(swID) ? 1 : 0;

    QuadtreeTileID seID(swID.level, swID.x + 1, swID.y);
    uint32_t se = availability.isTileAvailable(seID) ? 1 : 0;

    QuadtreeTileID nwID(swID.level, swID.x, swID.y + 1);
    uint32_t nw = availability.isTileAvailable(nwID) ? 1 : 0;

    QuadtreeTileID neID(swID.level, swID.x + 1, swID.y + 1);
    uint32_t ne = availability.isTileAvailable(neID) ? 1 : 0;

    size_t childCount = sw + se + nw + ne;
    if (childCount > 0) {
      // If any children are available, we need to create all four in order to
      // avoid holes. But non-available tiles will be upsampled instead of
      // loaded.
      // TODO: this is the right thing to do for terrain, which is the only use
      // of implicit tiling currently. But we may need to re-evaluate it if
      // we're using implicit tiling for buildings (for example) in the future.
      this->_children.resize(4);

      createImplicitTile(implicitContext, *this, this->_children[0], swID, sw);
      createImplicitTile(implicitContext, *this, this->_children[1], seID, se);
      createImplicitTile(implicitContext, *this, this->_children[2], nwID, nw);
      createImplicitTile(implicitContext, *this, this->_children[3], neID, ne);
    }
  }

  if (this->getState() == LoadState::Done &&
      this->getTileset()->supportsRasterOverlays()) {
    bool moreRasterDetailAvailable = false;

    for (size_t i = 0; i < this->_rasterTiles.size(); ++i) {
      RasterMappedTo3DTile& mappedRasterTile = this->_rasterTiles[i];

      RasterOverlayTile* pLoadingTile = mappedRasterTile.getLoadingTile();
      if (pLoadingTile && pLoadingTile->getState() ==
                              RasterOverlayTile::LoadState::Placeholder) {
        RasterOverlayTileProvider* pProvider =
            pLoadingTile->getOverlay().getTileProvider();

        // Try to replace this placeholder with real tiles.
        if (!pProvider->isPlaceholder()) {
          this->_rasterTiles.erase(
              this->_rasterTiles.begin() +
              static_cast<
                  std::vector<RasterMappedTo3DTile>::iterator::difference_type>(
                  i));
          --i;

          const CesiumGeospatial::GlobeRectangle* pRectangle =
              Cesium3DTiles::Impl::obtainGlobeRectangle(
                  &this->getBoundingVolume());
          pProvider->mapRasterTilesToGeometryTile(
              *pRectangle,
              this->getGeometricError(),
              this->_rasterTiles);
        }

        continue;
      }

      RasterMappedTo3DTile::MoreDetailAvailable moreDetailAvailable =
          mappedRasterTile.update(*this);
      moreRasterDetailAvailable |=
          moreDetailAvailable == RasterMappedTo3DTile::MoreDetailAvailable::Yes;
    }

    // If this tile still has no children after it's done loading, but it does
    // have raster tiles that are not the most detailed available, create fake
    // children to hang more detailed rasters on by subdividing this tile.
    if (moreRasterDetailAvailable && this->_children.empty()) {
      createQuadtreeSubdividedChildren(*this);
    }
  }
}

void Tile::markPermanentlyFailed() noexcept {
  if (this->getState() == LoadState::FailedTemporarily) {
    this->setState(LoadState::Failed);
  }
}

int64_t Tile::computeByteSize() const noexcept {
  int64_t bytes = 0;

  const TileContentLoadResult* pContent = this->getContent();
  if (pContent && pContent->model) {
    const CesiumGltf::Model& model = pContent->model.value();

    // Add up the glTF buffers
    for (const CesiumGltf::Buffer& buffer : model.buffers) {
      bytes += int64_t(buffer.cesium.data.size());
    }

    // For images loaded from buffers, subtract the buffer size and add
    // the decoded image size instead.
    const std::vector<CesiumGltf::BufferView>& bufferViews = model.bufferViews;
    for (const CesiumGltf::Image& image : model.images) {
      int32_t bufferView = image.bufferView;
      if (bufferView < 0 ||
          bufferView >= static_cast<int32_t>(bufferViews.size())) {
        continue;
      }

      bytes -= bufferViews[size_t(bufferView)].byteLength;
      bytes += int64_t(image.cesium.pixelData.size());
    }
  }

  return bytes;
}

void Tile::setState(LoadState value) noexcept {
  this->_state.store(value, std::memory_order::memory_order_release);
}

/*static*/ std::optional<CesiumGeospatial::BoundingRegion>
Tile::generateTextureCoordinates(
    CesiumGltf::Model& model,
    const BoundingVolume& boundingVolume,
    const std::vector<Projection>& projections) {
  std::optional<CesiumGeospatial::BoundingRegion> result;

  // Generate texture coordinates for each projection.
  if (!projections.empty()) {
    const CesiumGeospatial::GlobeRectangle* pRectangle =
        Cesium3DTiles::Impl::obtainGlobeRectangle(&boundingVolume);

    if (pRectangle) {
      for (size_t i = 0; i < projections.size(); ++i) {
        const Projection& projection = projections[i];
        uint32_t projectionID = static_cast<uint32_t>(i);

        CesiumGeometry::Rectangle rectangle =
            projectRectangleSimple(projection, *pRectangle);

        CesiumGeospatial::BoundingRegion boundingRegion =
            GltfContent::createRasterOverlayTextureCoordinates(
                model,
                projectionID,
                projection,
                rectangle);
        if (result) {
          result = boundingRegion.computeUnion(result.value());
        } else {
          result = boundingRegion;
        }
      }
    }
  }

  return result;
}

void Tile::upsampleParent(
    std::vector<CesiumGeospatial::Projection>&& projections) {
  Tile* pParent = this->getParent();
  const UpsampledQuadtreeNode* pSubdividedParentID =
      std::get_if<UpsampledQuadtreeNode>(&this->getTileID());

  assert(pParent != nullptr);
  assert(pParent->getState() == LoadState::Done);
  assert(pSubdividedParentID != nullptr);

  TileContentLoadResult* pParentContent = pParent->getContent();
  if (!pParentContent || !pParentContent->model) {
    this->setState(LoadState::ContentLoaded);
    return;
  }

  CesiumGltf::Model& parentModel = pParentContent->model.value();

  Tileset* pTileset = this->getTileset();
  pTileset->notifyTileStartLoading(this);

  struct LoadResult {
    LoadState state;
    std::unique_ptr<TileContentLoadResult> pContent;
    void* pRendererResources;
  };

  pTileset->getAsyncSystem()
      .runInWorkerThread([&parentModel,
                          transform = this->getTransform(),
                          projections,
                          pSubdividedParentID,
                          boundingVolume = this->getBoundingVolume(),
                          pPrepareRendererResources =
                              pTileset->getExternals()
                                  .pPrepareRendererResources]() {
        std::unique_ptr<TileContentLoadResult> pContent =
            std::make_unique<TileContentLoadResult>();
        pContent->model =
            upsampleGltfForRasterOverlays(parentModel, *pSubdividedParentID);
        if (pContent->model) {
          pContent->updatedBoundingVolume = Tile::generateTextureCoordinates(
              pContent->model.value(),
              boundingVolume,
              projections);
        }

        void* pRendererResources = nullptr;
        if (pContent->model && pPrepareRendererResources) {
          pRendererResources = pPrepareRendererResources->prepareInLoadThread(
              pContent->model.value(),
              transform);
        }

        return LoadResult{
            LoadState::ContentLoaded,
            std::move(pContent),
            pRendererResources};
      })
      .thenInMainThread([this](LoadResult&& loadResult) {
        this->_pContent = std::move(loadResult.pContent);
        this->_pRendererResources = loadResult.pRendererResources;
        this->getTileset()->notifyTileDoneLoading(this);
        this->setState(loadResult.state);
      })
      .catchInMainThread([this](const std::exception& /*e*/) {
        this->_pContent.reset();
        this->_pRendererResources = nullptr;
        this->getTileset()->notifyTileDoneLoading(this);
        this->setState(LoadState::Failed);
      });
}

} // namespace Cesium3DTiles
