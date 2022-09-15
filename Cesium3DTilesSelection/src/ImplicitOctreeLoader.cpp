#include "ImplicitOctreeLoader.h"

#include "logTileLoadResult.h"

#include <Cesium3DTilesSelection/GltfConverters.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/Uri.h>

#include <libmorton/morton.h>
#include <spdlog/logger.h>

namespace Cesium3DTilesSelection {
namespace {
struct BoundingVolumeSubdivision {
  BoundingVolume operator()(const CesiumGeospatial::BoundingRegion& region) {
    const CesiumGeospatial::GlobeRectangle& globeRect = region.getRectangle();
    double denominator = static_cast<double>(1 << tileID.level);
    double latSize =
        (globeRect.getNorth() - globeRect.getSouth()) / denominator;
    double longSize = (globeRect.getEast() - globeRect.getWest()) / denominator;
    double heightSize =
        (region.getMaximumHeight() - region.getMinimumHeight()) / denominator;

    double childWest = globeRect.getWest() + longSize * tileID.x;
    double childEast = globeRect.getWest() + longSize * (tileID.x + 1);

    double childSouth = globeRect.getSouth() + latSize * tileID.y;
    double childNorth = globeRect.getSouth() + latSize * (tileID.y + 1);

    double childMinHeight = region.getMinimumHeight() + heightSize * tileID.z;
    double childMaxHeight =
        region.getMinimumHeight() + heightSize * (tileID.z + 1);

    return CesiumGeospatial::BoundingRegion{
        CesiumGeospatial::GlobeRectangle(
            childWest,
            childSouth,
            childEast,
            childNorth),
        childMinHeight,
        childMaxHeight};
  }

  BoundingVolume operator()(const CesiumGeometry::OrientedBoundingBox& obb) {
    const glm::dmat3& halfAxes = obb.getHalfAxes();
    const glm::dvec3& center = obb.getCenter();

    double denominator = static_cast<double>(1 << tileID.level);
    glm::dvec3 min = center - halfAxes[0] - halfAxes[1] - halfAxes[2];

    glm::dvec3 xDim = halfAxes[0] * 2.0 / denominator;
    glm::dvec3 yDim = halfAxes[1] * 2.0 / denominator;
    glm::dvec3 zDim = halfAxes[2] * 2.0 / denominator;
    glm::dvec3 childMin = min + xDim * double(tileID.x) +
                          yDim * double(tileID.y) + zDim * double(tileID.z);
    glm::dvec3 childMax = min + xDim * double(tileID.x + 1) +
                          yDim * double(tileID.y + 1) +
                          zDim * double(tileID.z + 1);

    return CesiumGeometry::OrientedBoundingBox(
        (childMin + childMax) / 2.0,
        glm::dmat3{xDim / 2.0, yDim / 2.0, zDim / 2.0});
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
    uint32_t relativeTileLevel,
    uint64_t relativeTileMortonID,
    const Tile& tile,
    ImplicitOctreeLoader& loader) {
  if (relativeTileLevel >= subtreeLevels) {
    return {};
  }

  const CesiumGeometry::OctreeTileID& octreeID =
      std::get<CesiumGeometry::OctreeTileID>(tile.getTileID());

  std::vector<Tile> children;
  children.reserve(8);
  for (uint16_t y = 0; y < 2; ++y) {
    uint32_t childY = (octreeID.y << 1) | y;
    for (uint16_t z = 0; z < 2; ++z) {
      uint32_t childZ = (octreeID.z << 1) | z;
      for (uint16_t x = 0; x < 2; ++x) {
        uint32_t childX = (octreeID.x << 1) | x;

        CesiumGeometry::OctreeTileID childID{
            octreeID.level + 1,
            childX,
            childY,
            childZ};

        uint32_t childIndex =
            static_cast<uint32_t>(libmorton::morton3D_32_encode(x, y, z));
        uint64_t relativeChildMortonID = relativeTileMortonID << 3 | childIndex;
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
    }
  }

  return children;
}

bool isTileContentAvailable(
    const CesiumGeometry::OctreeTileID& subtreeID,
    const CesiumGeometry::OctreeTileID& octreeID,
    const SubtreeAvailability& subtreeAvailability) {
  uint32_t relativeTileLevel = octreeID.level - subtreeID.level;
  uint64_t relativeTileMortonIdx = libmorton::morton3D_64_encode(
      octreeID.x - (subtreeID.x << relativeTileLevel),
      octreeID.y - (subtreeID.y << relativeTileLevel),
      octreeID.z - (subtreeID.z << relativeTileLevel));
  return subtreeAvailability.isContentAvailable(
      relativeTileLevel,
      relativeTileMortonIdx,
      0);
}

CesiumAsync::Future<TileLoadResult> requestTileContent(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::string& tileUrl,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
    CesiumGltf::Ktx2TranscodeTargets ktx2TranscodeTargets) {
  return pAssetAccessor->get(asyncSystem, tileUrl, requestHeaders)
      .thenInWorkerThread([pLogger, ktx2TranscodeTargets](
                              std::shared_ptr<CesiumAsync::IAssetRequest>&&
                                  pCompletedRequest) mutable {
        const CesiumAsync::IAssetResponse* pResponse =
            pCompletedRequest->response();
        const std::string& tileUrl = pCompletedRequest->url();
        if (!pResponse) {
          SPDLOG_LOGGER_ERROR(
              pLogger,
              "Did not receive a valid response for tile content {}",
              tileUrl);
          return TileLoadResult::createFailedResult(
              std::move(pCompletedRequest));
        }

        uint16_t statusCode = pResponse->statusCode();
        if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
          SPDLOG_LOGGER_ERROR(
              pLogger,
              "Received status code {} for tile content {}",
              statusCode,
              tileUrl);
          return TileLoadResult::createFailedResult(
              std::move(pCompletedRequest));
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
          GltfConverterResult result = converter(responseData, gltfOptions);

          // Report any errors if there are any
          logTileLoadResult(pLogger, tileUrl, result.errors);
          if (result.errors || !result.model) {
            return TileLoadResult::createFailedResult(
                std::move(pCompletedRequest));
          }

          return TileLoadResult{
              std::move(*result.model),
              CesiumGeometry::Axis::Y,
              std::nullopt,
              std::nullopt,
              std::nullopt,
              std::move(pCompletedRequest),
              {},
              TileLoadResultState::Success};
        }

        // content type is not supported
        return TileLoadResult::createFailedResult(std::move(pCompletedRequest));
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

  // make sure the tile is a octree tile
  const CesiumGeometry::OctreeTileID* pOctreeID =
      std::get_if<CesiumGeometry::OctreeTileID>(&tile.getTileID());
  if (!pOctreeID) {
    return asyncSystem.createResolvedFuture(
        TileLoadResult::createFailedResult(nullptr));
  }

  // find the subtree ID
  uint32_t subtreeLevelIdx = pOctreeID->level / this->_subtreeLevels;
  if (subtreeLevelIdx >= this->_loadedSubtrees.size()) {
    return asyncSystem.createResolvedFuture(
        TileLoadResult::createFailedResult(nullptr));
  }

  uint64_t levelLeft = pOctreeID->level % this->_subtreeLevels;
  uint32_t subtreeLevel = this->_subtreeLevels * subtreeLevelIdx;
  uint32_t subtreeX = pOctreeID->x >> levelLeft;
  uint32_t subtreeY = pOctreeID->y >> levelLeft;
  uint32_t subtreeZ = pOctreeID->z >> levelLeft;
  CesiumGeometry::OctreeTileID subtreeID{
      subtreeLevel,
      subtreeX,
      subtreeY,
      subtreeZ};

  uint64_t subtreeMortonIdx =
      libmorton::morton3D_64_encode(subtreeX, subtreeY, subtreeZ);
  auto subtreeIt =
      this->_loadedSubtrees[subtreeLevelIdx].find(subtreeMortonIdx);
  if (subtreeIt == this->_loadedSubtrees[subtreeLevelIdx].end()) {
    // subtree is not loaded, so load it now.
    std::string subtreeUrl =
        resolveUrl(this->_baseUrl, this->_subtreeUrlTemplate, subtreeID);
    return SubtreeAvailability::loadSubtree(
               3,
               asyncSystem,
               pAssetAccessor,
               pLogger,
               subtreeUrl,
               requestHeaders)
        .thenInMainThread([this, subtreeID](std::optional<SubtreeAvailability>&&
                                                subtreeAvailability) mutable {
          if (subtreeAvailability) {
            this->addSubtreeAvailability(
                subtreeID,
                std::move(*subtreeAvailability));
          }

          // tell client to retry later
          return TileLoadResult::createRetryLaterResult(nullptr);
        });
  }

  // subtree is available, so check if tile has content or not. If it has, then
  // request it
  if (!isTileContentAvailable(subtreeID, *pOctreeID, subtreeIt->second)) {
    // check if tile has empty content
    return asyncSystem.createResolvedFuture(TileLoadResult{
        TileEmptyContent{},
        CesiumGeometry::Axis::Y,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        {},
        TileLoadResultState::Success});
  }

  std::string tileUrl =
      resolveUrl(this->_baseUrl, this->_contentUrlTemplate, *pOctreeID);
  return requestTileContent(
      pLogger,
      asyncSystem,
      pAssetAccessor,
      tileUrl,
      requestHeaders,
      contentOptions.ktx2TranscodeTargets);
}

TileChildrenResult ImplicitOctreeLoader::createTileChildren(const Tile& tile) {
  const CesiumGeometry::OctreeTileID* pOctreeID =
      std::get_if<CesiumGeometry::OctreeTileID>(&tile.getTileID());
  assert(pOctreeID != nullptr && "This loader only serves quadtree tile");

  // find the subtree ID
  uint32_t subtreeLevelIdx = pOctreeID->level / this->_subtreeLevels;
  if (subtreeLevelIdx >= this->_loadedSubtrees.size()) {
    return {{}, TileLoadResultState::Failed};
  }

  uint64_t levelLeft = pOctreeID->level % this->_subtreeLevels;
  uint32_t subtreeX = pOctreeID->x >> levelLeft;
  uint32_t subtreeY = pOctreeID->y >> levelLeft;
  uint32_t subtreeZ = pOctreeID->z >> levelLeft;

  uint64_t subtreeMortonIdx =
      libmorton::morton3D_64_encode(subtreeX, subtreeY, subtreeZ);
  auto subtreeIt =
      this->_loadedSubtrees[subtreeLevelIdx].find(subtreeMortonIdx);
  if (subtreeIt != this->_loadedSubtrees[subtreeLevelIdx].end()) {
    uint64_t relativeTileMortonIdx = libmorton::morton3D_64_encode(
        pOctreeID->x - (subtreeX << levelLeft),
        pOctreeID->y - (subtreeY << levelLeft),
        pOctreeID->z - (subtreeZ << levelLeft));
    auto children = populateSubtree(
        subtreeIt->second,
        this->_subtreeLevels,
        static_cast<std::uint32_t>(levelLeft),
        relativeTileMortonIdx,
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
      libmorton::morton3D_64_encode(subtreeID.x, subtreeID.y, subtreeID.z);

  this->_loadedSubtrees[levelIndex].insert_or_assign(
      subtreeMortonID,
      std::move(subtreeAvailability));
}

std::string ImplicitOctreeLoader::resolveUrl(
    const std::string& baseUrl,
    const std::string& urlTemplate,
    const CesiumGeometry::OctreeTileID& octreeID) {
  std::string url = CesiumUtility::Uri::substituteTemplateParameters(
      urlTemplate,
      [&octreeID](const std::string& placeholder) {
        if (placeholder == "level") {
          return std::to_string(octreeID.level);
        }
        if (placeholder == "x") {
          return std::to_string(octreeID.x);
        }
        if (placeholder == "y") {
          return std::to_string(octreeID.y);
        }
        if (placeholder == "z") {
          return std::to_string(octreeID.z);
        }

        return placeholder;
      });

  return CesiumUtility::Uri::resolve(baseUrl, url);
}
} // namespace Cesium3DTilesSelection
