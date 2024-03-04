#include "ImplicitOctreeLoader.h"

#include "logTileLoadResult.h"

#include <Cesium3DTilesContent/GltfConverters.h>
#include <Cesium3DTilesContent/ImplicitTilingUtilities.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/Uri.h>

#include <spdlog/logger.h>

#include <variant>

using namespace Cesium3DTilesContent;

namespace Cesium3DTilesSelection {
namespace {
struct BoundingVolumeSubdivision {
  BoundingVolume operator()(const CesiumGeospatial::BoundingRegion& region) {
    return ImplicitTilingUtilities::computeBoundingVolume(region, this->tileID);
  }

  BoundingVolume operator()(const CesiumGeometry::OrientedBoundingBox& obb) {
    return ImplicitTilingUtilities::computeBoundingVolume(obb, this->tileID);
  }

  const CesiumGeometry::OctreeTileID& tileID;
};

BoundingVolume subdivideBoundingVolume(
    const CesiumGeometry::OctreeTileID& tileID,
    const ImplicitOctreeBoundingVolume& rootBoundingVolume) {
  return std::visit(BoundingVolumeSubdivision{tileID}, rootBoundingVolume);
}

std::vector<Tile> populateSubtree(
    const SubtreeAvailability& subtreeAvailability,
    uint32_t subtreeLevels,
    const CesiumGeometry::OctreeTileID& subtreeRootID,
    const Tile& tile,
    ImplicitOctreeLoader& loader) {
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
        child.setBoundingVolume(
            subdivideBoundingVolume(childID, loader.getBoundingVolume()));
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
        child.setBoundingVolume(
            subdivideBoundingVolume(childID, loader.getBoundingVolume()));
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
    const std::string& tileUrl,
    const gsl::span<const std::byte>& responseData,
    CesiumGltf::Ktx2TranscodeTargets ktx2TranscodeTargets,
    bool applyTextureTransform) {
  return asyncSystem.runInWorkerThread([pLogger,
                                        ktx2TranscodeTargets,
                                        applyTextureTransform,
                                        tileUrl = tileUrl,
                                        responseData = responseData]() mutable {
    // find gltf converter
    auto converter = GltfConverters::getConverterByMagic(responseData);
    if (!converter) {
      converter = GltfConverters::getConverterByFileExtension(tileUrl);
    }

    if (converter) {
      // Convert to gltf
      CesiumGltfReader::GltfReaderOptions gltfOptions;
      gltfOptions.ktx2TranscodeTargets = ktx2TranscodeTargets;
      gltfOptions.applyTextureTransform = applyTextureTransform;
      GltfConverterResult result = converter(responseData, gltfOptions);

      // Report any errors if there are any
      logTileLoadResult(pLogger, tileUrl, result.errors);
      if (result.errors || !result.model) {
        return TileLoadResult::createFailedResult();
      }

      return TileLoadResult{
          std::move(*result.model),
          CesiumGeometry::Axis::Y,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          tileUrl,
          {},
          CesiumAsync::RequestData{},
          TileLoadResultState::Success};
    }

    // content type is not supported
    return TileLoadResult::createFailedResult();
  });
}
} // namespace

CesiumAsync::Future<TileLoadResult>
ImplicitOctreeLoader::loadTileContent(const TileLoadInput& loadInput) {
  const auto& tile = loadInput.tile;
  const auto& asyncSystem = loadInput.asyncSystem;
  const auto& pLogger = loadInput.pLogger;
  const auto& contentOptions = loadInput.contentOptions;
  const auto& responsesByUrl = loadInput.responsesByUrl;

  // make sure the tile is a octree tile
  const CesiumGeometry::OctreeTileID* pOctreeID =
      std::get_if<CesiumGeometry::OctreeTileID>(&tile.getTileID());
  if (!pOctreeID) {
    return asyncSystem.createResolvedFuture(
        TileLoadResult::createFailedResult());
  }

  // find the subtree ID
  CesiumGeometry::OctreeTileID subtreeID =
      ImplicitTilingUtilities::getSubtreeRootID(
          this->_subtreeLevels,
          *pOctreeID);
  uint32_t subtreeLevelIdx = subtreeID.level / this->_subtreeLevels;
  if (subtreeLevelIdx >= _loadedSubtrees.size()) {
    return asyncSystem.createResolvedFuture(
        TileLoadResult::createFailedResult());
  }

  uint64_t subtreeMortonIdx =
      ImplicitTilingUtilities::computeMortonIndex(subtreeID);
  auto subtreeIt =
      this->_loadedSubtrees[subtreeLevelIdx].find(subtreeMortonIdx);
  if (subtreeIt == this->_loadedSubtrees[subtreeLevelIdx].end()) {
    std::string subtreeUrl = ImplicitTilingUtilities::resolveUrl(
        this->_baseUrl,
        this->_subtreeUrlTemplate,
        subtreeID);

    // If subtree url is not loaded, request it and come back later
    auto foundIt = responsesByUrl.find(subtreeUrl);
    if (foundIt == responsesByUrl.end()) {
      return asyncSystem.createResolvedFuture<TileLoadResult>(
          TileLoadResult::createRequestResult(
              CesiumAsync::RequestData{subtreeUrl, {}}));
    }

    auto baseResponse = foundIt->second.pResponse;

    uint16_t statusCode = baseResponse->statusCode();
    if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
      SPDLOG_LOGGER_ERROR(
          pLogger,
          "Received status code {} for tile content {}",
          statusCode,
          subtreeUrl);
      return asyncSystem.createResolvedFuture<TileLoadResult>(
          TileLoadResult::createFailedResult());
    }

    return SubtreeAvailability::loadSubtree(
               ImplicitTileSubdivisionScheme::Octree,
               this->_subtreeLevels,
               asyncSystem,
               pLogger,
               subtreeUrl,
               baseResponse,
               responsesByUrl)
        .thenInMainThread(
            [this,
             subtreeID](SubtreeAvailability::LoadResult&& loadResult) mutable {
              if (loadResult.first) {
                // Availability success
                this->addSubtreeAvailability(
                    subtreeID,
                    std::move(*loadResult.first));

                // tell client to retry later
                return TileLoadResult::createRetryLaterResult();
              } else if (!loadResult.second.url.empty()) {
                // No availability, but a url was requested
                // Let this work go back into the request queue
                return TileLoadResult::createRequestResult(loadResult.second);
              } else {
                // Subtree load failed, so this tile fails, too.
                return TileLoadResult::createFailedResult();
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
        std::string(),
        {},
        CesiumAsync::RequestData{},
        TileLoadResultState::Success});
  }

  std::string tileUrl = ImplicitTilingUtilities::resolveUrl(
      this->_baseUrl,
      this->_contentUrlTemplate,
      *pOctreeID);

  // If tile url is not loaded, request it and come back later
  auto foundIt = responsesByUrl.find(tileUrl);
  if (foundIt == responsesByUrl.end()) {
    return asyncSystem.createResolvedFuture<TileLoadResult>(
        TileLoadResult::createRequestResult(
            CesiumAsync::RequestData{tileUrl, {}}));
  }

  const CesiumAsync::ResponseData& responseData = foundIt->second;
  assert(responseData.pResponse);
  uint16_t statusCode = responseData.pResponse->statusCode();
  assert(statusCode != 0);
  if (statusCode < 200 || statusCode >= 300) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Received status code {} for tile content {}",
        statusCode,
        tileUrl);
    return asyncSystem.createResolvedFuture<TileLoadResult>(
        TileLoadResult::createFailedResult());
  }

  return requestTileContent(
      pLogger,
      asyncSystem,
      tileUrl,
      foundIt->second.pResponse->data(),
      contentOptions.ktx2TranscodeTargets,
      contentOptions.applyTextureTransform);
}

void ImplicitOctreeLoader::getLoadWork(
    const Tile*,
    CesiumAsync::RequestData&,
    TileLoaderCallback& outCallback) {
  // loadTileContent will control request / processing flow
  outCallback = [](const std::vector<TileLoadInput>& allLoadInput,
                   TilesetContentLoader* loader,
                   std::vector<CesiumAsync::Future<TileLoadResult>>& out) {
    for (auto& loadInput : allLoadInput) {
      auto future = loader->loadTileContent(loadInput);
      out.push_back(std::move(future));
    }
  };
}

TileChildrenResult ImplicitOctreeLoader::createTileChildren(const Tile& tile) {
  const CesiumGeometry::OctreeTileID* pOctreeID =
      std::get_if<CesiumGeometry::OctreeTileID>(&tile.getTileID());
  assert(pOctreeID != nullptr && "This loader only serves octree tile");

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
        *this);

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
