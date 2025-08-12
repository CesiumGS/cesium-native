#include "TilesetContentManager.h"

#include "LayerJsonTerrainLoader.h"
#include "RasterOverlayUpsampler.h"
#include "TileContentLoadInfo.h"
#include "TilesetJsonLoader.h"

#include <Cesium3DTilesSelection/BoundingVolume.h>
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
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/RasterOverlayUtilities.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Math.h>
#include <CesiumUtility/ReferenceCounted.h>
#include <CesiumUtility/Tracing.h>
#include <CesiumUtility/joinToString.h>

#include <fmt/format.h>
#include <glm/common.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/gtc/matrix_access.hpp>
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

std::vector<CesiumGeospatial::Projection> mapOverlaysToTile(
    Tile& tile,
    RasterOverlayCollection& overlays,
    const TilesetOptions& tilesetOptions) {
  // when tile fails temporarily, it may still have mapped raster tiles, so
  // clear it here
  tile.getMappedRasterTiles().clear();

  std::vector<CesiumGeospatial::Projection> projections;
  const std::vector<CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>>&
      tileProviders = overlays.getTileProviders();
  const std::vector<CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>>&
      placeholders = overlays.getPlaceholderTileProviders();
  CESIUM_ASSERT(tileProviders.size() == placeholders.size());

  const CesiumGeospatial::Ellipsoid& ellipsoid = tilesetOptions.ellipsoid;

  for (size_t i = 0; i < tileProviders.size() && i < placeholders.size(); ++i) {
    RasterOverlayTileProvider& tileProvider = *tileProviders[i];
    RasterOverlayTileProvider& placeholder = *placeholders[i];

    RasterMappedTo3DTile* pMapped = RasterMappedTo3DTile::mapOverlayToTile(
        tilesetOptions.maximumScreenSpaceError,
        tileProvider,
        placeholder,
        tile,
        projections,
        ellipsoid);
    if (pMapped) {
      // Try to load now, but if the mapped raster tile is a placeholder this
      // won't do anything.
      pMapped->loadThrottled();
    }
  }

  return projections;
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
    const std::any& rendererOptions) {
  CESIUM_ASSERT(
      result.state == TileLoadResultState::Success &&
      "This function requires result to be success");

  CesiumGltf::Model& model = std::get<CesiumGltf::Model>(result.contentKind);

  // Download any external image or buffer urls in the gltf if there are any
  CesiumGltfReader::GltfReaderResult gltfResult{std::move(model), {}, {}};

  CesiumAsync::HttpHeaders requestHeaders;
  std::string baseUrl;
  if (result.pCompletedRequest) {
    requestHeaders = result.pCompletedRequest->headers();
    baseUrl = result.pCompletedRequest->url();
  }

  CesiumGltfReader::GltfReaderOptions gltfOptions;
  gltfOptions.ktx2TranscodeTargets =
      tileLoadInfo.contentOptions.ktx2TranscodeTargets;
  gltfOptions.applyTextureTransform =
      tileLoadInfo.contentOptions.applyTextureTransform;
  if (tileLoadInfo.pSharedAssetSystem) {
    gltfOptions.pSharedAssetSystem = tileLoadInfo.pSharedAssetSystem;
  }

  auto asyncSystem = tileLoadInfo.asyncSystem;
  auto pAssetAccessor = result.pAssetAccessor;
  return CesiumGltfReader::GltfReader::resolveExternalData(
             asyncSystem,
             baseUrl,
             requestHeaders,
             pAssetAccessor,
             gltfOptions,
             std::move(gltfResult))
      .thenInWorkerThread([result = std::move(result),
                           projections = std::move(projections),
                           tileLoadInfo = std::move(tileLoadInfo),
                           rendererOptions](CesiumGltfReader::GltfReaderResult&&
                                                gltfResult) mutable {
        if (!gltfResult.errors.empty()) {
          if (result.pCompletedRequest) {
            SPDLOG_LOGGER_ERROR(
                tileLoadInfo.pLogger,
                "Failed resolving external glTF buffers from {}:\n- {}",
                result.pCompletedRequest->url(),
                CesiumUtility::joinToString(gltfResult.errors, "\n- "));
          } else {
            SPDLOG_LOGGER_ERROR(
                tileLoadInfo.pLogger,
                "Failed resolving external glTF buffers:\n- {}",
                CesiumUtility::joinToString(gltfResult.errors, "\n- "));
          }
        }

        if (!gltfResult.warnings.empty()) {
          if (result.pCompletedRequest) {
            SPDLOG_LOGGER_WARN(
                tileLoadInfo.pLogger,
                "Warning when resolving external gltf buffers from "
                "{}:\n- {}",
                result.pCompletedRequest->url(),
                CesiumUtility::joinToString(gltfResult.warnings, "\n- "));
          } else {
            SPDLOG_LOGGER_ERROR(
                tileLoadInfo.pLogger,
                "Warning resolving external glTF buffers:\n- {}",
                CesiumUtility::joinToString(gltfResult.warnings, "\n- "));
          }
        }

        if (!gltfResult.model) {
          return tileLoadInfo.asyncSystem.createResolvedFuture(
              TileLoadResultAndRenderResources{
                  TileLoadResult::createFailedResult(
                      result.pAssetAccessor,
                      nullptr),
                  nullptr});
        }

        result.contentKind = std::move(*gltfResult.model);
        result.initialBoundingVolume = tileLoadInfo.tileBoundingVolume;
        result.initialContentBoundingVolume =
            tileLoadInfo.tileContentBoundingVolume;

        postProcessGltfInWorkerThread(
            result,
            std::move(projections),
            tileLoadInfo);

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

CesiumAsync::Future<void> registerGltfModifier(
    TilesetContentManager& contentManager,
    const Tile* pRootTile) {
  std::shared_ptr<GltfModifier> pGltfModifier =
      contentManager.getExternals().pGltfModifier;

  if (pGltfModifier && pRootTile) {
    const TileExternalContent* pExternal =
        pRootTile->getContent().getExternalContent();
    return pGltfModifier
        ->onRegister(
            contentManager.getExternals().asyncSystem,
            contentManager.getExternals().pAssetAccessor,
            contentManager.getExternals().pLogger,
            pExternal ? pExternal->metadata : TilesetMetadata())
        .catchInMainThread([&contentManager](std::exception&&) {
          // Disable the failed glTF modifier.
          contentManager.getExternals().pGltfModifier.reset();
        });
  }

  return contentManager.getExternals().asyncSystem.createResolvedFuture();
}

} // namespace

TilesetContentManager::TilesetContentManager(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    std::unique_ptr<TilesetContentLoader>&& pLoader,
    std::unique_ptr<Tile>&& pRootTile)
    : _externals{externals},
      _requestHeaders{tilesetOptions.requestHeaders},
      _pLoader{std::move(pLoader)},
      _pRootTile{nullptr},
      _userCredit(
          (tilesetOptions.credit && externals.pCreditSystem)
              ? std::optional<Credit>(externals.pCreditSystem->createCredit(
                    tilesetOptions.credit.value(),
                    tilesetOptions.showCreditsOnScreen))
              : std::nullopt),
      _tilesetCredits{},
      _overlayCollection(
          LoadedTileEnumerator(pRootTile.get()),
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
      _requestersWithRequests() {
  CESIUM_ASSERT(this->_pLoader != nullptr);
  this->_upsampler.setOwner(*this);

  this->notifyTileStartLoading(nullptr);

  registerGltfModifier(*this, pRootTile.get())
      .thenInMainThread([this, pRootTile = std::move(pRootTile)]() mutable {
        this->_pRootTile = std::move(pRootTile);
        this->_overlayCollection.setLoadedTileEnumerator(
            LoadedTileEnumerator(this->_pRootTile.get()));
        this->_pLoader->setOwner(*this);
        this->_rootTileAvailablePromise.resolve();
        this->notifyTileDoneLoading(this->_pRootTile.get());
      });
}

TilesetContentManager::TilesetContentManager(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    const std::string& url)
    : _externals{externals},
      _requestHeaders{tilesetOptions.requestHeaders},
      _pLoader{},
      _pRootTile{},
      _userCredit(
          (tilesetOptions.credit && externals.pCreditSystem)
              ? std::optional<Credit>(externals.pCreditSystem->createCredit(
                    tilesetOptions.credit.value(),
                    tilesetOptions.showCreditsOnScreen))
              : std::nullopt),
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
      _requestersWithRequests() {
  this->_upsampler.setOwner(*this);

  if (!url.empty()) {
    this->notifyTileStartLoading(nullptr);

    CesiumUtility::IntrusivePointer<TilesetContentManager> thiz = this;
    const CesiumGeospatial::Ellipsoid& ellipsoid = tilesetOptions.ellipsoid;

    externals.pAssetAccessor
        ->get(externals.asyncSystem, url, this->_requestHeaders)
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
            [thiz](TilesetContentLoaderResult<TilesetContentLoader>&& result) {
              return registerGltfModifier(*thiz, result.pRootTile.get())
                  .thenImmediately([result = std::move(result)]() mutable {
                    return std::move(result);
                  });
            })
        .thenInMainThread(
            [thiz,
             asyncSystem = externals.asyncSystem,
             errorCallback = tilesetOptions.loadErrorCallback,
             pGltfModifier = externals.pGltfModifier](
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
    TilesetContentLoaderFactory&& loaderFactory)
    : _externals{externals},
      _requestHeaders{tilesetOptions.requestHeaders},
      _pLoader{},
      _pRootTile{},
      _userCredit(
          (tilesetOptions.credit && externals.pCreditSystem)
              ? std::optional<Credit>(externals.pCreditSystem->createCredit(
                    tilesetOptions.credit.value(),
                    tilesetOptions.showCreditsOnScreen))
              : std::nullopt),
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
      _requestersWithRequests() {
  this->_upsampler.setOwner(*this);

  if (loaderFactory.isValid()) {
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

    loaderFactory
        .createLoader(externals, tilesetOptions, authorizationChangeListener)
        .thenInMainThread(
            [thiz](TilesetContentLoaderResult<TilesetContentLoader>&& result) {
              return registerGltfModifier(*thiz, result.pRootTile.get())
                  .thenImmediately([result = std::move(result)]() mutable {
                    return std::move(result);
                  });
            })
        .thenInMainThread(
            [thiz, errorCallback = tilesetOptions.loadErrorCallback](
                TilesetContentLoaderResult<TilesetContentLoader>&& result) {
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

TilesetContentManager::TilesetContentManager(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    int64_t ionAssetID,
    const std::string& ionAccessToken,
    const std::string& ionAssetEndpointUrl)
    : TilesetContentManager(
          externals,
          tilesetOptions,
          CesiumIonTilesetContentLoaderFactory(
              static_cast<uint32_t>(ionAssetID),
              ionAccessToken,
              ionAssetEndpointUrl)) {}

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

void TilesetContentManager::loadTileContent(
    Tile& tile,
    const TilesetOptions& tilesetOptions) {
  CESIUM_TRACE("TilesetContentManager::loadTileContent");

  // Case of a tile previously loaded, but now outdated with respect to the
  // modifier version (see matching condition in Tile::needsWorkerThreadLoading)
  // => worker-thread phase of glTF modifier should be started.
  if (_externals.pGltfModifier &&
      _externals.pGltfModifier->getCurrentVersion().has_value() &&
      tile.getState() == TileLoadState::Done) {
    auto* renderContent = tile.getContent().getRenderContent();
    if (renderContent &&
        renderContent->getGltfModifierState() == GltfModifier::State::Idle &&
        renderContent->getModel().version !=
            _externals.pGltfModifier->getCurrentVersion()) {
      renderContent->setGltfModifierState(GltfModifier::State::WorkerRunning);
      glm::dvec4 rootTranslation = glm::dvec4(0., 0., 0., 1.);
      if (this->_pRootTile)
        rootTranslation = glm::column(this->_pRootTile->getTransform(), 3);
      _externals.asyncSystem
          .runInWorkerThread([pGltfModifier = _externals.pGltfModifier,
                              pPrepareRendererResources =
                                  _externals.pPrepareRendererResources,
                              asyncSystem = _externals.asyncSystem,
                              &tile,
                              rendererOptions = tilesetOptions.rendererOptions,
                              rootTranslation] {
            // already known as being non-null
            auto* renderContent = tile.getContent().getRenderContent();
            auto& initialModel = renderContent->getModel();
            CesiumGltf::Model modifiedModel;
            bool const wasModified = pGltfModifier->apply(
                initialModel,
                tile.getTransform(),
                rootTranslation,
                modifiedModel);
            TileLoadResult tileLoadResult;
            tileLoadResult.glTFUpAxis = [&](CesiumGltf::Model const& model) {
              const auto it = model.extras.find("gltfUpAxis");
              if (it == model.extras.end()) {
                CESIUM_ASSERT(false);
                return CesiumGeometry::Axis::Y;
              }
              return static_cast<CesiumGeometry::Axis>(
                  it->second.getSafeNumberOrDefault(1));
            }(wasModified ? modifiedModel : initialModel);
            tileLoadResult.contentKind =
                std::move(wasModified ? modifiedModel : initialModel);
            tileLoadResult.state = TileLoadResultState::Success;
            return pPrepareRendererResources->prepareInLoadThread(
                asyncSystem,
                std::move(tileLoadResult),
                tile.getTransform(),
                rendererOptions);
          })
          .thenInMainThread(
              [&tile,
               // Keep the manager alive while the glTF modification is in
               // progress.
               thiz = CesiumUtility::IntrusivePointer<TilesetContentManager>(
                   this)](TileLoadResultAndRenderResources&& pair) {
                tile.getContent().getRenderContent()->setGltfModifierState(
                    GltfModifier::State::WorkerDone);
                tile.getContent()
                    .getRenderContent()
                    ->setModifiedModelAndRenderResources(
                        std::move(std::get<CesiumGltf::Model>(
                            pair.result.contentKind)),
                        pair.pRenderResources);
              });
      return;
    }
    // else: we may be in the case related to raster tiles, see just below
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
      mapOverlaysToTile(tile, this->_overlayCollection, tilesetOptions);

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

  // Keep the manager alive while the load is in progress.
  CesiumUtility::IntrusivePointer<TilesetContentManager> thiz = this;

  // Compute root translation (useful for glTF modifiers, eg. to handle
  // materials).
  glm::dvec4 rootTranslation = glm::dvec4(0., 0., 0., 1.);
  bool const isLoadingRootTile = (tile.getParent() == nullptr);
  if (this->_pRootTile && !isLoadingRootTile) {
    rootTranslation = glm::column(this->_pRootTile->getTransform(), 3);
  }
  pLoader->loadTileContent(loadInput)
      .thenImmediately([tileLoadInfo = std::move(tileLoadInfo),
                        projections = std::move(projections),
                        rendererOptions = tilesetOptions.rendererOptions,
                        pGltfModifier = _externals.pGltfModifier,
                        rootTranslation = std::move(rootTranslation),
                        isLoadingRootTile](TileLoadResult&& result) mutable {
        // the reason we run immediate continuation, instead of in the
        // worker thread, is that the loader may run the task in the main
        // thread. And most often than not, those main thread task is very
        // light weight. So when those tasks return, there is no need to
        // spawn another worker thread if the result of the task isn't
        // related to render content. We only ever spawn a new task in the
        // worker thread if the content is a render content
        if (result.state == TileLoadResultState::Success) {
          if (std::holds_alternative<CesiumGltf::Model>(result.contentKind)) {
            auto asyncSystem = tileLoadInfo.asyncSystem;
            // update root translation now it has been loaded:
            if (isLoadingRootTile)
              rootTranslation = glm::column(tileLoadInfo.tileTransform, 3);
            return asyncSystem.runInWorkerThread(
                [result = std::move(result),
                 projections = std::move(projections),
                 tileLoadInfo = std::move(tileLoadInfo),
                 rendererOptions,
                 pGltfModifier,
                 rootTranslation = std::move(rootTranslation)]() mutable {
                  if (pGltfModifier &&
                      pGltfModifier->getCurrentVersion().has_value()) {
                    // Apply the glTF modifier right away, otherwise it will be
                    // triggered immediately after the renderer-side resources
                    // have been created, which is both inefficient and a cause
                    // of visual glitches (the model will appear briefly in its
                    // unmodified state before stabilizing)
                    auto& model =
                        std::get<CesiumGltf::Model>(result.contentKind);
                    pGltfModifier->apply(
                        model,
                        tileLoadInfo.tileTransform,
                        rootTranslation,
                        model);
                  }
                  return postProcessContentInWorkerThread(
                      std::move(result),
                      std::move(projections),
                      std::move(tileLoadInfo),
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
  if (_externals.pGltfModifier && state == TileLoadState::Done) {
    auto* renderContent = tile.getContent().getRenderContent();
    if (renderContent) {
      switch (renderContent->getGltfModifierState()) {
      case GltfModifier::State::WorkerRunning:
        // Worker thread is running, we cannot unload yet.
        return UnloadTileContentResult::Keep;
      case GltfModifier::State::WorkerDone:
        // Free temporary render resources.
        CESIUM_ASSERT(renderContent->getModifiedRenderResources());
        _externals.pPrepareRendererResources->free(
            tile,
            renderContent->getModifiedRenderResources(),
            nullptr);
        renderContent->resetModifiedRenderResources();
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

void TilesetContentManager::waitUntilIdle() {
  // Wait for all asynchronous loading to terminate.
  // If you're hanging here, it's most likely caused by _tileLoadsInProgress not
  // being decremented correctly when an async load ends.
  while (this->_tileLoadsInProgress > 0) {
    this->_externals.pAssetAccessor->tick();
    this->_externals.asyncSystem.dispatchMainThreadTasks();
  }

  // Wait for all overlays to wrap up their loading, too.
  uint32_t rasterOverlayTilesLoading = 1;
  while (rasterOverlayTilesLoading > 0) {
    this->_externals.pAssetAccessor->tick();
    this->_externals.asyncSystem.dispatchMainThreadTasks();

    rasterOverlayTilesLoading = 0;
    for (const auto& pTileProvider :
         this->_overlayCollection.getTileProviders()) {
      rasterOverlayTilesLoading += pTileProvider->getNumberOfTilesLoading();
    }
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
  for (const auto& pTileProvider :
       this->_overlayCollection.getTileProviders()) {
    bytes += pTileProvider->getTileDataBytes();
  }

  return bytes;
}

bool TilesetContentManager::discardOutdatedRenderResources(
    Tile& tile,
    TileRenderContent& renderContent) {
  if (!_externals.pGltfModifier)
    return false;
  bool bModifiedModel = renderContent.getModifiedModel().has_value();
  const CesiumGltf::Model& model = bModifiedModel
                                       ? (*renderContent.getModifiedModel())
                                       : renderContent.getModel();
  if (model.version != _externals.pGltfModifier->getCurrentVersion()) {
    _externals.pPrepareRendererResources->free(
        tile,
        bModifiedModel ? renderContent.getModifiedRenderResources()
                       : renderContent.getRenderResources(),
        nullptr);
    if (bModifiedModel)
      renderContent.resetModifiedRenderResources();
    else
      renderContent.setRenderResources(nullptr);
    renderContent.setGltfModifierState(GltfModifier::State::Idle);
    tile.setState(TileLoadState::Done);
    return true;
  }
  return false;
}

void TilesetContentManager::finishLoading(
    Tile& tile,
    const TilesetOptions& tilesetOptions) {
  TileContent& content = tile.getContent();
  TileRenderContent* pRenderContent = content.getRenderContent();
  CESIUM_ASSERT(pRenderContent != nullptr);

  // Case of a tile previously loaded that went outdated with respect to the
  // glTF modifier version => main-thread phase should be performed.
  if (tile.getState() == TileLoadState::Done) {
    CESIUM_ASSERT( // see matching condition in Tile::needsMainThreadLoading
        _externals.pGltfModifier && pRenderContent->getGltfModifierState() ==
                                        GltfModifier::State::WorkerDone);
    CESIUM_ASSERT(pRenderContent->getModifiedRenderResources());
    if (discardOutdatedRenderResources(tile, *pRenderContent)) {
      return;
    }
    pRenderContent->setGltfModifierState(GltfModifier::State::Idle);
    // free outdated render resources before replacing them
    _externals.pPrepareRendererResources->free(
        tile,
        nullptr,
        pRenderContent->getRenderResources());
    // Replace model and render resources with the newly modified versions,
    // discarding the old ones
    pRenderContent->replaceWithModifiedModel();
    // Run the main thread part of loading
    pRenderContent->setRenderResources(
        _externals.pPrepareRendererResources->prepareInMainThread(
            tile,
            pRenderContent->getRenderResources()));
    return;
  }
  CESIUM_ASSERT(tile.getState() == TileLoadState::ContentLoaded);

  // The tile was just loaded and may even have immediately been processed by
  // the glTF modifier (_second_ occurrence of 'pGltfModifier->apply(...)' in
  // loadTileContent): but this happened in a concurrent thread, so it might
  // already be outdated
  // => discard the half constructed renderer resources, and skip the main
  // thread preparation since the tile will be modified immediately afterwards.
  // The members tested below are not used in this case: model is modified "in
  // place", since the tile isn't displayed yet:
  CESIUM_ASSERT(
      !pRenderContent->getModifiedModel() &&
      !pRenderContent->getModifiedRenderResources());
  if (discardOutdatedRenderResources(tile, *pRenderContent)) {
    return;
  }

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

    // We double-check that the tile is still in the ContentLoaded state here,
    // in case something (such as a child that needs to upsample from this
    // parent) already pushed the tile into the Done state. Because in that
    // case, calling finishLoading here would assert or crash.
    if (pToLoad->getState() == TileLoadState::ContentLoaded &&
        pToLoad->isRenderContent()) {
      this->finishLoading(*pToLoad, options);
    } else {
      // Test if main-thread phase of glTF modifier should be performed.
      const auto* renderContent = pToLoad->getContent().getRenderContent();
      if (renderContent && renderContent->getGltfModifierState() ==
                               GltfModifier::State::WorkerDone)
        this->finishLoading(*pToLoad, options);
    }

    auto time = std::chrono::system_clock::now();
    if (timeBudget > 0.0 && time >= end) {
      break;
    }
  }
}

void TilesetContentManager::markTilesetDestroyed() noexcept {
  this->_tilesetDestroyed = true;
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
              missingProjections,
              ellipsoid);

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
    this->_tilesetCredits.emplace_back(_externals.pCreditSystem->createCredit(
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
} // namespace Cesium3DTilesSelection
