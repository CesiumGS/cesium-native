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
#include <CesiumUtility/JsonHelpers.h>
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
      _boundingVolume(OrientedBoundingBox(glm::dvec3(), glm::dmat3())),
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

double Tile::getNonZeroGeometricError() const noexcept {
  double geometricError = this->getGeometricError();
  if (geometricError > Math::EPSILON5) {
    // Tile's geometric error is good.
    return geometricError;
  }

  const Tile* pParent = this->getParent();
  double divisor = 1.0;

  while (pParent) {
    if (!pParent->getUnconditionallyRefine()) {
      divisor *= 2.0;
      double ancestorError = pParent->getGeometricError();
      if (ancestorError > Math::EPSILON5) {
        return ancestorError / divisor;
      }
    }

    pParent = pParent->getParent();
  }

  // No sensible geometric error all the way to the root of the tile tree.
  // So just use a tiny geometric error and raster selection will be limited by
  // quadtree tile count or texture resolution size.
  return Math::EPSILON5;
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
std::vector<Projection> mapOverlaysToTile(Tile& tile) {
  Tileset& tileset = *tile.getTileset();
  RasterOverlayCollection& overlays = tileset.getOverlays();

  std::vector<Projection> projections;

  for (auto& pOverlay : overlays) {
    RasterMappedTo3DTile* pMapped =
        RasterMappedTo3DTile::mapOverlayToTile(*pOverlay, tile, projections);
    if (pMapped) {
      // Try to load now, but if the mapped raster tile is a placeholder this
      // won't do anything.
      pMapped->loadThrottled();
    }
  }

  return projections;
}

const BoundingVolume& getEffectiveBoundingVolume(
    const BoundingVolume& tileBoundingVolume,
    const std::optional<BoundingVolume>& updatedTileBoundingVolume,
    [[maybe_unused]] const std::optional<BoundingVolume>&
        updatedTileContentBoundingVolume) {
  // If we have an updated tile bounding volume, use it.
  if (updatedTileBoundingVolume) {
    return *updatedTileBoundingVolume;
  }

  // If we _only_ have an updated _content_ bounding volume, that's a developer
  // error.
  assert(!updatedTileContentBoundingVolume);

  return tileBoundingVolume;
}

const BoundingVolume& getEffectiveContentBoundingVolume(
    const BoundingVolume& tileBoundingVolume,
    const std::optional<BoundingVolume>& tileContentBoundingVolume,
    const std::optional<BoundingVolume>& updatedTileBoundingVolume,
    const std::optional<BoundingVolume>& updatedTileContentBoundingVolume) {
  // If we have an updated tile content bounding volume, use it.
  if (updatedTileContentBoundingVolume) {
    return *updatedTileContentBoundingVolume;
  }

  // Next best thing is an updated tile non-content bounding volume.
  if (updatedTileBoundingVolume) {
    return *updatedTileBoundingVolume;
  }

  // Then a content bounding volume attached to the tile.
  if (tileContentBoundingVolume) {
    return *tileContentBoundingVolume;
  }

  // And finally the regular tile bounding volume.
  return tileBoundingVolume;
}

/**
 * @brief Called in a worker thread to prepare newly loaded or upsampled content
 * for use.
 *
 * @param pPrepareRendererResources
 * @param pLogger
 * @param content
 * @param generateMissingNormalsSmooth
 * @param gltfUpAxis
 * @param tileTransform
 * @param contentBoundingVolume
 * @param boundingVolume
 * @param projections
 * @return The opaque pointer to the renderer resources as returned by
 * {@link IPrepareRendererResources::prepareInLoadThread}.
 */
void* processNewTileContent(
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    TileContentLoadResult& content,
    bool generateMissingNormalsSmooth,
    CesiumGeometry::Axis gltfUpAxis,
    const glm::dmat4& tileTransform,
    const std::optional<BoundingVolume>& tileContentBoundingVolume,
    const BoundingVolume& tileBoundingVolume,
    std::vector<Projection>&& projections) {
  if (!content.model) {
    return nullptr;
  }

  CesiumGltf::Model& model = *content.model;

  // TODO The `extras` are currently the only way to
  // pass arbitrary information to the consumer, so the
  // up-axis is stored here:
  model.extras["gltfUpAxis"] =
      static_cast<std::underlying_type_t<Axis>>(gltfUpAxis);

  const BoundingVolume& contentBoundingVolume =
      getEffectiveContentBoundingVolume(
          tileBoundingVolume,
          tileContentBoundingVolume,
          content.updatedBoundingVolume,
          content.updatedContentBoundingVolume);

  // If we have projections, generate texture coordinates for all of them. Also
  // remember the min and max height so that we can use them for upsampling.
  const BoundingRegion* pRegion =
      getBoundingRegionFromBoundingVolume(contentBoundingVolume);
  content.overlayDetails = GltfContent::createRasterOverlayTextureCoordinates(
      model,
      tileTransform,
      0,
      pRegion ? std::make_optional(pRegion->getRectangle()) : std::nullopt,
      std::move(projections));

  if (pRegion && content.overlayDetails) {
    // If the original bounding region was wrong, report it.
    const GlobeRectangle& original = pRegion->getRectangle();
    const GlobeRectangle& computed =
        content.overlayDetails->boundingRegion.getRectangle();
    if (!Math::equalsEpsilon(computed.getWest(), original.getWest(), 0.01) &&
            computed.getWest() < original.getWest() ||
        !Math::equalsEpsilon(computed.getSouth(), original.getSouth(), 0.01) &&
            computed.getSouth() < original.getSouth() ||
        !Math::equalsEpsilon(computed.getEast(), original.getEast(), 0.01) &&
            computed.getEast() > original.getEast() ||
        !Math::equalsEpsilon(computed.getNorth(), original.getNorth(), 0.01) &&
            computed.getNorth() > original.getNorth()) {

      auto it = model.extras.find("Cesium3DTiles_TileUrl");
      std::string url = it != model.extras.end()
                            ? it->second.getStringOrDefault("Unknown Tile URL")
                            : "Unknown Tile URL";
      SPDLOG_LOGGER_WARN(
          pLogger,
          "Tile has a bounding volume that does not include all of its "
          "content, so culling and raster overlays may be incorrect: {}",
          url);
    }
  }

  // If our tile bounding region has loose fitting heights, find the real ones.
  const BoundingVolume& boundingVolume = getEffectiveBoundingVolume(
      tileBoundingVolume,
      content.updatedBoundingVolume,
      content.updatedContentBoundingVolume);
  if (std::get_if<BoundingRegionWithLooseFittingHeights>(&boundingVolume) !=
      nullptr) {
    if (content.overlayDetails) {
      // We already computed the bounding region for overlays, so use it.
      content.updatedBoundingVolume = content.overlayDetails->boundingRegion;
    } else {
      // We need to compute an accurate bounding region
      content.updatedBoundingVolume =
          GltfContent::computeBoundingRegion(model, tileTransform);
    }
  }

  if (generateMissingNormalsSmooth) {
    content.model->generateMissingNormalsSmooth();
  }

  void* pRendererResources = nullptr;

  if (pPrepareRendererResources) {
    CESIUM_TRACE("prepareInLoadThread");
    pRendererResources =
        pPrepareRendererResources->prepareInLoadThread(model, tileTransform);
  }

  return pRendererResources;
}

} // namespace

void Tile::loadContent() {
  if (this->getState() != LoadState::Unloaded) {
    // No need to load geometry, but give previously-throttled
    // raster overlay tiles a chance to load.
    for (RasterMappedTo3DTile& mapped : this->getMappedRasterTiles()) {
      mapped.loadThrottled();
    }
    return;
  }

  this->setState(LoadState::ContentLoading);

  Tileset& tileset = *this->getTileset();

  // If this is an upsampled tile, we need to derive this tile's content from
  // its parent.
  const UpsampledQuadtreeNode* pSubdivided =
      std::get_if<UpsampledQuadtreeNode>(&this->getTileID());
  if (pSubdivided) {
    // We can't upsample this tile until its parent tile is done loading.
    if (this->getParent()) {
      if (this->getParent()->getState() == LoadState::Done) {
        std::vector<Projection> projections = mapOverlaysToTile(*this);
        this->upsampleParent(std::move(projections));
      } else {
        // Try again later. Parent tile is LoadState::Unloaded so attempt to
        // load its content.

        // Note: Since the current tile is an upsampled node, we can assume
        // that either the parent is also upsampled, or the parent has content.
        this->getParent()->loadContent();
        this->setState(LoadState::Unloaded);
      }
    } else {
      // This shouldn't happen.
      assert(this->getParent() != nullptr);
    }

    return;
  }

  std::vector<Projection> projections = mapOverlaysToTile(*this);

  struct LoadResult {
    LoadState state = LoadState::Unloaded;
    std::unique_ptr<TileContentLoadResult> pContent = nullptr;
    void* pRendererResources = nullptr;
  };

  TileContentLoadInput loadInput(*this);

  const CesiumGeometry::Axis gltfUpAxis = tileset.getGltfUpAxis();
  tileset.requestTileContent(*this)
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
                    if (statusCode != 0 &&
                        (statusCode < 200 || statusCode >= 300)) {
                      return LoadResult{
                          LoadState::FailedTemporarily,
                          std::move(pContent),
                          nullptr};
                    }

                    pRendererResources = processNewTileContent(
                        pPrepareRendererResources,
                        loadInput.pLogger,
                        *pContent,
                        generateMissingNormalsSmooth,
                        gltfUpAxis,
                        loadInput.tileTransform,
                        loadInput.tileContentBoundingVolume,
                        loadInput.tileBoundingVolume,
                        std::move(projections));
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

void Tile::processLoadedContent() {
  const TilesetExternals& externals = this->getTileset()->getExternals();

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

        // Initialize the new contexts, if there are any.
        for (auto&& pNewContext : this->_pContent->newTileContexts) {
          if (pNewContext) {
            if (pNewContext->contextInitializerCallback) {
              pNewContext->contextInitializerCallback(
                  *this->_pContext,
                  *pNewContext);
            }

            this->_pContext->pTileset->addContext(std::move(pNewContext));
          }
        }
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
        const QuadtreeTileID* pQuadtreeTileID =
            std::get_if<QuadtreeTileID>(&this->getTileID());
        if (pQuadtreeTileID && context.quadtreeTilingScheme &&
            context.rectangleAvailability &&
            !this->_pContent->availableTileRectangles.empty()) {
          for (const QuadtreeTileRectangularRange& range :
               this->_pContent->availableTileRectangles) {
            context.rectangleAvailability->addAvailableTileRange(range);
          }
        }
      }
    }

    this->setState(LoadState::Done);
  }
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

namespace {

struct RegionAndCenter {
  BoundingRegion region;
  Cartographic center;
};

std::optional<RegionAndCenter>
getTileBoundingRegionForUpsampling(const Tile& parent) {
  // To create subdivided children, we need to know a bounding region for each.
  // If the parent is already loaded and we have Web Mercator or Geographic
  // textures coordinates, we're set. If it's not, but it has a bounding region,
  // we're still set. Otherwise, we can't upsample (yet?).

  // Get an accurate bounding region from the content first.
  const TileContentLoadResult* pParentContent = parent.getContent();
  if (pParentContent && pParentContent->overlayDetails) {
    const TileContentDetailsForOverlays& details =
        *pParentContent->overlayDetails;

    // If we don't have any overlay projections/rectangles, why are we
    // upsampling?
    assert(!details.rasterOverlayProjections.empty());
    assert(!details.rasterOverlayRectangles.empty());

    // Use the projected center of the tile as the subdivision center.
    // The tile will be subdivided by (0.5, 0.5) in the _first_ overlay's
    // texture coordinates (no matter which overlay(s) had more detail), at
    // least until https://github.com/CesiumGS/cesium-native/issues/385 is
    // addressed. So pick the center accordingly.
    glm::dvec2 centerProjected = details.rasterOverlayRectangles[0].getCenter();
    Cartographic center = unprojectPosition(
        details.rasterOverlayProjections[0],
        glm::dvec3(centerProjected, 0.0));

    return RegionAndCenter{details.boundingRegion, center};
  }

  // We shouldn't be upsampling from a tile until that tile is loaded.
  // If it has no content after loading, we can't upsample from it.
  return std::nullopt;
}

void createQuadtreeSubdividedChildren(Tile& parent) {
  std::optional<RegionAndCenter> maybeRegionAndCenter =
      getTileBoundingRegionForUpsampling(parent);
  if (!maybeRegionAndCenter) {
    return;
  }

  // The quadtree tile ID doesn't actually matter, because we're not going to
  // use the standard tile bounds for the ID. But having a tile ID that reflects
  // the level and _approximate_ location is helpful for debugging.
  const QuadtreeTileID* pRealParentTileID =
      std::get_if<QuadtreeTileID>(&parent.getTileID());
  if (!pRealParentTileID) {
    const UpsampledQuadtreeNode* pUpsampledID =
        std::get_if<UpsampledQuadtreeNode>(&parent.getTileID());
    if (pUpsampledID) {
      pRealParentTileID = &pUpsampledID->tileID;
    }
  }

  QuadtreeTileID parentTileID =
      pRealParentTileID ? *pRealParentTileID : QuadtreeTileID(0, 0, 0);

  // QuadtreeTileID can't handle higher than level 30 because the x and y
  // coordinates (uint32_t) will overflow. So just start over at level 0.
  if (parentTileID.level >= 30U) {
    parentTileID = QuadtreeTileID(0, 0, 0);
  }

  // The parent tile must not have a zero geometric error, even if it's a leaf
  // tile. Otherwise we'd never refine it.
  parent.setGeometricError(parent.getNonZeroGeometricError());

  // The parent must use REPLACE refinement.
  parent.setRefine(TileRefine::Replace);

  const QuadtreeTileID swID(
      parentTileID.level + 1,
      parentTileID.x * 2,
      parentTileID.y * 2);
  const QuadtreeTileID seID(swID.level, swID.x + 1, swID.y);
  const QuadtreeTileID nwID(swID.level, swID.x, swID.y + 1);
  const QuadtreeTileID neID(swID.level, swID.x + 1, swID.y + 1);

  parent.createChildTiles(4);

  const gsl::span<Tile> children = parent.getChildren();
  Tile& sw = children[0];
  Tile& se = children[1];
  Tile& nw = children[2];
  Tile& ne = children[3];

  // Use the root tile's context, because that's guaranteed not to do any
  // implicit tiling.
  assert(
      parent.getContext() != nullptr &&
      parent.getContext()->pTileset != nullptr &&
      parent.getContext()->pTileset->getRootTile() != nullptr);
  TileContext* pRootContext =
      parent.getContext()->pTileset->getRootTile()->getContext();
  assert(pRootContext != nullptr);
  sw.setContext(pRootContext);
  se.setContext(pRootContext);
  nw.setContext(pRootContext);
  ne.setContext(pRootContext);

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

  const double minimumHeight = maybeRegionAndCenter->region.getMinimumHeight();
  const double maximumHeight = maybeRegionAndCenter->region.getMaximumHeight();

  const GlobeRectangle& parentRectangle =
      maybeRegionAndCenter->region.getRectangle();
  const Cartographic& center = maybeRegionAndCenter->center;
  sw.setBoundingVolume(BoundingRegionWithLooseFittingHeights(BoundingRegion(
      GlobeRectangle(
          parentRectangle.getWest(),
          parentRectangle.getSouth(),
          center.longitude,
          center.latitude),
      minimumHeight,
      maximumHeight)));

  se.setBoundingVolume(BoundingRegionWithLooseFittingHeights(BoundingRegion(
      GlobeRectangle(
          center.longitude,
          parentRectangle.getSouth(),
          parentRectangle.getEast(),
          center.latitude),
      minimumHeight,
      maximumHeight)));

  nw.setBoundingVolume(BoundingRegionWithLooseFittingHeights(BoundingRegion(
      GlobeRectangle(
          parentRectangle.getWest(),
          center.latitude,
          center.longitude,
          parentRectangle.getNorth()),
      minimumHeight,
      maximumHeight)));

  ne.setBoundingVolume(BoundingRegionWithLooseFittingHeights(BoundingRegion(
      GlobeRectangle(
          center.longitude,
          center.latitude,
          parentRectangle.getEast(),
          parentRectangle.getNorth()),
      minimumHeight,
      maximumHeight)));

  sw.setTransform(parent.getTransform());
  se.setTransform(parent.getTransform());
  nw.setTransform(parent.getTransform());
  ne.setTransform(parent.getTransform());
}

} // namespace

void Tile::update(
    int32_t /*previousFrameNumber*/,
    int32_t /*currentFrameNumber*/) {

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

  // TODO: if there's no model, we can actually free any existing overlays.
  if (this->getState() == LoadState::Done &&
      this->getTileset()->supportsRasterOverlays() && this->getContent() &&
      this->getContent()->model) {
    bool moreRasterDetailAvailable = false;

    for (size_t i = 0; i < this->_rasterTiles.size(); ++i) {
      RasterMappedTo3DTile& mappedRasterTile = this->_rasterTiles[i];

      RasterOverlayTile* pLoadingTile = mappedRasterTile.getLoadingTile();
      if (pLoadingTile && pLoadingTile->getState() ==
                              RasterOverlayTile::LoadState::Placeholder) {
        RasterOverlayTileProvider* pProvider =
            pLoadingTile->getOverlay().getTileProvider();

        // Try to replace this placeholder with real tiles.
        if (pProvider && !pProvider->isPlaceholder()) {
          // Remove the existing placeholder mapping
          this->_rasterTiles.erase(
              this->_rasterTiles.begin() +
              static_cast<
                  std::vector<RasterMappedTo3DTile>::iterator::difference_type>(
                  i));
          --i;

          // Add a new mapping.
          std::vector<Projection> missingProjections;
          RasterMappedTo3DTile::mapOverlayToTile(
              pProvider->getOwner(),
              *this,
              missingProjections);

          if (!missingProjections.empty()) {
            // The mesh doesn't have the right texture coordinates for this
            // overlay's projection, so we need to kick it back to the unloaded
            // state to fix that.
            // In the future, we could add the ability to add the required
            // texture coordinates without starting over from scratch.
            this->unloadContent();
            return;
          }
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

void Tile::setEmptyContent() noexcept {
  this->_pContent = std::make_unique<TileContentLoadResult>();
}

void Tile::setState(LoadState value) noexcept {
  this->_state.store(value, std::memory_order::memory_order_release);
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
      .runInWorkerThread(
          [&parentModel,
           transform = this->getTransform(),
           projections = std::move(projections),
           pSubdividedParentID,
           tileBoundingVolume = this->getBoundingVolume(),
           tileContentBoundingVolume = this->getContentBoundingVolume(),
           gltfUpAxis = pTileset->getGltfUpAxis(),
           generateMissingNormalsSmooth =
               pTileset->getOptions()
                   .contentOptions.generateMissingNormalsSmooth,
           pLogger = pTileset->getExternals().pLogger,
           pPrepareRendererResources =
               pTileset->getExternals().pPrepareRendererResources]() mutable {
            std::unique_ptr<TileContentLoadResult> pContent =
                std::make_unique<TileContentLoadResult>();
            pContent->model = upsampleGltfForRasterOverlays(
                parentModel,
                *pSubdividedParentID);

            // We can't necessarily trust our original bounding volume, so
            // recompute it here. See:
            // https://github.com/CesiumGS/cesium-native/issues/385
            pContent->updatedBoundingVolume =
                GltfContent::computeBoundingRegion(*pContent->model, transform);

            void* pRendererResources = processNewTileContent(
                pPrepareRendererResources,
                pLogger,
                *pContent,
                generateMissingNormalsSmooth,
                gltfUpAxis,
                transform,
                tileContentBoundingVolume,
                tileBoundingVolume,
                std::move(projections));

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

} // namespace Cesium3DTilesSelection
