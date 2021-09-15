#include "Cesium3DTilesSelection/Tile.h"
#include "Cesium3DTilesSelection/GltfContent.h"
#include "Cesium3DTilesSelection/IPrepareRendererResources.h"
#include "Cesium3DTilesSelection/TileContentFactory.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumAsync/ITaskProcessor.h"
#include "CesiumGeometry/Axis.h"
#include "CesiumGeometry/AxisTransforms.h"
#include "CesiumGeometry/Rectangle.h"
#include "CesiumGeospatial/Transforms.h"
#include "CesiumGltf/Model.h"
#include "CesiumUtility/Tracing.h"
#include "TileUtilities.h"
#include "upsampleGltfForRasterOverlays.h"
#include <cstddef>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;
using namespace std::string_literals;

namespace Cesium3DTilesSelection {

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

namespace {
int32_t getTextureCoordinatesForProjection(
    std::vector<CesiumGeospatial::Projection>& projections,
    const Projection& projection) {
  auto existingCoordinatesIt =
      std::find(projections.begin(), projections.end(), projection);
  if (existingCoordinatesIt == projections.end()) {
    // New set of texture coordinates.
    projections.push_back(projection);
    return int32_t(projections.size()) - 1;
  } else {
    // Use previously-added texture coordinates.
    return static_cast<int32_t>(existingCoordinatesIt - projections.begin());
  }
}

void mapRasterOverlaysToTile(
    Tile& tile,
    const RasterOverlayCollection& overlays,
    std::vector<CesiumGeospatial::Projection>& projections) {

  assert(tile.getMappedRasterTiles().empty());

  const CesiumGeospatial::GlobeRectangle* pRectangle =
      Impl::obtainGlobeRectangle(&tile.getBoundingVolume());
  if (!pRectangle) {
    return;
  }

  // Find the unique projections; we'll create texture coordinates for each
  // later.
  for (const auto& pOverlay : overlays) {
    const RasterOverlayTileProvider* pProvider = pOverlay->getTileProvider();

    // Project the globe rectangle to the raster's coordinate system.
    // TODO: we can compute a much more precise projected rectangle by
    //       projecting each vertex, but that would require us to have
    //       geometry first.
    Rectangle overlayRectangle =
        projectRectangleSimple(pProvider->getProjection(), *pRectangle);

    IntrusivePointer<RasterOverlayTile> pRaster =
        pOverlay->getTileProvider()->getTile(
            overlayRectangle,
            tile.getGeometricError());
    if (pRaster) {
      RasterMappedTo3DTile& mappedTile =
          tile.getMappedRasterTiles().emplace_back(pRaster);

      const CesiumGeospatial::Projection& projection =
          pRaster->getOverlay().getTileProvider()->getProjection();

      int32_t projectionID =
          getTextureCoordinatesForProjection(projections, projection);

      mappedTile.setTextureCoordinateID(projectionID);

      if (pRaster->getState() != RasterOverlayTile::LoadState::Placeholder) {
        pOverlay->getTileProvider()->loadTileThrottled(*pRaster);
      }
    }
  }

  // Add geographic texture coordinates for water mask.
  if (tile.getTileset()->getOptions().contentOptions.enableWaterMask) {
    getTextureCoordinatesForProjection(
        projections,
        CesiumGeospatial::GeographicProjection());
  }
}
} // namespace

void Tile::loadContent() {
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

  std::vector<CesiumGeospatial::Projection> projections;

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
        this->loadOverlays(projections);
        this->upsampleParent(std::move(projections));
      } else {
        // Try again later. Push the parent tile loading along if we can.
        if (this->getParent()) {
          this->getParent()->loadContent();
        }
        this->setState(LoadState::Unloaded);
      }
    } else {
      this->setState(LoadState::ContentLoaded);
    }

    return;
  }

  this->loadOverlays(projections);

  struct LoadResult {
    LoadState state;
    std::unique_ptr<TileContentLoadResult> pContent;
    void* pRendererResources;
  };

  const CesiumGeometry::Axis gltfUpAxis = tileset.getGltfUpAxis();
  std::move(maybeRequestFuture.value())
      // TODO: for some reason, this first lambda completely breaks if we make
      // it mutable
      .thenInWorkerThread([this,
                           &asyncSystem = tileset.getAsyncSystem(),
                           &pLogger_ = tileset.getExternals().pLogger,
                           &pAssetAccessor_ =
                               tileset.getExternals().pAssetAccessor](
                              std::shared_ptr<IAssetRequest>&& pRequest) mutable {
        CESIUM_TRACE("loadContent worker thread");

        // WORKAROUND:
        // Manually make shared_ptr copies. If we do it in the lambda
        // capture list it turns out we can't later move those copies into
        // TileContentLoadInput without making the lambda mutable. For some
        // seemingly random reason making this lambda mutable breaks it.
        auto pLogger = pLogger_;
        auto pAssetAccessor = pAssetAccessor_;

        const IAssetResponse* pResponse = pRequest->response();
        if (!pResponse) {
          SPDLOG_LOGGER_ERROR(
              pLogger,
              "Did not receive a valid response for tile content {}",
              pRequest->url());
          auto pLoadResult = std::make_unique<TileContentLoadResult>();
          pLoadResult->httpStatusCode = 0;
          return asyncSystem.createResolvedFuture(std::move(pLoadResult));
        }

        if (pResponse->statusCode() != 0 &&
            (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300)) {
          SPDLOG_LOGGER_ERROR(
              pLogger,
              "Received status code {} for tile content {}",
              pResponse->statusCode(),
              pRequest->url());
          auto pLoadResult = std::make_unique<TileContentLoadResult>();
          pLoadResult->httpStatusCode = pResponse->statusCode();
          return asyncSystem.createResolvedFuture(std::move(pLoadResult));
        }

        TileContentLoadInput loadInput(
            asyncSystem,
            std::move(pLogger),
            std::move(pAssetAccessor),
            std::move(pRequest),
            std::nullopt,
            *this);

        return TileContentFactory::createContent(loadInput)
            // Forward status code to the load result.
            .thenImmediately(
                [statusCode = pResponse->statusCode()](
                    std::unique_ptr<TileContentLoadResult>&& pContent) {
                  pContent->httpStatusCode = statusCode;
                  return std::move(pContent);
                });
      })
      .thenImmediately(
          [gltfUpAxis,
           &boundingVolume = this->_boundingVolume,
           &transform = this->_transform,
           projections = std::move(projections),
           generateMissingNormalsSmooth =
               tileset.getOptions().contentOptions.generateMissingNormalsSmooth,
           pPrepareRendererResources =
               tileset.getExternals().pPrepareRendererResources](
              std::unique_ptr<TileContentLoadResult>&& pContent) mutable {
            if (pContent->httpStatusCode < 200 ||
                pContent->httpStatusCode >= 300) {
              return LoadResult{
                  LoadState::FailedTemporarily,
                  std::move(pContent),
                  nullptr};
            }

            void* pRendererResources = nullptr;

            if (pContent) {
              if (pContent->model) {

                CesiumGltf::Model& model = pContent->model.value();

                // TODO The `extras` are currently the only way to pass
                // arbitrary information to the consumer, so the up-axis
                // is stored here:
                model.extras["gltfUpAxis"] = gltfUpAxis;

                Tile::generateTextureCoordinates(
                    model,
                    transform,
                    boundingVolume,
                    projections);

                pContent->rasterOverlayProjections = std::move(projections);

                if (generateMissingNormalsSmooth) {
                  pContent->model->generateMissingNormalsSmooth();
                }

                if (pPrepareRendererResources) {
                  CESIUM_TRACE("prepareInLoadThread");
                  pRendererResources =
                      pPrepareRendererResources->prepareInLoadThread(
                          pContent->model.value(),
                          transform);
                }
              }
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
      .catchInMainThread([this](const std::exception& e) {
        this->_pContent.reset();
        this->_pRendererResources = nullptr;
        this->getTileset()->notifyTileDoneLoading(this);
        this->setState(LoadState::Failed);

        SPDLOG_LOGGER_ERROR(
            this->getTileset()->getExternals().pLogger,
            "An exception occurred while loading tile: {}",
            e.what());
      });
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
        // Technically we don't need to completely unload the tile. We only
        // need to re-request the tile content. But the transition from
        // Unloaded -> LoadingContent does other things too (creating raster
        // overlays for one thing), so we'll call unloadContent to keep our
        // state machine sane (-ish). Refreshing an ion token could be a bit
        // faster if we did smarter things here, but it's not worth the
        // trouble.
        this->unloadContent();
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

  const CesiumGeospatial::GlobeRectangle* pRectangle =
      Impl::obtainGlobeRectangle(&this->getBoundingVolume());
  if (!pRectangle) {
    return;
  }

  if (this->getState() == LoadState::Done &&
      this->getTileset()->supportsRasterOverlays() && pRectangle) {
    bool moreRasterDetailAvailable = false;

    for (size_t i = 0; i < this->_rasterTiles.size(); ++i) {
      RasterMappedTo3DTile& mappedRasterTile = this->_rasterTiles[i];

      RasterOverlayTile* pLoadingTile = mappedRasterTile.getLoadingTile();
      if (pLoadingTile && pLoadingTile->getState() ==
                              RasterOverlayTile::LoadState::Placeholder) {
        RasterOverlayTileProvider* pProvider =
            pLoadingTile->getOverlay().getTileProvider();

        // Try to replace this placeholder with real tiles.
        if (pProvider && !pProvider->isPlaceholder() && this->getContent()) {
          // Find a suitable projection in the content.
          // If there isn't one, the mesh doesn't have the right texture
          // coordinates for this overlay and we need to kick it back to the
          // unloaded state to fix that.
          // In the future, we could add the ability to add the required
          // texture coordinates without starting over from scratch.
          const auto& projections =
              this->getContent()->rasterOverlayProjections;
          auto it = std::find(
              projections.begin(),
              projections.end(),
              pProvider->getProjection());
          if (it == projections.end()) {
            this->unloadContent();
            return;
          }

          this->_rasterTiles.erase(
              this->_rasterTiles.begin() +
              static_cast<
                  std::vector<RasterMappedTo3DTile>::iterator::difference_type>(
                  i));
          --i;

          Rectangle projectedRectangle =
              projectRectangleSimple(pProvider->getProjection(), *pRectangle);
          CesiumUtility::IntrusivePointer<RasterOverlayTile> pTile =
              pProvider->getTile(projectedRectangle, this->getGeometricError());

          RasterMappedTo3DTile& mapped = this->_rasterTiles.emplace_back(pTile);
          mapped.setTextureCoordinateID(int32_t(it - projections.begin()));
        }

        continue;
      }

      RasterOverlayTile::MoreDetailAvailable moreDetailAvailable =
          mappedRasterTile.update(*this);
      moreRasterDetailAvailable |=
          moreDetailAvailable == RasterOverlayTile::MoreDetailAvailable::Yes;
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
    const glm::dmat4& transform,
    const BoundingVolume& boundingVolume,
    const std::vector<CesiumGeospatial::Projection>& projections) {
  std::optional<CesiumGeospatial::BoundingRegion> result;

  // Generate texture coordinates for each projection.
  if (!projections.empty()) {
    const CesiumGeospatial::GlobeRectangle* pRectangle =
        Impl::obtainGlobeRectangle(&boundingVolume);
    if (pRectangle) {
      for (size_t i = 0; i < projections.size(); ++i) {
        const Projection& projection = projections[i];

        int textureCoordinateID = static_cast<int32_t>(i);

        CesiumGeometry::Rectangle rectangle =
            projectRectangleSimple(projection, *pRectangle);

        CesiumGeospatial::BoundingRegion boundingRegion =
            GltfContent::createRasterOverlayTextureCoordinates(
                model,
                transform,
                textureCoordinateID,
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
                          projections = std::move(projections),
                          pSubdividedParentID,
                          boundingVolume = this->getBoundingVolume(),
                          pPrepareRendererResources =
                              pTileset->getExternals()
                                  .pPrepareRendererResources]() mutable {
        std::unique_ptr<TileContentLoadResult> pContent =
            std::make_unique<TileContentLoadResult>();
        pContent->model =
            upsampleGltfForRasterOverlays(parentModel, *pSubdividedParentID);

        if (pContent->model) {
          pContent->updatedBoundingVolume = Tile::generateTextureCoordinates(
              pContent->model.value(),
              transform,
              boundingVolume,
              projections);
          pContent->rasterOverlayProjections = std::move(projections);
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

void Tile::loadOverlays(
    std::vector<CesiumGeospatial::Projection>& projections) {
  assert(this->_rasterTiles.empty());
  assert(this->_state == LoadState::ContentLoading);

  Tileset* pTileset = this->getTileset();

  if (pTileset->supportsRasterOverlays()) {
    RasterOverlayCollection& overlays = pTileset->getOverlays();
    mapRasterOverlaysToTile(*this, overlays, projections);
  }
}

} // namespace Cesium3DTilesSelection
