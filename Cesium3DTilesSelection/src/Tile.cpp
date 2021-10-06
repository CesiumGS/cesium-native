#include "Cesium3DTilesSelection/Tile.h"

#include "Cesium3DTilesSelection/GltfContent.h"
#include "Cesium3DTilesSelection/IPrepareRendererResources.h"
#include "Cesium3DTilesSelection/TileContentFactory.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "CesiumGeometry/TileAvailabilityFlags.h"
#include "TileUtilities.h"
#include "upsampleGltfForRasterOverlays.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumAsync/ITaskProcessor.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/AxisTransforms.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Transforms.h>
#include <CesiumGltf/Model.h>
#include <CesiumUtility/Tracing.h>

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
      _availability(0),
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
      _availability(rhs._availability),
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
    this->_availability = rhs._availability;
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
          [](const RasterMappedTo3DTile& rasterTile) noexcept {
            return rasterTile.getReadyTile() != nullptr;
          });
    }
  }

  return false;
}

bool Tile::isExternalTileset() const noexcept {
  return this->getState() >= LoadState::ContentLoaded && this->_pContent &&
         !this->_pContent->model;
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
    const CesiumGeometry::Rectangle overlayRectangle =
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

      const int32_t projectionID =
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
  std::optional<Future<std::shared_ptr<IAssetRequest>>>
      maybeSubtreeRequestFuture = tileset.requestAvailabilitySubtree(*this);

  // TODO: rethink some of this logic, in implicit tiling, a tile may be
  // available but have no content. It may still have children. Should we
  // upsample according to "tile unavailablility" or "content unavailability".
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

  struct RequestResults {
    std::shared_ptr<IAssetRequest> pContentRequest;
    std::shared_ptr<IAssetRequest> pSubtreeRequest;
  };

  struct LoadResult {
    LoadState state = LoadState::Unloaded;
    std::unique_ptr<TileContentLoadResult> pContent = nullptr;
    void* pRendererResources = nullptr;
  };

  TileContentLoadInput loadInput(*this);

  const CesiumGeometry::Axis gltfUpAxis = tileset.getGltfUpAxis();
  std::move(maybeRequestFuture.value())
      .thenInWorkerThread(
          [asyncSystem = tileset.getAsyncSystem(),
           maybeSubtreeRequestFuture = std::move(maybeSubtreeRequestFuture)](
              std::shared_ptr<IAssetRequest>&& pContentRequest) mutable {
            if (maybeSubtreeRequestFuture) {
              return std::move(maybeSubtreeRequestFuture.value())
                  .thenInWorkerThread(
                      [pContentRequest = std::move(pContentRequest)](
                          std::shared_ptr<IAssetRequest>&& pSubtreeRequest) {
                        return RequestResults{
                            std::move(pContentRequest),
                            std::move(pSubtreeRequest)};
                      });
            } else {
              return asyncSystem.createResolvedFuture(
                  RequestResults{std::move(pContentRequest), nullptr});
            }
          })
      .thenInWorkerThread(
          [loadInput = std::move(loadInput),
           asyncSystem = tileset.getAsyncSystem(),
           pLogger = tileset.getExternals().pLogger,
           pAssetAccessor = tileset.getExternals().pAssetAccessor,
           gltfUpAxis,
           projections = std::move(projections),
           generateMissingNormalsSmooth =
               tileset.getOptions().contentOptions.generateMissingNormalsSmooth,
           pPrepareRendererResources =
               tileset.getExternals().pPrepareRendererResources](
              RequestResults&& requestResults) mutable {
            CESIUM_TRACE("loadContent worker thread");

            std::shared_ptr<IAssetRequest>& pRequest =
                requestResults.pContentRequest;
            std::shared_ptr<IAssetRequest>& pSubtreeRequest =
                requestResults.pSubtreeRequest;

            const IAssetResponse* pResponse = pRequest->response();
            if (!pResponse) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Did not receive a valid response for tile content {}",
                  pRequest->url());
              auto pLoadResult = std::make_unique<TileContentLoadResult>();
              pLoadResult->httpStatusCode = 0;
              return asyncSystem.createResolvedFuture(LoadResult{
                  LoadState::FailedTemporarily,
                  std::move(pLoadResult),
                  nullptr});
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
              return asyncSystem.createResolvedFuture(LoadResult{
                  LoadState::FailedTemporarily,
                  std::move(pLoadResult),
                  nullptr});
            }

            loadInput.asyncSystem = std::move(asyncSystem);
            loadInput.pLogger = std::move(pLogger);
            loadInput.pAssetAccessor = std::move(pAssetAccessor);
            loadInput.pRequest = std::move(pRequest);
            loadInput.pSubtreeRequest = std::move(pSubtreeRequest);

            return TileContentFactory::createContent(loadInput)
                // Forward status code to the load result.
                .thenInWorkerThread([statusCode = pResponse->statusCode(),
                                     loadInput = std::move(loadInput),
                                     gltfUpAxis,
                                     projections = std::move(projections),
                                     generateMissingNormalsSmooth,
                                     pPrepareRendererResources =
                                         std::move(pPrepareRendererResources)](
                                        std::unique_ptr<TileContentLoadResult>&&
                                            pContent) mutable {
                  void* pRendererResources = nullptr;

                  if (pContent) {
                    pContent->httpStatusCode = statusCode;
                    if (statusCode < 200 || statusCode >= 300) {
                      return LoadResult{
                          LoadState::FailedTemporarily,
                          std::move(pContent),
                          nullptr};
                    }

                    if (pContent->model) {

                      CesiumGltf::Model& model = pContent->model.value();

                      // TODO The `extras` are currently the only way to
                      // pass arbitrary information to the consumer, so the
                      // up-axis is stored here:
                      model.extras["gltfUpAxis"] =
                          static_cast<std::underlying_type_t<Axis>>(gltfUpAxis);

                      Tile::generateTextureCoordinates(
                          model,
                          loadInput.tileTransform,
                          // Would it be better to use the content bounding
                          // volume, if it exists?
                          // What about
                          // TileContentLoadResult::updatedBoundingVolume?
                          loadInput.tileBoundingVolume,
                          projections);

                      pContent->rasterOverlayProjections =
                          std::move(projections);

                      if (generateMissingNormalsSmooth) {
                        pContent->model->generateMissingNormalsSmooth();
                      }

                      if (pPrepareRendererResources) {
                        CESIUM_TRACE("prepareInLoadThread");
                        pRendererResources =
                            pPrepareRendererResources->prepareInLoadThread(
                                pContent->model.value(),
                                loadInput.tileTransform);
                      }
                    }
                  }

                  return LoadResult{
                      LoadState::ContentLoaded,
                      std::move(pContent),
                      pRendererResources};
                });
          })
      .thenInMainThread([this](LoadResult&& loadResult) noexcept {
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

static void createImplicitQuadtreeTile(
    const ImplicitTilingContext& implicitContext,
    Tile& parent,
    Tile& child,
    const QuadtreeTileID& childID,
    uint8_t availability) {

  if (!implicitContext.quadtreeTilingScheme) {
    return;
  }

  child.setContext(parent.getContext());
  child.setParent(&parent);
  child.setAvailability(availability);

  if (availability & TileAvailabilityFlags::TILE_AVAILABLE) {
    child.setTileID(childID);
  } else /*if (upsample) */ {
    child.setTileID(UpsampledQuadtreeNode{childID});
  }

  // TODO: check override geometric error from metadata
  child.setGeometricError(parent.getGeometricError() * 0.5);

  const BoundingRegion* pRegion =
      std::get_if<BoundingRegion>(&implicitContext.implicitRootBoundingVolume);
  const BoundingRegionWithLooseFittingHeights* pLooseRegion =
      std::get_if<BoundingRegionWithLooseFittingHeights>(
          &implicitContext.implicitRootBoundingVolume);
  const OrientedBoundingBox* pBox = std::get_if<OrientedBoundingBox>(
      &implicitContext.implicitRootBoundingVolume);

  if (!pRegion && pLooseRegion) {
    pRegion = &pLooseRegion->getBoundingRegion();
  }

  if (pRegion && implicitContext.projection) {
    double minimumHeight = -1000.0;
    double maximumHeight = 9000.0;

    const BoundingRegion* pParentRegion =
        std::get_if<BoundingRegion>(&parent.getBoundingVolume());
    if (!pParentRegion) {
      const BoundingRegionWithLooseFittingHeights* pParentLooseRegion =
          std::get_if<BoundingRegionWithLooseFittingHeights>(
              &parent.getBoundingVolume());
      if (pParentLooseRegion) {
        pParentRegion = &pParentLooseRegion->getBoundingRegion();
      }
    }

    if (pParentRegion) {
      minimumHeight = pParentRegion->getMinimumHeight();
      maximumHeight = pParentRegion->getMaximumHeight();
    }

    child.setBoundingVolume(
        BoundingRegionWithLooseFittingHeights(BoundingRegion(
            unprojectRectangleSimple(
                *implicitContext.projection,
                implicitContext.quadtreeTilingScheme->tileToRectangle(childID)),
            minimumHeight,
            maximumHeight)));

  } else if (pBox) {
    CesiumGeometry::Rectangle rectangleLocal =
        implicitContext.quadtreeTilingScheme->tileToRectangle(childID);
    glm::dvec2 centerLocal = rectangleLocal.getCenter();
    const glm::dmat3& rootHalfAxes = pBox->getHalfAxes();
    child.setBoundingVolume(OrientedBoundingBox(
        rootHalfAxes * glm::dvec3(centerLocal.x, centerLocal.y, 0.0),
        glm::dmat3(
            0.5 * rectangleLocal.computeWidth() * rootHalfAxes[0],
            0.5 * rectangleLocal.computeHeight() * rootHalfAxes[1],
            rootHalfAxes[2])));
  }
}

/*
static void createImplicitOctreeTile(
    const ImplicitTilingContext& implicitContext,
    Tile& parent,
    Tile& child,
    const OctreeTileID& childID,
    bool available) {

  if (!implicitContext.octreeTilingScheme) {
    return;
  }

  child.setContext(parent.getContext());
  child.setParent(&parent);

  if (available) {
    child.setTileID(childID);
  } else {
    // TODO: upsample missing octree tiles
    // child.setTileID(UpsampledOctreeNode{childID});
  }

  // TODO: check for overrided geometric error metadata
  child.setGeometricError(parent.getGeometricError() * 0.5);

  const BoundingRegion* pRegion =
      std::get_if<BoundingRegion>(&implicitContext.implicitRootBoundingVolume);
  const OrientedBoundingBox* pBox =
      std::get_if<OrientedBoundingBox>(&implicitContext.implicitRootBoundingVolume);

  if (pRegion && implicitContext.projection) {
    child.setBoundingVolume(
        unprojectRegionSimple(
            *implicitContext.projection,
            implicitContext.octreeTilingScheme->tileToBox(childID)));
  } else if (pBox) {
    AxisAlignedBox childLocal =
        implicitContext.octreeTilingScheme->tileToBox(childID);
    const glm::dvec3& centerLocal = childLocal.center;
    const glm::dmat3& rootHalfAxes = pBox->getHalfAxes();
    child.setBoundingVolume(
        OrientedBoundingBox(
          rootHalfAxes * centerLocal,
          glm::dmat3(
            0.5 * childLocal.lengthX * rootHalfAxes[0],
            0.5 * childLocal.lengthY * rootHalfAxes[1],
            0.5 * childLocal.lengthZ * rootHalfAxes[2])));
  }
}*/

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

  ImplicitTilingContext& implicitContext =
      *parent.getContext()->implicitContext;

  if (!implicitContext.quadtreeTilingScheme || !implicitContext.projection) {
    return;
  }

  const QuadtreeTileID swID(
      pParentTileID->level + 1,
      pParentTileID->x * 2,
      pParentTileID->y * 2);
  const QuadtreeTileID seID(swID.level, swID.x + 1, swID.y);
  const QuadtreeTileID nwID(swID.level, swID.x, swID.y + 1);
  const QuadtreeTileID neID(swID.level, swID.x + 1, swID.y + 1);

  QuadtreeTilingScheme& tilingScheme = *implicitContext.quadtreeTilingScheme;
  Projection& projection = *implicitContext.projection;

  parent.createChildTiles(4);

  const gsl::span<Tile> children = parent.getChildren();
  Tile& sw = children[0];
  Tile& se = children[1];
  Tile& nw = children[2];
  Tile& ne = children[3];

  sw.setContext(parent.getContext());
  se.setContext(parent.getContext());
  nw.setContext(parent.getContext());
  ne.setContext(parent.getContext());

  const double geometricError = parent.getGeometricError() * 0.5;
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

  const double minimumHeight = pRegion->getMinimumHeight();
  const double maximumHeight = pRegion->getMaximumHeight();

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
      const FailedTileAction action =
          this->_pContext->failedTileCallback(*this);
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

  QuadtreeTileID* pQuadtreeTileID = std::get_if<QuadtreeTileID>(&this->_id);
  OctreeTileID* pOctreeTileID = std::get_if<OctreeTileID>(&this->_id);

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

      // If this tile has no model, we want to unconditionally refine past it.
      // Note that "no" model is different from having a model, but it is blank.
      // In the latter case, we'll happily render nothing in the space of this
      // tile, which is sometimes useful.
      if (!this->_pContent->model) {
        this->setUnconditionallyRefine();
      }

      // A new and improved bounding volume.
      if (this->_pContent->updatedBoundingVolume) {
        this->setBoundingVolume(this->_pContent->updatedBoundingVolume.value());
      }

      if (this->getContext()->implicitContext) {
        ImplicitTilingContext& context = *this->getContext()->implicitContext;

        if (pQuadtreeTileID && context.quadtreeTilingScheme) {
          if (context.rectangleAvailability) {
            if (!this->_pContent->availableTileRectangles.empty()) {
              for (const QuadtreeTileRectangularRange& range :
                   this->_pContent->availableTileRectangles) {
                context.rectangleAvailability->addAvailableTileRange(range);
              }
            }
          } else if (context.quadtreeSubtreeAvailability) {
            if (this->_pContent->subtreeLoadResult) {
              context.quadtreeSubtreeAvailability->addSubtree(
                  *pQuadtreeTileID,
                  std::move(*this->_pContent->subtreeLoadResult));
            }
          }
        } else if (
            pOctreeTileID && context.octreeTilingScheme
            /*&& context.octreeSubtreeAvailability*/) {
          // TODO: handle octree case
        }
      }
    }

    this->setState(LoadState::Done);
  }

  if (this->getContext()->implicitContext && this->getChildren().empty()) {
    if (pQuadtreeTileID) {
      // Check if any child tiles are known to be available, and create them if
      // they are.
      const ImplicitTilingContext& implicitContext =
          this->getContext()->implicitContext.value();

      // TODO: generalize availability
      const QuadtreeTileID swID(
          pQuadtreeTileID->level + 1,
          pQuadtreeTileID->x * 2,
          pQuadtreeTileID->y * 2);
      const QuadtreeTileID seID(swID.level, swID.x + 1, swID.y);
      const QuadtreeTileID nwID(swID.level, swID.x, swID.y + 1);
      const QuadtreeTileID neID(swID.level, swID.x + 1, swID.y + 1);

      uint8_t sw = 0;
      uint8_t se = 0;
      uint8_t nw = 0;
      uint8_t ne = 0;

      if (implicitContext.rectangleAvailability) {
        sw = implicitContext.rectangleAvailability->isTileAvailable(swID) ? 1
                                                                          : 0;
        se = implicitContext.rectangleAvailability->isTileAvailable(seID) ? 1
                                                                          : 0;
        nw = implicitContext.rectangleAvailability->isTileAvailable(nwID) ? 1
                                                                          : 0;
        ne = implicitContext.rectangleAvailability->isTileAvailable(neID) ? 1
                                                                          : 0;
      } else if (implicitContext.quadtreeSubtreeAvailability) {
        sw = implicitContext.quadtreeSubtreeAvailability->computeAvailability(
            swID);
        se = implicitContext.quadtreeSubtreeAvailability->computeAvailability(
            seID);
        nw = implicitContext.quadtreeSubtreeAvailability->computeAvailability(
            nwID);
        ne = implicitContext.quadtreeSubtreeAvailability->computeAvailability(
            neID);
      }

      size_t childCount = sw + se + nw + ne;
      if (childCount > 0) {
        // If any children are available, we need to create all four in order to
        // avoid holes. But non-available tiles will be upsampled instead of
        // loaded.
        // TODO: this is the right thing to do for terrain, which is the only
        // use of implicit tiling currently. But we may need to re-evaluate it
        // if we're using implicit tiling for buildings (for example) in the
        // future.
        this->_children.resize(4);

        createImplicitQuadtreeTile(
            implicitContext,
            *this,
            this->_children[0],
            swID,
            sw);
        createImplicitQuadtreeTile(
            implicitContext,
            *this,
            this->_children[1],
            seID,
            se);
        createImplicitQuadtreeTile(
            implicitContext,
            *this,
            this->_children[2],
            nwID,
            nw);
        createImplicitQuadtreeTile(
            implicitContext,
            *this,
            this->_children[3],
            neID,
            ne);
      }

    } else if (pOctreeTileID) {
      // TODO: handle Octree case
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

          const CesiumGeometry::Rectangle projectedRectangle =
              projectRectangleSimple(pProvider->getProjection(), *pRectangle);
          CesiumUtility::IntrusivePointer<RasterOverlayTile> pTile =
              pProvider->getTile(projectedRectangle, this->getGeometricError());

          RasterMappedTo3DTile& mapped = this->_rasterTiles.emplace_back(pTile);
          mapped.setTextureCoordinateID(int32_t(it - projections.begin()));
        }

        continue;
      }

      const RasterOverlayTile::MoreDetailAvailable moreDetailAvailable =
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
      const int32_t bufferView = image.bufferView;
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

        const int textureCoordinateID = static_cast<int32_t>(i);

        const CesiumGeometry::Rectangle rectangle =
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
      .thenInMainThread([this](LoadResult&& loadResult) noexcept {
        this->_pContent = std::move(loadResult.pContent);
        this->_pRendererResources = loadResult.pRendererResources;
        this->getTileset()->notifyTileDoneLoading(this);
        this->setState(loadResult.state);
      })
      .catchInMainThread([this](const std::exception& /*e*/) noexcept {
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
    const RasterOverlayCollection& overlays = pTileset->getOverlays();
    mapRasterOverlaysToTile(*this, overlays, projections);
  }
}

} // namespace Cesium3DTilesSelection
