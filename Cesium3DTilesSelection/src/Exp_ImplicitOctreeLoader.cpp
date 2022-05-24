#include <Cesium3DTilesSelection/Exp_GltfConverters.h>
#include <Cesium3DTilesSelection/Exp_ImplicitOctreeLoader.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/Uri.h>
#include <CesiumUtility/joinToString.h>

#include <libmorton/morton.h>
#include <spdlog/logger.h>

namespace Cesium3DTilesSelection {
namespace {
void logErrors(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& url,
    const std::vector<std::string>& errors) {
  if (!errors.empty()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Failed to load {}:\n- {}",
        url,
        CesiumUtility::joinToString(errors, "\n- "));
  }
}

void logWarnings(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& url,
    const std::vector<std::string>& warnings) {
  if (!warnings.empty()) {
    SPDLOG_LOGGER_WARN(
        pLogger,
        "Warning when loading {}:\n- {}",
        url,
        CesiumUtility::joinToString(warnings, "\n- "));
  }
}

void logErrorsAndWarnings(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& url,
    const ErrorList& errorLists) {
  logErrors(pLogger, url, errorLists.errors);
  logWarnings(pLogger, url, errorLists.warnings);
}

CesiumGeospatial::BoundingRegion subdivideRegion(
    const CesiumGeometry::OctreeTileID& tileID,
    const CesiumGeospatial::BoundingRegion& region) {
  const CesiumGeospatial::GlobeRectangle& globeRect = region.getRectangle();
  size_t denominator = size_t(1) << tileID.level;
  double latSize = (globeRect.getNorth() - globeRect.getSouth()) / denominator;
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

CesiumGeometry::OrientedBoundingBox subdivideOrientedBoundingBox(
    const CesiumGeometry::OctreeTileID& tileID,
    const CesiumGeometry::OrientedBoundingBox& obb) {
  const glm::dmat3& halfAxes = obb.getHalfAxes();
  const glm::dvec3& center = obb.getCenter();

  size_t denominator = size_t(1) << tileID.level;
  glm::dvec3 min = center - halfAxes[0] - halfAxes[1] - halfAxes[2];

  glm::dvec3 xDim = halfAxes[0] * 2.0 / double(denominator);
  glm::dvec3 yDim = halfAxes[1] * 2.0 / double(denominator);
  glm::dvec3 zDim = halfAxes[2] * 2.0 / double(denominator);
  glm::dvec3 childMin = min + xDim * double(tileID.x) +
                        yDim * double(tileID.y) + zDim * double(tileID.z);
  glm::dvec3 childMax = min + xDim * double(tileID.x + 1) +
                        yDim * double(tileID.y + 1) +
                        zDim * double(tileID.z + 1);

  return CesiumGeometry::OrientedBoundingBox(
      (childMin + childMax) / 2.0,
      glm::dmat3{xDim / 2.0, yDim / 2.0, zDim / 2.0});
}

BoundingVolume subdivideBoundingVolume(
    const CesiumGeometry::OctreeTileID& tileID,
    const ImplicitOctreeBoundingVolume& rootBoundingVolume) {
  auto pRegion =
      std::get_if<CesiumGeospatial::BoundingRegion>(&rootBoundingVolume);
  if (pRegion) {
    return subdivideRegion(tileID, *pRegion);
  }

  auto pOBB =
      std::get_if<CesiumGeometry::OrientedBoundingBox>(&rootBoundingVolume);
  return subdivideOrientedBoundingBox(tileID, *pOBB);
}

void populateSubtree(
    const SubtreeAvailability& subtreeAvailability,
    uint32_t subtreeLevels,
    uint32_t relativeTileLevel,
    uint64_t relativeTileMortonID,
    Tile& tile,
    ImplicitOctreeLoader& loader) {
  if (relativeTileLevel >= subtreeLevels) {
    return;
  }

  const CesiumGeometry::OctreeTileID* pOctreeID =
      std::get_if<CesiumGeometry::OctreeTileID>(&tile.getTileID());

  std::vector<Tile> children;
  children.reserve(8);
  for (uint16_t y = 0; y < 2; ++y) {
    uint32_t childY = (pOctreeID->y << 1) | y;
    for (uint16_t z = 0; z < 2; ++z) {
      uint32_t childZ = (pOctreeID->z << 1) | z;
      for (uint16_t x = 0; x < 2; ++x) {
        uint32_t childX = (pOctreeID->x << 1) | x;

        CesiumGeometry::OctreeTileID childID{
            pOctreeID->level + 1,
            childX,
            childY,
            childZ};

        uint32_t childIndex = libmorton::morton3D_32_encode(x, y, z);
        uint64_t relativeChildMortonID = relativeTileMortonID << 3 | childIndex;
        uint32_t relativeChildLevel = relativeTileLevel + 1;
        if (relativeChildLevel == subtreeLevels) {
          if (subtreeAvailability.isSubtreeAvailable(relativeChildMortonID)) {
            Tile& child = children.emplace_back();
            child.setTransform(tile.getTransform());
            child.setBoundingVolume(
                subdivideBoundingVolume(childID, loader.getBoundingVolume()));
            child.setGeometricError(tile.getGeometricError() * 0.5);
            child.setRefine(tile.getRefine());
            child.setTileID(childID);
            child.setContent(std::make_unique<TileContent>(&loader));
          }
        } else {
          if (subtreeAvailability.isTileAvailable(
                  relativeChildLevel,
                  relativeChildMortonID)) {
            Tile& child = children.emplace_back();
            child.setTransform(tile.getTransform());
            child.setBoundingVolume(
                subdivideBoundingVolume(childID, loader.getBoundingVolume()));
            child.setGeometricError(tile.getGeometricError() * 0.5);
            child.setRefine(tile.getRefine());
            child.setTileID(childID);

            if (subtreeAvailability.isContentAvailable(
                    relativeChildLevel,
                    relativeChildMortonID,
                    0)) {
              child.setContent(std::make_unique<TileContent>(&loader));
            } else {
              child.setContent(
                  std::make_unique<TileContent>(&loader, TileEmptyContent{}));
            }

            populateSubtree(
                subtreeAvailability,
                subtreeLevels,
                relativeChildLevel,
                relativeChildMortonID,
                child,
                loader);
          }
        }
      }
    }
  }

  tile.createChildTiles(std::move(children));
}

struct SubtreeContentInitializer {
public:
  SubtreeContentInitializer(
      ImplicitOctreeLoader* pImplicitLoader,
      const CesiumGeometry::OctreeTileID& subtreeID)
      : subtreeAvailability{std::nullopt},
        subtreeID{subtreeID},
        pLoader{pImplicitLoader} {}

  void operator()(Tile& tile) {
    uint32_t subtreeLevels = pLoader->getSubtreeLevels();
    const CesiumGeometry::OctreeTileID* pOctreeID =
        std::get_if<CesiumGeometry::OctreeTileID>(&tile.getTileID());

    if (pOctreeID) {
      // ensure that this tile is a root of a subtree
      if (pOctreeID->level % subtreeLevels == 0) {
        if (tile.getChildren().empty()) {
          populateSubtree(
              *subtreeAvailability,
              subtreeLevels,
              0,
              0,
              tile,
              *pLoader);
        }
      }

      pLoader->addSubtreeAvailability(
          subtreeID,
          std::move(*subtreeAvailability));
    }
  }

  std::optional<SubtreeAvailability> subtreeAvailability;
  CesiumGeometry::OctreeTileID subtreeID;
  ImplicitOctreeLoader* pLoader;
};

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
    CesiumGltf::Ktx2TranscodeTargets ktx2TranscodeTargets,
    std::optional<SubtreeContentInitializer>&& subtreeInitializer) {
  return pAssetAccessor->get(asyncSystem, tileUrl, requestHeaders)
      .thenInWorkerThread([pLogger,
                           ktx2TranscodeTargets,
                           subtreeInitializer = std::move(subtreeInitializer)](
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
          return TileLoadResult{
              TileUnknownContent{},
              TileLoadResultState::Failed,
              nullptr,
              {}};
        }

        uint16_t statusCode = pResponse->statusCode();
        if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
          SPDLOG_LOGGER_ERROR(
              pLogger,
              "Received status code {} for tile content {}",
              statusCode,
              tileUrl);
          return TileLoadResult{
              TileUnknownContent{},
              TileLoadResultState::Failed,
              nullptr,
              {}};
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
          logErrorsAndWarnings(pLogger, tileUrl, result.errors);
          if (result.errors) {
            return TileLoadResult{
                TileRenderContent{std::nullopt},
                TileLoadResultState::Failed,
                std::move(pCompletedRequest),
                {}};
          }

          std::function<void(Tile&)> tileInitializer;
          if (subtreeInitializer) {
            tileInitializer = std::move(*subtreeInitializer);
          }
          return TileLoadResult{
              TileRenderContent{std::move(result.model)},
              TileLoadResultState::Success,
              std::move(pCompletedRequest),
              std::move(tileInitializer)};
        }

        // content type is not supported
        return TileLoadResult{
            TileRenderContent{std::nullopt},
            TileLoadResultState::Failed,
            std::move(pCompletedRequest),
            {}};
      });
}
} // namespace

CesiumAsync::Future<TileLoadResult> ImplicitOctreeLoader::loadTileContent(
    [[maybe_unused]] TilesetContentLoader& currentLoader,
    const TileContentLoadInfo& loadInfo,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders) {
  // make sure the tile is a octree tile
  const CesiumGeometry::OctreeTileID* pOctreeID =
      std::get_if<CesiumGeometry::OctreeTileID>(&loadInfo.tileID);
  if (!pOctreeID) {
    return loadInfo.asyncSystem.createResolvedFuture<TileLoadResult>(
        TileLoadResult{
            TileUnknownContent{},
            TileLoadResultState::Failed,
            nullptr,
            {}});
  }

  // find the subtree ID
  uint32_t subtreeLevelIdx = pOctreeID->level / _subtreeLevels;
  if (subtreeLevelIdx >= _loadedSubtrees.size()) {
    return loadInfo.asyncSystem.createResolvedFuture<TileLoadResult>(
        TileLoadResult{
            TileUnknownContent{},
            TileLoadResultState::Failed,
            nullptr,
            {}});
  }

  uint64_t levelLeft = pOctreeID->level % _subtreeLevels;
  uint32_t subtreeLevel = _subtreeLevels * subtreeLevelIdx;
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
  auto subtreeIt = _loadedSubtrees[subtreeLevelIdx].find(subtreeMortonIdx);
  if (subtreeIt == _loadedSubtrees[subtreeLevelIdx].end()) {
    // subtree is not loaded, so load it now.
    std::string subtreeUrl =
        resolveUrl(_baseUrl, _subtreeUrlTemplate, subtreeID);
    std::string tileUrl = resolveUrl(_baseUrl, _contentUrlTemplate, *pOctreeID);
    CesiumGltf::Ktx2TranscodeTargets ktx2TranscodeTargets =
        loadInfo.contentOptions.ktx2TranscodeTargets;

    SubtreeContentInitializer subtreeInitializer{this, subtreeID};
    return SubtreeAvailability::loadSubtree(
               3,
               loadInfo.asyncSystem,
               loadInfo.pAssetAccessor,
               loadInfo.pLogger,
               subtreeUrl,
               requestHeaders)
        .thenInWorkerThread(
            [pLogger = loadInfo.pLogger,
             asyncSystem = loadInfo.asyncSystem,
             pAssetAccessor = loadInfo.pAssetAccessor,
             tileUrl = std::move(tileUrl),
             subtreeID,
             octreeID = *pOctreeID,
             requestHeaders,
             ktx2TranscodeTargets,
             subtreeInitializer = std::move(subtreeInitializer)](
                std::optional<SubtreeAvailability>&&
                    subtreeAvailability) mutable {
              if (subtreeAvailability) {
                bool tileHasContent = isTileContentAvailable(
                    subtreeID,
                    octreeID,
                    *subtreeAvailability);

                subtreeInitializer.subtreeAvailability =
                    std::move(*subtreeAvailability);

                // subtree is available, so check if tile has content or not. If
                // it has, then request it
                if (!tileHasContent) {
                  // check if tile has empty content
                  return asyncSystem.createResolvedFuture(TileLoadResult{
                      TileEmptyContent{},
                      TileLoadResultState::Success,
                      nullptr,
                      std::move(subtreeInitializer)});
                }

                return requestTileContent(
                    pLogger,
                    asyncSystem,
                    pAssetAccessor,
                    tileUrl,
                    requestHeaders,
                    ktx2TranscodeTargets,
                    std::move(subtreeInitializer));
              }

              return asyncSystem.createResolvedFuture<TileLoadResult>(
                  TileLoadResult{
                      TileUnknownContent{},
                      TileLoadResultState::Failed,
                      nullptr,
                      {}});
            });
  }

  // subtree is available, so check if tile has content or not. If it has, then
  // request it
  if (!isTileContentAvailable(subtreeID, *pOctreeID, subtreeIt->second)) {
    // check if tile has empty content
    return loadInfo.asyncSystem.createResolvedFuture(TileLoadResult{
        TileEmptyContent{},
        TileLoadResultState::Success,
        nullptr,
        {}});
  }

  std::string tileUrl = resolveUrl(_baseUrl, _contentUrlTemplate, *pOctreeID);
  return requestTileContent(
      loadInfo.pLogger,
      loadInfo.asyncSystem,
      loadInfo.pAssetAccessor,
      tileUrl,
      requestHeaders,
      loadInfo.contentOptions.ktx2TranscodeTargets,
      std::nullopt);
}

uint32_t ImplicitOctreeLoader::getSubtreeLevels() const noexcept {
  return _subtreeLevels;
}

uint32_t ImplicitOctreeLoader::getAvailableLevels() const noexcept {
  return _availableLevels;
}

const ImplicitOctreeBoundingVolume&
ImplicitOctreeLoader::getBoundingVolume() const noexcept {
  return _boundingVolume;
}

void ImplicitOctreeLoader::addSubtreeAvailability(
    const CesiumGeometry::OctreeTileID& subtreeID,
    SubtreeAvailability&& subtreeAvailability) {
  uint32_t levelIndex = subtreeID.level / _subtreeLevels;
  if (levelIndex >= _loadedSubtrees.size()) {
    return;
  }

  uint64_t subtreeMortonID =
      libmorton::morton3D_64_encode(subtreeID.x, subtreeID.y, subtreeID.z);

  _loadedSubtrees[levelIndex].insert_or_assign(
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
