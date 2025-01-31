#include "ImplicitOctreeLoader.h"

#include "logTileLoadResult.h"

#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <Cesium3DTilesContent/GltfConverters.h>
#include <Cesium3DTilesContent/ImplicitTilingUtilities.h>
#include <Cesium3DTilesContent/SubtreeAvailability.h>
#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/OctreeTileID.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGltf/Ktx2TranscodeTargets.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/Assert.h>

#include <glm/ext/matrix_double4x4.hpp>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace Cesium3DTilesContent;

namespace Cesium3DTilesSelection {
namespace {
struct BoundingVolumeSubdivision {
  BoundingVolume operator()(const CesiumGeospatial::BoundingRegion& region) {
    return ImplicitTilingUtilities::computeBoundingVolume(
        region,
        this->tileID,
        this->ellipsoid);
  }

  BoundingVolume operator()(const CesiumGeometry::OrientedBoundingBox& obb) {
    return ImplicitTilingUtilities::computeBoundingVolume(obb, this->tileID);
  }

  const CesiumGeometry::OctreeTileID& tileID;
  const CesiumGeospatial::Ellipsoid& ellipsoid;
};

BoundingVolume subdivideBoundingVolume(
    const CesiumGeometry::OctreeTileID& tileID,
    const ImplicitOctreeBoundingVolume& rootBoundingVolume,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  return std::visit(
      BoundingVolumeSubdivision{tileID, ellipsoid},
      rootBoundingVolume);
}

std::vector<Tile> populateSubtree(
    const SubtreeAvailability& subtreeAvailability,
    uint32_t subtreeLevels,
    const CesiumGeometry::OctreeTileID& subtreeRootID,
    const Tile& tile,
    ImplicitOctreeLoader& loader,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  const CesiumGeometry::OctreeTileID& octreeID =
      std::get<CesiumGeometry::OctreeTileID>(tile.getTileID());

  uint32_t relativeTileLevel = octreeID.level - subtreeRootID.level;
  if (relativeTileLevel >= subtreeLevels) {
    return {};
  }

  OctreeChildren childIDs = ImplicitTilingUtilities::getChildren(octreeID);

  std::vector<Tile> children;
  children.reserve(childIDs.size());

  for (const CesiumGeometry::OctreeTileID& childID : childIDs) {
    uint64_t relativeChildMortonID =
        ImplicitTilingUtilities::computeRelativeMortonIndex(
            subtreeRootID,
            childID);

    uint32_t relativeChildLevel = relativeTileLevel + 1;
    if (relativeChildLevel == subtreeLevels) {
      if (subtreeAvailability.isSubtreeAvailable(relativeChildMortonID)) {
        Tile& child = children.emplace_back(&loader);
        child.setTransform(tile.getTransform());
        child.setBoundingVolume(subdivideBoundingVolume(
            childID,
            loader.getBoundingVolume(),
            ellipsoid));
        child.setGeometricError(tile.getGeometricError() * 0.5);
        child.setRefine(tile.getRefine());
        child.setTileID(childID);
      }
    } else {
      if (subtreeAvailability.isTileAvailable(
              relativeChildLevel,
              relativeChildMortonID)) {
        if (subtreeAvailability.isContentAvailable(
                relativeChildLevel,
                relativeChildMortonID,
                0)) {
          children.emplace_back(&loader);
        } else {
          children.emplace_back(&loader, TileEmptyContent{});
        }

        Tile& child = children.back();
        child.setTransform(tile.getTransform());
        child.setBoundingVolume(subdivideBoundingVolume(
            childID,
            loader.getBoundingVolume(),
            ellipsoid));
        child.setGeometricError(tile.getGeometricError() * 0.5);
        child.setRefine(tile.getRefine());
        child.setTileID(childID);
      }
    }
  }

  return children;
}

CesiumAsync::Future<TileLoadResult> requestTileContent(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::string& tileUrl,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
    CesiumGltf::Ktx2TranscodeTargets ktx2TranscodeTargets,
    bool applyTextureTransform,
    const glm::dmat4& tileTransform,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  return pAssetAccessor->get(asyncSystem, tileUrl, requestHeaders)
      .thenInWorkerThread(
          [pLogger,
           ktx2TranscodeTargets,
           applyTextureTransform,
           &asyncSystem,
           pAssetAccessor = pAssetAccessor,
           tileTransform,
           requestHeaders,
           ellipsoid](std::shared_ptr<CesiumAsync::IAssetRequest>&&
                          pCompletedRequest) mutable {
            const CesiumAsync::IAssetResponse* pResponse =
                pCompletedRequest->response();
            auto fail = [&]() {
              return asyncSystem.createResolvedFuture(
                  TileLoadResult::createFailedResult(
                      std::move(pAssetAccessor),
                      std::move(pCompletedRequest)));
            };
            const std::string& tileUrl = pCompletedRequest->url();
            if (!pResponse) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Did not receive a valid response for tile content {}",
                  tileUrl);
              return fail();
            }

            uint16_t statusCode = pResponse->statusCode();
            if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Received status code {} for tile content {}",
                  statusCode,
                  tileUrl);
              return fail();
            }

            // find gltf converter
            const auto& responseData = pResponse->data();
            auto converter = GltfConverters::getConverterByMagic(responseData);
            if (!converter) {
              converter = GltfConverters::getConverterByFileExtension(
                  pCompletedRequest->url());
            }

            if (converter) {
              // Convert to gltf
              CesiumGltfReader::GltfReaderOptions gltfOptions;
              gltfOptions.ktx2TranscodeTargets = ktx2TranscodeTargets;
              gltfOptions.applyTextureTransform = applyTextureTransform;
              AssetFetcher assetFetcher{
                  asyncSystem,
                  pAssetAccessor,
                  tileUrl,
                  tileTransform,
                  requestHeaders,
                  CesiumGeometry::Axis::Y};
              return converter(responseData, gltfOptions, assetFetcher)
                  .thenImmediately(
                      [pAssetAccessor = std::move(pAssetAccessor),
                       pLogger,
                       tileUrl,
                       pCompletedRequest,
                       ellipsoid](GltfConverterResult&& result) mutable {
                        // Report any errors if there are any
                        logTileLoadResult(pLogger, tileUrl, result.errors);
                        if (result.errors || !result.model) {
                          return TileLoadResult::createFailedResult(
                              std::move(pAssetAccessor),
                              std::move(pCompletedRequest));
                        }

                        return TileLoadResult{
                            std::move(*result.model),
                            CesiumGeometry::Axis::Y,
                            std::nullopt,
                            std::nullopt,
                            std::nullopt,
                            std::move(pAssetAccessor),
                            std::move(pCompletedRequest),
                            {},
                            TileLoadResultState::Success,
                            ellipsoid};
                      });
            }
            // content type is not supported
            return fail();
          });
}
} // namespace

CesiumAsync::Future<TileLoadResult>
ImplicitOctreeLoader::loadTileContent(const TileLoadInput& loadInput) {
  const auto& tile = loadInput.tile;
  const auto& asyncSystem = loadInput.asyncSystem;
  const auto& pAssetAccessor = loadInput.pAssetAccessor;
  const auto& pLogger = loadInput.pLogger;
  const auto& requestHeaders = loadInput.requestHeaders;
  const auto& contentOptions = loadInput.contentOptions;
  const auto& ellipsoid = loadInput.ellipsoid;

  // make sure the tile is a octree tile
  const CesiumGeometry::OctreeTileID* pOctreeID =
      std::get_if<CesiumGeometry::OctreeTileID>(&tile.getTileID());
  if (!pOctreeID) {
    return asyncSystem.createResolvedFuture(
        TileLoadResult::createFailedResult(pAssetAccessor, nullptr));
  }

  // find the subtree ID
  CesiumGeometry::OctreeTileID subtreeID =
      ImplicitTilingUtilities::getSubtreeRootID(
          this->_subtreeLevels,
          *pOctreeID);
  uint32_t subtreeLevelIdx = subtreeID.level / this->_subtreeLevels;
  if (subtreeLevelIdx >= _loadedSubtrees.size()) {
    return asyncSystem.createResolvedFuture<TileLoadResult>(
        TileLoadResult::createFailedResult(pAssetAccessor, nullptr));
  }

  uint64_t subtreeMortonIdx =
      ImplicitTilingUtilities::computeMortonIndex(subtreeID);
  auto subtreeIt =
      this->_loadedSubtrees[subtreeLevelIdx].find(subtreeMortonIdx);
  if (subtreeIt == this->_loadedSubtrees[subtreeLevelIdx].end()) {
    // subtree is not loaded, so load it now.
    std::string subtreeUrl = ImplicitTilingUtilities::resolveUrl(
        this->_baseUrl,
        this->_subtreeUrlTemplate,
        subtreeID);
    return SubtreeAvailability::loadSubtree(
               ImplicitTileSubdivisionScheme::Octree,
               this->_subtreeLevels,
               asyncSystem,
               pAssetAccessor,
               pLogger,
               subtreeUrl,
               requestHeaders)
        .thenInMainThread([this, pAssetAccessor, subtreeID](
                              std::optional<SubtreeAvailability>&&
                                  subtreeAvailability) mutable {
          if (subtreeAvailability) {
            this->addSubtreeAvailability(
                subtreeID,
                std::move(*subtreeAvailability));

            // tell client to retry later
            return TileLoadResult::createRetryLaterResult(
                pAssetAccessor,
                nullptr);
          } else {
            // Subtree load failed, so this tile fails, too.
            return TileLoadResult::createFailedResult(pAssetAccessor, nullptr);
          }
        });
  }

  // subtree is available, so check if tile has content or not. If it has, then
  // request it
  if (!subtreeIt->second.isContentAvailable(subtreeID, *pOctreeID, 0)) {
    // check if tile has empty content
    return asyncSystem.createResolvedFuture(TileLoadResult{
        TileEmptyContent{},
        CesiumGeometry::Axis::Y,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        nullptr,
        {},
        TileLoadResultState::Success,
        ellipsoid});
  }

  std::string tileUrl = ImplicitTilingUtilities::resolveUrl(
      this->_baseUrl,
      this->_contentUrlTemplate,
      *pOctreeID);
  return requestTileContent(
      pLogger,
      asyncSystem,
      pAssetAccessor,
      tileUrl,
      requestHeaders,
      contentOptions.ktx2TranscodeTargets,
      contentOptions.applyTextureTransform,
      tile.getTransform(),
      ellipsoid);
}

TileChildrenResult ImplicitOctreeLoader::createTileChildren(
    const Tile& tile,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  const CesiumGeometry::OctreeTileID* pOctreeID =
      std::get_if<CesiumGeometry::OctreeTileID>(&tile.getTileID());
  CESIUM_ASSERT(pOctreeID != nullptr && "This loader only serves octree tile");

  // find the subtree ID
  CesiumGeometry::OctreeTileID subtreeID =
      ImplicitTilingUtilities::getSubtreeRootID(
          this->_subtreeLevels,
          *pOctreeID);

  uint32_t subtreeLevelIdx = subtreeID.level / this->_subtreeLevels;
  if (subtreeLevelIdx >= this->_loadedSubtrees.size()) {
    return {{}, TileLoadResultState::Failed};
  }

  uint64_t subtreeMortonIdx =
      ImplicitTilingUtilities::computeMortonIndex(subtreeID);

  auto subtreeIt =
      this->_loadedSubtrees[subtreeLevelIdx].find(subtreeMortonIdx);
  if (subtreeIt != this->_loadedSubtrees[subtreeLevelIdx].end()) {
    auto children = populateSubtree(
        subtreeIt->second,
        this->_subtreeLevels,
        subtreeID,
        tile,
        *this,
        ellipsoid);

    return {std::move(children), TileLoadResultState::Success};
  }

  return {{}, TileLoadResultState::RetryLater};
}

uint32_t ImplicitOctreeLoader::getSubtreeLevels() const noexcept {
  return this->_subtreeLevels;
}

uint32_t ImplicitOctreeLoader::getAvailableLevels() const noexcept {
  return this->_availableLevels;
}

const ImplicitOctreeBoundingVolume&
ImplicitOctreeLoader::getBoundingVolume() const noexcept {
  return this->_boundingVolume;
}

void ImplicitOctreeLoader::addSubtreeAvailability(
    const CesiumGeometry::OctreeTileID& subtreeID,
    SubtreeAvailability&& subtreeAvailability) {
  uint32_t levelIndex = subtreeID.level / this->_subtreeLevels;
  if (levelIndex >= this->_loadedSubtrees.size()) {
    return;
  }

  uint64_t subtreeMortonID =
      ImplicitTilingUtilities::computeMortonIndex(subtreeID);

  this->_loadedSubtrees[levelIndex].insert_or_assign(
      subtreeMortonID,
      std::move(subtreeAvailability));
}
} // namespace Cesium3DTilesSelection
