#include <Cesium3DTilesSelection/Exp_ImplicitQuadtreeLoader.h>
#include <Cesium3DTilesSelection/Exp_GltfConverters.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumUtility/Uri.h>
#include <libmorton/morton.h>
#include <variant>
#include <type_traits>

namespace Cesium3DTilesSelection {
namespace {
CesiumAsync::Future<TileLoadResult> requestTileContent(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::string& tileUrl,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
    const CesiumGeometry::QuadtreeTileID& subtreeID,
    const CesiumGeometry::QuadtreeTileID& quadtreeID,
    const SubtreeAvailability& subtreeAvailability,
    CesiumGltf::Ktx2TranscodeTargets ktx2TranscodeTargets) {
  uint32_t relativeTileLevel = quadtreeID.level - subtreeID.level;
  uint64_t relativeTileMortonIdx = libmorton::morton2D_64_encode(
      quadtreeID.x - subtreeID.x << relativeTileLevel,
      quadtreeID.y - subtreeID.y << relativeTileLevel);

  // check if tile has empty content
  if (!subtreeAvailability.isContentAvailable(relativeTileLevel, relativeTileMortonIdx, 0)) {
    return asyncSystem.createResolvedFuture(TileLoadResult{
        TileUnknownContent{},
        TileLoadResultState::Success,
        nullptr,
        {}});
  }

  return pAssetAccessor->get(asyncSystem, tileUrl, requestHeaders)
      .thenInWorkerThread(
          [ktx2TranscodeTargets](
              std::shared_ptr<CesiumAsync::IAssetRequest>&& pCompletedRequest) {
            const CesiumAsync::IAssetResponse* pResponse =
                pCompletedRequest->response();
            if (!pResponse) {
              return TileLoadResult{
                  TileUnknownContent{},
                  TileLoadResultState::Failed,
                  nullptr,
                  {}};
            }

            uint16_t statusCode = pResponse->statusCode();
            if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
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
              if (result.errors) {
                return TileLoadResult{
                    TileRenderContent{std::nullopt},
                    TileLoadResultState::Failed,
                    std::move(pCompletedRequest),
                    {}};
              }

              return TileLoadResult{
                  TileRenderContent{std::move(result.model)},
                  TileLoadResultState::Success,
                  std::move(pCompletedRequest),
                  {}};
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

  // check that the subtree for this tile is loaded. If not, we load the
  // subtree, then load the tile
  size_t subtreeLevelIdx = pQuadtreeID->level / _subtreeLevels;
  if (subtreeLevelIdx >= _loadedSubtrees.size()) {
    return loadInfo.asyncSystem.createResolvedFuture<TileLoadResult>(
        TileLoadResult{
            TileUnknownContent{},
            TileLoadResultState::Failed,
            nullptr,
            {}});
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
  uint64_t levelLeft = pQuadtreeID->level % _subtreeLevels;
  uint32_t subtreeLevel = _subtreeLevels * subtreeLevelIdx;
  uint32_t subtreeX = pQuadtreeID->x >> levelLeft;
  uint32_t subtreeY = pQuadtreeID->y >> levelLeft;
  CesiumGeometry::QuadtreeTileID subtreeID{subtreeLevel, subtreeX, subtreeY};

  uint64_t subtreeMortonIdx = libmorton::morton2D_64_encode(subtreeX, subtreeY);
  auto subtreeIt = _loadedSubtrees[subtreeLevelIdx].find(subtreeMortonIdx);
  if (subtreeIt == _loadedSubtrees[subtreeLevelIdx].end()) {
    // the subtree is not loaded, so load it now
    std::string subtreeUrl =
        resolveUrl(_baseUrl, _subtreeUrlTemplate, *pQuadtreeID);
    std::string tileUrl =
        resolveUrl(_baseUrl, _contentUrlTemplate, *pQuadtreeID);
    return SubtreeAvailability::loadSubtree(
               4,
               loadInfo.asyncSystem,
               loadInfo.pAssetAccessor,
               loadInfo.pLogger,
               subtreeUrl,
               requestHeaders)
        .thenInWorkerThread(
            [asyncSystem = loadInfo.asyncSystem,
             pAssetAccessor = loadInfo.pAssetAccessor,
             tileUrl = std::move(tileUrl),
             subtreeID,
             quadtreeID = *pQuadtreeID,
             requestHeaders,
             ktx2TranscodeTargets =
                 loadInfo.contentOptions.ktx2TranscodeTargets](
                std::optional<SubtreeAvailability>&& subtreeAvailability) {
              if (subtreeAvailability) {
                return requestTileContent(
                    asyncSystem,
                    pAssetAccessor,
                    tileUrl,
                    requestHeaders,
                    subtreeID,
                    quadtreeID,
                    *subtreeAvailability,
                    ktx2TranscodeTargets);
              }

              return asyncSystem.createResolvedFuture<TileLoadResult>(
                  TileLoadResult{
                      TileUnknownContent{},
                      TileLoadResultState::Failed,
                      nullptr,
                      {}});
            });
  }

  std::string tileUrl = resolveUrl(_baseUrl, _contentUrlTemplate, *pQuadtreeID);
  return requestTileContent(
      loadInfo.asyncSystem,
      loadInfo.pAssetAccessor,
      tileUrl,
      requestHeaders,
      subtreeID,
      *pQuadtreeID,
      subtreeIt->second,
      loadInfo.contentOptions.ktx2TranscodeTargets);
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
