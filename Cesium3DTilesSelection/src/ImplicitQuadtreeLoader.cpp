#include "ImplicitQuadtreeLoader.h"

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
#include <CesiumGeometry/QuadtreeTileID.h>
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
#include <type_traits>
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

  BoundingVolume
  operator()(const CesiumGeospatial::S2CellBoundingVolume& s2Volume) {
    return ImplicitTilingUtilities::computeBoundingVolume(
        s2Volume,
        this->tileID,
        this->ellipsoid);
  }

  BoundingVolume operator()(const CesiumGeometry::OrientedBoundingBox& obb) {
    return ImplicitTilingUtilities::computeBoundingVolume(obb, this->tileID);
  }

  const CesiumGeometry::QuadtreeTileID& tileID;
  const CesiumGeospatial::Ellipsoid& ellipsoid;
};

BoundingVolume subdivideBoundingVolume(
    const CesiumGeometry::QuadtreeTileID& tileID,
    const ImplicitQuadtreeBoundingVolume& rootBoundingVolume,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  return std::visit(
      BoundingVolumeSubdivision{tileID, ellipsoid},
      rootBoundingVolume);
}

std::vector<Tile> populateSubtree(
    const SubtreeAvailability& subtreeAvailability,
    uint32_t subtreeLevels,
    const CesiumGeometry::QuadtreeTileID& subtreeRootID,
    const Tile& tile,
    ImplicitQuadtreeLoader& loader,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  const CesiumGeometry::QuadtreeTileID& quadtreeID =
      std::get<CesiumGeometry::QuadtreeTileID>(tile.getTileID());

  uint32_t relativeTileLevel = quadtreeID.level - subtreeRootID.level;
  if (relativeTileLevel >= subtreeLevels) {
    return {};
  }

  QuadtreeChildren childIDs = ImplicitTilingUtilities::getChildren(quadtreeID);

  std::vector<Tile> children;
  children.reserve(childIDs.size());

  for (const CesiumGeometry::QuadtreeTileID& childID : childIDs) {
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
      .thenInWorkerThread([ellipsoid,
                           pLogger,
                           ktx2TranscodeTargets,
                           applyTextureTransform,
                           &asyncSystem,
                           pAssetAccessor,
                           tileTransform,
                           requestHeaders](
                              std::shared_ptr<CesiumAsync::IAssetRequest>&&
                                  pCompletedRequest) mutable {
        const CesiumAsync::IAssetResponse* pResponse =
            pCompletedRequest->response();
        auto fail = [&]() {
          return asyncSystem.createResolvedFuture(
              TileLoadResult::createFailedResult(
                  pAssetAccessor,
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
                  [ellipsoid,
                   pLogger,
                   tileUrl,
                   pAssetAccessor,
                   pCompletedRequest](GltfConverterResult&& result) mutable {
                    // Report any errors if there are any
                    logTileLoadResult(pLogger, tileUrl, result.errors);
                    if (result.errors || !result.model) {
                      return TileLoadResult::createFailedResult(
                          pAssetAccessor,
                          std::move(pCompletedRequest));
                    }

                    return TileLoadResult{
                        std::move(*result.model),
                        CesiumGeometry::Axis::Y,
                        std::nullopt,
                        std::nullopt,
                        std::nullopt,
                        pAssetAccessor,
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
ImplicitQuadtreeLoader::loadTileContent(const TileLoadInput& loadInput) {
  const auto& tile = loadInput.tile;
  const auto& asyncSystem = loadInput.asyncSystem;
  const auto& pAssetAccessor = loadInput.pAssetAccessor;
  const auto& pLogger = loadInput.pLogger;
  const auto& requestHeaders = loadInput.requestHeaders;
  const auto& contentOptions = loadInput.contentOptions;
  const auto& ellipsoid = loadInput.ellipsoid;

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
        TileLoadResult::createFailedResult(pAssetAccessor, nullptr));
  }

  // find the subtree ID
  CesiumGeometry::QuadtreeTileID subtreeID =
      ImplicitTilingUtilities::getSubtreeRootID(
          this->_subtreeLevels,
          *pQuadtreeID);
  uint32_t subtreeLevelIdx = subtreeID.level / this->_subtreeLevels;
  if (subtreeLevelIdx >= _loadedSubtrees.size()) {
    return asyncSystem.createResolvedFuture<TileLoadResult>(
        TileLoadResult::createFailedResult(pAssetAccessor, nullptr));
  }

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
               ImplicitTileSubdivisionScheme::Quadtree,
               this->_subtreeLevels,
               asyncSystem,
               pAssetAccessor,
               pLogger,
               subtreeUrl,
               requestHeaders)
        .thenInMainThread([this, pAssetAccessor = pAssetAccessor, subtreeID](
                              std::optional<SubtreeAvailability>&&
                                  subtreeAvailability) mutable {
          if (subtreeAvailability) {
            this->addSubtreeAvailability(
                subtreeID,
                std::move(*subtreeAvailability));

            // tell client to retry later
            return TileLoadResult::createRetryLaterResult(
                std::move(pAssetAccessor),
                nullptr);
          } else {
            // Subtree load failed, so this tile fails, too.
            return TileLoadResult::createFailedResult(
                std::move(pAssetAccessor),
                nullptr);
          }
        });
  }

  // subtree is available, so check if tile has content or not. If it has, then
  // request it
  if (!subtreeIt->second.isContentAvailable(subtreeID, *pQuadtreeID, 0)) {
    // check if tile has empty content
    return asyncSystem.createResolvedFuture(TileLoadResult{
        TileEmptyContent{},
        CesiumGeometry::Axis::Y,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        pAssetAccessor,
        nullptr,
        {},
        TileLoadResultState::Success,
        ellipsoid});
  }

  std::string tileUrl = ImplicitTilingUtilities::resolveUrl(
      this->_baseUrl,
      this->_contentUrlTemplate,
      *pQuadtreeID);
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

TileChildrenResult ImplicitQuadtreeLoader::createTileChildren(
    const Tile& tile,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  const CesiumGeometry::QuadtreeTileID* pQuadtreeID =
      std::get_if<CesiumGeometry::QuadtreeTileID>(&tile.getTileID());
  CESIUM_ASSERT(
      pQuadtreeID != nullptr && "This loader only serves quadtree tile");

  // find the subtree ID
  CesiumGeometry::QuadtreeTileID subtreeID =
      ImplicitTilingUtilities::getSubtreeRootID(
          this->_subtreeLevels,
          *pQuadtreeID);

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
      ImplicitTilingUtilities::computeMortonIndex(subtreeID);

  this->_loadedSubtrees[levelIndex].insert_or_assign(
      subtreeMortonID,
      std::move(subtreeAvailability));
}
} // namespace Cesium3DTilesSelection
