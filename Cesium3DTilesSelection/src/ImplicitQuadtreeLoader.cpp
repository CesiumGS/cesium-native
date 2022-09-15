#include "ImplicitQuadtreeLoader.h"

#include "logTileLoadResult.h"

#include <Cesium3DTilesSelection/GltfConverters.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumUtility/Uri.h>

#include <libmorton/morton.h>
#include <spdlog/logger.h>

#include <type_traits>
#include <variant>

namespace Cesium3DTilesSelection {
namespace {
struct BoundingVolumeSubdivision {
  BoundingVolume operator()(const CesiumGeospatial::BoundingRegion& region) {
    const CesiumGeospatial::GlobeRectangle& globeRect = region.getRectangle();
    double denominator = static_cast<double>(1 << tileID.level);
    double latSize =
        (globeRect.getNorth() - globeRect.getSouth()) / denominator;
    double longSize = (globeRect.getEast() - globeRect.getWest()) / denominator;

    double childWest = globeRect.getWest() + longSize * tileID.x;
    double childEast = globeRect.getWest() + longSize * (tileID.x + 1);

    double childSouth = globeRect.getSouth() + latSize * tileID.y;
    double childNorth = globeRect.getSouth() + latSize * (tileID.y + 1);

    return CesiumGeospatial::BoundingRegion{
        CesiumGeospatial::GlobeRectangle(
            childWest,
            childSouth,
            childEast,
            childNorth),
        region.getMinimumHeight(),
        region.getMaximumHeight()};
  }

  BoundingVolume
  operator()(const CesiumGeospatial::S2CellBoundingVolume& s2Volume) {
    return CesiumGeospatial::S2CellBoundingVolume(
        CesiumGeospatial::S2CellID::fromQuadtreeTileID(
            s2Volume.getCellID().getFace(),
            tileID),
        s2Volume.getMinimumHeight(),
        s2Volume.getMaximumHeight());
  }

  BoundingVolume operator()(const CesiumGeometry::OrientedBoundingBox& obb) {
    const glm::dmat3& halfAxes = obb.getHalfAxes();
    const glm::dvec3& center = obb.getCenter();

    double denominator = static_cast<double>(1 << tileID.level);
    glm::dvec3 min = center - halfAxes[0] - halfAxes[1] - halfAxes[2];

    glm::dvec3 xDim = halfAxes[0] * 2.0 / denominator;
    glm::dvec3 yDim = halfAxes[1] * 2.0 / denominator;
    glm::dvec3 childMin =
        min + xDim * double(tileID.x) + yDim * double(tileID.y);
    glm::dvec3 childMax = min + xDim * double(tileID.x + 1) +
                          yDim * double(tileID.y + 1) + halfAxes[2] * 2.0;

    return CesiumGeometry::OrientedBoundingBox(
        (childMin + childMax) / 2.0,
        glm::dmat3{xDim / 2.0, yDim / 2.0, halfAxes[2]});
  }

  const CesiumGeometry::QuadtreeTileID& tileID;
};

BoundingVolume subdivideBoundingVolume(
    const CesiumGeometry::QuadtreeTileID& tileID,
    const ImplicitQuadtreeBoundingVolume& rootBoundingVolume) {
  return std::visit(BoundingVolumeSubdivision{tileID}, rootBoundingVolume);
}

std::vector<Tile> populateSubtree(
    const SubtreeAvailability& subtreeAvailability,
    uint32_t subtreeLevels,
    uint32_t relativeTileLevel,
    uint64_t relativeTileMortonID,
    const Tile& tile,
    ImplicitQuadtreeLoader& loader) {
  if (relativeTileLevel >= subtreeLevels) {
    return {};
  }

  const CesiumGeometry::QuadtreeTileID& quadtreeID =
      std::get<CesiumGeometry::QuadtreeTileID>(tile.getTileID());

  std::vector<Tile> children;
  children.reserve(4);
  for (uint16_t y = 0; y < 2; ++y) {
    uint32_t childY = (quadtreeID.y << 1) | y;
    for (uint16_t x = 0; x < 2; ++x) {
      uint32_t childX = (quadtreeID.x << 1) | x;
      CesiumGeometry::QuadtreeTileID childID{
          quadtreeID.level + 1,
          childX,
          childY};

      uint32_t childIndex =
          static_cast<uint32_t>(libmorton::morton2D_32_encode(x, y));
      uint64_t relativeChildMortonID = relativeTileMortonID << 2 | childIndex;
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

  return children;
}

bool isTileContentAvailable(
    const CesiumGeometry::QuadtreeTileID& subtreeID,
    const CesiumGeometry::QuadtreeTileID& quadtreeID,
    const SubtreeAvailability& subtreeAvailability) {
  uint32_t relativeTileLevel = quadtreeID.level - subtreeID.level;
  uint64_t relativeTileMortonIdx = libmorton::morton2D_64_encode(
      quadtreeID.x - (subtreeID.x << relativeTileLevel),
      quadtreeID.y - (subtreeID.y << relativeTileLevel));
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
ImplicitQuadtreeLoader::loadTileContent(const TileLoadInput& loadInput) {
  const auto& tile = loadInput.tile;
  const auto& asyncSystem = loadInput.asyncSystem;
  const auto& pAssetAccessor = loadInput.pAssetAccessor;
  const auto& pLogger = loadInput.pLogger;
  const auto& requestHeaders = loadInput.requestHeaders;
  const auto& contentOptions = loadInput.contentOptions;

  // Ensure CesiumGeometry::QuadtreeTileID only has 32-bit components. There are
  // solutions below if the ID has more than 32-bit components.
  static_assert(
      std::is_same_v<
          decltype(std::declval<CesiumGeometry::QuadtreeTileID>().x),
          uint32_t>,
      "This loader cannot support more than 32-bit ID");
  static_assert(
      std::is_same_v<
          decltype(std::declval<CesiumGeometry::QuadtreeTileID>().y),
          uint32_t>,
      "This loader cannot support more than 32-bit ID");
  static_assert(
      std::is_same_v<
          decltype(std::declval<CesiumGeometry::QuadtreeTileID>().level),
          uint32_t>,
      "This loader cannot support more than 32-bit ID");

  // make sure the tile is a quadtree tile
  const CesiumGeometry::QuadtreeTileID* pQuadtreeID =
      std::get_if<CesiumGeometry::QuadtreeTileID>(&tile.getTileID());
  if (!pQuadtreeID) {
    return asyncSystem.createResolvedFuture<TileLoadResult>(
        TileLoadResult::createFailedResult(nullptr));
  }

  // find the subtree ID
  uint32_t subtreeLevelIdx = pQuadtreeID->level / this->_subtreeLevels;
  if (subtreeLevelIdx >= _loadedSubtrees.size()) {
    return asyncSystem.createResolvedFuture<TileLoadResult>(
        TileLoadResult::createFailedResult(nullptr));
  }

  uint64_t levelLeft = pQuadtreeID->level % this->_subtreeLevels;
  uint32_t subtreeLevel = this->_subtreeLevels * subtreeLevelIdx;
  uint32_t subtreeX = pQuadtreeID->x >> levelLeft;
  uint32_t subtreeY = pQuadtreeID->y >> levelLeft;
  CesiumGeometry::QuadtreeTileID subtreeID{subtreeLevel, subtreeX, subtreeY};

  // the below morton index hash to the subtree assumes that tileID's components
  // x and y never exceed 32-bit. In other words, the max levels this loader can
  // support is 33 which will have 4^32 tiles in the level 32th. The 64-bit
  // morton index below can support that much tiles without overflow. More than
  // 33 levels, this loader will fail. One solution for that is to create
  // multiple new ImplicitQuadtreeLoaders and assign them to any tiles that have
  // levels exceeding the maximum 33. Those new loaders will be added to the
  // current loader, and thus, create a hierarchical tree of loaders where each
  // loader will serve up to 33 levels with the level 0 being relative to the
  // parent loader. The solution isn't implemented at the moment, as implicit
  // tilesets that exceeds 33 levels are expected to be very rare
  uint64_t subtreeMortonIdx = libmorton::morton2D_64_encode(subtreeX, subtreeY);
  auto subtreeIt =
      this->_loadedSubtrees[subtreeLevelIdx].find(subtreeMortonIdx);
  if (subtreeIt == this->_loadedSubtrees[subtreeLevelIdx].end()) {
    // subtree is not loaded, so load it now.
    std::string subtreeUrl =
        resolveUrl(this->_baseUrl, this->_subtreeUrlTemplate, subtreeID);
    return SubtreeAvailability::loadSubtree(
               2,
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
  if (!isTileContentAvailable(subtreeID, *pQuadtreeID, subtreeIt->second)) {
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
      resolveUrl(this->_baseUrl, this->_contentUrlTemplate, *pQuadtreeID);
  return requestTileContent(
      pLogger,
      asyncSystem,
      pAssetAccessor,
      tileUrl,
      requestHeaders,
      contentOptions.ktx2TranscodeTargets);
}

TileChildrenResult
ImplicitQuadtreeLoader::createTileChildren(const Tile& tile) {
  const CesiumGeometry::QuadtreeTileID* pQuadtreeID =
      std::get_if<CesiumGeometry::QuadtreeTileID>(&tile.getTileID());
  assert(pQuadtreeID != nullptr && "This loader only serves quadtree tile");

  // find the subtree ID
  uint32_t subtreeLevelIdx = pQuadtreeID->level / this->_subtreeLevels;
  if (subtreeLevelIdx >= this->_loadedSubtrees.size()) {
    return {{}, TileLoadResultState::Failed};
  }

  uint64_t levelLeft = pQuadtreeID->level % this->_subtreeLevels;
  uint32_t subtreeX = pQuadtreeID->x >> levelLeft;
  uint32_t subtreeY = pQuadtreeID->y >> levelLeft;

  uint64_t subtreeMortonIdx = libmorton::morton2D_64_encode(subtreeX, subtreeY);
  auto subtreeIt =
      this->_loadedSubtrees[subtreeLevelIdx].find(subtreeMortonIdx);
  if (subtreeIt != this->_loadedSubtrees[subtreeLevelIdx].end()) {
    uint64_t relativeTileMortonIdx = libmorton::morton2D_64_encode(
        pQuadtreeID->x - (subtreeX << levelLeft),
        pQuadtreeID->y - (subtreeY << levelLeft));
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

uint32_t ImplicitQuadtreeLoader::getSubtreeLevels() const noexcept {
  return this->_subtreeLevels;
}

uint32_t ImplicitQuadtreeLoader::getAvailableLevels() const noexcept {
  return this->_availableLevels;
}

const ImplicitQuadtreeBoundingVolume&
ImplicitQuadtreeLoader::getBoundingVolume() const noexcept {
  return this->_boundingVolume;
}

void ImplicitQuadtreeLoader::addSubtreeAvailability(
    const CesiumGeometry::QuadtreeTileID& subtreeID,
    SubtreeAvailability&& subtreeAvailability) {
  uint32_t levelIndex = subtreeID.level / this->_subtreeLevels;
  if (levelIndex >= this->_loadedSubtrees.size()) {
    return;
  }

  uint64_t subtreeMortonID =
      libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y);

  this->_loadedSubtrees[levelIndex].insert_or_assign(
      subtreeMortonID,
      std::move(subtreeAvailability));
}

std::string ImplicitQuadtreeLoader::resolveUrl(
    const std::string& baseUrl,
    const std::string& urlTemplate,
    const CesiumGeometry::QuadtreeTileID& quadtreeID) {
  std::string url = CesiumUtility::Uri::substituteTemplateParameters(
      urlTemplate,
      [&quadtreeID](const std::string& placeholder) {
        if (placeholder == "level") {
          return std::to_string(quadtreeID.level);
        }
        if (placeholder == "x") {
          return std::to_string(quadtreeID.x);
        }
        if (placeholder == "y") {
          return std::to_string(quadtreeID.y);
        }

        return placeholder;
      });

  return CesiumUtility::Uri::resolve(baseUrl, url);
}
} // namespace Cesium3DTilesSelection
