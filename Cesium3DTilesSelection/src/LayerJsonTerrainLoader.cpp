#include "LayerJsonTerrainLoader.h"

#include "TilesetContentLoaderResult.h"

#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTileRectangularRange.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/BoundingRegionWithLooseFittingHeights.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumGeospatial/calcQuadtreeMaxGeometricError.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumQuantizedMeshTerrain/QuantizedMeshLoader.h>
#include <CesiumRasterOverlays/RasterOverlayUtilities.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Uri.h>

#include <fmt/format.h>
#include <glm/ext/matrix_double4x4.hpp>
#include <libmorton/morton.h>
#include <rapidjson/document.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace CesiumAsync;
using namespace Cesium3DTilesSelection;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumQuantizedMeshTerrain;
using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace {
struct LoadLayersResult {
  std::optional<QuadtreeTilingScheme> tilingScheme;
  std::optional<Projection> projection;
  std::optional<BoundingVolume> boundingVolume;
  std::vector<LayerJsonTerrainLoader::Layer> layers;
  std::vector<std::string> layerCredits;
  ErrorList errors;
  uint16_t statusCode{200};
};

/**
 * @brief Creates a default {@link BoundingRegionWithLooseFittingHeights} for
 * the given rectangle.
 *
 * The heights of this bounding volume will have unspecified default values
 * that are suitable for the use on earth.
 *
 * @param globeRectangle The {@link CesiumGeospatial::GlobeRectangle}
 * @return The {@link BoundingRegionWithLooseFittingHeights}
 */
BoundingVolume createDefaultLooseEarthBoundingVolume(
    const CesiumGeospatial::GlobeRectangle& globeRectangle,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  return BoundingRegionWithLooseFittingHeights(
      BoundingRegion(globeRectangle, -1000.0, 9000.0, ellipsoid));
}

TileLoadResult convertToTileLoadResult(
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    QuantizedMeshLoadResult&& loadResult,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  if (loadResult.errors || !loadResult.model) {
    return TileLoadResult::createFailedResult(
        pAssetAccessor,
        std::move(loadResult.pRequest));
  }

  return TileLoadResult{
      std::move(*loadResult.model),
      CesiumGeometry::Axis::Y,
      loadResult.updatedBoundingVolume,
      std::nullopt,
      std::nullopt,
      pAssetAccessor,
      nullptr,
      {},
      TileLoadResultState::Success,
      ellipsoid};
}

TilesetContentLoaderResult<LayerJsonTerrainLoader>
convertToTilesetContentLoaderResult(
    const Ellipsoid& ellipsoid,
    LoadLayersResult&& loadLayersResult) {
  if (loadLayersResult.errors) {
    TilesetContentLoaderResult<LayerJsonTerrainLoader> result;
    result.errors = std::move(loadLayersResult.errors);
    result.statusCode = loadLayersResult.statusCode;
    return result;
  }

  if (!loadLayersResult.tilingScheme || !loadLayersResult.projection ||
      !loadLayersResult.boundingVolume) {
    TilesetContentLoaderResult<LayerJsonTerrainLoader> result;
    result.errors.emplaceError(
        "Could not deduce tiling scheme, projection, or bounding volume "
        "from layer.json.");
    return result;
  }

  auto pLoader = std::make_unique<LayerJsonTerrainLoader>(
      *loadLayersResult.tilingScheme,
      *loadLayersResult.projection,
      std::move(loadLayersResult.layers));

  std::unique_ptr<Tile> pRootTile =
      std::make_unique<Tile>(pLoader.get(), TileEmptyContent());
  pRootTile->setUnconditionallyRefine();
  pRootTile->setBoundingVolume(*loadLayersResult.boundingVolume);

  uint32_t quadtreeXTiles = loadLayersResult.tilingScheme->getRootTilesX();

  std::vector<Tile> childTiles;
  childTiles.reserve(quadtreeXTiles);

  for (uint32_t i = 0; i < quadtreeXTiles; ++i) {
    Tile& childTile = childTiles.emplace_back(pLoader.get());

    QuadtreeTileID id(0, i, 0);
    childTile.setTileID(id);

    const CesiumGeospatial::GlobeRectangle childGlobeRectangle =
        unprojectRectangleSimple(
            *loadLayersResult.projection,
            loadLayersResult.tilingScheme->tileToRectangle(id));
    childTile.setBoundingVolume(
        createDefaultLooseEarthBoundingVolume(childGlobeRectangle, ellipsoid));
    childTile.setGeometricError(
        8.0 * calcQuadtreeMaxGeometricError(ellipsoid) *
        childGlobeRectangle.computeWidth());
  }

  pRootTile->createChildTiles(std::move(childTiles));

  // add credits
  std::vector<LoaderCreditResult> credits;
  credits.reserve(loadLayersResult.layerCredits.size());
  for (auto& credit : loadLayersResult.layerCredits) {
    credits.emplace_back(LoaderCreditResult{std::move(credit), true});
  }

  return TilesetContentLoaderResult<LayerJsonTerrainLoader>{
      std::move(pLoader),
      std::move(pRootTile),
      std::move(credits),
      std::vector<IAssetAccessor::THeader>{},
      std::move(loadLayersResult.errors)};
}

size_t maxSubtreeInLayer(uint32_t maxZooms, int32_t availabilityLevels) {
  if (availabilityLevels > 0) {
    return static_cast<size_t>(std::ceil(
        static_cast<float>(maxZooms) / static_cast<float>(availabilityLevels)));
  }

  return 0;
}

void subtreeHash(
    const CesiumGeometry::QuadtreeTileID& subtreeID,
    const LayerJsonTerrainLoader::Layer& layer,
    uint32_t& subtreeLevelIdx,
    uint64_t& subtreeMortonIdx) {
  subtreeLevelIdx = subtreeID.level / uint32_t(layer.availabilityLevels);
  subtreeMortonIdx = libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y);
}

bool isSubtreeLoadedInLayer(
    const CesiumGeometry::QuadtreeTileID& subtreeID,
    const LayerJsonTerrainLoader::Layer& layer) {
  CESIUM_ASSERT(
      layer.availabilityLevels > 0 &&
      "Layer needs to support availabilityLevels");

  uint32_t subtreeLevelIdx;
  uint64_t subtreeMortonIdx;
  subtreeHash(subtreeID, layer, subtreeLevelIdx, subtreeMortonIdx);

  // it doesn't have the subtree exceeds max zooms, so just treat it
  // as loaded
  if (subtreeLevelIdx >= layer.loadedSubtrees.size()) {
    return true;
  }

  return layer.loadedSubtrees[subtreeLevelIdx].find(subtreeMortonIdx) !=
         layer.loadedSubtrees[subtreeLevelIdx].end();
}

void addRectangleAvailabilityToLayer(
    LayerJsonTerrainLoader::Layer& layer,
    const QuadtreeTileID& subtreeID,
    const std::vector<CesiumGeometry::QuadtreeTileRectangularRange>&
        rectangleAvailabilities) {
  for (const QuadtreeTileRectangularRange& range : rectangleAvailabilities) {
    layer.contentAvailability.addAvailableTileRange(range);
  }

  uint32_t subtreeLevelIdx;
  uint64_t subtreeMortonIdx;
  subtreeHash(subtreeID, layer, subtreeLevelIdx, subtreeMortonIdx);

  // it doesn't have the subtree exceeds max zooms, so just treat it
  // as loaded
  if (subtreeLevelIdx >= layer.loadedSubtrees.size()) {
    return;
  }

  layer.loadedSubtrees[subtreeLevelIdx].insert(subtreeMortonIdx);
}

void generateRasterOverlayUVs(
    const BoundingVolume& tileBoundingVolume,
    const glm::dmat4& tileTransform,
    const Projection& projection,
    TileLoadResult& result) {
  if (result.state != TileLoadResultState::Success) {
    return;
  }

  const BoundingRegion* pParentRegion = nullptr;
  if (result.updatedBoundingVolume) {
    pParentRegion =
        std::get_if<BoundingRegion>(&result.updatedBoundingVolume.value());
  } else {
    pParentRegion = std::get_if<BoundingRegion>(&tileBoundingVolume);
  }

  CesiumGltf::Model* pModel =
      std::get_if<CesiumGltf::Model>(&result.contentKind);
  if (pModel) {
    result.rasterOverlayDetails =
        RasterOverlayUtilities::createRasterOverlayTextureCoordinates(
            *pModel,
            tileTransform,
            pParentRegion ? std::make_optional<GlobeRectangle>(
                                pParentRegion->getRectangle())
                          : std::nullopt,
            {projection},
            false);
  }
}

/**
 * @brief Creates the query parameter string for the extensions in the given
 * list.
 *
 * This will check for the presence of all known extensions in the given list,
 * and create a string that can be appended as the value of the `extensions`
 * query parameter to the request URL.
 *
 * @param extensions The layer JSON
 * @return The extensions (possibly the empty string)
 */
std::string createExtensionsQueryParameter(
    const std::vector<std::string>& knownExtensions,
    const std::vector<std::string>& extensions) {

  std::string extensionsToRequest;
  for (const std::string& extension : knownExtensions) {
    if (std::find(extensions.begin(), extensions.end(), extension) !=
        extensions.end()) {
      if (!extensionsToRequest.empty()) {
        extensionsToRequest += "-";
      }
      extensionsToRequest += extension;
    }
  }
  return extensionsToRequest;
}

Future<LoadLayersResult> loadLayersRecursive(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::string& baseUrl,
    const std::vector<IAssetAccessor::THeader>& requestHeaders,
    const rapidjson::Document& layerJson,
    const QuadtreeTilingScheme& tilingScheme,
    bool useWaterMask,
    LoadLayersResult&& loadLayersResult) {
  std::string version;
  const auto tilesetVersionIt = layerJson.FindMember("version");
  if (tilesetVersionIt != layerJson.MemberEnd() &&
      tilesetVersionIt->value.IsString()) {
    version = tilesetVersionIt->value.GetString();
  }

  std::vector<std::string> urls = JsonHelpers::getStrings(layerJson, "tiles");
  if (urls.empty()) {
    loadLayersResult.errors.emplaceError(
        "Layer Json does not specify any tile URL templates");
    return asyncSystem.createResolvedFuture(std::move(loadLayersResult));
  }

  int32_t maxZoom = JsonHelpers::getInt32OrDefault(layerJson, "maxzoom", 30);

  std::vector<std::string> extensions =
      JsonHelpers::getStrings(layerJson, "extensions");

  // Request normals, watermask, and metadata if they're available
  std::vector<std::string> knownExtensions = {"octvertexnormals", "metadata"};

  if (useWaterMask) {
    knownExtensions.emplace_back("watermask");
  }

  std::string extensionsToRequest =
      createExtensionsQueryParameter(knownExtensions, extensions);

  const auto availabilityLevelsIt =
      layerJson.FindMember("metadataAvailability");

  QuadtreeRectangleAvailability availability(tilingScheme, uint32_t(maxZoom));

  int32_t availabilityLevels = -1;

  if (availabilityLevelsIt != layerJson.MemberEnd() &&
      availabilityLevelsIt->value.IsInt()) {
    availabilityLevels = availabilityLevelsIt->value.GetInt();
  } else {
    QuantizedMeshMetadataResult metadata =
        QuantizedMeshLoader::loadAvailabilityRectangles(layerJson, 0);
    loadLayersResult.errors.merge(metadata.errors);

    for (const auto& rectangle : metadata.availability) {
      availability.addAvailableTileRange(rectangle);
    }
  }

  const auto attributionIt = layerJson.FindMember("attribution");

  std::string creditString;
  if (attributionIt != layerJson.MemberEnd() &&
      attributionIt->value.IsString()) {
    creditString = attributionIt->value.GetString();
  }

  loadLayersResult.layers.emplace_back(
      baseUrl,
      std::move(version),
      std::move(urls),
      std::move(extensionsToRequest),
      std::move(availability),
      static_cast<uint32_t>(maxZoom),
      availabilityLevels);

  loadLayersResult.layerCredits.emplace_back(std::move(creditString));

  std::string parentUrl =
      JsonHelpers::getStringOrDefault(layerJson, "parentUrl", "");

  if (!parentUrl.empty()) {
    std::string resolvedUrl = Uri::resolve(baseUrl, parentUrl);
    // append forward slash if necessary
    if (resolvedUrl[resolvedUrl.size() - 1] != '/') {
      resolvedUrl += '/';
    }
    resolvedUrl += "layer.json";

    return pAssetAccessor->get(asyncSystem, resolvedUrl, requestHeaders)
        .thenInWorkerThread(
            [asyncSystem,
             pAssetAccessor,
             tilingScheme,
             useWaterMask,
             loadLayersResult = std::move(loadLayersResult)](
                std::shared_ptr<IAssetRequest>&& pCompletedRequest) mutable {
              const CesiumAsync::IAssetResponse* pResponse =
                  pCompletedRequest->response();
              const std::string& tileUrl = pCompletedRequest->url();
              if (!pResponse) {
                loadLayersResult.errors.emplaceWarning(fmt::format(
                    "Did not receive a valid response for parent layer {}",
                    pCompletedRequest->url()));
                return asyncSystem.createResolvedFuture(
                    std::move(loadLayersResult));
              }

              uint16_t statusCode = pResponse->statusCode();
              if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
                loadLayersResult.errors.emplaceWarning(fmt::format(
                    "Received status code {} for parent layer {}",
                    statusCode,
                    tileUrl));
                return asyncSystem.createResolvedFuture(
                    std::move(loadLayersResult));
              }

              std::span<const std::byte> layerJsonBinary = pResponse->data();

              rapidjson::Document layerJson;
              layerJson.Parse(
                  reinterpret_cast<const char*>(layerJsonBinary.data()),
                  layerJsonBinary.size());
              if (layerJson.HasParseError()) {
                loadLayersResult.errors.emplaceWarning(fmt::format(
                    "Error when parsing layer.json, error code {} at byte "
                    "offset {}",
                    layerJson.GetParseError(),
                    layerJson.GetErrorOffset()));
                return asyncSystem.createResolvedFuture(
                    std::move(loadLayersResult));
              }

              const CesiumAsync::HttpHeaders& completedRequestHeaders =
                  pCompletedRequest->headers();
              std::vector<IAssetAccessor::THeader> flatHeaders(
                  completedRequestHeaders.begin(),
                  completedRequestHeaders.end());
              return loadLayersRecursive(
                  asyncSystem,
                  pAssetAccessor,
                  pCompletedRequest->url(),
                  flatHeaders,
                  layerJson,
                  tilingScheme,
                  useWaterMask,
                  std::move(loadLayersResult));
            });
  }

  return asyncSystem.createResolvedFuture(std::move(loadLayersResult));
}

Future<LoadLayersResult> loadLayerJson(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::string& baseUrl,
    const std::vector<IAssetAccessor::THeader>& requestHeaders,
    const rapidjson::Document& layerJson,
    bool useWaterMask,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  // Use the projection and tiling scheme of the main layer.
  // Any underlying layers must use the same.
  std::string projectionString =
      JsonHelpers::getStringOrDefault(layerJson, "projection", "EPSG:4326");

  CesiumGeospatial::Projection projection = WebMercatorProjection(ellipsoid);
  CesiumGeospatial::GlobeRectangle quadtreeRectangleGlobe(0.0, 0.0, 0.0, 0.0);
  CesiumGeometry::Rectangle quadtreeRectangleProjected(0.0, 0.0, 0.0, 0.0);
  uint32_t quadtreeXTiles;

  // Consistent with CesiumJS behavior, we ignore the "bounds" property.
  // Some non-Cesium terrain tilers seem to provide incorrect bounds.
  // See https://community.cesium.com/t/cesium-terrain-for-unreal/17940/18

  if (projectionString == "EPSG:4326") {
    CesiumGeospatial::GeographicProjection geographic(ellipsoid);
    projection = geographic;
    quadtreeRectangleGlobe = GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
    quadtreeRectangleProjected = geographic.project(quadtreeRectangleGlobe);
    quadtreeXTiles = 2;
  } else if (projectionString == "EPSG:3857") {
    CesiumGeospatial::WebMercatorProjection webMercator(ellipsoid);
    projection = webMercator;
    quadtreeRectangleGlobe = WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;
    quadtreeRectangleProjected = webMercator.project(quadtreeRectangleGlobe);
    quadtreeXTiles = 1;
  } else {
    LoadLayersResult result;
    result.errors.emplaceError(fmt::format(
        "Tileset layer.json contained an unknown projection value: {}",
        projectionString));
    return asyncSystem.createResolvedFuture(std::move(result));
  }

  BoundingVolume boundingVolume =
      createDefaultLooseEarthBoundingVolume(quadtreeRectangleGlobe, ellipsoid);

  CesiumGeometry::QuadtreeTilingScheme tilingScheme(
      quadtreeRectangleProjected,
      quadtreeXTiles,
      1);

  LoadLayersResult
      loadLayersResult{tilingScheme, projection, boundingVolume, {}, {}, {}};

  return loadLayersRecursive(
      asyncSystem,
      pAssetAccessor,
      baseUrl,
      requestHeaders,
      layerJson,
      tilingScheme,
      useWaterMask,
      std::move(loadLayersResult));
}

Future<LoadLayersResult> loadLayerJson(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::string& baseUrl,
    const std::vector<IAssetAccessor::THeader>& requestHeaders,
    const std::span<const std::byte>& layerJsonBinary,
    bool useWaterMask,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  rapidjson::Document layerJson;
  layerJson.Parse(
      reinterpret_cast<const char*>(layerJsonBinary.data()),
      layerJsonBinary.size());
  if (layerJson.HasParseError()) {
    LoadLayersResult result;
    result.errors.emplaceError(fmt::format(
        "Error when parsing layer.json, error code {} at byte offset {}",
        layerJson.GetParseError(),
        layerJson.GetErrorOffset()));
    return asyncSystem.createResolvedFuture(std::move(result));
  }

  return loadLayerJson(
      asyncSystem,
      pAssetAccessor,
      baseUrl,
      requestHeaders,
      layerJson,
      useWaterMask,
      ellipsoid);
}
} // namespace

/*static*/ CesiumAsync::Future<
    TilesetContentLoaderResult<LayerJsonTerrainLoader>>
LayerJsonTerrainLoader::createLoader(
    const TilesetExternals& externals,
    const TilesetContentOptions& contentOptions,
    const std::string& layerJsonUrl,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  bool useWaterMask = contentOptions.enableWaterMask;

  return externals.pAssetAccessor
      ->get(externals.asyncSystem, layerJsonUrl, requestHeaders)
      .thenInWorkerThread(
          [externals,
           ellipsoid,
           asyncSystem = externals.asyncSystem,
           pAssetAccessor = externals.pAssetAccessor,
           useWaterMask](
              std::shared_ptr<CesiumAsync::IAssetRequest>&& pCompletedRequest) {
            const CesiumAsync::IAssetResponse* pResponse =
                pCompletedRequest->response();
            const std::string& tileUrl = pCompletedRequest->url();
            if (!pResponse) {
              LoadLayersResult result;
              result.errors.emplaceError(fmt::format(
                  "Did not receive a valid response for tile content {}",
                  tileUrl));
              return asyncSystem.createResolvedFuture(std::move(result));
            }

            uint16_t statusCode = pResponse->statusCode();
            if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
              LoadLayersResult result;
              result.errors.emplaceError(fmt::format(
                  "Received status code {} for tile content {}",
                  statusCode,
                  tileUrl));
              result.statusCode = statusCode;
              return asyncSystem.createResolvedFuture(std::move(result));
            }

            const CesiumAsync::HttpHeaders& completedRequestHeaders =
                pCompletedRequest->headers();
            std::vector<IAssetAccessor::THeader> flatHeaders(
                completedRequestHeaders.begin(),
                completedRequestHeaders.end());

            return loadLayerJson(
                asyncSystem,
                pAssetAccessor,
                pCompletedRequest->url(),
                flatHeaders,
                pResponse->data(),
                useWaterMask,
                ellipsoid);
          })
      .thenInMainThread([ellipsoid](LoadLayersResult&& loadLayersResult) {
        return convertToTilesetContentLoaderResult(
            ellipsoid,
            std::move(loadLayersResult));
      });
}

CesiumAsync::Future<TilesetContentLoaderResult<LayerJsonTerrainLoader>>
Cesium3DTilesSelection::LayerJsonTerrainLoader::createLoader(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const TilesetContentOptions& contentOptions,
    const std::string& layerJsonUrl,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
    const rapidjson::Document& layerJson,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  return loadLayerJson(
             asyncSystem,
             pAssetAccessor,
             layerJsonUrl,
             requestHeaders,
             layerJson,
             contentOptions.enableWaterMask,
             ellipsoid)
      .thenInMainThread([ellipsoid](LoadLayersResult&& loadLayersResult) {
        return convertToTilesetContentLoaderResult(
            ellipsoid,
            std::move(loadLayersResult));
      });
}

LayerJsonTerrainLoader::Layer::Layer(
    const std::string& baseUrl_,
    std::string&& version_,
    std::vector<std::string>&& tileTemplateUrls_,
    std::string&& extensionsToRequest_,
    CesiumGeometry::QuadtreeRectangleAvailability&& contentAvailability_,
    uint32_t maxZooms_,
    int32_t availabilityLevels_)
    : baseUrl{baseUrl_},
      version{std::move(version_)},
      tileTemplateUrls{std::move(tileTemplateUrls_)},
      extensionsToRequest{std::move(extensionsToRequest_)},
      contentAvailability{std::move(contentAvailability_)},
      loadedSubtrees(maxSubtreeInLayer(maxZooms_, availabilityLevels_)),
      availabilityLevels{availabilityLevels_} {}

LayerJsonTerrainLoader::LayerJsonTerrainLoader(
    const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
    const CesiumGeospatial::Projection& projection,
    std::vector<Layer>&& layers)
    : _tilingScheme(tilingScheme),
      _projection(projection),
      _layers(std::move(layers)) {}

namespace {

std::string resolveTileUrl(
    const QuadtreeTileID& tileID,
    const LayerJsonTerrainLoader::Layer& layer) {
  if (layer.tileTemplateUrls.empty()) {
    return std::string();
  }

  Uri uri(
      layer.baseUrl,
      Uri::substituteTemplateParameters(
          layer.tileTemplateUrls[0],
          [&tileID, &layer](const std::string& placeholder) -> std::string {
            if (placeholder == "level" || placeholder == "z") {
              return std::to_string(tileID.level);
            }
            if (placeholder == "x") {
              return std::to_string(tileID.x);
            }
            if (placeholder == "y") {
              return std::to_string(tileID.y);
            }
            if (placeholder == "version") {
              return layer.version;
            }

            return placeholder;
          }),
      true);

  if (!layer.extensionsToRequest.empty()) {
    UriQuery params(uri);
    params.setValue("extensions", layer.extensionsToRequest);
    uri.setQuery(params.toQueryString());
  }

  return std::string(uri.toString());
}

Future<QuantizedMeshLoadResult> requestTileContent(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const QuadtreeTileID& tileID,
    const BoundingRegion& boundingRegion,
    const LayerJsonTerrainLoader::Layer& layer,
    const std::vector<IAssetAccessor::THeader>& requestHeaders,
    bool enableWaterMask,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  std::string url = resolveTileUrl(tileID, layer);
  return pAssetAccessor->get(asyncSystem, url, requestHeaders)
      .thenInWorkerThread([ellipsoid,
                           asyncSystem,
                           pLogger,
                           tileID,
                           boundingRegion,
                           enableWaterMask](
                              std::shared_ptr<IAssetRequest>&& pRequest) {
        const IAssetResponse* pResponse = pRequest->response();
        if (!pResponse) {
          QuantizedMeshLoadResult result;
          result.errors.emplaceError(fmt::format(
              "Did not receive a valid response for tile content {}",
              pRequest->url()));
          result.pRequest = std::move(pRequest);
          return result;
        }

        if (pResponse->statusCode() != 0 &&
            (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300)) {
          QuantizedMeshLoadResult result;
          result.errors.emplaceError(fmt::format(
              "Received status code {} for tile content {}",
              pResponse->statusCode(),
              pRequest->url()));
          result.pRequest = std::move(pRequest);
          return result;
        }

        return QuantizedMeshLoader::load(
            tileID,
            boundingRegion,
            pRequest->url(),
            pResponse->data(),
            enableWaterMask,
            ellipsoid);
      });
}

Future<int> loadTileAvailability(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const QuadtreeTileID& tileID,
    LayerJsonTerrainLoader::Layer& layer,
    const std::vector<IAssetAccessor::THeader>& requestHeaders) {
  std::string url = resolveTileUrl(tileID, layer);
  return pAssetAccessor->get(asyncSystem, url, requestHeaders)
      .thenInWorkerThread([pLogger,
                           tileID](std::shared_ptr<IAssetRequest>&& pRequest) {
        const IAssetResponse* pResponse = pRequest->response();
        if (pResponse) {
          uint16_t statusCode = pResponse->statusCode();

          if (!(statusCode != 0 && (statusCode < 200 || statusCode >= 300))) {
            return QuantizedMeshLoader::loadMetadata(pResponse->data(), tileID);
          }
        }

        SPDLOG_LOGGER_ERROR(
            pLogger,
            "Failed to load availability data from {}",
            pRequest->url());
        return QuantizedMeshMetadataResult();
      })
      .thenInMainThread([&layer,
                         tileID](QuantizedMeshMetadataResult&& metadata) {
        addRectangleAvailabilityToLayer(layer, tileID, metadata.availability);
        return 0;
      });
}
} // namespace

Future<TileLoadResult>
LayerJsonTerrainLoader::loadTileContent(const TileLoadInput& loadInput) {
  const auto& tile = loadInput.tile;
  const auto& asyncSystem = loadInput.asyncSystem;
  const auto& pAssetAccessor = loadInput.pAssetAccessor;
  const auto& pLogger = loadInput.pLogger;
  const auto& requestHeaders = loadInput.requestHeaders;
  const auto& contentOptions = loadInput.contentOptions;
  const auto& ellipsoid = loadInput.ellipsoid;

  // This type of loader should never have child loaders.
  CESIUM_ASSERT(tile.getLoader() == this);

  const QuadtreeTileID* pQuadtreeTileID =
      std::get_if<QuadtreeTileID>(&tile.getTileID());
  if (!pQuadtreeTileID) {
    // check if we need to upsample this tile
    const UpsampledQuadtreeNode* pUpsampleTileID =
        std::get_if<UpsampledQuadtreeNode>(&tile.getTileID());
    if (!pUpsampleTileID) {
      // This loader only handles QuadtreeTileIDs and UpsampledQuadtreeNode.
      return asyncSystem.createResolvedFuture(
          TileLoadResult::createFailedResult(pAssetAccessor, nullptr));
    }

    // now do upsampling
    return upsampleParentTile(tile, asyncSystem);
  }

  // Always request the tile from the first layer in which this tile ID is
  // available.
  auto firstAvailableIt = this->_layers.begin();
  while (firstAvailableIt != this->_layers.end() &&
         !firstAvailableIt->contentAvailability.isTileAvailable(
             *pQuadtreeTileID)) {
    ++firstAvailableIt;
  }

  if (firstAvailableIt == this->_layers.end()) {
    // No layer has this tile available.
    return asyncSystem.createResolvedFuture(
        TileLoadResult::createFailedResult(pAssetAccessor, nullptr));
  }

  // Also load the same tile in any underlying layers for which this tile
  // is an availability level. This is necessary because, when we later
  // create this tile's children, we need to be able to create children
  // that are only available from an underlying layer, and we can only do
  // that if we know they're available.
  std::vector<Future<int>> availabilityRequests;

  auto it = firstAvailableIt;
  ++it;

  while (it != this->_layers.end()) {
    if (it->availabilityLevels >= 1 &&
        (int32_t(pQuadtreeTileID->level) % it->availabilityLevels) == 0) {
      if (!isSubtreeLoadedInLayer(*pQuadtreeTileID, *it)) {
        availabilityRequests.emplace_back(loadTileAvailability(
            pLogger,
            asyncSystem,
            pAssetAccessor,
            *pQuadtreeTileID,
            *it,
            requestHeaders));
      }
    }

    ++it;
  }

  const BoundingRegion* pRegion =
      getBoundingRegionFromBoundingVolume(tile.getBoundingVolume());
  if (!pRegion) {
    // This tile does not have the required bounding volume type.
    return asyncSystem.createResolvedFuture(
        TileLoadResult::createFailedResult(pAssetAccessor, nullptr));
  }

  // Start the actual content request.
  auto& currentLayer = *firstAvailableIt;
  Future<QuantizedMeshLoadResult> futureQuantizedMesh = requestTileContent(
      pLogger,
      asyncSystem,
      pAssetAccessor,
      *pQuadtreeTileID,
      *pRegion,
      currentLayer,
      requestHeaders,
      contentOptions.enableWaterMask,
      ellipsoid);

  // determine if this tile is at the availability level of the current layer
  // and if we need to add the availability rectangles to the current layer. We
  // only add the rectangles if those are not not loaded
  bool shouldCurrLayerLoadAvailability = false;
  if (firstAvailableIt->availabilityLevels >= 1 &&
      int32_t(pQuadtreeTileID->level) % currentLayer.availabilityLevels == 0) {
    shouldCurrLayerLoadAvailability =
        !isSubtreeLoadedInLayer(*pQuadtreeTileID, currentLayer);
  }

  // If this tile has availability data, we need to add it to the layer in the
  // main thread.
  if (!availabilityRequests.empty() || shouldCurrLayerLoadAvailability) {
    auto finalFuture =
        availabilityRequests.empty()
            ? std::move(futureQuantizedMesh)
            : asyncSystem.all(std::move(availabilityRequests))
                  .thenImmediately(
                      [futureQuantizedMesh = std::move(futureQuantizedMesh)](
                          std::vector<int>&&) mutable {
                        return std::move(futureQuantizedMesh);
                      });

    return std::move(finalFuture)
        .thenInMainThread([this,
                           asyncSystem,
                           pAssetAccessor,
                           pLogger,
                           ellipsoid,
                           &currentLayer,
                           &tile,
                           shouldCurrLayerLoadAvailability](
                              QuantizedMeshLoadResult&& loadResult) mutable {
          loadResult.errors.logWarning(
              pLogger,
              "Warnings loading quantized mesh terrain");
          loadResult.errors.logError(
              pLogger,
              "Errors loading quantized mesh terrain");

          if (shouldCurrLayerLoadAvailability) {
            const QuadtreeTileID& tileID =
                std::get<QuadtreeTileID>(tile.getTileID());
            addRectangleAvailabilityToLayer(
                currentLayer,
                tileID,
                loadResult.availableTileRectangles);
          }

          // if this tile has one of the children that needs to be upsampled, we
          // will need to generate the tile raster overlay UVs in the worker
          // thread based on the projection of the loader since the upsampler
          // needs this UV to do the upsampling
          auto finalResult = convertToTileLoadResult(
              pAssetAccessor,
              std::move(loadResult),
              ellipsoid);
          bool doesTileHaveUpsampledChild = tileHasUpsampledChild(tile);
          if (doesTileHaveUpsampledChild &&
              finalResult.state == TileLoadResultState::Success) {
            return asyncSystem.runInWorkerThread(
                [finalResult = std::move(finalResult),
                 projection = _projection,
                 tileTransform = tile.getTransform(),
                 tileBoundingVolume = tile.getBoundingVolume()]() mutable {
                  generateRasterOverlayUVs(
                      tileBoundingVolume,
                      tileTransform,
                      projection,
                      finalResult);
                  return finalResult;
                });
          }

          return asyncSystem.createResolvedFuture(std::move(finalResult));
        });
  }

  bool doesTileHaveUpsampledChild = tileHasUpsampledChild(tile);
  return std::move(futureQuantizedMesh)
      .thenImmediately(
          [pAssetAccessor,
           pLogger,
           doesTileHaveUpsampledChild,
           projection = this->_projection,
           tileTransform = tile.getTransform(),
           tileBoundingVolume = tile.getBoundingVolume(),
           ellipsoid](QuantizedMeshLoadResult&& loadResult) mutable {
            loadResult.errors.logWarning(
                pLogger,
                "Warnings loading quantized mesh terrain");
            loadResult.errors.logError(
                pLogger,
                "Errors loading quantized mesh terrain");

            // if this tile has one of the children needs to be upsampled, we
            // will need to generate its raster overlay UVs in the worker thread
            // based on the projection of the loader since the upsampler needs
            // this UV to do the upsampling
            auto result = convertToTileLoadResult(
                pAssetAccessor,
                std::move(loadResult),
                ellipsoid);
            if (doesTileHaveUpsampledChild &&
                result.state == TileLoadResultState::Success) {
              generateRasterOverlayUVs(
                  tileBoundingVolume,
                  tileTransform,
                  projection,
                  result);
            }

            return result;
          });
}

TileChildrenResult LayerJsonTerrainLoader::createTileChildren(
    const Tile& tile,
    [[maybe_unused]] const CesiumGeospatial::Ellipsoid& ellipsoid) {
  const CesiumGeometry::QuadtreeTileID* pQuadtreeID =
      std::get_if<CesiumGeometry::QuadtreeTileID>(&tile.getTileID());
  if (pQuadtreeID) {
    // For the tile that is in the middle of subtree, it is safe to create the
    // children. However for tile that is at the availability level, we have
    // to wait for the tile to be finished loading, since there are multiple
    // subtrees in the background layers being on the flight as well. Once the
    // tile finishes loading, all the subtrees are resolved
    bool isTileAtAvailabilityLevel = false;
    for (const auto& layer : this->_layers) {
      if (layer.availabilityLevels > 0 &&
          int32_t(pQuadtreeID->level) % layer.availabilityLevels == 0) {
        isTileAtAvailabilityLevel = true;
        break;
      }
    }

    if (isTileAtAvailabilityLevel &&
        tile.getState() <= TileLoadState::ContentLoading) {
      return {{}, TileLoadResultState::RetryLater};
    }

    return {createTileChildrenImpl(tile), TileLoadResultState::Success};
  }

  return {{}, TileLoadResultState::Failed};
}

const CesiumGeometry::QuadtreeTilingScheme&
LayerJsonTerrainLoader::getTilingScheme() const noexcept {
  return this->_tilingScheme;
}

const CesiumGeospatial::Projection&
LayerJsonTerrainLoader::getProjection() const noexcept {
  return this->_projection;
}

const std::vector<LayerJsonTerrainLoader::Layer>&
LayerJsonTerrainLoader::getLayers() const noexcept {
  return this->_layers;
}

bool LayerJsonTerrainLoader::tileHasUpsampledChild(const Tile& tile) const {
  const auto& tileChildren = tile.getChildren();
  if (tileChildren.empty()) {
    const QuadtreeTileID* pQuadtreeTileID =
        std::get_if<QuadtreeTileID>(&tile.getTileID());

    const QuadtreeTileID swID(
        pQuadtreeTileID->level + 1,
        pQuadtreeTileID->x * 2,
        pQuadtreeTileID->y * 2);
    const QuadtreeTileID seID(swID.level, swID.x + 1, swID.y);
    const QuadtreeTileID nwID(swID.level, swID.x, swID.y + 1);
    const QuadtreeTileID neID(swID.level, swID.x + 1, swID.y + 1);

    uint32_t totalChildren = 0;
    totalChildren += this->tileIsAvailableInAnyLayer(swID);
    totalChildren += this->tileIsAvailableInAnyLayer(seID);
    totalChildren += this->tileIsAvailableInAnyLayer(nwID);
    totalChildren += this->tileIsAvailableInAnyLayer(neID);
    return totalChildren > 0 && totalChildren < 4;
  } else {
    for (const auto& child : tileChildren) {
      if (std::holds_alternative<UpsampledQuadtreeNode>(child.getTileID())) {
        return true;
      }
    }

    return false;
  }
}

std::vector<Tile>
LayerJsonTerrainLoader::createTileChildrenImpl(const Tile& tile) {
  const QuadtreeTileID* pQuadtreeTileID =
      std::get_if<QuadtreeTileID>(&tile.getTileID());

  // Now that all our availability is sorted out, create this tile's
  // children.
  const QuadtreeTileID swID(
      pQuadtreeTileID->level + 1,
      pQuadtreeTileID->x * 2,
      pQuadtreeTileID->y * 2);
  const QuadtreeTileID seID(swID.level, swID.x + 1, swID.y);
  const QuadtreeTileID nwID(swID.level, swID.x, swID.y + 1);
  const QuadtreeTileID neID(swID.level, swID.x + 1, swID.y + 1);

  // If _any_ child is available, we create _all_ children
  bool sw = this->tileIsAvailableInAnyLayer(swID);
  bool se = this->tileIsAvailableInAnyLayer(seID);
  bool nw = this->tileIsAvailableInAnyLayer(nwID);
  bool ne = this->tileIsAvailableInAnyLayer(neID);

  if (sw || se || nw || ne) {
    std::vector<Tile> children;
    children.reserve(4);

    createChildTile(tile, children, swID, sw);
    createChildTile(tile, children, seID, se);
    createChildTile(tile, children, nwID, nw);
    createChildTile(tile, children, neID, ne);
    return children;
  }

  return {};
}

bool LayerJsonTerrainLoader::tileIsAvailableInAnyLayer(
    const QuadtreeTileID& tileID) const {
  for (const Layer& layer : this->_layers) {
    auto availableState = tileIsAvailableInLayer(tileID, layer);
    if (availableState == AvailableState::Available) {
      return true;
    }
  }

  return false;
}

LayerJsonTerrainLoader::AvailableState
LayerJsonTerrainLoader::tileIsAvailableInLayer(
    const CesiumGeometry::QuadtreeTileID& tileID,
    const Layer& layer) const {
  if (layer.contentAvailability.isTileAvailable(tileID)) {
    return AvailableState::Available;
  } else {
    // this layer doesn't use subtree at all and list
    // all availability rectanges in the layer.json. So
    // this tile is not available
    if (layer.availabilityLevels <= 0) {
      return AvailableState::NotAvailable;
    }

    // this tile ID is also a subtree ID. So no need to calc
    // subtree ID
    if (int32_t(tileID.level) % layer.availabilityLevels == 0) {
      if (isSubtreeLoadedInLayer(tileID, layer)) {
        return AvailableState::NotAvailable;
      }
    }

    // calc the subtree ID this tile belongs to and determine it's loaded
    uint32_t subtreeLevelIdx =
        tileID.level / uint32_t(layer.availabilityLevels);
    uint64_t levelLeft = tileID.level % uint32_t(layer.availabilityLevels);
    uint32_t subtreeLevel =
        subtreeLevelIdx * uint32_t(layer.availabilityLevels);
    uint32_t subtreeX = tileID.x >> levelLeft;
    uint32_t subtreeY = tileID.y >> levelLeft;
    CesiumGeometry::QuadtreeTileID subtreeID{subtreeLevel, subtreeX, subtreeY};
    if (isSubtreeLoadedInLayer(subtreeID, layer)) {
      return AvailableState::NotAvailable;
    }
  }

  return AvailableState::Unknown;
}

void LayerJsonTerrainLoader::createChildTile(
    const Tile& parent,
    std::vector<Tile>& children,
    const QuadtreeTileID& childID,
    bool isAvailable) {
  Tile& child = children.emplace_back(this);
  child.setRefine(parent.getRefine());
  child.setTransform(parent.getTransform());
  if (isAvailable) {
    child.setTileID(childID);
  } else {
    child.setTileID(UpsampledQuadtreeNode{childID});
  }
  child.setGeometricError(parent.getGeometricError() * 0.5);

  const BoundingVolume& parentBoundingVolume = parent.getBoundingVolume();
  const BoundingRegion* pRegion =
      std::get_if<BoundingRegion>(&parentBoundingVolume);
  const BoundingRegionWithLooseFittingHeights* pLooseRegion =
      std::get_if<BoundingRegionWithLooseFittingHeights>(&parentBoundingVolume);

  double minHeight = -1000.0;
  double maxHeight = 9000.0;

  if (pRegion) {
    minHeight = pRegion->getMinimumHeight();
    maxHeight = pRegion->getMaximumHeight();
  } else if (pLooseRegion) {
    minHeight = pLooseRegion->getBoundingRegion().getMinimumHeight();
    maxHeight = pLooseRegion->getBoundingRegion().getMaximumHeight();
  }

  const CesiumGeospatial::GlobeRectangle childGlobeRectangle =
      unprojectRectangleSimple(
          this->_projection,
          this->_tilingScheme.tileToRectangle(childID));
  child.setBoundingVolume(BoundingRegionWithLooseFittingHeights(BoundingRegion(
      childGlobeRectangle,
      minHeight,
      maxHeight,
      getProjectionEllipsoid(this->_projection))));
}

CesiumAsync::Future<TileLoadResult> LayerJsonTerrainLoader::upsampleParentTile(
    const Tile& tile,
    const CesiumAsync::AsyncSystem& asyncSystem) {
  const Tile* pParent = tile.getParent();
  const TileContent& parentContent = pParent->getContent();
  const TileRenderContent* pParentRenderContent =
      parentContent.getRenderContent();
  if (!pParentRenderContent) {
    return asyncSystem.createResolvedFuture(
        TileLoadResult::createFailedResult(nullptr, nullptr));
  }

  const UpsampledQuadtreeNode* pUpsampledTileID =
      std::get_if<UpsampledQuadtreeNode>(&tile.getTileID());

  int32_t index = -1;
  const std::vector<CesiumGeospatial::Projection>& parentProjections =
      pParentRenderContent->getRasterOverlayDetails().rasterOverlayProjections;
  auto it = std::find(
      parentProjections.begin(),
      parentProjections.end(),
      this->_projection);

  // Cannot find raster overlay UVs that has this projection, so we can't
  // upsample right now
  CESIUM_ASSERT(
      it != parentProjections.end() &&
      "Cannot find raster overlay UVs that has this projection. "
      "Should not happen");

  index = int32_t(it - parentProjections.begin());

  const CesiumGeospatial::Ellipsoid& ellipsoid =
      getProjectionEllipsoid(this->_projection);

  // it's totally safe to capture the const ref parent model in the worker
  // thread. The tileset content manager will guarantee that the parent tile
  // will not be unloaded when upsampled tile is on the fly.
  const CesiumGltf::Model& parentModel = pParentRenderContent->getModel();
  return asyncSystem.runInWorkerThread(
      [&parentModel,
       ellipsoid,
       boundingVolume = tile.getBoundingVolume(),
       textureCoordinateIndex = index,
       tileID = *pUpsampledTileID]() mutable {
        auto model = RasterOverlayUtilities::upsampleGltfForRasterOverlays(
            parentModel,
            tileID,
            false,
            RasterOverlayUtilities::DEFAULT_TEXTURE_COORDINATE_BASE_NAME,
            textureCoordinateIndex,
            ellipsoid);
        if (!model) {
          return TileLoadResult::createFailedResult(nullptr, nullptr);
        }

        return TileLoadResult{
            std::move(*model),
            CesiumGeometry::Axis::Y,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            nullptr,
            nullptr,
            {},
            TileLoadResultState::Success,
            ellipsoid};
      });
}
