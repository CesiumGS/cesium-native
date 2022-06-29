#include "LayerJsonTerrainLoader.h"

#include "QuantizedMeshLoader.h"
#include "calcQuadtreeMaxGeometricError.h"
#include "upsampleGltfForRasterOverlays.h"

#include <Cesium3DTilesSelection/GltfUtilities.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Uri.h>

#include <libmorton/morton.h>
#include <rapidjson/document.h>

using namespace CesiumAsync;
using namespace Cesium3DTilesSelection;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace {
TileLoadResult convertToTileLoadResult(QuantizedMeshLoadResult&& loadResult) {
  return TileLoadResult{
      TileRenderContent{std::move(loadResult.model)},
      loadResult.updatedBoundingVolume,
      std::nullopt,
      loadResult.errors.hasErrors() ? TileLoadResultState::Failed
                                    : TileLoadResultState::Success,
      nullptr,
      {}};
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
  assert(
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
      BoundingRegion(globeRectangle, -1000.0, 9000.0));
}

struct LoadLayersResult {
  std::optional<QuadtreeTilingScheme> tilingScheme;
  std::optional<Projection> projection;
  std::optional<BoundingVolume> boundingVolume;
  std::vector<LayerJsonTerrainLoader::Layer> layers;
  ErrorList errors;
};

Future<LoadLayersResult> loadLayersRecursive(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
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

  loadLayersResult.layers.emplace_back(LayerJsonTerrainLoader::Layer{
      baseUrl,
      std::move(version),
      std::move(urls),
      std::move(availability),
      static_cast<uint32_t>(maxZoom),
      availabilityLevels,
      std::move(creditString),
      std::nullopt});

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

    return pAssetAccessor->get(asyncSystem, resolvedUrl, flatHeaders)
        .thenInWorkerThread(
            [asyncSystem,
             pAssetAccessor,
             tilingScheme,
             useWaterMask,
             showCreditsOnScreen,
             loadLayersResult = std::move(loadLayersResult)](
                std::shared_ptr<IAssetRequest>&& pCompletedRequest) mutable {
              const CesiumAsync::IAssetResponse* pResponse =
                  pCompletedRequest->response();
              const std::string& tileUrl = pCompletedRequest->url();
              if (!pResponse) {
                loadLayersResult.errors.emplace_warning(fmt::format(
                    "Did not receive a valid response for parent layer {}",
                    pCompletedRequest->url()));
                return asyncSystem.createResolvedFuture(
                    std::move(loadLayersResult));
              }

              uint16_t statusCode = pResponse->statusCode();
              if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
                loadLayersResult.errors.emplace_warning(fmt::format(
                    "Received status code {} for parent layer {}",
                    statusCode,
                    tileUrl));
                return asyncSystem.createResolvedFuture(
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
                return asyncSystem.createResolvedFuture(
                    std::move(loadLayersResult));
              }

              return loadLayersRecursive(
                  asyncSystem,
                  pAssetAccessor,
                  pCompletedRequest->url(),
                  pCompletedRequest->headers(),
                  layerJson,
                  tilingScheme,
                  useWaterMask,
                  showCreditsOnScreen,
                  std::move(loadLayersResult));
            });
  }

  return asyncSystem.createResolvedFuture(std::move(loadLayersResult));
}

Future<LoadLayersResult> loadLayerJson(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
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
    LoadLayersResult result;
    result.errors.emplace_error(fmt::format(
        "Error when parsing layer.json, error code {} at byte offset {}",
        layerJson.GetParseError(),
        layerJson.GetErrorOffset()));
    return asyncSystem.createResolvedFuture(std::move(result));
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
    LoadLayersResult result;
    result.errors.emplace_error(fmt::format(
        "Tileset layer.json contained an unknown projection value: {}",
        projectionString));
    return asyncSystem.createResolvedFuture(std::move(result));
  }

  BoundingVolume boundingVolume =
      createDefaultLooseEarthBoundingVolume(quadtreeRectangleGlobe);

  CesiumGeometry::QuadtreeTilingScheme tilingScheme(
      quadtreeRectangleProjected,
      quadtreeXTiles,
      1);

  LoadLayersResult
      loadLayersResult{tilingScheme, projection, boundingVolume, {}, {}};

  return loadLayersRecursive(
      asyncSystem,
      pAssetAccessor,
      baseUrl,
      requestHeaders,
      layerJson,
      tilingScheme,
      useWaterMask,
      showCreditsOnScreen,
      std::move(loadLayersResult));
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
      .thenInWorkerThread(
          [asyncSystem = externals.asyncSystem,
           pAssetAccessor = externals.pAssetAccessor,
           useWaterMask,
           showCreditsOnScreen](
              std::shared_ptr<CesiumAsync::IAssetRequest>&& pCompletedRequest) {
            const CesiumAsync::IAssetResponse* pResponse =
                pCompletedRequest->response();
            const std::string& tileUrl = pCompletedRequest->url();
            if (!pResponse) {
              LoadLayersResult result;
              result.errors.emplace_error(fmt::format(
                  "Did not receive a valid response for tile content {}",
                  tileUrl));
              return asyncSystem.createResolvedFuture(std::move(result));
            }

            uint16_t statusCode = pResponse->statusCode();
            if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
              LoadLayersResult result;
              result.errors.emplace_error(fmt::format(
                  "Received status code {} for tile content {}",
                  statusCode,
                  tileUrl));
              return asyncSystem.createResolvedFuture(std::move(result));
            }

            return loadLayerJson(
                asyncSystem,
                pAssetAccessor,
                pCompletedRequest->url(),
                pCompletedRequest->headers(),
                pResponse->data(),
                useWaterMask,
                showCreditsOnScreen);
          })
      .thenInMainThread([](LoadLayersResult&& loadLayersResult) {
        if (!loadLayersResult.tilingScheme || !loadLayersResult.projection ||
            !loadLayersResult.boundingVolume) {
          TilesetContentLoaderResult result;
          result.errors.emplace_error(
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

        uint32_t quadtreeXTiles =
            loadLayersResult.tilingScheme->getRootTilesX();

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

LayerJsonTerrainLoader::Layer::Layer(
    const std::string& baseUrl_,
    std::string&& version_,
    std::vector<std::string>&& tileTemplateUrls_,
    CesiumGeometry::QuadtreeRectangleAvailability&& contentAvailability_,
    uint32_t maxZooms_,
    int32_t availabilityLevels_,
    std::string&& creditString_,
    std::optional<Credit> credit_)
    : baseUrl{baseUrl_},
      version{std::move(version_)},
      tileTemplateUrls{std::move(tileTemplateUrls_)},
      contentAvailability{std::move(contentAvailability_)},
      loadedSubtrees(maxSubtreeInLayer(maxZooms_, availabilityLevels_)),
      availabilityLevels{availabilityLevels_},
      creditString{std::move(creditString_)},
      credit{credit_} {}

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

  return CesiumUtility::Uri::resolve(
      layer.baseUrl,
      CesiumUtility::Uri::substituteTemplateParameters(
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
          }));
}

Future<QuantizedMeshLoadResult> requestTileContent(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const QuadtreeTileID& tileID,
    const BoundingVolume& boundingVolume,
    const LayerJsonTerrainLoader::Layer& layer,
    const std::vector<IAssetAccessor::THeader>& requestHeaders,
    bool enableWaterMask) {
  std::string url = resolveTileUrl(tileID, layer);
  return pAssetAccessor->get(asyncSystem, url, requestHeaders)
      .thenInWorkerThread(
          [asyncSystem, pLogger, tileID, boundingVolume, enableWaterMask](
              std::shared_ptr<IAssetRequest>&& pRequest) {
            const IAssetResponse* pResponse = pRequest->response();
            if (!pResponse) {
              QuantizedMeshLoadResult result;
              result.errors.emplace_error(fmt::format(
                  "Did not receive a valid response for tile content {}",
                  pRequest->url()));
              return result;
            }

            if (pResponse->statusCode() != 0 &&
                (pResponse->statusCode() < 200 ||
                 pResponse->statusCode() >= 300)) {
              QuantizedMeshLoadResult result;
              return result;
            }

            return QuantizedMeshLoader::load(
                tileID,
                boundingVolume,
                pRequest->url(),
                pResponse->data(),
                enableWaterMask);
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

Future<TileLoadResult> LayerJsonTerrainLoader::loadTileContent(
    Tile& tile,
    const TilesetContentOptions& contentOptions,
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::vector<IAssetAccessor::THeader>& requestHeaders) {
  // This type of loader should never have child loaders.
  assert(tile.getContent().getLoader() == this);

  const QuadtreeTileID* pQuadtreeTileID =
      std::get_if<QuadtreeTileID>(&tile.getTileID());
  if (!pQuadtreeTileID) {
    // check if we need to upsample this tile
    const UpsampledQuadtreeNode* pUpsampleTileID =
        std::get_if<UpsampledQuadtreeNode>(&tile.getTileID());
    if (!pUpsampleTileID) {
      // This loader only handles QuadtreeTileIDs and UpsampledQuadtreeNode.
      return asyncSystem.createResolvedFuture<TileLoadResult>(TileLoadResult{
          TileUnknownContent{},
          std::nullopt,
          std::nullopt,
          TileLoadResultState::Failed,
          nullptr,
          {}});
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
    return asyncSystem.createResolvedFuture<TileLoadResult>(TileLoadResult{
        TileUnknownContent{},
        std::nullopt,
        std::nullopt,
        TileLoadResultState::Failed,
        nullptr,
        {}});
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

  // Start the actual content request.
  auto& currentLayer = *firstAvailableIt;
  Future<QuantizedMeshLoadResult> futureQuantizedMesh = requestTileContent(
      pLogger,
      asyncSystem,
      pAssetAccessor,
      *pQuadtreeTileID,
      tile.getBoundingVolume(),
      currentLayer,
      requestHeaders,
      contentOptions.enableWaterMask);

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
        .thenInMainThread([&currentLayer,
                           tileID = *pQuadtreeTileID,
                           shouldCurrLayerLoadAvailability](
                              QuantizedMeshLoadResult&& loadResult) {
          if (shouldCurrLayerLoadAvailability) {
            addRectangleAvailabilityToLayer(
                currentLayer,
                tileID,
                loadResult.availableTileRectangles);
          }

          return convertToTileLoadResult(std::move(loadResult));
        });
  }

  return std::move(futureQuantizedMesh)
      .thenImmediately([](QuantizedMeshLoadResult&& loadResult) mutable {
        return convertToTileLoadResult(std::move(loadResult));
      });
}

bool LayerJsonTerrainLoader::updateTileContent(Tile& tile) {
  const CesiumGeometry::QuadtreeTileID* pQuadtreeID =
      std::get_if<CesiumGeometry::QuadtreeTileID>(&tile.getTileID());
  if (pQuadtreeID) {
    if (tile.getChildren().empty()) {
      // For the tile that is in the middle of subtree, it is safe to create the
      // children. However for tile that is at the availability level, we have
      // to wait for the tile to be finished loading, since there are multiple
      // subtrees in the background layers being on the flight as well. Once the
      // tile finishes loading, all the subtrees are resolved
      bool isTileAtAvailabilityLevel = false;
      for (const auto& layer : _layers) {
        if (layer.availabilityLevels > 0 &&
            int32_t(pQuadtreeID->level) % layer.availabilityLevels == 0) {
          isTileAtAvailabilityLevel = true;
          break;
        }
      }

      if (isTileAtAvailabilityLevel &&
          tile.getState() <= TileLoadState::ContentLoading) {
        return true;
      }

      createTileChildren(tile);
      return false;
    }

    return true;
  }

  return false;
}

void LayerJsonTerrainLoader::createTileChildren(Tile& tile) {

  if (tile.getChildren().empty()) {
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

      tile.createChildTiles(std::move(children));
    }
  }
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
  child.setBoundingVolume(BoundingRegionWithLooseFittingHeights(
      BoundingRegion(childGlobeRectangle, minHeight, maxHeight)));
}

CesiumAsync::Future<TileLoadResult> LayerJsonTerrainLoader::upsampleParentTile(
    Tile& tile,
    const CesiumAsync::AsyncSystem& asyncSystem) {
  Tile* pParent = tile.getParent();
  TileContent& parentContent = pParent->getContent();
  TileRenderContent* pParentRenderContent = parentContent.getRenderContent();
  if (!pParentRenderContent || !pParentRenderContent->model) {
    return asyncSystem.createResolvedFuture(TileLoadResult{
        TileUnknownContent{},
        std::nullopt,
        std::nullopt,
        TileLoadResultState::Failed,
        nullptr,
        {}});
  }

  const UpsampledQuadtreeNode* pUpsampledTileID =
      std::get_if<UpsampledQuadtreeNode>(&tile.getTileID());

  int32_t index = -1;
  const std::vector<CesiumGeospatial::Projection>& parentProjections =
      parentContent.getRasterOverlayDetails().rasterOverlayProjections;
  auto it = std::find(
      parentProjections.begin(),
      parentProjections.end(),
      _projection);
  if (it != parentProjections.end()) {
    index = int32_t(it - parentProjections.begin());
  } else {
    const BoundingRegion* pParentRegion =
        std::get_if<BoundingRegion>(&pParent->getBoundingVolume());

    std::optional<RasterOverlayDetails> overlayDetails =
        GltfUtilities::createRasterOverlayTextureCoordinates(
            *pParentRenderContent->model,
            pParent->getTransform(),
            int32_t(parentProjections.size()),
            pParentRegion ? std::make_optional<GlobeRectangle>(
                                pParentRegion->getRectangle())
                          : std::nullopt,
            {_projection});

    if (overlayDetails) {
      index = int32_t(parentProjections.size());
      parentContent.getRasterOverlayDetails().merge(std::move(*overlayDetails));
    }
  }

  // Cannot find raster overlay UVs that has this projection, so we can't
  // upsample right now
  if (index == -1) {
    return asyncSystem.createResolvedFuture(TileLoadResult{
        TileUnknownContent{},
        std::nullopt,
        std::nullopt,
        TileLoadResultState::Failed,
        nullptr,
        {}});
  }

  const CesiumGltf::Model& parentModel = pParentRenderContent->model.value();
  return asyncSystem.runInWorkerThread(
      [&parentModel,
       boundingVolume = tile.getBoundingVolume(),
       textureCoordinateIndex = index,
       tileID = *pUpsampledTileID]() mutable {
        auto model = upsampleGltfForRasterOverlays(
            parentModel,
            tileID,
            textureCoordinateIndex);
        if (!model) {
          return TileLoadResult{
              TileRenderContent{std::nullopt},
              std::nullopt,
              std::nullopt,
              TileLoadResultState::Failed,
              nullptr,
              {}};
        }

        return TileLoadResult{
            TileRenderContent{std::move(model)},
            std::nullopt,
            std::nullopt,
            TileLoadResultState::Success,
            nullptr,
            {}};
      });
}
