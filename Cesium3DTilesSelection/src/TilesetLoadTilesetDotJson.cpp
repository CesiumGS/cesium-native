#include "TilesetLoadTilesetDotJson.h"

#include "Cesium3DTilesSelection/Tileset.h"
#include "Cesium3DTilesSelection/TilesetLoadFailureDetails.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumGeometry/QuadtreeTileID.h"
#include "CesiumGeospatial/GeographicProjection.h"
#include "CesiumUtility/JsonHelpers.h"
#include "CesiumUtility/Uri.h"
#include "QuantizedMeshContent.h"
#include "TilesetLoadTileFromJson.h"
#include "calcQuadtreeMaxGeometricError.h"

#include <rapidjson/document.h>

using namespace Cesium3DTilesSelection;
using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace {

struct LoadResult {
  std::unique_ptr<TileContext> pContext;
  std::unique_ptr<Tile> pRootTile;
  bool supportsRasterOverlays;
  std::optional<TilesetLoadFailureDetails> failure;
};

} // namespace

struct Tileset::LoadTilesetDotJson::Private {
  static Future<LoadResult> workerThreadHandleResponse(
      std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest,
      std::unique_ptr<TileContext>&& pContext,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<spdlog::logger>& pLogger,
      bool useWaterMask);

  static Future<void> workerThreadLoadTileContext(
      const rapidjson::Document& layerJson,
      TileContext& context,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<spdlog::logger>& pLogger,
      bool useWaterMask);

  static Future<void> workerThreadLoadTerrainTile(
      Tile& tile,
      const rapidjson::Document& layerJson,
      TileContext& context,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<spdlog::logger>& pLogger,
      bool useWaterMask);

  template <typename T>
  static auto handlePotentialError(Tileset& tileset, T&& operation) {
    return std::move(operation)
        .catchInMainThread([&tileset](const std::exception& e) {
          TilesetLoadFailureDetails failure;
          failure.pTileset = &tileset;
          failure.pRequest = nullptr;
          failure.type = TilesetLoadType::TilesetJson;
          failure.message = fmt::format(
              "Unhandled error for asset {}: {}",
              tileset.getUrl().value_or(""),
              e.what());
          return std::make_optional(failure);
        })
        .thenImmediately(
            [&tileset](
                std::optional<TilesetLoadFailureDetails>&& maybeFailure) {
              if (maybeFailure) {
                tileset.reportError(std::move(*maybeFailure));
              }
            })
        .catchImmediately([](const std::exception& /*e*/) {
          // We should only land here if tileset.reportError above throws an
          // exception, which it shouldn't. Flag it in a debug build and ignore
          // it in a release build.
          assert(false);
        });
  }
};

CesiumAsync::Future<void> Tileset::LoadTilesetDotJson::start(
    Tileset& tileset,
    const std::string& url,
    const std::vector<std::pair<std::string, std::string>>& headers,
    std::unique_ptr<TileContext>&& pContext) {
  if (!pContext) {
    pContext = std::make_unique<TileContext>();
  }
  pContext->pTileset = &tileset;

  CESIUM_TRACE_BEGIN_IN_TRACK("Load tileset.json");

  return Private::handlePotentialError(
             tileset,
             tileset.getExternals()
                 .pAssetAccessor->get(tileset.getAsyncSystem(), url, headers)
                 .thenInWorkerThread(
                     [pLogger = tileset.getExternals().pLogger,
                      asyncSystem = tileset.getAsyncSystem(),
                      pContext = std::move(pContext),
                      useWaterMask =
                          tileset.getOptions().contentOptions.enableWaterMask](
                         std::shared_ptr<IAssetRequest>&& pRequest) mutable {
                       return Private::workerThreadHandleResponse(
                           std::move(pRequest),
                           std::move(pContext),
                           asyncSystem,
                           pLogger,
                           useWaterMask);
                     })
                 .thenInMainThread(
                     [&tileset](LoadResult&& loadResult)
                         -> std::optional<TilesetLoadFailureDetails> {
                       tileset._supportsRasterOverlays =
                           loadResult.supportsRasterOverlays;
                       tileset.addContext(std::move(loadResult.pContext));
                       tileset._pRootTile = std::move(loadResult.pRootTile);
                       return loadResult.failure;
                     }))
      .thenImmediately(
          []() { CESIUM_TRACE_END_IN_TRACK("Load tileset.json"); });
}

namespace {
/**
 * @brief Obtains the up-axis that should be used for glTF content of the
 * tileset.
 *
 * If the given tileset JSON does not contain an `asset.gltfUpAxis` string
 * property, then the default value of CesiumGeometry::Axis::Y is returned.
 *
 * Otherwise, a warning is printed, saying that the `gltfUpAxis` property is
 * not strictly compliant to the 3D tiles standard, and the return value
 * will depend on the string value of this property, which may be "X", "Y", or
 * "Z", case-insensitively, causing CesiumGeometry::Axis::X,
 * CesiumGeometry::Axis::Y, or CesiumGeometry::Axis::Z to be returned,
 * respectively.
 *
 * @param tileset The tileset JSON
 * @return The up-axis to use for glTF content
 */
CesiumGeometry::Axis obtainGltfUpAxis(const rapidjson::Document& tileset) {
  const auto assetIt = tileset.FindMember("asset");
  if (assetIt == tileset.MemberEnd()) {
    return CesiumGeometry::Axis::Y;
  }
  const rapidjson::Value& assetJson = assetIt->value;
  const auto gltfUpAxisIt = assetJson.FindMember("gltfUpAxis");
  if (gltfUpAxisIt == assetJson.MemberEnd()) {
    return CesiumGeometry::Axis::Y;
  }

  SPDLOG_WARN("The tileset contains a gltfUpAxis property. "
              "This property is not part of the specification. "
              "All glTF content should use the Y-axis as the up-axis.");

  const rapidjson::Value& gltfUpAxisJson = gltfUpAxisIt->value;
  auto gltfUpAxisString = std::string(gltfUpAxisJson.GetString());
  if (gltfUpAxisString == "X" || gltfUpAxisString == "x") {
    return CesiumGeometry::Axis::X;
  }
  if (gltfUpAxisString == "Y" || gltfUpAxisString == "y") {
    return CesiumGeometry::Axis::Y;
  }
  if (gltfUpAxisString == "Z" || gltfUpAxisString == "z") {
    return CesiumGeometry::Axis::Z;
  }
  SPDLOG_WARN("Unknown gltfUpAxis: {}, using default (Y)", gltfUpAxisString);
  return CesiumGeometry::Axis::Y;
}
} // namespace

Future<LoadResult>
Tileset::LoadTilesetDotJson::Private::workerThreadHandleResponse(
    std::shared_ptr<IAssetRequest>&& pRequest,
    std::unique_ptr<TileContext>&& pContext,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<spdlog::logger>& pLogger,
    bool useWaterMask) {
  const IAssetResponse* pResponse = pRequest->response();
  if (!pResponse) {
    std::string message = fmt::format(
        "Did not receive a valid response for tileset {}",
        pRequest->url());
    return asyncSystem.createResolvedFuture(LoadResult{
        std::move(pContext),
        nullptr,
        false,
        TilesetLoadFailureDetails{
            pContext->pTileset,
            TilesetLoadType::TilesetJson,
            std::move(pRequest),
            message}});
  }

  if (pResponse->statusCode() != 0 &&
      (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300)) {
    std::string message = fmt::format(
        "Received status code {} for tileset {}",
        pResponse->statusCode(),
        pRequest->url());
    return asyncSystem.createResolvedFuture(LoadResult{
        std::move(pContext),
        nullptr,
        false,
        TilesetLoadFailureDetails{
            pContext ? pContext->pTileset : nullptr,
            TilesetLoadType::TilesetJson,
            std::move(pRequest),
            message}});
  }

  pContext->baseUrl = pRequest->url();

  const gsl::span<const std::byte> data = pResponse->data();

  rapidjson::Document tileset;
  tileset.Parse(reinterpret_cast<const char*>(data.data()), data.size());

  if (tileset.HasParseError()) {
    std::string message = fmt::format(
        "Error when parsing tileset JSON, error code {} at byte offset {}",
        tileset.GetParseError(),
        tileset.GetErrorOffset());
    return asyncSystem.createResolvedFuture(LoadResult{
        std::move(pContext),
        nullptr,
        false,
        TilesetLoadFailureDetails{
            pContext->pTileset,
            TilesetLoadType::TilesetJson,
            std::move(pRequest),
            message}});
  }

  pContext->pTileset->_gltfUpAxis = obtainGltfUpAxis(tileset);

  std::unique_ptr<Tile> pRootTile = std::make_unique<Tile>();
  pRootTile->setContext(pContext.get());

  const auto rootIt = tileset.FindMember("root");
  const auto formatIt = tileset.FindMember("format");

  bool supportsRasterOverlays = false;

  if (rootIt != tileset.MemberEnd()) {
    const rapidjson::Value& rootJson = rootIt->value;
    std::vector<std::unique_ptr<TileContext>> newContexts;

    LoadTileFromJson::execute(
        *pRootTile,
        newContexts,
        rootJson,
        glm::dmat4(1.0),
        TileRefine::Replace,
        *pContext,
        pLogger);

    for (auto&& pNewContext : newContexts) {
      pContext->pTileset->addContext(std::move(pNewContext));
    }

    supportsRasterOverlays = true;

  } else if (
      formatIt != tileset.MemberEnd() && formatIt->value.IsString() &&
      std::string(formatIt->value.GetString()) == "quantized-mesh-1.0") {
    Private::workerThreadLoadTerrainTile(
        *pRootTile,
        tileset,
        *pContext,
        asyncSystem,
        pLogger,
        useWaterMask);
    supportsRasterOverlays = true;
  }

  return asyncSystem.createResolvedFuture(LoadResult{
      std::move(pContext),
      std::move(pRootTile),
      supportsRasterOverlays,
      std::nullopt});
}

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

} // namespace

Future<void> Tileset::LoadTilesetDotJson::Private::workerThreadLoadTileContext(
    const rapidjson::Document& layerJson,
    TileContext& context,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<spdlog::logger>& pLogger,
    bool useWaterMask) {
  const auto tilesetVersionIt = layerJson.FindMember("version");
  if (tilesetVersionIt != layerJson.MemberEnd() &&
      tilesetVersionIt->value.IsString()) {
    context.version = tilesetVersionIt->value.GetString();
  }
  std::optional<std::vector<double>> optionalBounds =
      JsonHelpers::getDoubles(layerJson, -1, "bounds");
  std::vector<double> bounds;
  if (optionalBounds) {
    bounds = optionalBounds.value();
  }

  std::string projectionString =
      JsonHelpers::getStringOrDefault(layerJson, "projection", "EPSG:4326");

  CesiumGeospatial::Projection projection;
  CesiumGeospatial::GlobeRectangle quadtreeRectangleGlobe(0.0, 0.0, 0.0, 0.0);
  CesiumGeometry::Rectangle quadtreeRectangleProjected(0.0, 0.0, 0.0, 0.0);
  uint32_t quadtreeXTiles;

  if (projectionString == "EPSG:4326") {
    CesiumGeospatial::GeographicProjection geographic;
    projection = geographic;
    quadtreeRectangleGlobe =
        bounds.size() >= 4 ? CesiumGeospatial::GlobeRectangle::fromDegrees(
                                 bounds[0],
                                 bounds[1],
                                 bounds[2],
                                 bounds[3])
                           : GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
    quadtreeRectangleProjected = geographic.project(quadtreeRectangleGlobe);
    quadtreeXTiles = 2;
  } else if (projectionString == "EPSG:3857") {
    CesiumGeospatial::WebMercatorProjection webMercator;
    projection = webMercator;
    quadtreeRectangleGlobe =
        bounds.size() >= 4 ? CesiumGeospatial::GlobeRectangle::fromDegrees(
                                 bounds[0],
                                 bounds[1],
                                 bounds[2],
                                 bounds[3])
                           : WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;
    quadtreeRectangleProjected = webMercator.project(quadtreeRectangleGlobe);
    quadtreeXTiles = 1;
  } else {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Tileset contained an unknown projection value: {}",
        projectionString);
    return asyncSystem.createResolvedFuture();
  }

  BoundingVolume boundingVolume =
      createDefaultLooseEarthBoundingVolume(quadtreeRectangleGlobe);

  const std::optional<CesiumGeometry::QuadtreeTilingScheme> tilingScheme =
      std::make_optional<CesiumGeometry::QuadtreeTilingScheme>(
          quadtreeRectangleProjected,
          quadtreeXTiles,
          1);

  std::vector<std::string> urls = JsonHelpers::getStrings(layerJson, "tiles");
  uint32_t maxZoom = JsonHelpers::getUint32OrDefault(layerJson, "maxzoom", 30);

  context.implicitContext = {
      urls,
      std::nullopt,
      tilingScheme,
      std::nullopt,
      boundingVolume,
      projection,
      std::make_optional<CesiumGeometry::QuadtreeRectangleAvailability>(
          *tilingScheme,
          maxZoom),
      std::nullopt,
      std::nullopt};

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
    for (std::string& url : context.implicitContext.value().tileTemplateUrls) {
      url =
          CesiumUtility::Uri::addQuery(url, "extensions", extensionsToRequest);
    }
  }

  const auto availabilityLevelsIt =
      layerJson.FindMember("metadataAvailability");

  if (availabilityLevelsIt != layerJson.MemberEnd() &&
      availabilityLevelsIt->value.IsInt()) {
    context.implicitContext->availabilityLevels =
        std::make_optional<uint32_t>(availabilityLevelsIt->value.GetInt());
  } else {
    std::vector<CesiumGeometry::QuadtreeTileRectangularRange>
        availableTileRectangles =
            QuantizedMeshContent::loadAvailabilityRectangles(layerJson, 0);
    if (availableTileRectangles.size() > 0) {
      for (const auto& rectangle : availableTileRectangles) {
        context.implicitContext->rectangleAvailability->addAvailableTileRange(
            rectangle);
      }
    }
  }

  const auto attributionIt = layerJson.FindMember("attribution");

  if (attributionIt != layerJson.MemberEnd() &&
      attributionIt->value.IsString()) {
    context.implicitContext->credit = std::make_optional<Credit>(
        context.pTileset->getExternals().pCreditSystem->createCredit(
            attributionIt->value.GetString(),
            context.pTileset->_options.showCreditsOnScreen));
  }

  std::string parentUrl =
      JsonHelpers::getStringOrDefault(layerJson, "parentUrl", "");

  if (!parentUrl.empty()) {
    std::string resolvedUrl = Uri::resolve(context.baseUrl, parentUrl);
    // append forward slash if necessary
    if (resolvedUrl[resolvedUrl.size() - 1] != '/') {
      resolvedUrl += '/';
    }
    resolvedUrl += "layer.json";
    auto pAsyncSystem = context.pTileset->getAsyncSystem();
    auto pAssetAccessor = context.pTileset->getExternals().pAssetAccessor;
    return pAssetAccessor
        ->get(pAsyncSystem, resolvedUrl, context.requestHeaders)
        .thenInWorkerThread(
            [asyncSystem, pLogger, &context, useWaterMask](
                std::shared_ptr<IAssetRequest>&& pRequest) mutable {
              const IAssetResponse* pResponse = pRequest->response();
              if (!pResponse) {
                SPDLOG_LOGGER_ERROR(
                    pLogger,
                    "Did not receive a valid response for parent layer.json {}",
                    pRequest->url());
                return;
              }

              if (pResponse->statusCode() != 0 &&
                  (pResponse->statusCode() < 200 ||
                   pResponse->statusCode() >= 300)) {
                SPDLOG_LOGGER_ERROR(
                    pLogger,
                    "Received status code {} for parent layer.json {}",
                    pResponse->statusCode(),
                    pRequest->url());
                return;
              }

              const gsl::span<const std::byte> data = pResponse->data();
              rapidjson::Document parentLayerJson;
              parentLayerJson.Parse(
                  reinterpret_cast<const char*>(data.data()),
                  data.size());

              if (parentLayerJson.HasParseError()) {
                SPDLOG_LOGGER_ERROR(
                    pLogger,
                    "Error when parsing layer.json, error code {} at byte "
                    "offset ",
                    "{}",
                    parentLayerJson.GetParseError(),
                    parentLayerJson.GetErrorOffset());
                return;
              }

              context.pUnderlyingContext = std::make_unique<TileContext>();
              context.pUnderlyingContext->baseUrl = pRequest->url();
              context.pUnderlyingContext->pTileset = context.pTileset;
              context.pUnderlyingContext->requestHeaders =
                  context.requestHeaders;
              Private::workerThreadLoadTileContext(
                  parentLayerJson,
                  *context.pUnderlyingContext,
                  asyncSystem,
                  pLogger,
                  useWaterMask);
            });
  }
  return asyncSystem.createResolvedFuture();
}

Future<void> Tileset::LoadTilesetDotJson::Private::workerThreadLoadTerrainTile(
    Tile& tile,
    const rapidjson::Document& layerJson,
    TileContext& context,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<spdlog::logger>& pLogger,
    bool useWaterMask) {
  context.requestHeaders.push_back(std::make_pair(
      "Accept",
      "application/vnd.quantized-mesh,application/octet-stream;q=0.9,*/"
      "*;q=0.01"));
  return Private::workerThreadLoadTileContext(
             layerJson,
             context,
             asyncSystem,
             pLogger,
             useWaterMask)
      .thenImmediately([&tile, &context]() {
        tile.setContext(&context);
        tile.setBoundingVolume(
            context.implicitContext->implicitRootBoundingVolume);
        tile.setGeometricError(999999999.0);

        const auto& tilingScheme =
            context.implicitContext->quadtreeTilingScheme;
        auto quadtreeXTiles = tilingScheme->getRootTilesX();
        const auto& projection = *context.implicitContext->projection;
        tile.createChildTiles(quadtreeXTiles);

        for (uint32_t i = 0; i < quadtreeXTiles; ++i) {
          Tile& childTile = tile.getChildren()[i];
          QuadtreeTileID id(0, i, 0);

          childTile.setContext(&context);
          childTile.setParent(&tile);
          childTile.setTileID(id);
          const CesiumGeospatial::GlobeRectangle childGlobeRectangle =
              unprojectRectangleSimple(
                  projection,
                  tilingScheme->tileToRectangle(id));
          childTile.setBoundingVolume(
              createDefaultLooseEarthBoundingVolume(childGlobeRectangle));
          childTile.setGeometricError(
              8.0 * calcQuadtreeMaxGeometricError(Ellipsoid::WGS84) *
              childGlobeRectangle.computeWidth());
        }
      });
}
