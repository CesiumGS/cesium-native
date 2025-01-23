#include "TilesetContentManager.h"

#include "CesiumIonTilesetLoader.h"
#include "LayerJsonTerrainLoader.h"
#include "RasterOverlayUpsampler.h"
#include "TileContentLoadInfo.h"
#include "TilesetContentLoaderResult.h"
#include "TilesetJsonLoader.h"

#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/IPrepareRendererResources.h>
#include <Cesium3DTilesSelection/RasterMappedTo3DTile.h>
#include <Cesium3DTilesSelection/RasterOverlayCollection.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TileRefine.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
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
#include <CesiumUtility/Tracing.h>
#include <CesiumUtility/joinToString.h>

#include <fmt/format.h>
#include <glm/ext/vector_double2.hpp>
#include <rapidjson/document.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <any>
#include <cassert>
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
} // namespace

TilesetContentManager::TilesetContentManager(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    RasterOverlayCollection&& overlayCollection,
    std::unique_ptr<TilesetContentLoader>&& pLoader,
    std::unique_ptr<Tile>&& pRootTile)
    : _externals{externals},
      _requestHeaders{tilesetOptions.requestHeaders},
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
      _pSharedAssetSystem(externals.pSharedAssetSystem),
      _destructionCompletePromise{externals.asyncSystem.createPromise<void>()},
      _destructionCompleteFuture{
          this->_destructionCompletePromise.getFuture().share()},
      _rootTileAvailablePromise{externals.asyncSystem.createPromise<void>()},
      _rootTileAvailableFuture{
          this->_rootTileAvailablePromise.getFuture().share()} {
  this->_rootTileAvailablePromise.resolve();
}

TilesetContentManager::TilesetContentManager(
    const TilesetExternals& externals,
    const TilesetOptions& tilesetOptions,
    RasterOverlayCollection&& overlayCollection,
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
      _overlayCollection{std::move(overlayCollection)},
      _tileLoadsInProgress{0},
      _loadedTilesCount{0},
      _tilesDataUsed{0},
      _pSharedAssetSystem(externals.pSharedAssetSystem),
      _destructionCompletePromise{externals.asyncSystem.createPromise<void>()},
      _destructionCompleteFuture{
          this->_destructionCompletePromise.getFuture().share()},
      _rootTileAvailablePromise{externals.asyncSystem.createPromise<void>()},
      _rootTileAvailableFuture{
          this->_rootTileAvailablePromise.getFuture().share()} {
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
                           tilesetJson,
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
      _overlayCollection{std::move(overlayCollection)},
      _tileLoadsInProgress{0},
      _loadedTilesCount{0},
      _tilesDataUsed{0},
      _pSharedAssetSystem(externals.pSharedAssetSystem),
      _destructionCompletePromise{externals.asyncSystem.createPromise<void>()},
      _destructionCompleteFuture{
          this->_destructionCompletePromise.getFuture().share()},
      _rootTileAvailablePromise{externals.asyncSystem.createPromise<void>()},
      _rootTileAvailableFuture{
          this->_rootTileAvailablePromise.getFuture().share()} {
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
        tilesetOptions.showCreditsOnScreen,
        tilesetOptions.ellipsoid)
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

  pLoader->loadTileContent(loadInput)
      .thenImmediately([tileLoadInfo = std::move(tileLoadInfo),
                        projections = std::move(projections),
                        rendererOptions = tilesetOptions.rendererOptions](
                           TileLoadResult&& result) mutable {
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
            return asyncSystem.runInWorkerThread(
                [result = std::move(result),
                 projections = std::move(projections),
                 tileLoadInfo = std::move(tileLoadInfo),
                 rendererOptions]() mutable {
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
      .thenInMainThread([&tile, thiz](TileLoadResultAndRenderResources&& pair) {
        setTileContent(tile, std::move(pair.result), pair.pRenderResources);

        thiz->notifyTileDoneLoading(&tile);
      })
      .catchInMainThread([pLogger = this->_externals.pLogger, &tile, thiz](
                             std::exception&& e) {
        thiz->notifyTileDoneLoading(&tile);
        SPDLOG_LOGGER_ERROR(
            pLogger,
            "An unexpected error occurs when loading tile: {}",
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
  CESIUM_ASSERT(tile.getState() == TileLoadState::ContentLoaded);

  // Run the main thread part of loading.
  TileContent& content = tile.getContent();
  TileRenderContent* pRenderContent = content.getRenderContent();

  CESIUM_ASSERT(pRenderContent != nullptr);

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
      tile.setContentBoundingVolume(result.updatedContentBoundingVolume);
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

  void* pMainThreadRenderResources = pRenderContent->getRenderResources();
  this->_externals.pPrepareRendererResources->free(
      tile,
      nullptr,
      pMainThreadRenderResources);
  pRenderContent->setRenderResources(nullptr);
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
} // namespace Cesium3DTilesSelection
