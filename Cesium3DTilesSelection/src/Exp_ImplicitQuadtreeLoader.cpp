#include <Cesium3DTilesSelection/Exp_GltfConverters.h>
#include <Cesium3DTilesSelection/Exp_ImplicitQuadtreeLoader.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumUtility/Uri.h>
#include <CesiumUtility/joinToString.h>

#include <libmorton/morton.h>
#include <spdlog/logger.h>

#include <type_traits>
#include <variant>

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
    const CesiumGeometry::QuadtreeTileID& tileID,
    const CesiumGeospatial::BoundingRegion& region) {
  const CesiumGeospatial::GlobeRectangle& globeRect = region.getRectangle();
  size_t denominator = size_t(1) << tileID.level;
  double latSize = (globeRect.getNorth() - globeRect.getSouth()) / denominator;
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

CesiumGeospatial::S2CellBoundingVolume subdivideS2Volume(
    const CesiumGeometry::QuadtreeTileID& tileID,
    const CesiumGeospatial::S2CellBoundingVolume& s2Volume) {
  return CesiumGeospatial::S2CellBoundingVolume(
      CesiumGeospatial::S2CellID::fromQuadtreeTileID(
          s2Volume.getCellID().getFace(),
          tileID),
      s2Volume.getMinimumHeight(),
      s2Volume.getMaximumHeight());
}

CesiumGeometry::OrientedBoundingBox subdivideOrientedBoundingBox(
    const CesiumGeometry::QuadtreeTileID& tileID,
    const CesiumGeometry::OrientedBoundingBox& obb) {
  const glm::dmat3& halfAxes = obb.getHalfAxes();
  const glm::dvec3& center = obb.getCenter();

  size_t denominator = size_t(1) << tileID.level;
  glm::dvec3 min = center - halfAxes[0] - halfAxes[1] - halfAxes[2];

  glm::dvec3 xDim = halfAxes[0] * 2.0 / double(denominator);
  glm::dvec3 yDim = halfAxes[1] * 2.0 / double(denominator);
  glm::dvec3 childMin = min + xDim * double(tileID.x) + yDim * double(tileID.y);
  glm::dvec3 childMax =
      min + xDim * double(tileID.x + 1) + yDim * double(tileID.y + 1) + halfAxes[2] * 2.0;

  return CesiumGeometry::OrientedBoundingBox(
      (childMin + childMax) / 2.0,
      glm::dmat3{xDim / 2.0, yDim / 2.0, halfAxes[2]});
}

BoundingVolume subdivideBoundingVolume(
    const CesiumGeometry::QuadtreeTileID& tileID,
    const ImplicitQuadtreeBoundingVolume& rootBoundingVolume) {
  auto pRegion =
      std::get_if<CesiumGeospatial::BoundingRegion>(&rootBoundingVolume);
  if (pRegion) {
    return subdivideRegion(tileID, *pRegion);
  }

  auto pS2 =
      std::get_if<CesiumGeospatial::S2CellBoundingVolume>(&rootBoundingVolume);
  if (pS2) {
    return subdivideS2Volume(tileID, *pS2);
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
    ImplicitQuadtreeLoader& loader) {
  if (relativeTileLevel >= subtreeLevels) {
    return;
  }

  const CesiumGeometry::QuadtreeTileID* pQuadtreeID =
      std::get_if<CesiumGeometry::QuadtreeTileID>(&tile.getTileID());

  std::vector<Tile> children;
  children.reserve(4);
  for (uint16_t y = 0; y < 2; ++y) {
    uint32_t childY = (pQuadtreeID->y << 1) | y;
    for (uint16_t x = 0; x < 2; ++x) {
      uint32_t childX = (pQuadtreeID->x << 1) | x;
      CesiumGeometry::QuadtreeTileID childID{
          pQuadtreeID->level + 1,
          childX,
          childY};

      uint32_t childIndex = libmorton::morton2D_32_encode(x, y);
      uint64_t relativeChildMortonID = relativeTileMortonID << 2 | childIndex;
      uint32_t relativeChildLevel = relativeTileLevel + 1;
      if (relativeChildLevel == subtreeLevels) {
        if (subtreeAvailability.isSubtreeAvailable(relativeChildMortonID)) {
          Tile& child = children.emplace_back();
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

  tile.createChildTiles(std::move(children));
}

struct SubtreeContentInitializer {
public:
  SubtreeContentInitializer(
      ImplicitQuadtreeLoader* pImplicitLoader,
      const CesiumGeometry::QuadtreeTileID& subtreeID)
      : subtreeAvailability{std::nullopt},
        subtreeID{subtreeID},
        pLoader{pImplicitLoader} {}

  void operator()(Tile& tile) {
    uint32_t subtreeLevels = pLoader->getSubtreeLevels();
    const CesiumGeometry::QuadtreeTileID* pQuadtreeID =
        std::get_if<CesiumGeometry::QuadtreeTileID>(&tile.getTileID());

    if (pQuadtreeID) {
      // ensure that this tile is a root of a subtree
      if (pQuadtreeID->level % subtreeLevels == 0) {
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
  CesiumGeometry::QuadtreeTileID subtreeID;
  ImplicitQuadtreeLoader* pLoader;
};

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

CesiumAsync::Future<TileLoadResult> ImplicitQuadtreeLoader::loadTileContent(
    [[maybe_unused]] TilesetContentLoader& currentLoader,
    const TileContentLoadInfo& loadInfo,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders) {
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
      std::get_if<CesiumGeometry::QuadtreeTileID>(&loadInfo.tileID);
  if (!pQuadtreeID) {
    return loadInfo.asyncSystem.createResolvedFuture<TileLoadResult>(
        TileLoadResult{
            TileUnknownContent{},
            TileLoadResultState::Failed,
            nullptr,
            {}});
  }

  // find the subtree ID
  uint32_t subtreeLevelIdx = pQuadtreeID->level / _subtreeLevels;
  if (subtreeLevelIdx >= _loadedSubtrees.size()) {
    return loadInfo.asyncSystem.createResolvedFuture<TileLoadResult>(
        TileLoadResult{
            TileUnknownContent{},
            TileLoadResultState::Failed,
            nullptr,
            {}});
  }

  uint64_t levelLeft = pQuadtreeID->level % _subtreeLevels;
  uint32_t subtreeLevel = _subtreeLevels * subtreeLevelIdx;
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
  auto subtreeIt = _loadedSubtrees[subtreeLevelIdx].find(subtreeMortonIdx);
  if (subtreeIt == _loadedSubtrees[subtreeLevelIdx].end()) {
    // subtree is not loaded, so load it now.
    std::string subtreeUrl =
        resolveUrl(_baseUrl, _subtreeUrlTemplate, subtreeID);
    std::string tileUrl =
        resolveUrl(_baseUrl, _contentUrlTemplate, *pQuadtreeID);
    CesiumGltf::Ktx2TranscodeTargets ktx2TranscodeTargets =
        loadInfo.contentOptions.ktx2TranscodeTargets;

    SubtreeContentInitializer subtreeInitializer{this, subtreeID};
    return SubtreeAvailability::loadSubtree(
               4,
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
             quadtreeID = *pQuadtreeID,
             requestHeaders,
             ktx2TranscodeTargets,
             subtreeInitializer = std::move(subtreeInitializer)](
                std::optional<SubtreeAvailability>&&
                    subtreeAvailability) mutable {
              if (subtreeAvailability) {
                bool tileHasContent = isTileContentAvailable(
                    subtreeID,
                    quadtreeID,
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
  if (!isTileContentAvailable(subtreeID, *pQuadtreeID, subtreeIt->second)) {
    // check if tile has empty content
    return loadInfo.asyncSystem.createResolvedFuture(TileLoadResult{
        TileEmptyContent{},
        TileLoadResultState::Success,
        nullptr,
        {}});
  }

  std::string tileUrl = resolveUrl(_baseUrl, _contentUrlTemplate, *pQuadtreeID);
  return requestTileContent(
      loadInfo.pLogger,
      loadInfo.asyncSystem,
      loadInfo.pAssetAccessor,
      tileUrl,
      requestHeaders,
      loadInfo.contentOptions.ktx2TranscodeTargets,
      std::nullopt);
}

uint32_t ImplicitQuadtreeLoader::getSubtreeLevels() const noexcept {
  return _subtreeLevels;
}

uint32_t ImplicitQuadtreeLoader::getAvailableLevels() const noexcept {
  return _availableLevels;
}

const ImplicitQuadtreeBoundingVolume&
ImplicitQuadtreeLoader::getBoundingVolume() const noexcept {
  return _boundingVolume;
}

void ImplicitQuadtreeLoader::addSubtreeAvailability(
    const CesiumGeometry::QuadtreeTileID& subtreeID,
    SubtreeAvailability&& subtreeAvailability) {
  uint32_t levelIndex = subtreeID.level / _subtreeLevels;
  if (levelIndex >= _loadedSubtrees.size()) {
    return;
  }

  uint64_t subtreeMortonID =
      libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y);

  _loadedSubtrees[levelIndex].insert_or_assign(
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
