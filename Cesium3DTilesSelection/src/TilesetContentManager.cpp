#include "TilesetContentManager.h"

#include "LayerJsonTerrainLoader.h"
#include "RasterOverlayUpsampler.h"
#include "TileContentLoadInfo.h"
#include "TilesetJsonLoader.h"

#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/GltfModifier.h>
#include <Cesium3DTilesSelection/GltfModifierState.h>
#include <Cesium3DTilesSelection/GltfModifierVersionExtension.h>
#include <Cesium3DTilesSelection/IPrepareRendererResources.h>
#include <Cesium3DTilesSelection/RasterMappedTo3DTile.h>
#include <Cesium3DTilesSelection/RasterOverlayCollection.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileID.h>
#include <Cesium3DTilesSelection/TileLoadRequester.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TileRefine.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderFactory.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetLoadFailureDetails.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumAsync/SharedFuture.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/BoundingRegionWithLooseFittingHeights.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGltf/Image.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumRasterOverlays/ActivatedRasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/RasterOverlayUtilities.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Math.h>
#include <CesiumUtility/ReferenceCounted.h>
#include <CesiumUtility/Tracing.h>
#include <CesiumUtility/joinToString.h>
#include <CesiumVectorData/GeoJsonDocument.h>

#include <fmt/format.h>
#include <glm/common.hpp>
#include <glm/ext/vector_double2.hpp>
#include <rapidjson/document.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <any>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

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
      if (!image.pAsset)
        continue;

      // If the image size hasn't been overridden, store the pixelData
      // size now. We'll be adding this number to our total memory usage soon,
      // and remove it when the tile is later unloaded, and we must use
      // the same size in each case.
      if (image.pAsset->sizeBytes < 0) {
        image.pAsset->sizeBytes = int64_t(image.pAsset->pixelData.size());
      }
    }

    auto pRenderContent = std::make_unique<TileRenderContent>(std::move(model));
    pRenderContent->setRenderResources(pRenderResources);
    if (rasterOverlayDetails) {
      pRenderContent->setRasterOverlayDetails(std::move(*rasterOverlayDetails));
    }

    tileContent.setContentKind(std::move(pRenderContent));
  }

  void operator()(CesiumVectorData::GeoJsonDocument&& content) {
    tileContent.setContentKind(
        std::make_unique<TileFeatureContent>(std::move(content)));
  }

  TileContent& tileContent;
  std::optional<RasterOverlayDetails> rasterOverlayDetails;
  void* pRenderResources;
};

void unloadTileRecursively(
    Tile& tile,
    TilesetContentManager& tilesetContentManager) {
  for (Tile& child : tile.getChildren()) {
    unloadTileRecursively(child, tilesetContentManager);
  }

  tilesetContentManager.unloadTileContent(tile);
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
  CESIUM_ASSERT(
      pRenderContent && "This function only deal with render content");

  const RasterOverlayDetails& details =
      pRenderContent->getRasterOverlayDetails();

  // If we don't have any overlay projections/rectangles, why are we
  // upsampling?
  CESIUM_ASSERT(!details.rasterOverlayProjections.empty());
  CESIUM_ASSERT(!details.rasterOverlayRectangles.empty());

  // Use the projected center of the tile as the subdivision center.
  // The tile will be subdivided by (0.5, 0.5) in the first overlay's
  // texture coordinates which overlay had more detail.
  for (const RasterMappedTo3DTile& mapped : parent.getMappedRasterTiles()) {
    if (mapped.isMoreDetailAvailable()) {
      const RasterOverlayTile* pReadyTile = mapped.getReadyTile();
      if (!pReadyTile) {
        assert(false);
        continue;
      }
      const CesiumGeospatial::Projection& projection =
          pReadyTile->getTileProvider().getProjection();
      const CesiumGeometry::Rectangle* pRectangle =
          details.findRectangleForOverlayProjection(projection);
      if (!pRectangle) {
        assert(false);
        continue;
      }

      // The subdivision center must be at exactly the location of the (0.5,
      // 0.5) raster overlay texture coordinate for this projection.
      glm::dvec2 centerProjected = pRectangle->getCenter();
      CesiumGeospatial::Cartographic center =
          CesiumGeospatial::unprojectPosition(
              projection,
              glm::dvec3(centerProjected, 0.0));

      // Subdivide the same rectangle that was used to generate the raster
      // overlay texture coordinates. But union it with the tight-fitting
      // content bounds in order to avoid error from repeated subdivision in
      // extreme cases.
      CesiumGeospatial::GlobeRectangle globeRectangle =
          CesiumGeospatial::unprojectRectangleSimple(projection, *pRectangle);
      globeRectangle =
          globeRectangle.computeUnion(details.boundingRegion.getRectangle());

      return RegionAndCenter{
          CesiumGeospatial::BoundingRegion(
              globeRectangle,
              details.boundingRegion.getMinimumHeight(),
              details.boundingRegion.getMaximumHeight(),
              getProjectionEllipsoid(projection)),
          center};
    }
  }

  // We shouldn't be upsampling from a tile until that tile is loaded.
  // If it has no content after loading, we can't upsample from it.
  return std::nullopt;
}

void createQuadtreeSubdividedChildren(
    const CesiumGeospatial::Ellipsoid& ellipsoid,
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
  std::span<Tile> childrenView = parent.getChildren();
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
          maximumHeight,
          ellipsoid)));

  se.setBoundingVolume(CesiumGeospatial::BoundingRegionWithLooseFittingHeights(
      CesiumGeospatial::BoundingRegion(
          CesiumGeospatial::GlobeRectangle(
              center.longitude,
              parentRectangle.getSouth(),
              parentRectangle.getEast(),
              center.latitude),
          minimumHeight,
          maximumHeight,
          ellipsoid)));

  nw.setBoundingVolume(CesiumGeospatial::BoundingRegionWithLooseFittingHeights(
      CesiumGeospatial::BoundingRegion(
          CesiumGeospatial::GlobeRectangle(
              parentRectangle.getWest(),
              center.latitude,
              center.longitude,
              parentRectangle.getNorth()),
          minimumHeight,
          maximumHeight,
          ellipsoid)));

  ne.setBoundingVolume(CesiumGeospatial::BoundingRegionWithLooseFittingHeights(
      CesiumGeospatial::BoundingRegion(
          CesiumGeospatial::GlobeRectangle(
              center.longitude,
              center.latitude,
              parentRectangle.getEast(),
              parentRectangle.getNorth()),
          minimumHeight,
          maximumHeight,
          ellipsoid)));

  // set children transforms
  sw.setTransform(parent.getTransform());
  se.setTransform(parent.getTransform());
  nw.setTransform(parent.getTransform());
  ne.setTransform(parent.getTransform());
}

// Really, these two methods are risky and could easily result in bugs, but
// they're private methods used only in this class, so it's easier to just tell
// clang-tidy to ignore them.

// NOLINTBEGIN(bugprone-return-const-ref-from-parameter)
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
  CESIUM_ASSERT(!updatedTileContentBoundingVolume);

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
// NOLINTEND(bugprone-return-const-ref-from-parameter)

void calcRasterOverlayDetailsInWorkerThread(
    TileLoadResult& result,
    std::vector<CesiumGeospatial::Projection>&& projections,
    const TileContentLoadInfo& tileLoadInfo) {
  CesiumGltf::Model& model = std::get<CesiumGltf::Model>(result.contentKind);

  // we will use the best-fitting bounding volume to calculate raster overlay
  // details below
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
    result.rasterOverlayDetails->merge(*overlayDetails, result.ellipsoid);
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
          tileLoadInfo.tileTransform,
          result.ellipsoid);
    }
  }
}

void postProcessGltfInWorkerThread(
    TileLoadResult& result,
    std::vector<CesiumGeospatial::Projection>&& projections,
    const TileContentLoadInfo& tileLoadInfo) {
  CesiumGltf::Model& model = std::get<CesiumGltf::Model>(result.contentKind);

  if (result.pCompletedRequest) {
    model.extras["Cesium3DTiles_TileUrl"] = result.pCompletedRequest->url();
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

CesiumAsync::Future<TileLoadResultAndRenderResources>
postProcessContentInWorkerThread(
    TileLoadResult&& result,
    std::vector<CesiumGeospatial::Projection>&& projections,
    TileContentLoadInfo&& tileLoadInfo,
    const std::any& rendererOptions,
    const std::shared_ptr<GltfModifier>& pGltfModifier) {
  CESIUM_ASSERT(
      result.state == TileLoadResultState::Success &&
      "This function requires result to be success");

  std::optional<int64_t> version =
      pGltfModifier ? pGltfModifier->getCurrentVersion() : std::nullopt;

  auto asyncSystem = tileLoadInfo.asyncSystem;

  result.initialBoundingVolume = tileLoadInfo.tileBoundingVolume;
  result.initialContentBoundingVolume = tileLoadInfo.tileContentBoundingVolume;

  postProcessGltfInWorkerThread(result, std::move(projections), tileLoadInfo);

  auto applyGltfModifier = [&]() {
    // Apply the glTF modifier right away, otherwise it will be
    // triggered immediately after the renderer-side resources
    // have been created, which is both inefficient and a cause
    // of visual glitches (the model will appear briefly in its
    // unmodified state before stabilizing)
    const CesiumGltf::Model& model =
        std::get<CesiumGltf::Model>(result.contentKind);
    return pGltfModifier
        ->apply(GltfModifierInput{
            .version = *version,
            .asyncSystem = tileLoadInfo.asyncSystem,
            .pAssetAccessor = tileLoadInfo.pAssetAccessor,
            .pLogger = tileLoadInfo.pLogger,
            .previousModel = model,
            .tileTransform = tileLoadInfo.tileTransform})
        .thenInWorkerThread(
            [result = std::move(result),
             version](std::optional<GltfModifierOutput>&& modified) mutable {
              if (modified) {
                result.contentKind = std::move(modified->modifiedModel);
              }

              CesiumGltf::Model* pModel =
                  std::get_if<CesiumGltf::Model>(&result.contentKind);
              if (pModel) {
                GltfModifierVersionExtension::setVersion(*pModel, *version);
              }

              return result;
            })
        .thenPassThrough(std::move(tileLoadInfo));
  };

  CesiumAsync::Future<std::tuple<TileContentLoadInfo, TileLoadResult>> future =
      pGltfModifier && version
          ? applyGltfModifier()
          : tileLoadInfo.asyncSystem.createResolvedFuture(std::move(result))
                .thenPassThrough(std::move(tileLoadInfo));

  return std::move(future).thenInWorkerThread(
      [rendererOptions](
          std::tuple<TileContentLoadInfo, TileLoadResult>&& tuple) {
        auto& [tileLoadInfo, result] = tuple;

        // create render resources
        if (tileLoadInfo.pPrepareRendererResources) {
          return tileLoadInfo.pPrepareRendererResources->prepareInLoadThread(
              tileLoadInfo.asyncSystem,
              std::move(result),
              tileLoadInfo.tileTransform,
              rendererOptions);
        } else {
          return tileLoadInfo.asyncSystem
              .createResolvedFuture<TileLoadResultAndRenderResources>(
                  TileLoadResultAndRenderResources{std::move(result), nullptr});
        }
      });
}

std::optional<Credit> createUserCredit(
    const TilesetOptions& tilesetOptions,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const CreditSource& creditSource) {
  if (!tilesetOptions.credit || !pCreditSystem)
    return std::nullopt;

  return pCreditSystem->createCredit(
      creditSource,
      *tilesetOptions.credit,
      tilesetOptions.showCreditsOnScreen);
}

} // namespace

TilesetContentManager::TilesetContentManager(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions)
    : _externals{externals},
      _requestHeaders{tilesetOptions.requestHeaders},
      _pLoader{nullptr},
      _pRootTile{nullptr},
      _userCredit(),
      _tilesetCredits{},
      _overlayCollection(
          LoadedTileEnumerator(nullptr),
          externals,
          tilesetOptions.ellipsoid),
      _tileLoadsInProgress{0},
      _loadedTilesCount{0},
      _tilesDataUsed{0},
      _tilesetDestroyed(false),
      _pSharedAssetSystem(externals.pSharedAssetSystem),
      _destructionCompletePromise{externals.asyncSystem.createPromise<void>()},
      _destructionCompleteFuture{
          this->_destructionCompletePromise.getFuture().share()},
      _rootTileAvailablePromise{externals.asyncSystem.createPromise<void>()},
      _rootTileAvailableFuture{
          this->_rootTileAvailablePromise.getFuture().share()},
      _tilesEligibleForContentUnloading(),
      _requesters(),
      _roundRobinValueWorker(0.0),
      _roundRobinValueMain(0.0),
      _requesterFractions(),
      _requestersWithRequests(),
      _creditSource(externals.pCreditSystem) {
  this->_userCredit = createUserCredit(
      tilesetOptions,
      externals.pCreditSystem,
      this->_creditSource);

  this->_upsampler.setOwner(*this);
}

/* static */ CesiumUtility::IntrusivePointer<TilesetContentManager>
TilesetContentManager::createFromLoader(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    std::unique_ptr<TilesetContentLoader>&& pLoader,
    std::unique_ptr<Tile>&& pRootTile) {
  CESIUM_ASSERT(pLoader != nullptr);

  IntrusivePointer<TilesetContentManager> pManager =
      new TilesetContentManager(externals, tilesetOptions);

  pManager->notifyTileStartLoading(nullptr);

  pManager->registerGltfModifier(pRootTile.get())
      .thenInMainThread([pManager,
                         pLoader = std::move(pLoader),
                         pRootTile = std::move(pRootTile)]() mutable {
        pManager->notifyTileDoneLoading(pRootTile.get());

        pManager->propagateTilesetContentLoaderResult(
            TilesetLoadType::TilesetJson,
            [](const TilesetLoadFailureDetails&) {},
            TilesetContentLoaderResult<TilesetContentLoader>(
                std::move(pLoader),
                std::move(pRootTile),
                {},
                {},
                ErrorList()));

        pManager->_rootTileAvailablePromise.resolve();
      });

  return pManager;
}

/* static */ CesiumUtility::IntrusivePointer<TilesetContentManager>
TilesetContentManager::createFromUrl(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    const std::string& url) {
  IntrusivePointer<TilesetContentManager> pManager =
      new TilesetContentManager(externals, tilesetOptions);

  if (!url.empty()) {
    pManager->notifyTileStartLoading(nullptr);

    const CesiumGeospatial::Ellipsoid& ellipsoid = tilesetOptions.ellipsoid;

    externals.pAssetAccessor
        ->get(externals.asyncSystem, url, pManager->_requestHeaders)
        .thenInWorkerThread(
            [externals,
             ellipsoid,
             pLogger = externals.pLogger,
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
              std::span<const std::byte> tilesetJsonBinary = pResponse->data();
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
                return TilesetJsonLoader::createLoader(
                           asyncSystem,
                           pAssetAccessor,
                           pLogger,
                           url,
                           pCompletedRequest->headers(),
                           std::move(tilesetJson),
                           ellipsoid)
                    .thenImmediately(
                        [](TilesetContentLoaderResult<TilesetContentLoader>&&
                               result) { return std::move(result); });
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
                             tilesetJson,
                             ellipsoid)
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
            [pManager](
                TilesetContentLoaderResult<TilesetContentLoader>&& result) {
              return pManager->registerGltfModifier(result.pRootTile.get())
                  .thenImmediately([result = std::move(result)]() mutable {
                    return std::move(result);
                  });
            })
        .thenInMainThread(
            [pManager, errorCallback = tilesetOptions.loadErrorCallback](
                TilesetContentLoaderResult<TilesetContentLoader>&& result) {
              pManager->notifyTileDoneLoading(result.pRootTile.get());
              pManager->propagateTilesetContentLoaderResult(
                  TilesetLoadType::TilesetJson,
                  errorCallback,
                  std::move(result));
              pManager->_rootTileAvailablePromise.resolve();
            })
        .catchInMainThread([pManager](std::exception&& e) {
          pManager->notifyTileDoneLoading(nullptr);
          SPDLOG_LOGGER_ERROR(
              pManager->_externals.pLogger,
              "An unexpected error occurred when loading tile: {}",
              e.what());
          pManager->_rootTileAvailablePromise.reject(
              std::runtime_error("Root tile failed to load."));
        });
  }

  return pManager;
}

/* static */ CesiumUtility::IntrusivePointer<TilesetContentManager>
TilesetContentManager::createFromLoaderFactory(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    TilesetContentLoaderFactory&& loaderFactory) {
  IntrusivePointer<TilesetContentManager> pManager =
      new TilesetContentManager(externals, tilesetOptions);

  if (loaderFactory.isValid()) {
    auto authorizationChangeListener = [pManagerRaw = pManager.get()](
                                           const std::string& header,
                                           const std::string& headerValue) {
      auto& requestHeaders = pManagerRaw->_requestHeaders;
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

    pManager->notifyTileStartLoading(nullptr);

    loaderFactory
        .createLoader(externals, tilesetOptions, authorizationChangeListener)
        .thenInMainThread(
            [pManager](
                TilesetContentLoaderResult<TilesetContentLoader>&& result) {
              return pManager->registerGltfModifier(result.pRootTile.get())
                  .thenImmediately([result = std::move(result)]() mutable {
                    return std::move(result);
                  });
            })
        .thenInMainThread(
            [pManager, errorCallback = tilesetOptions.loadErrorCallback](
                TilesetContentLoaderResult<TilesetContentLoader>&& result) {
              pManager->notifyTileDoneLoading(result.pRootTile.get());
              pManager->propagateTilesetContentLoaderResult(
                  TilesetLoadType::CesiumIon,
                  errorCallback,
                  std::move(result));
              pManager->_rootTileAvailablePromise.resolve();
            })
        .catchInMainThread([pManager](std::exception&& e) {
          pManager->notifyTileDoneLoading(nullptr);
          SPDLOG_LOGGER_ERROR(
              pManager->_externals.pLogger,
              "An unexpected error occurred when loading tile: {}",
              e.what());
          pManager->_rootTileAvailablePromise.reject(
              std::runtime_error("Root tile failed to load."));
        });
  }

  return pManager;
}

/* static */ CesiumUtility::IntrusivePointer<TilesetContentManager>
TilesetContentManager::createFromCesiumIon(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    int64_t ionAssetID,
    const std::string& ionAccessToken,
    const std::string& ionAssetEndpointUrl) {
  return TilesetContentManager::createFromLoaderFactory(
      externals,
      tilesetOptions,
      CesiumIonTilesetContentLoaderFactory(
          static_cast<uint32_t>(ionAssetID),
          ionAccessToken,
          ionAssetEndpointUrl));
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
  CESIUM_ASSERT(this->_tileLoadsInProgress == 0);
  this->unloadAll();

  this->_destructionCompletePromise.resolve();
}

void TilesetContentManager::reapplyGltfModifier(
    Tile& tile,
    const TilesetOptions& tilesetOptions,
    TileRenderContent* pRenderContent) noexcept {
  // An existing modified model must be the wrong version (or we wouldn't be
  // here), so discard it.
  if (pRenderContent->getModifiedModel()) {
    CESIUM_ASSERT(
        pRenderContent->getGltfModifierState() ==
        GltfModifierState::WorkerDone);
    this->_externals.pPrepareRendererResources->free(
        tile,
        pRenderContent->getModifiedRenderResources(),
        nullptr);
    pRenderContent->resetModifiedModelAndRenderResources();
    pRenderContent->setGltfModifierState(GltfModifierState::Idle);
  }

  this->notifyTileStartLoading(&tile);
  pRenderContent->setGltfModifierState(GltfModifierState::WorkerRunning);

  const CesiumGltf::Model& previousModel = pRenderContent->getModel();

  const TilesetExternals& externals = this->getExternals();

  // Get the version here, in the main thread, because it may change while the
  // worker is running.
  CESIUM_ASSERT(externals.pGltfModifier->getCurrentVersion());
  int64_t version = externals.pGltfModifier->getCurrentVersion().value_or(-1);

  // It is safe to capture the TilesetExternals and Model by reference because
  // the TilesetContentManager guarantees both will continue to exist and are
  // immutable while modification is in progress.
  //
  // This is largely true of Tile as well, except that Tiles are nodes in a
  // larger graph, and the entire graph is _not_ guaranteed to be immutable. So
  // best to avoid handing a `Tile` instance to a worker thread.
  //
  // We capture an IntrusivePointer to a Tile in the main thread continuations
  // at the end in order to keep the Tile, its content, and the
  // TilesetContentManager alive during the entire process.
  externals.asyncSystem
      .runInWorkerThread([&externals,
                          &previousModel,
                          version,
                          tileTransform = tile.getTransform()] {
        return externals.pGltfModifier->apply(GltfModifierInput{
            .version = version,
            .asyncSystem = externals.asyncSystem,
            .pAssetAccessor = externals.pAssetAccessor,
            .pLogger = externals.pLogger,
            .previousModel = previousModel,
            .tileTransform = tileTransform});
      })
      .thenInWorkerThread([&externals,
                           &previousModel,
                           pRenderContent,
                           tileTransform = tile.getTransform(),
                           tileBoundingVolume = tile.getBoundingVolume(),
                           tileContentBoundingVolume =
                               tile.getContentBoundingVolume(),
                           rendererOptions = tilesetOptions.rendererOptions](
                              std::optional<GltfModifierOutput>&& modified) {
        TileLoadResult tileLoadResult;
        tileLoadResult.state = TileLoadResultState::Success;
        tileLoadResult.pAssetAccessor = externals.pAssetAccessor;
        tileLoadResult.rasterOverlayDetails =
            pRenderContent->getRasterOverlayDetails();
        tileLoadResult.initialBoundingVolume = tileBoundingVolume;
        tileLoadResult.initialContentBoundingVolume = tileContentBoundingVolume;

        {
          const CesiumGltf::Model& model =
              modified ? modified->modifiedModel : previousModel;
          const auto it = model.extras.find("gltfUpAxis");
          if (it != model.extras.end()) {
            tileLoadResult.glTFUpAxis = static_cast<CesiumGeometry::Axis>(
                it->second.getSafeNumberOrDefault(1));
          }
        }

        if (modified) {
          tileLoadResult.contentKind = std::move(modified->modifiedModel);
        } else {
          tileLoadResult.contentKind = previousModel;
        }

        if (modified && externals.pPrepareRendererResources) {
          return externals.pPrepareRendererResources->prepareInLoadThread(
              externals.asyncSystem,
              std::move(tileLoadResult),
              tileTransform,
              rendererOptions);
        } else {
          return externals.asyncSystem
              .createResolvedFuture<TileLoadResultAndRenderResources>(
                  TileLoadResultAndRenderResources{
                      std::move(tileLoadResult),
                      nullptr});
        }
      })
      .thenInMainThread([this, version, pTile = Tile::Pointer(&tile)](
                            TileLoadResultAndRenderResources&& pair) {
        TileRenderContent* pRenderContent =
            pTile->getContent().getRenderContent();
        CESIUM_ASSERT(pRenderContent != nullptr);

        this->notifyTileDoneLoading(pTile.get());

        if (std::holds_alternative<TileUnknownContent>(
                pair.result.contentKind)) {
          // GltfModifier did not actually modify the model.
          pRenderContent->resetModifiedModelAndRenderResources();
          pRenderContent->setGltfModifierState(GltfModifierState::Idle);

          // Even though the GltfModifier chose not to modify anything, the
          // model is now up-to-date with the version that triggered this run.
          GltfModifierVersionExtension::setVersion(
              pRenderContent->getModel(),
              version);
        } else if (pTile->getState() == TileLoadState::ContentLoaded) {
          // This Tile is in the ContentLoaded state, which means that it was
          // (partially) loaded before the GltfModifier triggered to the current
          // version. In that case, we need to free the previous renderer
          // resources but can then send the tile through the rest of the normal
          // pipeline. We know a Tile in the ContentLoaded state isn't already
          // being rendered.
          if (this->_externals.pPrepareRendererResources) {
            this->_externals.pPrepareRendererResources->free(
                *pTile,
                pRenderContent->getRenderResources(),
                nullptr);
          }
          pRenderContent->setModel(
              std::move(std::get<CesiumGltf::Model>(pair.result.contentKind)));
          pRenderContent->setRenderResources(pair.pRenderResources);
          pRenderContent->setGltfModifierState(GltfModifierState::Idle);

          // The model is now up-to-date with the version that triggered this
          // run.
          GltfModifierVersionExtension::setVersion(
              pRenderContent->getModel(),
              version);
        } else {
          // This is a reapply of the GltfModifier to an already loaded and
          // potentially rendered tile.
          CESIUM_ASSERT(pTile->getState() == TileLoadState::Done);
          pRenderContent->setGltfModifierState(GltfModifierState::WorkerDone);

          // The modified model is up-to-date with the version that triggered
          // this run.
          CesiumGltf::Model& modifiedModel =
              std::get<CesiumGltf::Model>(pair.result.contentKind);
          GltfModifierVersionExtension::setVersion(modifiedModel, version);

          pRenderContent->setModifiedModelAndRenderResources(
              std::move(modifiedModel),
              pair.pRenderResources);
          this->_externals.pGltfModifier->onWorkerThreadApplyComplete(*pTile);
        }
      })
      .catchInMainThread(
          [this, pTile = Tile::Pointer(&tile), &externals, version](
              std::exception&& e) {
            pTile->getContent().getRenderContent()->setGltfModifierState(
                GltfModifierState::WorkerDone);
            this->notifyTileDoneLoading(pTile.get());
            SPDLOG_LOGGER_ERROR(
                externals.pLogger,
                "An unexpected error occurred when reapplying GltfModifier: {}",
                e.what());

            // Even though the GltfModifier failed, we mark the model up-to-date
            // with the version that triggered this run so that we won't try to
            // run again for this version.
            TileRenderContent* pRenderContent =
                pTile->getContent().getRenderContent();
            CESIUM_ASSERT(pRenderContent != nullptr);
            GltfModifierVersionExtension::setVersion(
                pRenderContent->getModel(),
                version);
          });
}

void TilesetContentManager::loadTileContent(
    Tile& tile,
    const TilesetOptions& tilesetOptions) {
  CESIUM_ASSERT(this->_pLoader != nullptr);
  CESIUM_ASSERT(this->_pRootTile != nullptr);

  CESIUM_TRACE("TilesetContentManager::loadTileContent");

  if (this->_externals.pGltfModifier &&
      this->_externals.pGltfModifier->needsWorkerThreadModification(tile)) {
    TileRenderContent* pRenderContent = tile.getContent().getRenderContent();
    CESIUM_ASSERT(pRenderContent != nullptr);
    this->reapplyGltfModifier(tile, tilesetOptions, pRenderContent);
    return;
  }

  if (tile.getState() == TileLoadState::Unloading) {
    // We can't load a tile that is unloading; it has to finish unloading first.
    return;
  }

  if (tile.getState() != TileLoadState::Unloaded &&
      tile.getState() != TileLoadState::FailedTemporarily) {
    // No need to load geometry, but give previously-throttled
    // raster overlay tiles a chance to load.
    for (RasterMappedTo3DTile& rasterTile : tile.getMappedRasterTiles()) {
      rasterTile.loadThrottled();
    }

    return;
  }

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
      std::get_if<CesiumGeometry::UpsampledQuadtreeNode>(&tile.getTileID());
  if (pUpsampleID) {
    // We can't upsample this tile until its parent tile is done loading.
    Tile* pParentTile = tile.getParent();
    if (pParentTile) {
      if (pParentTile->getState() != TileLoadState::Done) {
        loadTileContent(*pParentTile, tilesetOptions);

        // Finalize the parent if necessary, otherwise it may never reach the
        // Done state. Also double check that we have render content in ensure
        // we don't assert / crash in finishLoading. The latter will only ever
        // be a problem in a pathological tileset with a non-renderable leaf
        // tile, but that sort of thing does happen.
        if (pParentTile->getState() == TileLoadState::ContentLoaded &&
            pParentTile->isRenderContent()) {
          finishLoading(*pParentTile, tilesetOptions);
        }
        return;
      }
    } else {
      // we cannot upsample this tile if it doesn't have parent
      return;
    }
  }

  // Reference this Tile while its content is loading.
  Tile::Pointer pTile = &tile;

  // map raster overlay to tile
  std::vector<CesiumGeospatial::Projection> projections =
      this->_overlayCollection.addTileOverlays(tile, tilesetOptions);

  // begin loading tile
  notifyTileStartLoading(&tile);
  tile.setState(TileLoadState::ContentLoading);

  TileContentLoadInfo tileLoadInfo{
      this->_externals.asyncSystem,
      this->_externals.pAssetAccessor,
      this->_externals.pPrepareRendererResources,
      this->_externals.pLogger,
      this->_pSharedAssetSystem,
      tilesetOptions.contentOptions,
      tile};

  TilesetContentLoader* pLoader;
  if (tile.getLoader() == &this->_upsampler) {
    pLoader = &this->_upsampler;
  } else {
    pLoader = this->_pLoader.get();
  }

  TileLoadInput loadInput{
      tile,
      tilesetOptions.contentOptions,
      this->_externals.asyncSystem,
      this->_externals.pAssetAccessor,
      this->_externals.pLogger,
      this->_requestHeaders,
      tilesetOptions.ellipsoid};
  loadInput.pSharedAssetSystem = tileLoadInfo.pSharedAssetSystem;

  // Keep the manager alive while the load is in progress.
  CesiumUtility::IntrusivePointer<TilesetContentManager> thiz = this;

  pLoader->loadTileContent(loadInput)
      .thenImmediately([asyncSystem = this->_externals.asyncSystem,
                        pAssetAccessor = this->_externals.pAssetAccessor,
                        pLogger = this->_externals.pLogger,
                        tileLoadInfo = std::move(tileLoadInfo),
                        projections = std::move(projections),
                        rendererOptions = tilesetOptions.rendererOptions,
                        pGltfModifier = _externals.pGltfModifier](
                           TileLoadResult&& result) mutable {
        // the reason we run immediate continuation, instead of in the
        // worker thread, is that the loader may run the task in the main
        // thread. And most often than not, those main thread task is very
        // light weight. So when those tasks return, there is no need to
        // spawn another worker thread if the result of the task isn't
        // related to render content. We only ever spawn a new task in the
        // worker thread if the content is a render content
        if (result.state == TileLoadResultState::Success) {
          auto asyncSystem = tileLoadInfo.asyncSystem;
          if (std::holds_alternative<CesiumGltf::Model>(result.contentKind)) {
            return asyncSystem.runInWorkerThread(
                [result = std::move(result),
                 projections = std::move(projections),
                 tileLoadInfo = std::move(tileLoadInfo),
                 rendererOptions,
                 pGltfModifier]() mutable {
                  return postProcessContentInWorkerThread(
                      std::move(result),
                      std::move(projections),
                      std::move(tileLoadInfo),
                      rendererOptions,
                      pGltfModifier);
                });
          } else if (
              std::holds_alternative<CesiumVectorData::GeoJsonDocument>(
                  result.contentKind) &&
              tileLoadInfo.pPrepareRendererResources) {
            return asyncSystem.runInWorkerThread(
                [result = std::move(result),
                 tileLoadInfo = std::move(tileLoadInfo),
                 rendererOptions]() mutable {
                  return tileLoadInfo.pPrepareRendererResources
                      ->prepareInLoadThread(
                          tileLoadInfo.asyncSystem,
                          std::move(result),
                          tileLoadInfo.tileTransform,
                          rendererOptions);
                });
          }
        }

        return tileLoadInfo.asyncSystem
            .createResolvedFuture<TileLoadResultAndRenderResources>(
                {std::move(result), nullptr});
      })
      .thenInMainThread([pTile, thiz](TileLoadResultAndRenderResources&& pair) {
        setTileContent(*pTile, std::move(pair.result), pair.pRenderResources);
        thiz->notifyTileDoneLoading(pTile.get());

        if (thiz->_externals.pGltfModifier) {
          const TileRenderContent* pRenderContent =
              pTile->getContent().getRenderContent();
          CESIUM_ASSERT(!pRenderContent || !pRenderContent->getModifiedModel());
          if (pRenderContent &&
              GltfModifierVersionExtension::getVersion(
                  pRenderContent->getModel()) !=
                  thiz->_externals.pGltfModifier->getCurrentVersion()) {
            thiz->_externals.pGltfModifier->onOldVersionContentLoadingComplete(
                *pTile);
          }
        }
      })
      .catchInMainThread([pLogger = this->_externals.pLogger, pTile, thiz](
                             std::exception&& e) {
        pTile->getMappedRasterTiles().clear();
        pTile->setState(TileLoadState::Failed);
        thiz->notifyTileDoneLoading(pTile.get());
        SPDLOG_LOGGER_ERROR(
            pLogger,
            "An unexpected error occurred when loading tile: {}",
            e.what());
      });
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

  this->createLatentChildrenIfNecessary(tile, tilesetOptions);
}

void TilesetContentManager::createLatentChildrenIfNecessary(
    Tile& tile,
    const TilesetOptions& tilesetOptions) {
  if (!tile.getMightHaveLatentChildren())
    return;

  // If this tile has no children yet, attempt to create them.
  if (tile.getChildren().empty()) {
    TileChildrenResult childrenResult =
        this->_pLoader->createTileChildren(tile, tilesetOptions.ellipsoid);
    if (childrenResult.state == TileLoadResultState::Success) {
      tile.createChildTiles(std::move(childrenResult.children));
    }

    bool mightStillHaveLatentChildren =
        childrenResult.state == TileLoadResultState::RetryLater;
    tile.setMightHaveLatentChildren(mightStillHaveLatentChildren);
  } else {
    // A tile with real children can't have latent children.
    tile.setMightHaveLatentChildren(false);
  }
}

UnloadTileContentResult TilesetContentManager::unloadTileContent(Tile& tile) {
  // Don't unload tiles that can't be reloaded.
  if (!TileIdUtilities::isLoadable(tile.getTileID())) {
    return UnloadTileContentResult::Keep;
  }

  TileLoadState state = tile.getState();
  // Test if a glTF modifier is in progress.
  if (this->_externals.pGltfModifier) {
    TileRenderContent* pRenderContent = tile.getContent().getRenderContent();
    if (pRenderContent) {
      switch (pRenderContent->getGltfModifierState()) {
      case GltfModifierState::WorkerRunning:
        // Worker thread is running, we cannot unload yet.
        return UnloadTileContentResult::Keep;
      case GltfModifierState::WorkerDone:
        // Free temporary render resources.
        CESIUM_ASSERT(pRenderContent->getModifiedRenderResources());
        if (this->_externals.pPrepareRendererResources) {
          this->_externals.pPrepareRendererResources->free(
              tile,
              pRenderContent->getModifiedRenderResources(),
              nullptr);
        }
        pRenderContent->resetModifiedModelAndRenderResources();
        pRenderContent->setGltfModifierState(GltfModifierState::Idle);
        break;
      default:
        break;
      }
    }
  }
  if (state == TileLoadState::Unloaded) {
    return UnloadTileContentResult::Remove;
  }

  if (state == TileLoadState::ContentLoading) {
    return UnloadTileContentResult::Keep;
  }

  TileContent& content = tile.getContent();

  // We can unload empty content at any time.
  if (content.isEmptyContent()) {
    notifyTileUnloading(&tile);
    content.setContentKind(TileUnknownContent{});
    tile.setState(TileLoadState::Unloaded);
    tile.releaseReference("unloadTileContent: Empty");
    return UnloadTileContentResult::Remove;
  }

  // Detach raster tiles first so that the renderer's tile free
  // process doesn't need to worry about them.
  for (RasterMappedTo3DTile& mapped : tile.getMappedRasterTiles()) {
    mapped.detachFromTile(*this->_externals.pPrepareRendererResources, tile);
  }
  tile.getMappedRasterTiles().clear();

  if (content.isExternalContent()) {
    // We can unload an external content tile with one reference, because this
    // represents the external content itself. Any more than that indicates
    // references to the tile or a descendant tile (or their content), all of
    // which prevent this external content from being unloaded.
    if (tile.getReferenceCount() > 1) {
      return UnloadTileContentResult::Keep;
    }

    notifyTileUnloading(&tile);
    content.setContentKind(TileUnknownContent{});
    tile.setState(TileLoadState::Unloaded);
    tile.releaseReference("UnloadTileContent: External");
    return UnloadTileContentResult::RemoveAndClearChildren;
  }

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
      return UnloadTileContentResult::Keep;
    }
  }

  // If we make it this far, the tile's content will be fully unloaded.
  notifyTileUnloading(&tile);
  tile.setState(TileLoadState::Unloaded);
  if (!content.isUnknownContent()) {
    content.setContentKind(TileUnknownContent{});
    tile.releaseReference("UnloadTileContent: Other");
  }
  return UnloadTileContentResult::Remove;
}

void TilesetContentManager::unloadAll() {
  // TODO: use the linked-list of loaded tiles instead of walking the entire
  // tile tree.
  if (this->_pRootTile) {
    unloadTileRecursively(*this->_pRootTile, *this);
  }
}

bool TilesetContentManager::waitUntilIdle(
    double maximumWaitTimeInMilliseconds) {
  auto start = std::chrono::system_clock::now();
  auto end = (maximumWaitTimeInMilliseconds <= 0.0)
                 ? std::chrono::time_point<std::chrono::system_clock>::max()
                 : (start + std::chrono::microseconds(static_cast<int64_t>(
                                1000.0 * maximumWaitTimeInMilliseconds)));

  // Wait for all asynchronous loading to terminate.
  // If you're hanging here, it's most likely caused by _tileLoadsInProgress not
  // being decremented correctly when an async load ends.
  while (this->_tileLoadsInProgress > 0) {
    this->_externals.pAssetAccessor->tick();
    this->_externals.asyncSystem.dispatchMainThreadTasks();

    auto time = std::chrono::system_clock::now();
    if (time >= end) {
      break;
    }
  }

  // Wait for all overlays to wrap up their loading, too.
  uint32_t rasterOverlayTilesLoading = 1;
  while (rasterOverlayTilesLoading > 0) {
    this->_externals.pAssetAccessor->tick();
    this->_externals.asyncSystem.dispatchMainThreadTasks();

    rasterOverlayTilesLoading = 0;
    for (const auto& pActivated :
         this->_overlayCollection.getActivatedOverlays()) {
      rasterOverlayTilesLoading += pActivated->getNumberOfTilesLoading();
    }

    auto time = std::chrono::system_clock::now();
    if (time >= end) {
      break;
    }
  }

  return this->_tileLoadsInProgress == 0 && rasterOverlayTilesLoading == 0;
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

const CesiumUtility::IntrusivePointer<TilesetSharedAssetSystem>&
TilesetContentManager::getSharedAssetSystem() const noexcept {
  return this->_pSharedAssetSystem;
}

int32_t TilesetContentManager::getNumberOfTilesLoading() const noexcept {
  return this->_tileLoadsInProgress;
}

int32_t TilesetContentManager::getNumberOfTilesLoaded() const noexcept {
  return this->_loadedTilesCount;
}

int64_t TilesetContentManager::getTotalDataUsed() const noexcept {
  int64_t bytes = this->_tilesDataUsed;
  for (const auto& pActivated :
       this->_overlayCollection.getActivatedOverlays()) {
    bytes += pActivated->getTileDataBytes();
  }

  return bytes;
}

void TilesetContentManager::finishLoading(
    Tile& tile,
    const TilesetOptions& tilesetOptions) {
  TileContent& content = tile.getContent();

  // If this tile doesn't have render content, there's nothing to finish
  // loading.
  TileRenderContent* pRenderContent = content.getRenderContent();
  if (pRenderContent == nullptr)
    return;

  if (this->_externals.pGltfModifier &&
      this->_externals.pGltfModifier->needsMainThreadModification(tile)) {
    // Free outdated render resources before replacing them.
    if (this->_externals.pPrepareRendererResources) {
      this->_externals.pPrepareRendererResources->free(
          tile,
          nullptr,
          pRenderContent->getRenderResources());
    }

    // Replace model and render resources with the newly modified versions,
    // discarding the old ones
    pRenderContent->replaceWithModifiedModel();

    // Run the main thread part of loading.
    if (this->_externals.pPrepareRendererResources) {
      pRenderContent->setRenderResources(
          this->_externals.pPrepareRendererResources->prepareInMainThread(
              tile,
              pRenderContent->getRenderResources()));
    } else {
      pRenderContent->setRenderResources(nullptr);
    }

    pRenderContent->setGltfModifierState(GltfModifierState::Idle);
    return;
  }

  // None of the processing below makes sense until the Tile hits the
  // ContentLoaded state.
  if (tile.getState() != TileLoadState::ContentLoaded)
    return;

  // add copyright
  CreditSystem* pCreditSystem = this->_externals.pCreditSystem.get();
  if (pCreditSystem) {
    std::vector<std::string_view> creditStrings =
        GltfUtilities::parseGltfCopyright(pRenderContent->getModel());

    std::vector<Credit> credits;
    credits.reserve(creditStrings.size());

    for (const std::string_view& creditString : creditStrings) {
      credits.emplace_back(pCreditSystem->createCredit(
          this->_creditSource,
          std::string(creditString),
          tilesetOptions.showCreditsOnScreen));
    }

    pRenderContent->setCredits(credits);
  }

  // Run the main thread part of loading.
  void* pWorkerRenderResources = pRenderContent->getRenderResources();
  if (this->_externals.pPrepareRendererResources) {
    void* pMainThreadRenderResources =
        this->_externals.pPrepareRendererResources->prepareInMainThread(
            tile,
            pWorkerRenderResources);
    pRenderContent->setRenderResources(pMainThreadRenderResources);
  }

  tile.setState(TileLoadState::Done);

  // This allows the raster tile to be updated and children to be created, if
  // necessary.
  updateTileContent(tile, tilesetOptions);
}

void TilesetContentManager::markTileIneligibleForContentUnloading(Tile& tile) {
  this->_tilesEligibleForContentUnloading.remove(tile);
}

void TilesetContentManager::markTileEligibleForContentUnloading(Tile& tile) {
  // If the tile is not yet in the list, add it to the end (most recently used).
  if (!this->_tilesEligibleForContentUnloading.contains(tile)) {
    this->_tilesEligibleForContentUnloading.insertAtTail(tile);
  }

  // If the Tileset has already been destroyed, unload this unused Tile
  // immediately to allow the TilesetContentManager destruction process to
  // proceed.
  if (this->_tilesetDestroyed) {
    this->unloadTileContent(tile);
  }
}

void TilesetContentManager::unloadCachedBytes(
    int64_t maximumCachedBytes,
    double timeBudgetMilliseconds) {
  Tile* pTile = this->_tilesEligibleForContentUnloading.head();

  // A time budget of 0.0 indicates we shouldn't throttle cache unloads. So set
  // the end time to the max time_point in that case.
  auto start = std::chrono::system_clock::now();
  auto end = (timeBudgetMilliseconds <= 0.0)
                 ? std::chrono::time_point<std::chrono::system_clock>::max()
                 : (start + std::chrono::microseconds(static_cast<int64_t>(
                                1000.0 * timeBudgetMilliseconds)));

  std::vector<Tile*> tilesNeedingChildrenCleared;

  while (this->getTotalDataUsed() > maximumCachedBytes) {
    if (pTile == nullptr) {
      // We've removed all unused tiles.
      break;
    }

    Tile* pNext = this->_tilesEligibleForContentUnloading.next(*pTile);

    const UnloadTileContentResult removed = this->unloadTileContent(*pTile);
    if (removed != UnloadTileContentResult::Keep) {
      this->_tilesEligibleForContentUnloading.remove(*pTile);
    }

    if (removed == UnloadTileContentResult::RemoveAndClearChildren) {
      tilesNeedingChildrenCleared.emplace_back(pTile);
    }

    pTile = pNext;

    auto time = std::chrono::system_clock::now();
    if (time >= end) {
      break;
    }
  }

  if (!tilesNeedingChildrenCleared.empty()) {
    for (Tile* pTileToClear : tilesNeedingChildrenCleared) {
      CESIUM_ASSERT(pTileToClear->getReferenceCount() == 0);
      this->clearChildrenRecursively(pTileToClear);
    }
  }
}

void TilesetContentManager::clearChildrenRecursively(Tile* pTile) noexcept {
  // Iterate through all children, calling this method recursively to make sure
  // children are all removed from _tilesEligibleForContentUnloading.
  for (Tile& child : pTile->getChildren()) {
    CESIUM_ASSERT(
        !TileIdUtilities::isLoadable(child.getTileID()) ||
        child.getState() == TileLoadState::Unloaded);
    CESIUM_ASSERT(child.getReferenceCount() == 0);
    CESIUM_ASSERT(!child.hasReferencingContent());
    this->_tilesEligibleForContentUnloading.remove(child);
    this->clearChildrenRecursively(&child);
    child.setParent(nullptr);
  }

  pTile->clearChildren();
}

void TilesetContentManager::registerTileRequester(
    TileLoadRequester& requester) {
  if (requester._pTilesetContentManager.get() == this) {
    return;
  }

  if (requester._pTilesetContentManager != nullptr) {
    requester._pTilesetContentManager->unregisterTileRequester(requester);
  }

  requester._pTilesetContentManager = this;

  CESIUM_ASSERT(
      std::find(
          this->_requesters.begin(),
          this->_requesters.end(),
          &requester) == this->_requesters.end());
  this->_requesters.emplace_back(&requester);
}

void TilesetContentManager::unregisterTileRequester(
    TileLoadRequester& requester) {
  auto it =
      std::find(this->_requesters.begin(), this->_requesters.end(), &requester);
  CESIUM_ASSERT(it != this->_requesters.end());
  if (it != this->_requesters.end()) {
    this->_requesters.erase(it);
  }

  requester._pTilesetContentManager = nullptr;
}

namespace {

/**
 * @brief A round-robin mechanism that selects the next requester to load tiles
 * from, based on the requesters' weights.
 */
class WeightedRoundRobin {
public:
  typedef bool (TileLoadRequester::*HasMoreTilesToLoad)() const;
  typedef const Tile* (TileLoadRequester::*GetNextTileToLoad)();

  WeightedRoundRobin(
      double& roundRobinValue,
      const std::vector<TileLoadRequester*>& allRequesters,
      std::vector<TileLoadRequester*>& requestersWithRequests,
      std::vector<double>& fractions,
      HasMoreTilesToLoad hasMoreTilesToLoad,
      GetNextTileToLoad getNextTileToLoad)
      : _roundRobinValue(roundRobinValue),
        _allRequesters(allRequesters),
        _requestersWithRequests(requestersWithRequests),
        _fractions(fractions),
        _hasMoreTilesToLoad(hasMoreTilesToLoad),
        _getNextTileToLoad(getNextTileToLoad) {
    this->recomputeRequesterFractions();
  }

  Tile* getNextTileToLoad() {
    if (this->_requestersWithRequests.empty())
      return nullptr;

    // Use the golden ratio to move to the next requester so that we give each a
    // chance to load tiles in proportion to its weight.
    // Inspired by:
    // https://blog.demofox.org/2020/06/23/weighted-round-robin-using-the-golden-ratio-low-discrepancy-sequence/
    this->_roundRobinValue =
        glm::fract(this->_roundRobinValue + Math::GoldenRatio);
    auto it = std::lower_bound(
        this->_fractions.begin(),
        this->_fractions.end(),
        this->_roundRobinValue);

    size_t index = size_t(it - this->_fractions.begin());
    CESIUM_ASSERT(index < this->_requestersWithRequests.size());
    if (index >= this->_requestersWithRequests.size())
      return nullptr;

    TileLoadRequester& requester = *this->_requestersWithRequests[index];

    const Tile* pToLoad = std::invoke(this->_getNextTileToLoad, requester);
    CESIUM_ASSERT(pToLoad);

    if (!pToLoad || !std::invoke(this->_hasMoreTilesToLoad, requester)) {
      // If this was the last tile from this requester, we'll need to remove the
      // requester from the weighting.
      this->recomputeRequesterFractions();
    }

    // The Tile is const from the perspective of the TileLoadRequester. But the
    // TilesetContentManager is going to load it, so cast away the const.
    return const_cast<Tile*>(pToLoad);
  }

private:
  /**
   * @brief Recompute the fractions for each requester to affect the frequency
   * that they are picked by the round robin. Each fraction is the requester's
   * own value plus the sum of the fractions that came before it, resulting in a
   * sorted array where values ascend from 0.0 to 1.0.
   */
  void recomputeRequesterFractions() {
    this->_fractions.reserve(this->_allRequesters.size());
    this->_requestersWithRequests.reserve(this->_allRequesters.size());

    this->_fractions.clear();
    this->_requestersWithRequests.clear();

    double sum = 0.0;

    for (size_t i = 0; i < this->_allRequesters.size(); ++i) {
      TileLoadRequester* pRequester = this->_allRequesters[i];
      CESIUM_ASSERT(pRequester);

      if (pRequester && std::invoke(this->_hasMoreTilesToLoad, *pRequester)) {
        double weight = pRequester->getWeight();
        CESIUM_ASSERT(weight > 0.0);

        sum += weight;
        this->_fractions.emplace_back(sum);
        this->_requestersWithRequests.emplace_back(pRequester);
      }
    }

    // Normalize to [0.0, 1.0].
    for (size_t i = 0; i < this->_fractions.size(); ++i) {
      this->_fractions[i] /= sum;
    }

    // The last fraction should always be exactly 1.0.
    if (!this->_fractions.empty()) {
      this->_fractions.back() = 1.0;
    }
  }

  double& _roundRobinValue;
  const std::vector<TileLoadRequester*>& _allRequesters;
  std::vector<TileLoadRequester*>& _requestersWithRequests;
  std::vector<double>& _fractions;
  HasMoreTilesToLoad _hasMoreTilesToLoad;
  GetNextTileToLoad _getNextTileToLoad;
};

} // namespace

void TilesetContentManager::processWorkerThreadLoadRequests(
    const TilesetOptions& options) {
  // Exit early if there are no loading slots available.
  if (this->getNumberOfTilesLoading() >=
      int32_t(options.maximumSimultaneousTileLoads)) {
    return;
  }

  WeightedRoundRobin wrr{
      this->_roundRobinValueWorker,
      this->_requesters,
      this->_requestersWithRequests,
      this->_requesterFractions,
      &TileLoadRequester::hasMoreTilesToLoadInWorkerThread,
      &TileLoadRequester::getNextTileToLoadInWorkerThread};

  while (this->getNumberOfTilesLoading() <
         int32_t(options.maximumSimultaneousTileLoads)) {
    Tile* pToLoad = wrr.getNextTileToLoad();
    if (pToLoad == nullptr)
      break;

    // It doesn't make sense to load tiles that are not referenced, because such
    // tiles are immediately eligible for unloading.
    CESIUM_ASSERT(pToLoad->_referenceCount > 0);
    if (pToLoad->_referenceCount == 0)
      continue;

    this->loadTileContent(*pToLoad, options);
  }
}

void TilesetContentManager::processMainThreadLoadRequests(
    const TilesetOptions& options) {
  WeightedRoundRobin wrr{
      this->_roundRobinValueMain,
      this->_requesters,
      this->_requestersWithRequests,
      this->_requesterFractions,
      &TileLoadRequester::hasMoreTilesToLoadInMainThread,
      &TileLoadRequester::getNextTileToLoadInMainThread};

  double timeBudget = options.mainThreadLoadingTimeLimit;

  auto start = std::chrono::system_clock::now();
  auto end = start + std::chrono::microseconds(
                         static_cast<int64_t>(1000.0 * timeBudget));

  while (true) {
    Tile* pToLoad = wrr.getNextTileToLoad();
    if (pToLoad == nullptr)
      break;

    // It doesn't make sense to load tiles that are not referenced, because such
    // tiles are immediately eligible for unloading.
    CESIUM_ASSERT(pToLoad->_referenceCount > 0);
    if (pToLoad->_referenceCount == 0)
      continue;

    this->finishLoading(*pToLoad, options);

    auto time = std::chrono::system_clock::now();
    if (timeBudget > 0.0 && time >= end) {
      break;
    }
  }
}

void TilesetContentManager::markTilesetDestroyed() noexcept {
  this->_tilesetDestroyed = true;

  // Unregister the GltfModifier. This will not destroy it, but it will ensure
  // it releases any references it holds to this TilesetContentManager so that
  // the TilesetContentManager can be destroyed.
  if (this->_externals.pGltfModifier) {
    this->_externals.pGltfModifier->onUnregister(*this);
  }
}

void TilesetContentManager::releaseReference() const {
  bool willStillBeAliveAfter = this->getReferenceCount() > 1;

  ReferenceCountedNonThreadSafe<TilesetContentManager>::releaseReference();

  // If the Tileset is already destroyed, but the TilesetContentManager isn't
  // yet, try again to unload all the tiles.
  if (willStillBeAliveAfter && this->_tilesetDestroyed) {
    // We can justify this const_cast on the basis that only unreferenced tiles
    // will be unloaded, so clients will not be able to perceive that
    // modification. And the `Tileset` is already destroyed, anyway.
    const_cast<TilesetContentManager*>(this)->unloadAll();
  }
}

TilesetExternals& TilesetContentManager::getExternals() {
  return this->_externals;
}
const TilesetExternals& TilesetContentManager::getExternals() const {
  return this->_externals;
}

const CreditSource& TilesetContentManager::getCreditSource() const noexcept {
  return this->_creditSource;
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
      tile.setContentBoundingVolume(result.updatedContentBoundingVolume);
    }

    auto& content = tile.getContent();
    CESIUM_ASSERT(content.isUnknownContent());

    std::visit(
        ContentKindSetter{
            content,
            std::move(result.rasterOverlayDetails),
            pWorkerRenderResources},
        std::move(result.contentKind));

    // "Unknown" content is not loaded, and does not get a reference.
    // Also, if a tile ID is not loadable, it can never be unloaded (because how
    // would we reload it?) and so that doesn't get a reference, either.
    // An example of non-loadable content is the "external content" at the root
    // of a regular tileset.json tileset. We can't unload it without destroying
    // the entire tileset.
    if (tile.hasReferencingContent()) {
      tile.addReference("setTileContent");
    }

    // Only render content should have raster overlays.
    if (!tile.getContent().isRenderContent()) {
      tile.getMappedRasterTiles().clear();
    }

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
  if (tile.getMightHaveLatentChildren()) {
    // This tile might have latent children, but we don't know yet whether it
    // *actually* has children. We need to know that before we can continue
    // this function, which will decide whether or not to create upsampled
    // children for this tile. It only makes sense to create upsampled children
    // for a tile that we know for sure doesn't have real children.
    return;
  }

  const CesiumGeospatial::Ellipsoid& ellipsoid = tilesetOptions.ellipsoid;

  // update raster overlay
  TileContent& content = tile.getContent();
  const TileRenderContent* pRenderContent = content.getRenderContent();
  if (pRenderContent) {
    TileRasterOverlayStatus status =
        this->_overlayCollection.updateTileOverlays(tile, tilesetOptions);

    if (status.firstIndexWithMissingProjection) {
      // The mesh doesn't have the right texture coordinates for this
      // overlay's projection, so we need to kick it back to the unloaded
      // state to fix that.
      // In the future, we could add the ability to add the required
      // texture coordinates without starting over from scratch.
      unloadTileContent(tile);
      return;
    }

    bool hasMoreDetailAvailable =
        status.firstIndexWithMoreDetailAvailable.has_value();
    bool hasUnknown = status.firstIndexWithUnknownAvailability.has_value();

    bool doSubdivide =
        hasMoreDetailAvailable &&
        (!hasUnknown || *status.firstIndexWithUnknownAvailability >
                            *status.firstIndexWithMoreDetailAvailable);

    // If this tile still has no children after it's done loading, but it does
    // have raster tiles that are not the most detailed available, create fake
    // children to hang more detailed rasters on by subdividing this tile.
    if (doSubdivide && tile.getChildren().empty()) {
      createQuadtreeSubdividedChildren(ellipsoid, tile, this->_upsampler);
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
  CESIUM_ASSERT(
      pRenderContent && "Tile must have render content to be unloaded");

  void* pWorkerRenderResources = pRenderContent->getRenderResources();
  if (this->_externals.pPrepareRendererResources) {
    this->_externals.pPrepareRendererResources->free(
        tile,
        pWorkerRenderResources,
        nullptr);
  }
  pRenderContent->setRenderResources(nullptr);
}

void TilesetContentManager::unloadDoneState(Tile& tile) {
  TileContent& content = tile.getContent();
  TileRenderContent* pRenderContent = content.getRenderContent();
  CESIUM_ASSERT(
      pRenderContent && "Tile must have render content to be unloaded");

  if (this->_externals.pPrepareRendererResources) {
    void* pMainThreadRenderResources = pRenderContent->getRenderResources();
    this->_externals.pPrepareRendererResources->free(
        tile,
        nullptr,
        pMainThreadRenderResources);
    pRenderContent->setRenderResources(nullptr);
  }
}

void TilesetContentManager::notifyTileStartLoading(
    [[maybe_unused]] const Tile* pTile) noexcept {
  ++this->_tileLoadsInProgress;
}

void TilesetContentManager::notifyTileDoneLoading(const Tile* pTile) noexcept {
  CESIUM_ASSERT(
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
    return;
  }

  this->_tilesetCredits.reserve(
      this->_tilesetCredits.size() + result.credits.size());
  for (const auto& creditResult : result.credits) {
    this->_tilesetCredits.emplace_back(
        this->_externals.pCreditSystem->createCredit(
            this->_creditSource,
            creditResult.creditText,
            creditResult.showOnScreen));
  }

  this->_requestHeaders = std::move(result.requestHeaders);
  this->_pLoader = std::move(result.pLoader);
  this->_pRootTile = std::move(result.pRootTile);

  this->_overlayCollection.setLoadedTileEnumerator(
      LoadedTileEnumerator(this->_pRootTile.get()));
  this->_pLoader->setOwner(*this);
}

CesiumAsync::Future<void>
TilesetContentManager::registerGltfModifier(const Tile* pRootTile) {
  std::shared_ptr<GltfModifier> pGltfModifier =
      this->getExternals().pGltfModifier;

  if (pGltfModifier && pRootTile) {
    const TileExternalContent* pExternal =
        pRootTile->getContent().getExternalContent();
    return pGltfModifier
        ->onRegister(
            *this,
            pExternal ? pExternal->metadata : TilesetMetadata(),
            *pRootTile)
        .catchInMainThread([this](std::exception&&) {
          // Disable the failed glTF modifier.
          this->getExternals().pGltfModifier.reset();
        });
  }

  return this->getExternals().asyncSystem.createResolvedFuture();
}

} // namespace Cesium3DTilesSelection
