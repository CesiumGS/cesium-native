#include "LayerJsonTerrainLoader.h"

#include "calcQuadtreeMaxGeometricError.h"

#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Uri.h>

#include <rapidjson/document.h>

using namespace CesiumAsync;
using namespace Cesium3DTilesSelection;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace {

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

std::vector<CesiumGeometry::QuadtreeTileRectangularRange>
loadAvailabilityRectangles(
    const rapidjson::Document& metadata,
    uint32_t startingLevel) {
  std::vector<CesiumGeometry::QuadtreeTileRectangularRange> ret;
  const auto availableIt = metadata.FindMember("available");
  if (availableIt == metadata.MemberEnd() || !availableIt->value.IsArray()) {
    return ret;
  }

  const auto& available = availableIt->value;
  if (available.Size() == 0) {
    return ret;
  }

  for (rapidjson::SizeType i = 0; i < available.Size(); ++i) {
    const auto& rangesAtLevelJson = available[i];
    if (!rangesAtLevelJson.IsArray()) {
      continue;
    }

    for (rapidjson::SizeType j = 0; j < rangesAtLevelJson.Size(); ++j) {
      const auto& rangeJson = rangesAtLevelJson[j];
      if (!rangeJson.IsObject()) {
        continue;
      }

      ret.push_back(CesiumGeometry::QuadtreeTileRectangularRange{
          startingLevel,
          JsonHelpers::getUint32OrDefault(rangeJson, "startX", 0),
          JsonHelpers::getUint32OrDefault(rangeJson, "startY", 0),
          JsonHelpers::getUint32OrDefault(rangeJson, "endX", 0),
          JsonHelpers::getUint32OrDefault(rangeJson, "endY", 0)});
    }

    ++startingLevel;
  }
  return ret;
}

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
    const CesiumGeospatial::GlobeRectangle& globeRectangle) {
  return BoundingRegionWithLooseFittingHeights(
      BoundingRegion(globeRectangle, -1000.0, -9000.0));
}

struct LoadLayersResult {
  std::vector<LayerJsonTerrainLoader::Layer> layers;
  ErrorList errors;
};

Future<LoadLayersResult> loadLayers(
    const TilesetExternals& externals,
    const std::string& baseUrl,
    const HttpHeaders& requestHeaders,
    const rapidjson::Document& layerJson,
    const QuadtreeTilingScheme& tilingScheme,
    bool useWaterMask,
    bool showCreditsOnScreen,
    LoadLayersResult&& loadLayersResult) {
  std::string version;
  const auto tilesetVersionIt = layerJson.FindMember("version");
  if (tilesetVersionIt != layerJson.MemberEnd() &&
      tilesetVersionIt->value.IsString()) {
    version = tilesetVersionIt->value.GetString();
  }

  std::vector<std::string> urls = JsonHelpers::getStrings(layerJson, "tiles");
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

  if (!extensionsToRequest.empty()) {
    for (std::string& url : urls) {
      url =
          CesiumUtility::Uri::addQuery(url, "extensions", extensionsToRequest);
    }
  }

  const auto availabilityLevelsIt =
      layerJson.FindMember("metadataAvailability");

  QuadtreeRectangleAvailability availability(tilingScheme, maxZoom);

  int32_t availabilityLevels = -1;

  if (availabilityLevelsIt != layerJson.MemberEnd() &&
      availabilityLevelsIt->value.IsInt()) {
    availabilityLevels = availabilityLevelsIt->value.GetInt();
  } else {
    std::vector<CesiumGeometry::QuadtreeTileRectangularRange>
        availableTileRectangles = loadAvailabilityRectangles(layerJson, 0);
    if (availableTileRectangles.size() > 0) {
      for (const auto& rectangle : availableTileRectangles) {
        availability.addAvailableTileRange(rectangle);
      }
    }
  }

  const auto attributionIt = layerJson.FindMember("attribution");

  std::optional<Credit> maybeCredit;
  if (attributionIt != layerJson.MemberEnd() &&
      attributionIt->value.IsString()) {
    maybeCredit =
        std::make_optional<Credit>(externals.pCreditSystem->createCredit(
            attributionIt->value.GetString(),
            showCreditsOnScreen));
  }

  loadLayersResult.layers.emplace_back(LayerJsonTerrainLoader::Layer{
      std::move(urls),
      std::move(availability),
      availabilityLevels,
      maybeCredit});

  std::string parentUrl =
      JsonHelpers::getStringOrDefault(layerJson, "parentUrl", "");

  if (!parentUrl.empty()) {
    std::string resolvedUrl = Uri::resolve(baseUrl, parentUrl);
    // append forward slash if necessary
    if (resolvedUrl[resolvedUrl.size() - 1] != '/') {
      resolvedUrl += '/';
    }
    resolvedUrl += "layer.json";

    std::vector<IAssetAccessor::THeader> flatHeaders(
        requestHeaders.begin(),
        requestHeaders.end());

    return externals.pAssetAccessor
        ->get(externals.asyncSystem, resolvedUrl, flatHeaders)
        .thenInWorkerThread(
            [tilingScheme,
             useWaterMask,
             showCreditsOnScreen,
             externals,
             loadLayersResult = std::move(loadLayersResult)](
                std::shared_ptr<IAssetRequest>&& pCompletedRequest) mutable {
              const CesiumAsync::IAssetResponse* pResponse =
                  pCompletedRequest->response();
              const std::string& tileUrl = pCompletedRequest->url();
              if (!pResponse) {
                loadLayersResult.errors.emplace_warning(fmt::format(
                    "Did not receive a valid response for parent layer {}",
                    pCompletedRequest->url()));
                return externals.asyncSystem.createResolvedFuture(
                    std::move(loadLayersResult));
              }

              uint16_t statusCode = pResponse->statusCode();
              if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
                loadLayersResult.errors.emplace_warning(fmt::format(
                    "Received status code {} for parent layer {}",
                    statusCode,
                    tileUrl));
                return externals.asyncSystem.createResolvedFuture(
                    std::move(loadLayersResult));
              }

              gsl::span<const std::byte> layerJsonBinary = pResponse->data();

              rapidjson::Document layerJson;
              layerJson.Parse(
                  reinterpret_cast<const char*>(layerJsonBinary.data()),
                  layerJsonBinary.size());
              if (layerJson.HasParseError()) {
                loadLayersResult.errors.emplace_warning(fmt::format(
                    "Error when parsing layer.json, error code {} at byte "
                    "offset {}",
                    layerJson.GetParseError(),
                    layerJson.GetErrorOffset()));
                return externals.asyncSystem.createResolvedFuture(
                    std::move(loadLayersResult));
              }

              return loadLayers(
                  externals,
                  pCompletedRequest->url(),
                  pCompletedRequest->headers(),
                  layerJson,
                  tilingScheme,
                  useWaterMask,
                  showCreditsOnScreen,
                  std::move(loadLayersResult));
            });
  }

  return externals.asyncSystem.createResolvedFuture(
      std::move(loadLayersResult));
}

Future<TilesetContentLoaderResult> loadLayerJson(
    const TilesetExternals& externals,
    const std::string& baseUrl,
    const HttpHeaders& requestHeaders,
    const gsl::span<const std::byte>& layerJsonBinary,
    bool useWaterMask,
    bool showCreditsOnScreen) {
  rapidjson::Document layerJson;
  layerJson.Parse(
      reinterpret_cast<const char*>(layerJsonBinary.data()),
      layerJsonBinary.size());
  if (layerJson.HasParseError()) {
    TilesetContentLoaderResult result;
    result.errors.emplace_error(fmt::format(
        "Error when parsing layer.json, error code {} at byte offset {}",
        layerJson.GetParseError(),
        layerJson.GetErrorOffset()));
    return externals.asyncSystem.createResolvedFuture(std::move(result));
  }

  // Use the projection and tiling scheme of the main layer.
  // Any underlying layers must use the same.
  std::string projectionString =
      JsonHelpers::getStringOrDefault(layerJson, "projection", "EPSG:4326");

  CesiumGeospatial::Projection projection;
  CesiumGeospatial::GlobeRectangle quadtreeRectangleGlobe(0.0, 0.0, 0.0, 0.0);
  CesiumGeometry::Rectangle quadtreeRectangleProjected(0.0, 0.0, 0.0, 0.0);
  uint32_t quadtreeXTiles;

  // Consistent with CesiumJS behavior, we ignore the "bounds" property.
  // Some non-Cesium terrain tilers seem to provide incorrect bounds.
  // See https://community.cesium.com/t/cesium-terrain-for-unreal/17940/18

  if (projectionString == "EPSG:4326") {
    CesiumGeospatial::GeographicProjection geographic;
    projection = geographic;
    quadtreeRectangleGlobe = GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
    quadtreeRectangleProjected = geographic.project(quadtreeRectangleGlobe);
    quadtreeXTiles = 2;
  } else if (projectionString == "EPSG:3857") {
    CesiumGeospatial::WebMercatorProjection webMercator;
    projection = webMercator;
    quadtreeRectangleGlobe = WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;
    quadtreeRectangleProjected = webMercator.project(quadtreeRectangleGlobe);
    quadtreeXTiles = 1;
  } else {
    TilesetContentLoaderResult result;
    result.errors.emplace_error(fmt::format(
        "Tileset layer.json contained an unknown projection value: {}",
        projectionString));
    return externals.asyncSystem.createResolvedFuture(std::move(result));
  }

  BoundingVolume boundingVolume =
      createDefaultLooseEarthBoundingVolume(quadtreeRectangleGlobe);

  CesiumGeometry::QuadtreeTilingScheme tilingScheme(
      quadtreeRectangleProjected,
      quadtreeXTiles,
      1);

  LoadLayersResult loadLayersResult;

  return loadLayers(
             externals,
             baseUrl,
             requestHeaders,
             layerJson,
             tilingScheme,
             useWaterMask,
             showCreditsOnScreen,
             std::move(loadLayersResult))
      .thenImmediately(
          [tilingScheme, projection, boundingVolume](
              LoadLayersResult&& loadLayersResult)
              -> TilesetContentLoaderResult {
            auto pLoader = std::make_unique<LayerJsonTerrainLoader>(
                tilingScheme,
                projection,
                std::move(loadLayersResult.layers));

            std::unique_ptr<Tile> pRootTile =
                std::make_unique<Tile>(pLoader.get(), TileEmptyContent());
            pRootTile->setUnconditionallyRefine();
            pRootTile->setBoundingVolume(boundingVolume);

            uint32_t quadtreeXTiles = tilingScheme.getRootTilesX();

            std::vector<Tile> childTiles;
            childTiles.reserve(quadtreeXTiles);

            for (uint32_t i = 0; i < quadtreeXTiles; ++i) {
              Tile& childTile = childTiles.emplace_back(pLoader.get());

              QuadtreeTileID id(0, i, 0);
              childTile.setTileID(id);

              const CesiumGeospatial::GlobeRectangle childGlobeRectangle =
                  unprojectRectangleSimple(
                      projection,
                      tilingScheme.tileToRectangle(id));
              childTile.setBoundingVolume(
                  createDefaultLooseEarthBoundingVolume(childGlobeRectangle));
              childTile.setGeometricError(
                  8.0 * calcQuadtreeMaxGeometricError(Ellipsoid::WGS84) *
                  childGlobeRectangle.computeWidth());
            }

            pRootTile->createChildTiles(std::move(childTiles));

            return TilesetContentLoaderResult{
                std::move(pLoader),
                std::move(pRootTile),
                CesiumGeometry::Axis::Y,
                std::vector<LoaderCreditResult>{},
                std::vector<IAssetAccessor::THeader>{},
                std::move(loadLayersResult.errors)};
          });
}

} // namespace

/*static*/ CesiumAsync::Future<TilesetContentLoaderResult>
LayerJsonTerrainLoader::createLoader(
    const TilesetExternals& externals,
    const TilesetContentOptions& contentOptions,
    const std::string& layerJsonUrl,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
    bool showCreditsOnScreen) {
  bool useWaterMask = contentOptions.enableWaterMask;

  return externals.pAssetAccessor
      ->get(externals.asyncSystem, layerJsonUrl, requestHeaders)
      .thenImmediately(
          [externals, useWaterMask, showCreditsOnScreen](
              std::shared_ptr<CesiumAsync::IAssetRequest>&& pCompletedRequest) {
            const CesiumAsync::IAssetResponse* pResponse =
                pCompletedRequest->response();
            const std::string& tileUrl = pCompletedRequest->url();
            if (!pResponse) {
              TilesetContentLoaderResult result;
              result.errors.emplace_error(fmt::format(
                  "Did not receive a valid response for tile content {}",
                  tileUrl));
              return externals.asyncSystem.createResolvedFuture(
                  std::move(result));
            }

            uint16_t statusCode = pResponse->statusCode();
            if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
              TilesetContentLoaderResult result;
              result.errors.emplace_error(fmt::format(
                  "Received status code {} for tile content {}",
                  statusCode,
                  tileUrl));
              return externals.asyncSystem.createResolvedFuture(
                  std::move(result));
            }

            return loadLayerJson(
                externals,
                pCompletedRequest->url(),
                pCompletedRequest->headers(),
                pResponse->data(),
                useWaterMask,
                showCreditsOnScreen);
          });
}

// Future<TileLoadResult> LayerJsonTerrainLoader::loadTileContent(
//     Tile& tile,
//     const TilesetContentOptions& contentOptions,
//     const AsyncSystem& asyncSystem,
//     const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
//     const std::shared_ptr<spdlog::logger>& pLogger,
//     const std::vector<IAssetAccessor::THeader>& requestHeaders) {

// }
