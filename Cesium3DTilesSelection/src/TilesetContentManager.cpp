#include "TilesetContentManager.h"

#include "CesiumIonTilesetLoader.h"
#include "LayerJsonTerrainLoader.h"
#include "TileContentLoadInfo.h"
#include "TilesetJsonLoader.h"

#include <Cesium3DTilesSelection/IPrepareRendererResources.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/RasterOverlayUtilities.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/joinToString.h>

#include <rapidjson/document.h>
#include <spdlog/logger.h>

#include <chrono>
#include <set>

using namespace CesiumGltfContent;
using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {
namespace {
struct RegionAndCenter {
  CesiumGeospatial::BoundingRegion region;
  CesiumGeospatial::Cartographic center;
};

struct ContentKindSetter {
  void operator()(TileUnknownContent content) {
    tileContent.setContentKind(content);
  }

  void operator()(TileEmptyContent content) {
    tileContent.setContentKind(content);
  }

  void operator()(TileExternalContent&& content) {
    tileContent.setContentKind(
        std::make_unique<TileExternalContent>(std::move(content)));
  }

  void operator()(CesiumGltf::Model&& model) {
    for (CesiumGltf::Image& image : model.images) {
      // If the image size hasn't been overridden, store the pixelData
      // size now. We'll be adding this number to our total memory usage soon,
      // and remove it when the tile is later unloaded, and we must use
      // the same size in each case.
      if (image.cesium.sizeBytes < 0) {
        image.cesium.sizeBytes = int64_t(image.cesium.pixelData.size());
      }
    }

    auto pRenderContent = std::make_unique<TileRenderContent>(std::move(model));
    pRenderContent->setRenderResources(pRenderResources);
    if (rasterOverlayDetails) {
      pRenderContent->setRasterOverlayDetails(std::move(*rasterOverlayDetails));
    }

    tileContent.setContentKind(std::move(pRenderContent));
  }

  TileContent& tileContent;
  std::optional<RasterOverlayDetails> rasterOverlayDetails;
  void* pRenderResources;
};

void unloadTileRecursively(
    Tile& tile,
    TilesetContentManager& tilesetContentManager) {
  tilesetContentManager.unloadTileContent(tile);
  for (Tile& child : tile.getChildren()) {
    unloadTileRecursively(child, tilesetContentManager);
  }
}

bool anyRasterOverlaysNeedLoading(const Tile& tile) noexcept {
  for (const RasterMappedTo3DTile& mapped : tile.getMappedRasterTiles()) {
    const RasterOverlayTile* pLoading = mapped.getLoadingTile();
    if (pLoading &&
        pLoading->getState() == RasterOverlayTile::LoadState::Unloaded) {
      return true;
    }
  }

  return false;
}

std::optional<RegionAndCenter>
getTileBoundingRegionForUpsampling(const Tile& parent) {
  // To create subdivided children, we need to know a bounding region for each.
  // If the parent is already loaded and we have Web Mercator or Geographic
  // textures coordinates, we're set. If it's not, but it has a bounding region,
  // we're still set. Otherwise, we can't upsample (yet?).

  // Get an accurate bounding region from the content first.
  const TileContent& parentContent = parent.getContent();
  const TileRenderContent* pRenderContent = parentContent.getRenderContent();
  assert(pRenderContent && "This function only deal with render content");

  const RasterOverlayDetails& details =
      pRenderContent->getRasterOverlayDetails();

  // If we don't have any overlay projections/rectangles, why are we
  // upsampling?
  assert(!details.rasterOverlayProjections.empty());
  assert(!details.rasterOverlayRectangles.empty());

  // Use the projected center of the tile as the subdivision center.
  // The tile will be subdivided by (0.5, 0.5) in the first overlay's
  // texture coordinates which overlay had more detail.
  for (const RasterMappedTo3DTile& mapped : parent.getMappedRasterTiles()) {
    if (mapped.isMoreDetailAvailable()) {
      const CesiumGeospatial::Projection& projection =
          mapped.getReadyTile()->getTileProvider().getProjection();
      glm::dvec2 centerProjected =
          details.findRectangleForOverlayProjection(projection)->getCenter();
      CesiumGeospatial::Cartographic center =
          CesiumGeospatial::unprojectPosition(
              projection,
              glm::dvec3(centerProjected, 0.0));

      return RegionAndCenter{details.boundingRegion, center};
    }
  }

  // We shouldn't be upsampling from a tile until that tile is loaded.
  // If it has no content after loading, we can't upsample from it.
  return std::nullopt;
}

void createQuadtreeSubdividedChildren(
    Tile& parent,
    RasterOverlayUpsampler& upsampler) {
  std::optional<RegionAndCenter> maybeRegionAndCenter =
      getTileBoundingRegionForUpsampling(parent);
  if (!maybeRegionAndCenter) {
    return;
  }

  // Don't try to upsample a parent tile without geometry.
  if (maybeRegionAndCenter->region.getMaximumHeight() <
      maybeRegionAndCenter->region.getMinimumHeight()) {
    return;
  }

  // The quadtree tile ID doesn't actually matter, because we're not going to
  // use the standard tile bounds for the ID. But having a tile ID that reflects
  // the level and _approximate_ location is helpful for debugging.
  const CesiumGeometry::QuadtreeTileID* pRealParentTileID =
      std::get_if<CesiumGeometry::QuadtreeTileID>(&parent.getTileID());
  if (!pRealParentTileID) {
    const CesiumGeometry::UpsampledQuadtreeNode* pUpsampledID =
        std::get_if<CesiumGeometry::UpsampledQuadtreeNode>(&parent.getTileID());
    if (pUpsampledID) {
      pRealParentTileID = &pUpsampledID->tileID;
    }
  }

  CesiumGeometry::QuadtreeTileID parentTileID =
      pRealParentTileID ? *pRealParentTileID
                        : CesiumGeometry::QuadtreeTileID(0, 0, 0);

  // QuadtreeTileID can't handle higher than level 30 because the x and y
  // coordinates (uint32_t) will overflow. So just start over at level 0.
  if (parentTileID.level >= 30U) {
    parentTileID = CesiumGeometry::QuadtreeTileID(0, 0, 0);
  }

  // The parent tile must not have a zero geometric error, even if it's a leaf
  // tile. Otherwise we'd never refine it.
  parent.setGeometricError(parent.getNonZeroGeometricError());

  // The parent must use REPLACE refinement.
  parent.setRefine(TileRefine::Replace);

  // add 4 children for parent
  std::vector<Tile> children;
  children.reserve(4);
  for (std::size_t i = 0; i < 4; ++i) {
    children.emplace_back(&upsampler);
  }
  parent.createChildTiles(std::move(children));

  // populate children metadata
  gsl::span<Tile> childrenView = parent.getChildren();
  Tile& sw = childrenView[0];
  Tile& se = childrenView[1];
  Tile& nw = childrenView[2];
  Tile& ne = childrenView[3];

  // set children geometric error
  const double geometricError = parent.getGeometricError() * 0.5;
  sw.setGeometricError(geometricError);
  se.setGeometricError(geometricError);
  nw.setGeometricError(geometricError);
  ne.setGeometricError(geometricError);

  // set children tile ID
  const CesiumGeometry::QuadtreeTileID swID(
      parentTileID.level + 1,
      parentTileID.x * 2,
      parentTileID.y * 2);
  const CesiumGeometry::QuadtreeTileID seID(swID.level, swID.x + 1, swID.y);
  const CesiumGeometry::QuadtreeTileID nwID(swID.level, swID.x, swID.y + 1);
  const CesiumGeometry::QuadtreeTileID neID(swID.level, swID.x + 1, swID.y + 1);

  sw.setTileID(CesiumGeometry::UpsampledQuadtreeNode{swID});
  se.setTileID(CesiumGeometry::UpsampledQuadtreeNode{seID});
  nw.setTileID(CesiumGeometry::UpsampledQuadtreeNode{nwID});
  ne.setTileID(CesiumGeometry::UpsampledQuadtreeNode{neID});

  // set children bounding volume
  const double minimumHeight = maybeRegionAndCenter->region.getMinimumHeight();
  const double maximumHeight = maybeRegionAndCenter->region.getMaximumHeight();

  const CesiumGeospatial::GlobeRectangle& parentRectangle =
      maybeRegionAndCenter->region.getRectangle();
  const CesiumGeospatial::Cartographic& center = maybeRegionAndCenter->center;
  sw.setBoundingVolume(CesiumGeospatial::BoundingRegionWithLooseFittingHeights(
      CesiumGeospatial::BoundingRegion(
          CesiumGeospatial::GlobeRectangle(
              parentRectangle.getWest(),
              parentRectangle.getSouth(),
              center.longitude,
              center.latitude),
          minimumHeight,
          maximumHeight)));

  se.setBoundingVolume(CesiumGeospatial::BoundingRegionWithLooseFittingHeights(
      CesiumGeospatial::BoundingRegion(
          CesiumGeospatial::GlobeRectangle(
              center.longitude,
              parentRectangle.getSouth(),
              parentRectangle.getEast(),
              center.latitude),
          minimumHeight,
          maximumHeight)));

  nw.setBoundingVolume(CesiumGeospatial::BoundingRegionWithLooseFittingHeights(
      CesiumGeospatial::BoundingRegion(
          CesiumGeospatial::GlobeRectangle(
              parentRectangle.getWest(),
              center.latitude,
              center.longitude,
              parentRectangle.getNorth()),
          minimumHeight,
          maximumHeight)));

  ne.setBoundingVolume(CesiumGeospatial::BoundingRegionWithLooseFittingHeights(
      CesiumGeospatial::BoundingRegion(
          CesiumGeospatial::GlobeRectangle(
              center.longitude,
              center.latitude,
              parentRectangle.getEast(),
              parentRectangle.getNorth()),
          minimumHeight,
          maximumHeight)));

  // set children transforms
  sw.setTransform(parent.getTransform());
  se.setTransform(parent.getTransform());
  nw.setTransform(parent.getTransform());
  ne.setTransform(parent.getTransform());
}

std::vector<CesiumGeospatial::Projection> mapOverlaysToTile(
    Tile& tile,
    RasterOverlayCollection& overlays,
    double maximumScreenSpaceError,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& defaultHeaders,
    std::vector<TilesetContentManager::RasterWorkChain>& outWork) {

  // We may still have mapped raster tiles that need to be reset if the tile
  // fails temporarily. It shouldn't be in the loading state, which would mean
  // it's still in the work manager
#ifndef NDEBUG
  for (RasterMappedTo3DTile& pMapped : tile.getMappedRasterTiles()) {
    RasterOverlayTile* pLoading = pMapped.getLoadingTile();
    assert(pLoading);
    assert(pLoading->getState() != RasterOverlayTile::LoadState::Loading);
  }
#endif
  tile.getMappedRasterTiles().clear();

  std::vector<CesiumGeospatial::Projection> projections;
  const std::vector<CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>>&
      tileProviders = overlays.getTileProviders();
  const std::vector<CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>>&
      placeholders = overlays.getPlaceholderTileProviders();
  assert(tileProviders.size() == placeholders.size());

  // Try to load now, but if tile is a placeholder this won't do anything
  for (size_t i = 0; i < tileProviders.size() && i < placeholders.size(); ++i) {
    RasterOverlayTileProvider& tileProvider = *tileProviders[i];
    RasterOverlayTileProvider& placeholder = *placeholders[i];
    RasterMappedTo3DTile::mapOverlayToTile(
        maximumScreenSpaceError,
        tileProvider,
        placeholder,
        tile,
        projections);
  }

  // Get the work from the mapped tiles
  for (RasterMappedTo3DTile& pMapped : tile.getMappedRasterTiles()) {
    // Default headers come from the this. Loader can override if needed
    CesiumAsync::RequestData requestData;
    requestData.headers = defaultHeaders;
    RasterProcessingCallback rasterCallback;

    // Can't do work without a loading tile
    RasterOverlayTile* pLoadingTile = pMapped.getLoadingTile();
    if (!pLoadingTile)
      continue;

    RasterOverlayTileProvider& provider = pLoadingTile->getTileProvider();
    provider.getLoadTileThrottledWork(
        *pLoadingTile,
        requestData,
        rasterCallback);

    if (!requestData.url.empty() || rasterCallback != nullptr) {
      TilesetContentManager::RasterWorkChain newWorkChain = {
          &pMapped,
          requestData,
          rasterCallback};
      outWork.push_back(newWorkChain);
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

void calcRasterOverlayDetailsInWorkerThread(
    TileLoadResult& result,
    std::vector<CesiumGeospatial::Projection>&& projections,
    const TileContentLoadInfo& tileLoadInfo) {
  CesiumGltf::Model& model = std::get<CesiumGltf::Model>(result.contentKind);

  // we will use the fittest bounding volume to calculate raster overlay details
  // below
  const BoundingVolume& contentBoundingVolume =
      getEffectiveContentBoundingVolume(
          tileLoadInfo.tileBoundingVolume,
          tileLoadInfo.tileContentBoundingVolume,
          result.updatedBoundingVolume,
          result.updatedContentBoundingVolume);

  // If we have projections, generate texture coordinates for all of them. Also
  // remember the min and max height so that we can use them for upsampling.
  const CesiumGeospatial::BoundingRegion* pRegion =
      getBoundingRegionFromBoundingVolume(contentBoundingVolume);

  // remove any projections that are already used to generated UV
  int32_t firstRasterOverlayTexCoord = 0;
  if (result.rasterOverlayDetails) {
    const std::vector<CesiumGeospatial::Projection>& existingProjections =
        result.rasterOverlayDetails->rasterOverlayProjections;
    firstRasterOverlayTexCoord =
        static_cast<int32_t>(existingProjections.size());
    auto removedProjectionIt = std::remove_if(
        projections.begin(),
        projections.end(),
        [&](const CesiumGeospatial::Projection& proj) {
          return std::find(
                     existingProjections.begin(),
                     existingProjections.end(),
                     proj) != existingProjections.end();
        });
    projections.erase(removedProjectionIt, projections.end());
  }

  // generate the overlay details from the rest of projections and merge it with
  // the existing one
  auto overlayDetails =
      RasterOverlayUtilities::createRasterOverlayTextureCoordinates(
          model,
          tileLoadInfo.tileTransform,
          pRegion ? std::make_optional(pRegion->getRectangle()) : std::nullopt,
          std::move(projections),
          false,
          RasterOverlayUtilities::DEFAULT_TEXTURE_COORDINATE_BASE_NAME,
          firstRasterOverlayTexCoord);

  if (pRegion && overlayDetails) {
    // If the original bounding region was wrong, report it.
    const CesiumGeospatial::GlobeRectangle& original = pRegion->getRectangle();
    const CesiumGeospatial::GlobeRectangle& computed =
        overlayDetails->boundingRegion.getRectangle();
    if ((!CesiumUtility::Math::equalsEpsilon(
             computed.getWest(),
             original.getWest(),
             0.01) &&
         computed.getWest() < original.getWest()) ||
        (!CesiumUtility::Math::equalsEpsilon(
             computed.getSouth(),
             original.getSouth(),
             0.01) &&
         computed.getSouth() < original.getSouth()) ||
        (!CesiumUtility::Math::equalsEpsilon(
             computed.getEast(),
             original.getEast(),
             0.01) &&
         computed.getEast() > original.getEast()) ||
        (!CesiumUtility::Math::equalsEpsilon(
             computed.getNorth(),
             original.getNorth(),
             0.01) &&
         computed.getNorth() > original.getNorth())) {

      auto it = model.extras.find("Cesium3DTiles_TileUrl");
      std::string url = it != model.extras.end()
                            ? it->second.getStringOrDefault("Unknown Tile URL")
                            : "Unknown Tile URL";
      SPDLOG_LOGGER_WARN(
          tileLoadInfo.pLogger,
          "Tile has a bounding volume that does not include all of its "
          "content, so culling and raster overlays may be incorrect: {}",
          url);
    }
  }

  if (result.rasterOverlayDetails && overlayDetails) {
    result.rasterOverlayDetails->merge(*overlayDetails);
  } else if (overlayDetails) {
    result.rasterOverlayDetails = std::move(*overlayDetails);
  }
}

void calcFittestBoundingRegionForLooseTile(
    TileLoadResult& result,
    const TileContentLoadInfo& tileLoadInfo) {
  CesiumGltf::Model& model = std::get<CesiumGltf::Model>(result.contentKind);

  const BoundingVolume& boundingVolume = getEffectiveBoundingVolume(
      tileLoadInfo.tileBoundingVolume,
      result.updatedBoundingVolume,
      result.updatedContentBoundingVolume);
  if (std::get_if<CesiumGeospatial::BoundingRegionWithLooseFittingHeights>(
          &boundingVolume) != nullptr) {
    if (result.rasterOverlayDetails) {
      // We already computed the bounding region for overlays, so use it.
      result.updatedBoundingVolume =
          result.rasterOverlayDetails->boundingRegion;
    } else {
      // We need to compute an accurate bounding region
      result.updatedBoundingVolume = GltfUtilities::computeBoundingRegion(
          model,
          tileLoadInfo.tileTransform);
    }
  }
}

void postProcessGltfInWorkerThread(
    TileLoadResult& result,
    std::vector<CesiumGeospatial::Projection>&& projections,
    const TileContentLoadInfo& tileLoadInfo) {
  CesiumGltf::Model& model = std::get<CesiumGltf::Model>(result.contentKind);

  if (!result.originalRequestUrl.empty()) {
    model.extras["Cesium3DTiles_TileUrl"] = result.originalRequestUrl;
  }

  // have to pass the up axis to extra for backward compatibility
  model.extras["gltfUpAxis"] =
      static_cast<std::underlying_type_t<CesiumGeometry::Axis>>(
          result.glTFUpAxis);

  // calculate raster overlay details
  calcRasterOverlayDetailsInWorkerThread(
      result,
      std::move(projections),
      tileLoadInfo);

  // If our tile bounding region has loose fitting heights, find the real ones.
  calcFittestBoundingRegionForLooseTile(result, tileLoadInfo);

  // generate missing smooth normal
  if (tileLoadInfo.contentOptions.generateMissingNormalsSmooth) {
    model.generateMissingNormalsSmooth();
  }
}

CesiumAsync::Future<TileLoadResultAndRenderResources> postProcessContent(
    TileLoadResult&& result,
    std::vector<CesiumGeospatial::Projection>&& projections,
    TileContentLoadInfo&& tileLoadInfo,
    const std::string& requestBaseUrl,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
    const std::any& rendererOptions) {
  assert(
      result.state == TileLoadResultState::Success &&
      "This function requires result to be success");

  CesiumGltf::Model& model = std::get<CesiumGltf::Model>(result.contentKind);

  // Download any external image or buffer urls in the gltf if there are any
  CesiumGltfReader::GltfReaderResult gltfResult{std::move(model), {}, {}};

  CesiumAsync::HttpHeaders httpHeaders;
  if (!requestBaseUrl.empty()) {
    for (auto pair : requestHeaders)
      httpHeaders[pair.first] = pair.second;
  }

  CesiumGltfReader::GltfReaderOptions gltfOptions;
  gltfOptions.ktx2TranscodeTargets =
      tileLoadInfo.contentOptions.ktx2TranscodeTargets;
  gltfOptions.applyTextureTransform =
      tileLoadInfo.contentOptions.applyTextureTransform;

  auto asyncSystem = tileLoadInfo.asyncSystem;
  auto pAssetAccessor = tileLoadInfo.pAssetAccessor;
  return CesiumGltfReader::GltfReader::resolveExternalData(
             asyncSystem,
             requestBaseUrl,
             httpHeaders,
             pAssetAccessor,
             gltfOptions,
             std::move(gltfResult))
      .thenImmediately(
          // Run this immediately. In non-error cases, we're already in a worker
          [result = std::move(result),
           projections = std::move(projections),
           tileLoadInfo = std::move(tileLoadInfo),
           requestBaseUrl,
           rendererOptions](
              CesiumGltfReader::GltfReaderResult&& gltfResult) mutable {
            if (!gltfResult.errors.empty()) {
              if (!requestBaseUrl.empty()) {
                SPDLOG_LOGGER_ERROR(
                    tileLoadInfo.pLogger,
                    "Failed resolving external glTF buffers from {}:\n- {}",
                    requestBaseUrl,
                    CesiumUtility::joinToString(gltfResult.errors, "\n- "));
              } else {
                SPDLOG_LOGGER_ERROR(
                    tileLoadInfo.pLogger,
                    "Failed resolving external glTF buffers:\n- {}",
                    CesiumUtility::joinToString(gltfResult.errors, "\n- "));
              }
            }

            if (!gltfResult.warnings.empty()) {
              if (!requestBaseUrl.empty()) {
                SPDLOG_LOGGER_WARN(
                    tileLoadInfo.pLogger,
                    "Warning when resolving external gltf buffers from "
                    "{}:\n- {}",
                    requestBaseUrl,
                    CesiumUtility::joinToString(gltfResult.errors, "\n- "));
              } else {
                SPDLOG_LOGGER_ERROR(
                    tileLoadInfo.pLogger,
                    "Warning resolving external glTF buffers:\n- {}",
                    CesiumUtility::joinToString(gltfResult.errors, "\n- "));
              }
            }

            if (!gltfResult.model) {
              return tileLoadInfo.asyncSystem.createResolvedFuture(
                  TileLoadResultAndRenderResources{
                      TileLoadResult::createFailedResult(),
                      nullptr});
            }

            result.contentKind = std::move(*gltfResult.model);

            postProcessGltfInWorkerThread(
                result,
                std::move(projections),
                tileLoadInfo);

            // create render resources
            return tileLoadInfo.pPrepareRendererResources->prepareInLoadThread(
                tileLoadInfo.asyncSystem,
                std::move(result),
                tileLoadInfo.tileTransform,
                rendererOptions);
          });
}
} // namespace

TilesetContentManager::TilesetContentManager(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    RasterOverlayCollection&& overlayCollection,
    std::vector<CesiumAsync::IAssetAccessor::THeader>&& requestHeaders,
    std::unique_ptr<TilesetContentLoader>&& pLoader,
    std::unique_ptr<Tile>&& pRootTile)
    : _externals{externals},
      _requestHeaders{std::move(requestHeaders)},
      _pLoader{std::move(pLoader)},
      _pRootTile{std::move(pRootTile)},
      _userCredit(
          (tilesetOptions.credit && externals.pCreditSystem)
              ? std::optional<Credit>(externals.pCreditSystem->createCredit(
                    tilesetOptions.credit.value(),
                    tilesetOptions.showCreditsOnScreen))
              : std::nullopt),
      _tilesetCredits{},
      _overlayCollection{std::move(overlayCollection)},
      _tileLoadsInProgress{0},
      _loadedTilesCount{0},
      _tilesDataUsed{0},
      _rasterLoadsInProgress{0},
      _loadedRastersCount{0},
      _destructionCompletePromise{externals.asyncSystem.createPromise<void>()},
      _destructionCompleteFuture{
          this->_destructionCompletePromise.getFuture().share()},
      _rootTileAvailablePromise{externals.asyncSystem.createPromise<void>()},
      _rootTileAvailableFuture{
          this->_rootTileAvailablePromise.getFuture().share()} {
  createWorkManager(externals);

  this->_rootTileAvailablePromise.resolve();
}

TilesetContentManager::TilesetContentManager(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    RasterOverlayCollection&& overlayCollection,
    const std::string& url)
    : _externals{externals},
      _requestHeaders{},
      _pLoader{},
      _pRootTile{},
      _userCredit(
          (tilesetOptions.credit && externals.pCreditSystem)
              ? std::optional<Credit>(externals.pCreditSystem->createCredit(
                    tilesetOptions.credit.value(),
                    tilesetOptions.showCreditsOnScreen))
              : std::nullopt),
      _tilesetCredits{},
      _overlayCollection{std::move(overlayCollection)},
      _tileLoadsInProgress{0},
      _loadedTilesCount{0},
      _tilesDataUsed{0},
      _rasterLoadsInProgress{0},
      _loadedRastersCount{0},
      _destructionCompletePromise{externals.asyncSystem.createPromise<void>()},
      _destructionCompleteFuture{
          this->_destructionCompletePromise.getFuture().share()},
      _rootTileAvailablePromise{externals.asyncSystem.createPromise<void>()},
      _rootTileAvailableFuture{
          this->_rootTileAvailablePromise.getFuture().share()} {
  createWorkManager(externals);

  if (!url.empty()) {
    this->notifyTileStartLoading(nullptr);

    CesiumUtility::IntrusivePointer<TilesetContentManager> thiz = this;

    externals.pAssetAccessor
        ->get(externals.asyncSystem, url, this->_requestHeaders)
        .thenInWorkerThread(
            [pLogger = externals.pLogger,
             asyncSystem = externals.asyncSystem,
             pAssetAccessor = externals.pAssetAccessor,
             contentOptions = tilesetOptions.contentOptions](
                const std::shared_ptr<CesiumAsync::IAssetRequest>&
                    pCompletedRequest) {
              // Check if request is successful
              const CesiumAsync::IAssetResponse* pResponse =
                  pCompletedRequest->response();
              const std::string& url = pCompletedRequest->url();
              if (!pResponse) {
                TilesetContentLoaderResult<TilesetContentLoader> result;
                result.errors.emplaceError(fmt::format(
                    "Did not receive a valid response for tileset {}",
                    url));
                return asyncSystem.createResolvedFuture(std::move(result));
              }

              uint16_t statusCode = pResponse->statusCode();
              if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
                TilesetContentLoaderResult<TilesetContentLoader> result;
                result.errors.emplaceError(fmt::format(
                    "Received status code {} for tileset {}",
                    statusCode,
                    url));
                return asyncSystem.createResolvedFuture(std::move(result));
              }

              // Parse Json response
              gsl::span<const std::byte> tilesetJsonBinary = pResponse->data();
              rapidjson::Document tilesetJson;
              tilesetJson.Parse(
                  reinterpret_cast<const char*>(tilesetJsonBinary.data()),
                  tilesetJsonBinary.size());
              if (tilesetJson.HasParseError()) {
                TilesetContentLoaderResult<TilesetContentLoader> result;
                result.errors.emplaceError(fmt::format(
                    "Error when parsing tileset JSON, error code {} at byte "
                    "offset {}",
                    tilesetJson.GetParseError(),
                    tilesetJson.GetErrorOffset()));
                return asyncSystem.createResolvedFuture(std::move(result));
              }

              // Check if the json is a tileset.json format or layer.json format
              // and create corresponding loader
              const auto rootIt = tilesetJson.FindMember("root");
              if (rootIt != tilesetJson.MemberEnd()) {
                TilesetContentLoaderResult<TilesetContentLoader> result =
                    TilesetJsonLoader::createLoader(pLogger, url, tilesetJson);
                return asyncSystem.createResolvedFuture(std::move(result));
              } else {
                const auto formatIt = tilesetJson.FindMember("format");
                bool isLayerJsonFormat = formatIt != tilesetJson.MemberEnd() &&
                                         formatIt->value.IsString();
                isLayerJsonFormat = isLayerJsonFormat &&
                                    std::string(formatIt->value.GetString()) ==
                                        "quantized-mesh-1.0";
                if (isLayerJsonFormat) {
                  const CesiumAsync::HttpHeaders& completedRequestHeaders =
                      pCompletedRequest->headers();
                  std::vector<CesiumAsync::IAssetAccessor::THeader> flatHeaders(
                      completedRequestHeaders.begin(),
                      completedRequestHeaders.end());
                  return LayerJsonTerrainLoader::createLoader(
                             asyncSystem,
                             pAssetAccessor,
                             contentOptions,
                             url,
                             flatHeaders,
                             tilesetJson)
                      .thenImmediately(
                          [](TilesetContentLoaderResult<TilesetContentLoader>&&
                                 result) { return std::move(result); });
                }

                TilesetContentLoaderResult<TilesetContentLoader> result;
                result.errors.emplaceError("tileset json has unsupport format");
                return asyncSystem.createResolvedFuture(std::move(result));
              }
            })
        .thenInMainThread(
            [thiz, errorCallback = tilesetOptions.loadErrorCallback](
                TilesetContentLoaderResult<TilesetContentLoader>&& result) {
              thiz->notifyTileDoneLoading(result.pRootTile.get());
              thiz->propagateTilesetContentLoaderResult(
                  TilesetLoadType::TilesetJson,
                  errorCallback,
                  std::move(result));
              thiz->_rootTileAvailablePromise.resolve();
            })
        .catchInMainThread([thiz](std::exception&& e) {
          thiz->notifyTileDoneLoading(nullptr);
          SPDLOG_LOGGER_ERROR(
              thiz->_externals.pLogger,
              "An unexpected error occurred when loading tile: {}",
              e.what());
          thiz->_rootTileAvailablePromise.reject(
              std::runtime_error("Root tile failed to load."));
        });
  }
}

TilesetContentManager::TilesetContentManager(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    RasterOverlayCollection&& overlayCollection,
    int64_t ionAssetID,
    const std::string& ionAccessToken,
    const std::string& ionAssetEndpointUrl)
    : _externals{externals},
      _requestHeaders{},
      _pLoader{},
      _pRootTile{},
      _userCredit(
          (tilesetOptions.credit && externals.pCreditSystem)
              ? std::optional<Credit>(externals.pCreditSystem->createCredit(
                    tilesetOptions.credit.value(),
                    tilesetOptions.showCreditsOnScreen))
              : std::nullopt),
      _tilesetCredits{},
      _overlayCollection{std::move(overlayCollection)},
      _tileLoadsInProgress{0},
      _loadedTilesCount{0},
      _tilesDataUsed{0},
      _rasterLoadsInProgress{0},
      _loadedRastersCount{0},
      _destructionCompletePromise{externals.asyncSystem.createPromise<void>()},
      _destructionCompleteFuture{
          this->_destructionCompletePromise.getFuture().share()},
      _rootTileAvailablePromise{externals.asyncSystem.createPromise<void>()},
      _rootTileAvailableFuture{
          this->_rootTileAvailablePromise.getFuture().share()} {
  createWorkManager(externals);

  if (ionAssetID > 0) {
    auto authorizationChangeListener = [this](
                                           const std::string& header,
                                           const std::string& headerValue) {
      auto& requestHeaders = this->_requestHeaders;
      auto authIt = std::find_if(
          requestHeaders.begin(),
          requestHeaders.end(),
          [&header](auto& headerPair) { return headerPair.first == header; });
      if (authIt != requestHeaders.end()) {
        authIt->second = headerValue;
      } else {
        requestHeaders.emplace_back(header, headerValue);
      }
    };

    this->notifyTileStartLoading(nullptr);

    CesiumUtility::IntrusivePointer<TilesetContentManager> thiz = this;

    CesiumIonTilesetLoader::createLoader(
        externals,
        tilesetOptions.contentOptions,
        static_cast<uint32_t>(ionAssetID),
        ionAccessToken,
        ionAssetEndpointUrl,
        authorizationChangeListener,
        tilesetOptions.showCreditsOnScreen)
        .thenInMainThread(
            [thiz, errorCallback = tilesetOptions.loadErrorCallback](
                TilesetContentLoaderResult<CesiumIonTilesetLoader>&& result) {
              thiz->notifyTileDoneLoading(result.pRootTile.get());
              thiz->propagateTilesetContentLoaderResult(
                  TilesetLoadType::CesiumIon,
                  errorCallback,
                  std::move(result));
              thiz->_rootTileAvailablePromise.resolve();
            })
        .catchInMainThread([thiz](std::exception&& e) {
          thiz->notifyTileDoneLoading(nullptr);
          SPDLOG_LOGGER_ERROR(
              thiz->_externals.pLogger,
              "An unexpected error occurred when loading tile: {}",
              e.what());
          thiz->_rootTileAvailablePromise.reject(
              std::runtime_error("Root tile failed to load."));
        });
  }
}

void TilesetContentManager::createWorkManager(
    const TilesetExternals& externals) {
  _pTileWorkManager = std::make_shared<TileWorkManager>(
      externals.asyncSystem,
      externals.pAssetAccessor,
      externals.pLogger);

  TileWorkManager::TileDispatchFunc tileDispatch =
      [this](
          TileProcessingData& processingData,
          const CesiumAsync::UrlResponseDataMap& responseDataMap,
          TileWorkManager::Work* work) {
        return this->dispatchTileWork(processingData, responseDataMap, work);
      };

  TileWorkManager::RasterDispatchFunc rasterDispatch =
      [this](
          RasterProcessingData& processingData,
          const CesiumAsync::UrlResponseDataMap& responseDataMap,
          TileWorkManager::Work* work) {
        return this->dispatchRasterWork(processingData, responseDataMap, work);
      };

  _pTileWorkManager->SetDispatchFunctions(tileDispatch, rasterDispatch);
}

CesiumAsync::SharedFuture<void>&
TilesetContentManager::getAsyncDestructionCompleteEvent() {
  return this->_destructionCompleteFuture;
}

CesiumAsync::SharedFuture<void>&
TilesetContentManager::getRootTileAvailableEvent() {
  return this->_rootTileAvailableFuture;
}

TilesetContentManager::~TilesetContentManager() noexcept {
  assert(this->_tileLoadsInProgress == 0);
  assert(this->_rasterLoadsInProgress == 0);
  this->unloadAll();

  this->_destructionCompletePromise.resolve();
}

void TilesetContentManager::processLoadRequests(
    std::vector<TileLoadRequest>& requests,
    TilesetOptions& options) {

  std::vector<TileWorkManager::Order> orders;
  discoverLoadWork(requests, options.maximumScreenSpaceError, options, orders);

  assert(options.maximumSimultaneousTileLoads > 0);
  size_t maxTileLoads =
      static_cast<size_t>(options.maximumSimultaneousTileLoads);

  std::vector<const TileWorkManager::Work*> workCreated;
  TileWorkManager::TryAddOrders(
      this->_pTileWorkManager,
      orders,
      maxTileLoads,
      workCreated);

  markWorkTilesAsLoading(workCreated);

  // Dispatch more processing work. More may have been added, or slots may have
  // freed up from any work that completed after update_view called
  // dispatchMainThreadTasks and now
  TileWorkManager::TryDispatchProcessing(this->_pTileWorkManager);

  // Finish main thread tasks for any work that completed after update_view
  // called dispatchMainThreadTasks and now
  handleCompletedWork();
}

void TilesetContentManager::updateTileContent(
    Tile& tile,
    const TilesetOptions& tilesetOptions) {
  if (tile.getState() == TileLoadState::Unloading) {
    unloadTileContent(tile);
  }

  if (tile.getState() == TileLoadState::ContentLoaded) {
    updateContentLoadedState(tile, tilesetOptions);
  }

  if (tile.getState() == TileLoadState::Done) {
    updateDoneState(tile, tilesetOptions);
  }

  if (tile.shouldContentContinueUpdating()) {
    TileChildrenResult childrenResult =
        this->_pLoader->createTileChildren(tile);
    if (childrenResult.state == TileLoadResultState::Success) {
      tile.createChildTiles(std::move(childrenResult.children));
    }

    bool shouldTileContinueUpdated =
        childrenResult.state == TileLoadResultState::RetryLater;
    tile.setContentShouldContinueUpdating(shouldTileContinueUpdated);
  }
}

bool TilesetContentManager::unloadTileContent(Tile& tile) {
  TileLoadState state = tile.getState();
  if (state == TileLoadState::Unloaded) {
    return true;
  }

  if (state == TileLoadState::ContentLoading) {
    return false;
  }

  TileContent& content = tile.getContent();

  // don't unload external or empty tile
  if (content.isExternalContent() || content.isEmptyContent()) {
    return false;
  }

  // Are any raster mapped tiles currently loading?
  // If so, wait until they are done before unloading
  for (RasterMappedTo3DTile& mapped : tile.getMappedRasterTiles()) {
    RasterOverlayTile* pLoadingTile = mapped.getLoadingTile();
    if (pLoadingTile &&
        pLoadingTile->getState() == RasterOverlayTile::LoadState::Loading)
      return false;
  }

  // Detach raster tiles first so that the renderer's tile free
  // process doesn't need to worry about them.
  for (RasterMappedTo3DTile& mapped : tile.getMappedRasterTiles()) {
    mapped.detachFromTile(*this->_externals.pPrepareRendererResources, tile);
  }
  tile.getMappedRasterTiles().clear();

  // Unload the renderer resources and clear any raster overlay tiles. We can do
  // this even if the tile can't be fully unloaded because this tile's geometry
  // is being using by an async upsample operation (checked below).
  switch (state) {
  case TileLoadState::ContentLoaded:
    unloadContentLoadedState(tile);
    break;
  case TileLoadState::Done:
    unloadDoneState(tile);
    break;
  default:
    break;
  }

  // Are any children currently being upsampled from this tile?
  for (const Tile& child : tile.getChildren()) {
    if (child.getState() == TileLoadState::ContentLoading &&
        std::holds_alternative<CesiumGeometry::UpsampledQuadtreeNode>(
            child.getTileID())) {
      // Yes, a child is upsampling from this tile, so it may be using the
      // tile's content from another thread via lambda capture. We can't unload
      // it right now. So mark the tile as in the process of unloading and stop
      // here.
      tile.setState(TileLoadState::Unloading);
      return false;
    }
  }

  // If we make it this far, the tile's content will be fully unloaded.
  notifyTileUnloading(&tile);
  content.setContentKind(TileUnknownContent{});
  tile.setState(TileLoadState::Unloaded);
  return true;
}

void TilesetContentManager::unloadAll() {
  this->_pTileWorkManager->Shutdown();

  // TODO: use the linked-list of loaded tiles instead of walking the entire
  // tile tree.
  if (this->_pRootTile) {
    unloadTileRecursively(*this->_pRootTile, *this);
  }
}

void TilesetContentManager::waitUntilIdle() {
  // Tiles are loaded either on construction (root tile) or through the work
  // manager. Wait for all asynchronous loading to terminate.
  bool workInProgress = this->_tileLoadsInProgress > 0 ||
                        this->_pTileWorkManager->GetActiveWorkCount() > 0;
  while (workInProgress) {
    this->_externals.pAssetAccessor->tick();
    this->_externals.asyncSystem.dispatchMainThreadTasks();
    workInProgress = this->_tileLoadsInProgress > 0 ||
                     this->_pTileWorkManager->GetActiveWorkCount() > 0;
  }
}

const Tile* TilesetContentManager::getRootTile() const noexcept {
  return this->_pRootTile.get();
}

Tile* TilesetContentManager::getRootTile() noexcept {
  return this->_pRootTile.get();
}

const std::vector<CesiumAsync::IAssetAccessor::THeader>&
TilesetContentManager::getRequestHeaders() const noexcept {
  return this->_requestHeaders;
}

std::vector<CesiumAsync::IAssetAccessor::THeader>&
TilesetContentManager::getRequestHeaders() noexcept {
  return this->_requestHeaders;
}

const RasterOverlayCollection&
TilesetContentManager::getRasterOverlayCollection() const noexcept {
  return this->_overlayCollection;
}

RasterOverlayCollection&
TilesetContentManager::getRasterOverlayCollection() noexcept {
  return this->_overlayCollection;
}

const Credit* TilesetContentManager::getUserCredit() const noexcept {
  if (this->_userCredit) {
    return &*this->_userCredit;
  }

  return nullptr;
}

const std::vector<Credit>&
TilesetContentManager::getTilesetCredits() const noexcept {
  return this->_tilesetCredits;
}

int32_t TilesetContentManager::getNumberOfTilesLoading() const noexcept {
  return this->_tileLoadsInProgress;
}

int32_t TilesetContentManager::getNumberOfTilesLoaded() const noexcept {
  return this->_loadedTilesCount;
}

int64_t TilesetContentManager::getTotalDataUsed() const noexcept {
  int64_t bytes = this->_tilesDataUsed;
  for (const auto& pTileProvider :
       this->_overlayCollection.getTileProviders()) {
    bytes += pTileProvider->getTileDataBytes();
  }

  return bytes;
}

int32_t TilesetContentManager::getNumberOfRastersLoading() const noexcept {
  return this->_rasterLoadsInProgress;
}

int32_t TilesetContentManager::getNumberOfRastersLoaded() const noexcept {
  return this->_loadedRastersCount;
}

size_t TilesetContentManager::getActiveWorkCount() {
  return this->_pTileWorkManager->GetActiveWorkCount();
}

void TilesetContentManager::getLoadingWorkStats(
    size_t& requestCount,
    size_t& inFlightCount,
    size_t& processingCount,
    size_t& failedCount) {
  return this->_pTileWorkManager->GetLoadingWorkStats(
      requestCount,
      inFlightCount,
      processingCount,
      failedCount);
}

bool TilesetContentManager::tileNeedsWorkerThreadLoading(
    const Tile& tile) const noexcept {
  auto state = tile.getState();
  return state == TileLoadState::Unloaded ||
         state == TileLoadState::FailedTemporarily ||
         anyRasterOverlaysNeedLoading(tile);
}

bool TilesetContentManager::tileNeedsMainThreadLoading(
    const Tile& tile) const noexcept {
  return tile.getState() == TileLoadState::ContentLoaded &&
         tile.isRenderContent();
}

void TilesetContentManager::finishLoading(
    Tile& tile,
    const TilesetOptions& tilesetOptions) {
  assert(tile.getState() == TileLoadState::ContentLoaded);

  // Run the main thread part of loading.
  TileContent& content = tile.getContent();
  TileRenderContent* pRenderContent = content.getRenderContent();

  assert(pRenderContent != nullptr);

  // add copyright
  CreditSystem* pCreditSystem = this->_externals.pCreditSystem.get();
  if (pCreditSystem) {
    std::vector<std::string_view> creditStrings =
        GltfUtilities::parseGltfCopyright(pRenderContent->getModel());

    std::vector<Credit> credits;
    credits.reserve(creditStrings.size());

    for (const std::string_view& creditString : creditStrings) {
      credits.emplace_back(pCreditSystem->createCredit(
          std::string(creditString),
          tilesetOptions.showCreditsOnScreen));
    }

    pRenderContent->setCredits(credits);
  }

  void* pWorkerRenderResources = pRenderContent->getRenderResources();
  void* pMainThreadRenderResources =
      this->_externals.pPrepareRendererResources->prepareInMainThread(
          tile,
          pWorkerRenderResources);

  pRenderContent->setRenderResources(pMainThreadRenderResources);
  tile.setState(TileLoadState::Done);

  // This allows the raster tile to be updated and children to be created, if
  // necessary.
  updateTileContent(tile, tilesetOptions);
}

void TilesetContentManager::setTileContent(
    Tile& tile,
    TileLoadResult&& result,
    void* pWorkerRenderResources) {
  if (result.state == TileLoadResultState::Failed) {
    tile.getMappedRasterTiles().clear();
    tile.setState(TileLoadState::Failed);
  } else if (result.state == TileLoadResultState::RetryLater) {
    tile.getMappedRasterTiles().clear();
    tile.setState(TileLoadState::FailedTemporarily);
  } else {
    // update tile if the result state is success
    if (result.updatedBoundingVolume) {
      tile.setBoundingVolume(*result.updatedBoundingVolume);
    }

    if (result.updatedContentBoundingVolume) {
      tile.setContentBoundingVolume(*result.updatedContentBoundingVolume);
    }

    auto& content = tile.getContent();
    std::visit(
        ContentKindSetter{
            content,
            std::move(result.rasterOverlayDetails),
            pWorkerRenderResources},
        std::move(result.contentKind));

    if (result.tileInitializer) {
      result.tileInitializer(tile);
    }

    tile.setState(TileLoadState::ContentLoaded);
  }
}

void TilesetContentManager::updateContentLoadedState(
    Tile& tile,
    const TilesetOptions& tilesetOptions) {
  // initialize this tile content first
  TileContent& content = tile.getContent();
  if (content.isExternalContent()) {
    // if tile is external tileset, then it will be refined no matter what
    tile.setUnconditionallyRefine();
    tile.setState(TileLoadState::Done);
  } else if (content.isRenderContent()) {
    // If the main thread part of render content loading is not throttled,
    // do it right away. Otherwise we'll do it later in
    // Tileset::_processMainThreadLoadQueue with prioritization and throttling.
    if (tilesetOptions.mainThreadLoadingTimeLimit <= 0.0) {
      finishLoading(tile, tilesetOptions);
    }
  } else if (content.isEmptyContent()) {
    // There are two possible ways to handle a tile with no content:
    //
    // 1. Treat it as a placeholder used for more efficient culling, but
    //    never render it. Refining to this tile is equivalent to refining
    //    to its children.
    // 2. Treat it as an indication that nothing need be rendered in this
    //    area at this level-of-detail. In other words, "render" it as a
    //    hole. To have this behavior, the tile should _not_ have content at
    //    all.
    //
    // We distinguish whether the tileset creator wanted (1) or (2) by
    // comparing this tile's geometricError to the geometricError of its
    // parent tile. If this tile's error is greater than or equal to its
    // parent, treat it as (1). If it's less, treat it as (2).
    //
    // For a tile with no parent there's no difference between the
    // behaviors.
    double myGeometricError = tile.getNonZeroGeometricError();
    const Tile* pAncestor = tile.getParent();
    while (pAncestor && pAncestor->getUnconditionallyRefine()) {
      pAncestor = pAncestor->getParent();
    }

    double parentGeometricError = pAncestor
                                      ? pAncestor->getNonZeroGeometricError()
                                      : myGeometricError * 2.0;
    if (myGeometricError >= parentGeometricError) {
      tile.setUnconditionallyRefine();
    }

    tile.setState(TileLoadState::Done);
  }
}

void TilesetContentManager::updateDoneState(
    Tile& tile,
    const TilesetOptions& tilesetOptions) {
  // The reason for this method to terminate early when
  // Tile::shouldContentContinueUpdating() returns true is that: When a tile has
  // Tile::shouldContentContinueUpdating() to be true, it means the tile's
  // children need to be created by the
  // TilesetContentLoader::createTileChildren() which is invoked in the
  // TilesetContentManager::updateTileContent() method. In the
  // updateDoneState(), RasterOverlayTiles that are mapped to the tile will
  // begin updating. If there are more RasterOverlayTiles with higher LOD and
  // the current tile is a leaf, more upsample children will be created for that
  // tile. So to accurately determine if a tile is a leaf, it needs the tile to
  // have no children and Tile::shouldContentContinueUpdating() to return false
  // which means the loader has no more children for this tile.
  if (tile.shouldContentContinueUpdating()) {
    return;
  }

  // update raster overlay
  TileContent& content = tile.getContent();
  const TileRenderContent* pRenderContent = content.getRenderContent();
  if (pRenderContent) {
    bool moreRasterDetailAvailable = false;
    bool skippedUnknown = false;
    std::vector<RasterMappedTo3DTile>& rasterTiles =
        tile.getMappedRasterTiles();
    for (size_t i = 0; i < rasterTiles.size(); ++i) {
      RasterMappedTo3DTile& mappedRasterTile = rasterTiles[i];

      RasterOverlayTile* pLoadingTile = mappedRasterTile.getLoadingTile();
      if (pLoadingTile && pLoadingTile->getState() ==
                              RasterOverlayTile::LoadState::Placeholder) {
        RasterOverlayTileProvider* pProvider =
            this->_overlayCollection.findTileProviderForOverlay(
                pLoadingTile->getOverlay());
        RasterOverlayTileProvider* pPlaceholder =
            this->_overlayCollection.findPlaceholderTileProviderForOverlay(
                pLoadingTile->getOverlay());

        // Try to replace this placeholder with real tiles.
        if (pProvider && pPlaceholder && !pProvider->isPlaceholder()) {
          // Remove the existing placeholder mapping
          rasterTiles.erase(
              rasterTiles.begin() +
              static_cast<std::vector<RasterMappedTo3DTile>::difference_type>(
                  i));
          --i;

          // Add a new mapping.
          std::vector<CesiumGeospatial::Projection> missingProjections;
          RasterMappedTo3DTile::mapOverlayToTile(
              tilesetOptions.maximumScreenSpaceError,
              *pProvider,
              *pPlaceholder,
              tile,
              missingProjections);

          if (!missingProjections.empty()) {
            // The mesh doesn't have the right texture coordinates for this
            // overlay's projection, so we need to kick it back to the unloaded
            // state to fix that.
            // In the future, we could add the ability to add the required
            // texture coordinates without starting over from scratch.
            unloadTileContent(tile);
            return;
          }
        }

        continue;
      }

      const RasterOverlayTile::MoreDetailAvailable moreDetailAvailable =
          mappedRasterTile.update(
              *this->_externals.pPrepareRendererResources,
              tile);

      if (moreDetailAvailable ==
              RasterOverlayTile::MoreDetailAvailable::Unknown &&
          !moreRasterDetailAvailable) {
        skippedUnknown = true;
      }

      moreRasterDetailAvailable |=
          moreDetailAvailable == RasterOverlayTile::MoreDetailAvailable::Yes;
    }

    // If this tile still has no children after it's done loading, but it does
    // have raster tiles that are not the most detailed available, create fake
    // children to hang more detailed rasters on by subdividing this tile.
    if (!skippedUnknown && moreRasterDetailAvailable &&
        tile.getChildren().empty()) {
      createQuadtreeSubdividedChildren(tile, this->_upsampler);
    }
  } else {
    // We can't hang raster images on a tile without geometry, and their
    // existence can prevent the tile from being deemed done loading. So clear
    // them out here.
    tile.getMappedRasterTiles().clear();
  }
}

void TilesetContentManager::unloadContentLoadedState(Tile& tile) {
  TileContent& content = tile.getContent();
  TileRenderContent* pRenderContent = content.getRenderContent();
  assert(pRenderContent && "Tile must have render content to be unloaded");

  void* pWorkerRenderResources = pRenderContent->getRenderResources();
  this->_externals.pPrepareRendererResources->free(
      tile,
      pWorkerRenderResources,
      nullptr);
  pRenderContent->setRenderResources(nullptr);
}

void TilesetContentManager::unloadDoneState(Tile& tile) {
  TileContent& content = tile.getContent();
  TileRenderContent* pRenderContent = content.getRenderContent();
  assert(pRenderContent && "Tile must have render content to be unloaded");

  void* pMainThreadRenderResources = pRenderContent->getRenderResources();
  this->_externals.pPrepareRendererResources->free(
      tile,
      nullptr,
      pMainThreadRenderResources);
  pRenderContent->setRenderResources(nullptr);
}

void TilesetContentManager::notifyRasterStartLoading() noexcept {
  ++this->_rasterLoadsInProgress;
}

void TilesetContentManager::notifyRasterDoneLoading() noexcept {
  assert(
      this->_rasterLoadsInProgress > 0 &&
      "There are no raster loads currently in flight");
  --this->_rasterLoadsInProgress;
  ++this->_loadedRastersCount;
}

void TilesetContentManager::notifyTileStartLoading(
    [[maybe_unused]] const Tile* pTile) noexcept {
  ++this->_tileLoadsInProgress;
}

void TilesetContentManager::notifyTileDoneLoading(const Tile* pTile) noexcept {
  assert(
      this->_tileLoadsInProgress > 0 &&
      "There are no tile loads currently in flight");
  --this->_tileLoadsInProgress;
  ++this->_loadedTilesCount;

  if (pTile) {
    this->_tilesDataUsed += pTile->computeByteSize();
  }
}

void TilesetContentManager::notifyTileUnloading(const Tile* pTile) noexcept {
  if (pTile) {
    this->_tilesDataUsed -= pTile->computeByteSize();
  }

  --this->_loadedTilesCount;
}

template <class TilesetContentLoaderType>
void TilesetContentManager::propagateTilesetContentLoaderResult(
    TilesetLoadType type,
    const std::function<void(const TilesetLoadFailureDetails&)>&
        loadErrorCallback,
    TilesetContentLoaderResult<TilesetContentLoaderType>&& result) {

  result.errors.logError(
      this->_externals.pLogger,
      "Errors when loading tileset");

  result.errors.logWarning(
      this->_externals.pLogger,
      "Warnings when loading tileset");

  if (result.errors) {
    if (loadErrorCallback) {
      loadErrorCallback(TilesetLoadFailureDetails{
          nullptr,
          type,
          result.statusCode,
          CesiumUtility::joinToString(result.errors.errors, "\n- ")});
    }
  }

  if (!result.errors) {
    this->_tilesetCredits.reserve(
        this->_tilesetCredits.size() + result.credits.size());
    for (const auto& creditResult : result.credits) {
      this->_tilesetCredits.emplace_back(_externals.pCreditSystem->createCredit(
          creditResult.creditText,
          creditResult.showOnScreen));
    }

    this->_requestHeaders = std::move(result.requestHeaders);
    this->_pLoader = std::move(result.pLoader);
    this->_pRootTile = std::move(result.pRootTile);
  }
}

void TilesetContentManager::discoverLoadWork(
    const std::vector<TileLoadRequest>& requests,
    double maximumScreenSpaceError,
    const TilesetOptions& tilesetOptions,
    std::vector<TileWorkManager::Order>& outOrders) {
  std::set<Tile*> tileWorkAdded;
  for (const TileLoadRequest& loadRequest : requests) {
    // Failed tiles don't get another chance
    if (loadRequest.pTile->getState() == TileLoadState::Failed)
      continue;

    std::vector<TilesetContentManager::ParsedTileWork> parsedTileWork;
    this->parseTileWork(
        loadRequest.pTile,
        0,
        maximumScreenSpaceError,
        parsedTileWork);

    // It's valid for a tile to not have any work
    // It may be waiting for a parent tile to complete
    if (parsedTileWork.empty())
      continue;

    // Sort by depth, which should bubble parent tasks up to the top
    std::sort(parsedTileWork.begin(), parsedTileWork.end());

    // Work with max depth is at top of list
    size_t maxDepth = parsedTileWork.begin()->depthIndex;

    // Add all the work, biasing priority by depth
    // Give parents a higher priority (lower value)
    for (ParsedTileWork& work : parsedTileWork) {
      double priorityBias = double(maxDepth - work.depthIndex);
      double resultPriority = loadRequest.priority + priorityBias;

      // We always need a source (non raster) tile
      assert(work.tileWorkChain.isValid());
      Tile* pTile = work.tileWorkChain.pTile;

      // If order for source tile already exists, skip adding more work for it
      // Ex. Tile work needs to load its parent, and multiple children point
      // to that same parent. Don't add the parent more than once
      if (tileWorkAdded.find(pTile) != tileWorkAdded.end())
        continue;
      tileWorkAdded.insert(pTile);

      auto& newOrder = outOrders.emplace_back(TileWorkManager::Order{
          std::move(work.tileWorkChain.requestData),
          TileProcessingData{
              work.tileWorkChain.pTile,
              work.tileWorkChain.tileCallback,
              work.projections,
              tilesetOptions.contentOptions,
              tilesetOptions.rendererOptions},
          loadRequest.group,
          resultPriority});

      // Embed child work in parent
      for (auto& rasterWorkChain : work.rasterWorkChains) {
        assert(rasterWorkChain.isValid());
        newOrder.childOrders.emplace_back(TileWorkManager::Order{
            std::move(rasterWorkChain.requestData),
            RasterProcessingData{
                rasterWorkChain.pRasterTile,
                rasterWorkChain.rasterCallback},
            loadRequest.group,
            resultPriority});
      }
    }
  }
}

void TilesetContentManager::markWorkTilesAsLoading(
    const std::vector<const TileWorkManager::Work*>& workVector) {

  for (const TileWorkManager::Work* work : workVector) {
    if (std::holds_alternative<TileProcessingData>(
            work->order.processingData)) {
      TileProcessingData tileProcessing =
          std::get<TileProcessingData>(work->order.processingData);
      assert(tileProcessing.pTile);
      assert(
          tileProcessing.pTile->getState() == TileLoadState::Unloaded ||
          tileProcessing.pTile->getState() == TileLoadState::FailedTemporarily);

      tileProcessing.pTile->setState(TileLoadState::ContentLoading);
    } else {
      RasterProcessingData rasterProcessing =
          std::get<RasterProcessingData>(work->order.processingData);
      assert(rasterProcessing.pRasterTile);

      RasterOverlayTile* pLoading =
          rasterProcessing.pRasterTile->getLoadingTile();
      assert(pLoading);
      assert(pLoading->getState() == RasterOverlayTile::LoadState::Unloaded);

      pLoading->setState(RasterOverlayTile::LoadState::Loading);
    }
  }
}

void TilesetContentManager::handleCompletedWork() {
  std::vector<TileWorkManager::DoneOrder> doneOrders;
  std::vector<TileWorkManager::FailedOrder> failedOrders;
  _pTileWorkManager->TakeCompletedWork(doneOrders, failedOrders);

  for (auto& doneOrder : doneOrders) {
    const TileWorkManager::Order& order = doneOrder.order;

    if (std::holds_alternative<TileProcessingData>(order.processingData)) {
      TileProcessingData tileProcessing =
          std::get<TileProcessingData>(order.processingData);
      assert(tileProcessing.pTile);

      this->setTileContent(
          *tileProcessing.pTile,
          std::move(doneOrder.loadResult),
          doneOrder.pRenderResources);
    };
  }

  for (auto& failedOrder : failedOrders) {
    const TileWorkManager::Order& order = failedOrder.order;

    SPDLOG_LOGGER_ERROR(
        this->_externals.pLogger,
        "{}: {}",
        failedOrder.failureReason,
        order.requestData.url);

    if (std::holds_alternative<TileProcessingData>(order.processingData)) {
      TileProcessingData tileProcessing =
          std::get<TileProcessingData>(order.processingData);
      assert(tileProcessing.pTile);
      tileProcessing.pTile->setState(TileLoadState::Failed);
    } else {
      RasterProcessingData rasterProcessing =
          std::get<RasterProcessingData>(order.processingData);
      assert(rasterProcessing.pRasterTile);

      RasterOverlayTile* pLoading =
          rasterProcessing.pRasterTile->getLoadingTile();
      assert(pLoading);

      pLoading->setState(RasterOverlayTile::LoadState::Failed);
    }
  }
}

void TilesetContentManager::dispatchTileWork(
    TileProcessingData& processingData,
    const CesiumAsync::UrlResponseDataMap& responseDataMap,
    TileWorkManager::Work* work) {
  Tile* pTile = processingData.pTile;

  // Optionally could move this to work manager
  this->notifyTileStartLoading(pTile);

  // Keep the manager alive while the load is in progress.
  CesiumUtility::IntrusivePointer<TilesetContentManager> thiz = this;

  TileContentLoadInfo tileLoadInfo{
      this->_externals.asyncSystem,
      this->_externals.pAssetAccessor,
      this->_externals.pPrepareRendererResources,
      this->_externals.pLogger,
      processingData.contentOptions,
      *pTile};

  TilesetContentLoader* pLoader;
  if (pTile->getLoader() == &this->_upsampler) {
    pLoader = &this->_upsampler;
  } else {
    pLoader = this->_pLoader.get();
  }

  TileLoadInput loadInput{
      *pTile,
      processingData.contentOptions,
      this->_externals.asyncSystem,
      this->_externals.pLogger,
      responseDataMap};

  assert(processingData.loaderCallback);

  processingData.loaderCallback(loadInput, pLoader)
      .thenImmediately([tileLoadInfo = std::move(tileLoadInfo),
                        &requestHeaders = this->_requestHeaders,
                        &projections = processingData.projections,
                        &rendererOptions = processingData.rendererOptions,
                        pThis = this,
                        pWorkManager = thiz->_pTileWorkManager,
                        _work = work](TileLoadResult&& result) mutable {
        // the reason we run immediate continuation, instead of in the
        // worker thread, is that the loader may run the task in the main
        // thread. And most often than not, those main thread task is very
        // light weight. So when those tasks return, there is no need to
        // spawn another worker thread if the result of the task isn't
        // related to render content. We only ever spawn a new task in the
        // worker thread if the content is a render content
        if (result.state == TileLoadResultState::RequestRequired) {
          // This work goes back into the work manager queue
          CesiumAsync::RequestData& request = result.additionalRequestData;

          // Add new requests here
          assert(
              _work->completedRequests.find(request.url) ==
              _work->completedRequests.end());
          _work->pendingRequests.push_back(std::move(request));

          TileWorkManager::RequeueWorkForRequest(pWorkManager, _work);

          return tileLoadInfo.asyncSystem
              .createResolvedFuture<TileLoadResultState>(
                  TileLoadResultState::RequestRequired);
        }

        if (result.state == TileLoadResultState::Success &&
            std::holds_alternative<CesiumGltf::Model>(result.contentKind)) {
          return postProcessContent(
                     std::move(result),
                     std::move(projections),
                     std::move(tileLoadInfo),
                     result.originalRequestUrl,
                     requestHeaders,
                     rendererOptions)
              .thenInMainThread(
                  [pThis = pThis, pWorkManager = pWorkManager, _work = _work](
                      TileLoadResultAndRenderResources&& pair) mutable {
                    _work->tileLoadResult = std::move(pair.result);
                    _work->pRenderResources = pair.pRenderResources;
                    pWorkManager->SignalWorkComplete(_work);

                    pThis->handleCompletedWork();
                    TileWorkManager::TryDispatchProcessing(pWorkManager);
                    return TileLoadResultState::Success;
                  });
        }

        // We're successful with no gltf model, or in a failure state
        _work->tileLoadResult = std::move(result);
        pWorkManager->SignalWorkComplete(_work);

        return tileLoadInfo.asyncSystem.runInMainThread(
            [pThis = pThis,
             pWorkManager = pWorkManager,
             state = _work->tileLoadResult.state]() mutable {
              pThis->handleCompletedWork();
              TileWorkManager::TryDispatchProcessing(pWorkManager);
              return state;
            });
      })
      .thenInMainThread(
          [_thiz = thiz, _pTile = pTile](TileLoadResultState state) mutable {
            // Wrap up this tile and also keep intrusive pointer alive
            if (state == TileLoadResultState::Success)
              _thiz->notifyTileDoneLoading(_pTile);
            else
              _thiz->notifyTileDoneLoading(nullptr);
          })
      .catchInMainThread(
          [_pTile = pTile, _thiz = this, pLogger = this->_externals.pLogger](
              std::exception&& e) {
            _pTile->setState(TileLoadState::Failed);

            _thiz->notifyTileDoneLoading(_pTile);
            SPDLOG_LOGGER_ERROR(
                pLogger,
                "An unexpected error occurs when loading tile: {}",
                e.what());
          });
}

void TilesetContentManager::dispatchRasterWork(
    RasterProcessingData& processingData,
    const CesiumAsync::UrlResponseDataMap& responseDataMap,
    TileWorkManager::Work* work) {
  RasterMappedTo3DTile* pRasterTile = processingData.pRasterTile;
  assert(pRasterTile);

  RasterOverlayTile* pLoadingTile = pRasterTile->getLoadingTile();
  if (!pLoadingTile) {
    // Can't do any work
    this->_pTileWorkManager->SignalWorkComplete(work);
    return;
  }

  // Optionally could move this to work manager
  this->notifyRasterStartLoading();

  RasterOverlayTileProvider& provider = pLoadingTile->getTileProvider();

  // Keep the these objects alive while the load is in progress.
  CesiumUtility::IntrusivePointer<TilesetContentManager> thiz = this;
  IntrusivePointer<RasterOverlayTile> pTile = pLoadingTile;
  IntrusivePointer<RasterOverlayTileProvider> pProvider = &provider;

  provider
      .loadTileThrottled(
          *pLoadingTile,
          responseDataMap,
          processingData.rasterCallback)
      .thenImmediately([pWorkManager = thiz->_pTileWorkManager,
                        _work = work](RasterLoadResult&& result) mutable {
        if (result.state == RasterOverlayTile::LoadState::RequestRequired) {
          // This work goes back into the work manager queue
          assert(!result.missingRequests.empty());

          for (auto& request : result.missingRequests) {
        // Make sure we're not requesting something we have
#ifndef NDEBUG
            assert(
                _work->completedRequests.find(request.url) ==
                _work->completedRequests.end());
            for (auto pending : _work->pendingRequests)
              assert(pending.url != request.url);
#endif
            // Add new requests here
            _work->pendingRequests.push_back(std::move(request));
          }

          TileWorkManager::RequeueWorkForRequest(pWorkManager, _work);
        } else {
          pWorkManager->SignalWorkComplete(_work);
        }

        return std::move(result);
      })
      .thenInMainThread([_thiz = thiz, pTile = pTile, pProvider = pProvider](
                            RasterLoadResult&& result) mutable {
        if (result.state == RasterOverlayTile::LoadState::RequestRequired) {
          // Nothing to do
        } else {
          pTile->_rectangle = result.rectangle;
          pTile->_pRendererResources = result.pRendererResources;
          assert(result.image.has_value());
          pTile->_image = std::move(result.image.value());
          pTile->_tileCredits = std::move(result.credits);
          pTile->_moreDetailAvailable =
              result.moreDetailAvailable
                  ? RasterOverlayTile::MoreDetailAvailable::Yes
                  : RasterOverlayTile::MoreDetailAvailable::No;
          pTile->setState(result.state);

          result.pTile = pTile;

          pProvider->incrementTileDataBytes(pTile->getImage());
        }

        _thiz->notifyRasterDoneLoading();

        TileWorkManager::TryDispatchProcessing(_thiz->_pTileWorkManager);
      })
      .catchInMainThread(
          [_thiz = thiz, pTile = pLoadingTile](const std::exception& /*e*/) {
            pTile->_pRendererResources = nullptr;
            pTile->_image = {};
            pTile->_tileCredits = {};
            pTile->_moreDetailAvailable =
                RasterOverlayTile::MoreDetailAvailable::No;
            pTile->setState(RasterOverlayTile::LoadState::Failed);

            _thiz->notifyRasterDoneLoading();
          });
}

void TilesetContentManager::parseTileWork(
    Tile* pTile,
    size_t depthIndex,
    double maximumScreenSpaceError,
    std::vector<ParsedTileWork>& outWork) {
  CESIUM_TRACE("TilesetContentManager::parseTileWork");

  // We can't load a tile that is unloading; it has to finish unloading first.
  if (pTile->getState() == TileLoadState::Unloading)
    return;

  assert(
      pTile->getState() == TileLoadState::Unloaded ||
      pTile->getState() == TileLoadState::FailedTemporarily);

  // Below are the guarantees the loader can assume about upsampled tile. If any
  // of those guarantees are wrong, it's a bug:
  // - Any tile that is marked as upsampled tile, we will guarantee that the
  // parent is always loaded. It lets the loader takes care of upsampling only
  // without requesting the parent tile. If a loader tries to upsample tile, but
  // the parent is not loaded, it is a bug.
  // - This manager will also guarantee that the parent tile will be alive until
  // the upsampled tile content returns to the main thread. So the loader can
  // capture the parent geometry by reference in the worker thread to upsample
  // the current tile. Warning: it's not thread-safe to modify the parent
  // geometry in the worker thread at the same time though
  const CesiumGeometry::UpsampledQuadtreeNode* pUpsampleID =
      std::get_if<CesiumGeometry::UpsampledQuadtreeNode>(&pTile->getTileID());
  if (pUpsampleID) {
    // We can't upsample this tile if no parent
    Tile* pParentTile = pTile->getParent();
    if (!pParentTile)
      return;

    TileLoadState parentState = pParentTile->getState();

    // If not currently loading, queue some work
    if (parentState < TileLoadState::ContentLoading) {
      parseTileWork(
          pParentTile,
          depthIndex + 1,
          maximumScreenSpaceError,
          outWork);
      return;
    }

    // We can't proceed until our parent is done. Wait another tick
    if (parentState != TileLoadState::Done)
      return;

    // Parent is done, continue adding work for this tile
  }

  // Parse any content fetch work
  TilesetContentLoader* pLoader;
  if (pTile->getLoader() == &this->_upsampler) {
    pLoader = &this->_upsampler;
  } else {
    pLoader = this->_pLoader.get();
  }

  // Default headers come from the this. Loader can override if needed
  CesiumAsync::RequestData requestData;
  requestData.headers = this->_requestHeaders;
  TileLoaderCallback tileCallback;

  if (pLoader->getLoadWork(pTile, requestData, tileCallback)) {
    // New work was found, add it and any raster work
    ParsedTileWork newWork = {
        depthIndex,
        TileWorkChain{pTile, requestData, tileCallback}};

    newWork.projections = mapOverlaysToTile(
        *pTile,
        this->_overlayCollection,
        maximumScreenSpaceError,
        this->_requestHeaders,
        newWork.rasterWorkChains);

    outWork.push_back(newWork);
  }
}

} // namespace Cesium3DTilesSelection
