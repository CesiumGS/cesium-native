
#include "Cesium3DTilesSelection/Tileset.h"

#include "Cesium3DTilesSelection/CreditSystem.h"
#include "Cesium3DTilesSelection/ExternalTilesetContent.h"
#include "Cesium3DTilesSelection/ITileExcluder.h"
#include "Cesium3DTilesSelection/RasterOverlayTile.h"
#include "Cesium3DTilesSelection/RasterizedPolygonsOverlay.h"
#include "Cesium3DTilesSelection/TileID.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"
#include "TileUtilities.h"
#include "calcQuadtreeMaxGeometricError.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumAsync/ITaskProcessor.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/QuadtreeTileAvailability.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Math.h>
#include <CesiumUtility/Tracing.h>
#include <CesiumUtility/Uri.h>

#include <glm/common.hpp>
#include <rapidjson/document.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <unordered_set>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

Tileset::Tileset(
    const TilesetExternals& externals,
    const std::string& url,
    const TilesetOptions& options)
    : _externals(externals),
      _asyncSystem(externals.asyncSystem),
      _userCredit(
          (options.credit && externals.pCreditSystem)
              ? std::optional<Credit>(externals.pCreditSystem->createCredit(
                    options.credit.value()))
              : std::nullopt),
      _url(url),
      _isRefreshingIonToken(false),
      _options(options),
      _pRootTile(),
      _previousFrameNumber(0),
      _loadsInProgress(0),
      _overlays(*this),
      _tileDataBytes(0),
      _supportsRasterOverlays(false),
      _gltfUpAxis(CesiumGeometry::Axis::Y),
      _distancesStack(),
      _nextDistancesVector(0) {
  CESIUM_TRACE_USE_TRACK_SET(this->_loadingSlots);
  ++this->_loadsInProgress;
  this->_loadTilesetJson(url);
}

Tileset::Tileset(
    const TilesetExternals& externals,
    uint32_t ionAssetID,
    const std::string& ionAccessToken,
    const TilesetOptions& options)
    : _externals(externals),
      _asyncSystem(externals.asyncSystem),
      _userCredit(
          (options.credit && externals.pCreditSystem)
              ? std::optional<Credit>(externals.pCreditSystem->createCredit(
                    options.credit.value()))
              : std::nullopt),
      _ionAssetID(ionAssetID),
      _ionAccessToken(ionAccessToken),
      _isRefreshingIonToken(false),
      _options(options),
      _pRootTile(),
      _previousFrameNumber(0),
      _loadsInProgress(0),
      _overlays(*this),
      _tileDataBytes(0),
      _supportsRasterOverlays(false),
      _gltfUpAxis(CesiumGeometry::Axis::Y),
      _distancesStack(),
      _nextDistancesVector(0) {
  CESIUM_TRACE_USE_TRACK_SET(this->_loadingSlots);
  CESIUM_TRACE_BEGIN_IN_TRACK("Tileset from ion startup");

  std::string ionUrl = "https://api.cesium.com/v1/assets/" +
                       std::to_string(ionAssetID) + "/endpoint";
  if (!ionAccessToken.empty()) {
    ionUrl += "?access_token=" + ionAccessToken;
  }

  ++this->_loadsInProgress;

  this->_externals.pAssetAccessor->requestAsset(this->_asyncSystem, ionUrl)
      .thenInMainThread([this](std::shared_ptr<IAssetRequest>&& pRequest) {
        return this->_handleAssetResponse(std::move(pRequest));
      })
      .catchInMainThread([this, &ionAssetID](const std::exception& e) {
        SPDLOG_LOGGER_ERROR(
            this->_externals.pLogger,
            "Unhandled error for asset {}: {}",
            ionAssetID,
            e.what());
        this->notifyTileDoneLoading(nullptr);
      })
      .thenImmediately(
          []() { CESIUM_TRACE_END_IN_TRACK("Tileset from ion startup"); });
}

Tileset::~Tileset() {
  // Wait for all asynchronous loading to terminate.
  // If you're hanging here, it's most likely caused by _loadsInProgress not
  // being decremented correctly when an async load ends.
  while (this->_loadsInProgress.load(std::memory_order::memory_order_acquire) >
         0) {
    this->_externals.pAssetAccessor->tick();
    this->_asyncSystem.dispatchMainThreadTasks();
  }

  // Wait for all overlays to wrap up their loading, too.
  uint32_t tilesLoading = 1;
  while (tilesLoading > 0) {
    this->_externals.pAssetAccessor->tick();
    this->_asyncSystem.dispatchMainThreadTasks();

    tilesLoading = 0;
    for (auto& pOverlay : this->_overlays) {
      tilesLoading += pOverlay->getTileProvider()->getNumberOfTilesLoading();
    }
  }
}

Future<void>
Tileset::_handleAssetResponse(std::shared_ptr<IAssetRequest>&& pRequest) {
  const IAssetResponse* pResponse = pRequest->response();
  if (!pResponse) {
    SPDLOG_LOGGER_ERROR(
        this->_externals.pLogger,
        "No response received for asset request {}",
        pRequest->url());
    this->notifyTileDoneLoading(nullptr);
    return this->getAsyncSystem().createResolvedFuture();
  }

  if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
    SPDLOG_LOGGER_ERROR(
        this->_externals.pLogger,
        "Received status code {} for asset response {}",
        pResponse->statusCode(),
        pRequest->url());
    this->notifyTileDoneLoading(nullptr);
    return this->getAsyncSystem().createResolvedFuture();
  }

  const gsl::span<const std::byte> data = pResponse->data();

  rapidjson::Document ionResponse;
  ionResponse.Parse(reinterpret_cast<const char*>(data.data()), data.size());

  if (ionResponse.HasParseError()) {
    SPDLOG_LOGGER_ERROR(
        this->_externals.pLogger,
        "Error when parsing Cesium ion response JSON, error code {} at byte "
        "offset {}",
        ionResponse.GetParseError(),
        ionResponse.GetErrorOffset());
    return this->getAsyncSystem().createResolvedFuture();
  }

  if (this->_externals.pCreditSystem) {

    const auto attributionsIt = ionResponse.FindMember("attributions");
    if (attributionsIt != ionResponse.MemberEnd() &&
        attributionsIt->value.IsArray()) {

      for (const rapidjson::Value& attribution :
           attributionsIt->value.GetArray()) {

        const auto html = attribution.FindMember("html");
        if (html != attribution.MemberEnd() && html->value.IsString()) {
          this->_tilesetCredits.push_back(
              this->_externals.pCreditSystem->createCredit(
                  html->value.GetString()));
        }
        // TODO: mandate the user show certain credits on screen, as opposed to
        // an expandable panel auto showOnScreen =
        // attribution.FindMember("collapsible");
        // ...
      }
    }
  }

  std::string url = JsonHelpers::getStringOrDefault(ionResponse, "url", "");
  std::string accessToken =
      JsonHelpers::getStringOrDefault(ionResponse, "accessToken", "");

  std::string type = JsonHelpers::getStringOrDefault(ionResponse, "type", "");
  if (type == "TERRAIN") {
    // For terrain resources, we need to append `/layer.json` to the end of the
    // URL.
    url = CesiumUtility::Uri::resolve(url, "layer.json", true);
  } else if (type != "3DTILES") {
    SPDLOG_LOGGER_ERROR(
        this->_externals.pLogger,
        "Received unsupported asset response type: {}",
        type);
    this->notifyTileDoneLoading(nullptr);
    return this->getAsyncSystem().createResolvedFuture();
  }

  auto pContext = std::make_unique<TileContext>();

  pContext->pTileset = this;
  pContext->baseUrl = url;
  pContext->requestHeaders.push_back(
      std::make_pair("Authorization", "Bearer " + accessToken));
  pContext->failedTileCallback = [this](Tile& failedTile) {
    return this->_onIonTileFailed(failedTile);
  };
  return this->_loadTilesetJson(
      pContext->baseUrl,
      pContext->requestHeaders,
      std::move(pContext));
}

static bool
operator<(const FogDensityAtHeight& fogDensity, double height) noexcept {
  return fogDensity.cameraHeight < height;
}

static double computeFogDensity(
    const std::vector<FogDensityAtHeight>& fogDensityTable,
    const ViewState& viewState) {
  const double height = viewState.getPositionCartographic()
                            .value_or(Cartographic(0.0, 0.0, 0.0))
                            .height;

  // Find the entry that is for >= this camera height.
  auto nextIt =
      std::lower_bound(fogDensityTable.begin(), fogDensityTable.end(), height);

  if (nextIt == fogDensityTable.end()) {
    return fogDensityTable.back().fogDensity;
  }
  if (nextIt == fogDensityTable.begin()) {
    return nextIt->fogDensity;
  }

  auto prevIt = nextIt - 1;

  const double heightA = prevIt->cameraHeight;
  const double densityA = prevIt->fogDensity;

  const double heightB = nextIt->cameraHeight;
  const double densityB = nextIt->fogDensity;

  const double t =
      glm::clamp((height - heightA) / (heightB - heightA), 0.0, 1.0);

  const double density = glm::mix(densityA, densityB, t);

  // CesiumJS will also fade out the fog based on the camera angle,
  // so when we're looking straight down there's no fog. This is unfortunate
  // because it prevents the fog culling from being used in place of horizon
  // culling. Horizon culling is the only thing in CesiumJS that prevents
  // tiles on the back side of the globe from being rendered.
  // Since we're not actually _rendering_ the fog in cesium-native (that's on
  // the renderer), we don't need to worry about the fog making the globe
  // looked washed out in straight down views. So here we don't fade by
  // angle at all.

  return density;
}

const ViewUpdateResult&
Tileset::updateViewOffline(const std::vector<ViewState>& frustums) {
  std::vector<Tile*> tilesRenderedPrevFrame =
      this->_updateResult.tilesToRenderThisFrame;

  this->updateView(frustums);
  while (this->_loadsInProgress > 0) {
    this->_externals.pAssetAccessor->tick();
    this->updateView(frustums);
  }

  std::unordered_set<Tile*> uniqueTilesToRenderedThisFrame(
      this->_updateResult.tilesToRenderThisFrame.begin(),
      this->_updateResult.tilesToRenderThisFrame.end());
  std::vector<Tile*> tilesToNoLongerRenderThisFrame;
  for (Tile* tile : tilesRenderedPrevFrame) {
    if (uniqueTilesToRenderedThisFrame.find(tile) ==
        uniqueTilesToRenderedThisFrame.end()) {
      tilesToNoLongerRenderThisFrame.emplace_back(tile);
    }
  }

  this->_updateResult.tilesToNoLongerRenderThisFrame =
      std::move(tilesToNoLongerRenderThisFrame);
  return this->_updateResult;
}

const ViewUpdateResult&
Tileset::updateView(const std::vector<ViewState>& frustums) {
  this->_asyncSystem.dispatchMainThreadTasks();

  const int32_t previousFrameNumber = this->_previousFrameNumber;
  const int32_t currentFrameNumber = previousFrameNumber + 1;

  ViewUpdateResult& result = this->_updateResult;
  // result.tilesLoading = 0;
  result.tilesToRenderThisFrame.clear();
  // result.newTilesToRenderThisFrame.clear();
  result.tilesToNoLongerRenderThisFrame.clear();
  result.tilesVisited = 0;
  result.culledTilesVisited = 0;
  result.tilesCulled = 0;
  result.maxDepthVisited = 0;

  Tile* pRootTile = this->getRootTile();
  if (!pRootTile) {
    return result;
  }

  if (!this->supportsRasterOverlays() && this->_overlays.size() > 0) {
    this->_externals.pLogger->warn(
        "Only quantized-mesh terrain tilesets currently support overlays.");
  }

  this->_loadQueueHigh.clear();
  this->_loadQueueMedium.clear();
  this->_loadQueueLow.clear();

  std::vector<double> fogDensities(frustums.size());
  std::transform(
      frustums.begin(),
      frustums.end(),
      fogDensities.begin(),
      [&fogDensityTable =
           this->_options.fogDensityTable](const ViewState& frustum) -> double {
        return computeFogDensity(fogDensityTable, frustum);
      });

  FrameState frameState{
      frustums,
      std::move(fogDensities),
      previousFrameNumber,
      currentFrameNumber};

  if (!frustums.empty()) {
    this->_visitTileIfNeeded(frameState, 0, false, *pRootTile, result);
  } else {
    result = ViewUpdateResult();
  }

  result.tilesLoadingLowPriority =
      static_cast<uint32_t>(this->_loadQueueLow.size());
  result.tilesLoadingMediumPriority =
      static_cast<uint32_t>(this->_loadQueueMedium.size());
  result.tilesLoadingHighPriority =
      static_cast<uint32_t>(this->_loadQueueHigh.size());

  this->_unloadCachedTiles();
  this->_processLoadQueue();

  // aggregate all the credits needed from this tileset for the current frame
  const std::shared_ptr<CreditSystem>& pCreditSystem =
      this->_externals.pCreditSystem;
  if (pCreditSystem && !result.tilesToRenderThisFrame.empty()) {
    // per-tileset user-specified credit
    if (this->_userCredit) {
      pCreditSystem->addCreditToFrame(this->_userCredit.value());
    }

    // per-tileset ion-specified credit
    for (const Credit& credit : this->_tilesetCredits) {
      pCreditSystem->addCreditToFrame(credit);
    }

    // per-raster overlay credit
    for (auto& pOverlay : this->_overlays) {
      const std::optional<Credit>& overlayCredit =
          pOverlay->getTileProvider()->getCredit();
      if (overlayCredit) {
        pCreditSystem->addCreditToFrame(overlayCredit.value());
      }
    }

    // per-tile credits
    for (auto& tile : result.tilesToRenderThisFrame) {
      const std::vector<RasterMappedTo3DTile>& mappedRasterTiles =
          tile->getMappedRasterTiles();
      for (const RasterMappedTo3DTile& mappedRasterTile : mappedRasterTiles) {
        const RasterOverlayTile* pRasterOverlayTile =
            mappedRasterTile.getReadyTile();
        if (pRasterOverlayTile != nullptr) {
          for (const Credit credit : pRasterOverlayTile->getCredits()) {
            pCreditSystem->addCreditToFrame(credit);
          }
        }
      }
    }
  }

  this->_previousFrameNumber = currentFrameNumber;

  return result;
}

void Tileset::notifyTileStartLoading(Tile* pTile) noexcept {
  ++this->_loadsInProgress;

  if (pTile) {
    CESIUM_TRACE_BEGIN_IN_TRACK(
        TileIdUtilities::createTileIdString(pTile->getTileID()).c_str());
  }
}

void Tileset::notifyTileDoneLoading(Tile* pTile) noexcept {
  assert(this->_loadsInProgress > 0);
  --this->_loadsInProgress;

  if (pTile) {
    this->_tileDataBytes += pTile->computeByteSize();

    CESIUM_TRACE_END_IN_TRACK(
        TileIdUtilities::createTileIdString(pTile->getTileID()).c_str());
  }
}

void Tileset::notifyTileUnloading(Tile* pTile) noexcept {
  if (pTile) {
    this->_tileDataBytes -= pTile->computeByteSize();
  }
}

void Tileset::loadTilesFromJson(
    Tile& rootTile,
    const rapidjson::Value& tilesetJson,
    const glm::dmat4& parentTransform,
    TileRefine parentRefine,
    const TileContext& context,
    const std::shared_ptr<spdlog::logger>& pLogger) {
  Tileset::_createTile(
      rootTile,
      tilesetJson["root"],
      parentTransform,
      parentRefine,
      context,
      pLogger);
}

std::optional<CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>>
Tileset::requestTileContent(Tile& tile) {
  std::string url = this->getResolvedContentUrl(tile);
  if (url.empty()) {
    return std::nullopt;
  }

  this->notifyTileStartLoading(&tile);

  return this->getExternals().pAssetAccessor->requestAsset(
      this->getAsyncSystem(),
      url,
      tile.getContext()->requestHeaders);
}

std::optional<CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>>
requestAvailabilitySubtree(Tile& tile) {
  if (tile.getContext()->implicitContext) {

  }

  return std::nullopt;
}

void Tileset::addContext(std::unique_ptr<TileContext>&& pNewContext) {
  this->_contexts.push_back(std::move(pNewContext));
}

void Tileset::forEachLoadedTile(
    const std::function<void(Tile& tile)>& callback) {
  Tile* pCurrent = this->_loadedTiles.head();
  while (pCurrent) {
    Tile* pNext = this->_loadedTiles.next(pCurrent);
    callback(*pCurrent);
    pCurrent = pNext;
  }
}

int64_t Tileset::getTotalDataBytes() const noexcept {
  int64_t bytes = this->_tileDataBytes;

  for (auto& pOverlay : this->_overlays) {
    const RasterOverlayTileProvider* pProvider = pOverlay->getTileProvider();
    if (pProvider) {
      bytes += pProvider->getTileDataBytes();
    }
  }

  return bytes;
}

Future<void> Tileset::_loadTilesetJson(
    const std::string& url,
    const std::vector<std::pair<std::string, std::string>>& headers,
    std::unique_ptr<TileContext>&& pContext) {
  if (!pContext) {
    pContext = std::make_unique<TileContext>();
  }
  pContext->pTileset = this;

  CESIUM_TRACE_BEGIN_IN_TRACK("Load tileset.json");

  return this->getExternals()
      .pAssetAccessor->requestAsset(this->getAsyncSystem(), url, headers)
      .thenInWorkerThread(
          [pLogger = this->_externals.pLogger,
           pContext = std::move(pContext),
           useWaterMask = this->getOptions().contentOptions.enableWaterMask](
              std::shared_ptr<IAssetRequest>&& pRequest) mutable {
            return Tileset::_handleTilesetResponse(
                std::move(pRequest),
                std::move(pContext),
                pLogger,
                useWaterMask);
          })
      .thenInMainThread([this](LoadResult&& loadResult) {
        this->_supportsRasterOverlays = loadResult.supportsRasterOverlays;
        this->addContext(std::move(loadResult.pContext));
        this->_pRootTile = std::move(loadResult.pRootTile);
        this->notifyTileDoneLoading(nullptr);
        CESIUM_TRACE_END_IN_TRACK("Load tileset.json");
      })
      .catchInMainThread([this, url](const std::exception& e) {
        SPDLOG_LOGGER_ERROR(
            this->_externals.pLogger,
            "Unhandled error for tileset {}: {}",
            url,
            e.what());
        this->_pRootTile.reset();
        this->notifyTileDoneLoading(nullptr);
        CESIUM_TRACE_END_IN_TRACK("Load tileset.json");
      });
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

/*static*/ Tileset::LoadResult Tileset::_handleTilesetResponse(
    std::shared_ptr<IAssetRequest>&& pRequest,
    std::unique_ptr<TileContext>&& pContext,
    const std::shared_ptr<spdlog::logger>& pLogger,
    bool useWaterMask) {
  const IAssetResponse* pResponse = pRequest->response();
  if (!pResponse) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Did not receive a valid response for tileset {}",
        pRequest->url());
    return LoadResult{std::move(pContext), nullptr, false};
  }

  if (pResponse->statusCode() != 0 &&
      (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300)) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Received status code {} for tileset {}",
        pResponse->statusCode(),
        pRequest->url());
    return LoadResult{std::move(pContext), nullptr, false};
  }

  pContext->baseUrl = pRequest->url();

  const gsl::span<const std::byte> data = pResponse->data();

  rapidjson::Document tileset;
  tileset.Parse(reinterpret_cast<const char*>(data.data()), data.size());

  if (tileset.HasParseError()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Error when parsing tileset JSON, error code {} at byte offset {}",
        tileset.GetParseError(),
        tileset.GetErrorOffset());
    return LoadResult{std::move(pContext), nullptr, false};
  }

  pContext->pTileset->_gltfUpAxis = obtainGltfUpAxis(tileset);

  std::unique_ptr<Tile> pRootTile = std::make_unique<Tile>();
  pRootTile->setContext(pContext.get());

  const auto rootIt = tileset.FindMember("root");
  const auto formatIt = tileset.FindMember("format");

  bool supportsRasterOverlays = false;

  if (rootIt != tileset.MemberEnd()) {
    const rapidjson::Value& rootJson = rootIt->value;
    Tileset::_createTile(
        *pRootTile,
        rootJson,
        glm::dmat4(1.0),
        TileRefine::Replace,
        *pContext,
        pLogger);
  } else if (
      formatIt != tileset.MemberEnd() && formatIt->value.IsString() &&
      std::string(formatIt->value.GetString()) == "quantized-mesh-1.0") {
    Tileset::_createTerrainTile(
        *pRootTile,
        tileset,
        *pContext,
        pLogger,
        useWaterMask);
    supportsRasterOverlays = true;
  }

  return LoadResult{
      std::move(pContext),
      std::move(pRootTile),
      supportsRasterOverlays};
}

static std::optional<BoundingVolume> getBoundingVolumeProperty(
    const rapidjson::Value& tileJson,
    const std::string& key) {
  const auto bvIt = tileJson.FindMember(key.c_str());
  if (bvIt == tileJson.MemberEnd() || !bvIt->value.IsObject()) {
    return std::nullopt;
  }

  const auto boxIt = bvIt->value.FindMember("box");
  if (boxIt != bvIt->value.MemberEnd() && boxIt->value.IsArray() &&
      boxIt->value.Size() >= 12) {
    const auto& a = boxIt->value.GetArray();
    for (rapidjson::SizeType i = 0; i < 12; ++i) {
      if (!a[i].IsNumber()) {
        return std::nullopt;
      }
    }
    return OrientedBoundingBox(
        glm::dvec3(a[0].GetDouble(), a[1].GetDouble(), a[2].GetDouble()),
        glm::dmat3(
            a[3].GetDouble(),
            a[4].GetDouble(),
            a[5].GetDouble(),
            a[6].GetDouble(),
            a[7].GetDouble(),
            a[8].GetDouble(),
            a[9].GetDouble(),
            a[10].GetDouble(),
            a[11].GetDouble()));
  }

  const auto regionIt = bvIt->value.FindMember("region");
  if (regionIt != bvIt->value.MemberEnd() && regionIt->value.IsArray() &&
      regionIt->value.Size() >= 6) {
    const auto& a = regionIt->value;
    for (rapidjson::SizeType i = 0; i < 6; ++i) {
      if (!a[i].IsNumber()) {
        return std::nullopt;
      }
    }
    return BoundingRegion(
        GlobeRectangle(
            a[0].GetDouble(),
            a[1].GetDouble(),
            a[2].GetDouble(),
            a[3].GetDouble()),
        a[4].GetDouble(),
        a[5].GetDouble());
  }

  const auto sphereIt = bvIt->value.FindMember("sphere");
  if (sphereIt != bvIt->value.MemberEnd() && sphereIt->value.IsArray() &&
      sphereIt->value.Size() >= 4) {
    const auto& a = sphereIt->value;
    for (rapidjson::SizeType i = 0; i < 4; ++i) {
      if (!a[i].IsNumber()) {
        return std::nullopt;
      }
    }
    return BoundingSphere(
        glm::dvec3(a[0].GetDouble(), a[1].GetDouble(), a[2].GetDouble()),
        a[3].GetDouble());
  }

  return std::nullopt;
}

/*static*/ void Tileset::_createTile(
    Tile& tile,
    const rapidjson::Value& tileJson,
    const glm::dmat4& parentTransform,
    TileRefine parentRefine,
    const TileContext& context,
    const std::shared_ptr<spdlog::logger>& pLogger) {
  if (!tileJson.IsObject()) {
    return;
  }

  tile.setContext(const_cast<TileContext*>(&context));

  const std::optional<glm::dmat4x4> tileTransform =
      JsonHelpers::getTransformProperty(tileJson, "transform");
  glm::dmat4x4 transform =
      parentTransform * tileTransform.value_or(glm::dmat4x4(1.0));
  tile.setTransform(transform);

  const auto contentIt = tileJson.FindMember("content");
  const auto childrenIt = tileJson.FindMember("children");

  if (contentIt != tileJson.MemberEnd() && contentIt->value.IsObject()) {
    auto uriIt = contentIt->value.FindMember("uri");
    if (uriIt == contentIt->value.MemberEnd() || !uriIt->value.IsString()) {
      uriIt = contentIt->value.FindMember("url");
    }

    if (uriIt != contentIt->value.MemberEnd() && uriIt->value.IsString()) {
      tile.setTileID(uriIt->value.GetString());
    }

    std::optional<BoundingVolume> contentBoundingVolume =
        getBoundingVolumeProperty(contentIt->value, "boundingVolume");
    if (contentBoundingVolume) {
      tile.setContentBoundingVolume(
          transformBoundingVolume(transform, contentBoundingVolume.value()));
    }
  }

  std::optional<BoundingVolume> boundingVolume =
      getBoundingVolumeProperty(tileJson, "boundingVolume");
  if (!boundingVolume) {
    SPDLOG_LOGGER_ERROR(pLogger, "Tile did not contain a boundingVolume");
    return;
  }

  std::optional<double> geometricError =
      JsonHelpers::getScalarProperty(tileJson, "geometricError");
  if (!geometricError) {
    SPDLOG_LOGGER_ERROR(pLogger, "Tile did not contain a geometricError");
    return;
  }

  tile.setBoundingVolume(
      transformBoundingVolume(transform, boundingVolume.value()));
  const glm::dvec3 scale = glm::dvec3(
      glm::length(transform[0]),
      glm::length(transform[1]),
      glm::length(transform[2]));
  const double maxScaleComponent =
      glm::max(scale.x, glm::max(scale.y, scale.z));
  tile.setGeometricError(geometricError.value() * maxScaleComponent);

  std::optional<BoundingVolume> viewerRequestVolume =
      getBoundingVolumeProperty(tileJson, "viewerRequestVolume");
  if (viewerRequestVolume) {
    tile.setViewerRequestVolume(
        transformBoundingVolume(transform, viewerRequestVolume.value()));
  }

  const auto refineIt = tileJson.FindMember("refine");
  if (refineIt != tileJson.MemberEnd() && refineIt->value.IsString()) {
    std::string refine = refineIt->value.GetString();
    if (refine == "REPLACE") {
      tile.setRefine(TileRefine::Replace);
    } else if (refine == "ADD") {
      tile.setRefine(TileRefine::Add);
    } else {
      SPDLOG_LOGGER_ERROR(
          pLogger,
          "Tile contained an unknown refine value: {}",
          refine);
    }
  } else {
    tile.setRefine(parentRefine);
  }

  if (childrenIt != tileJson.MemberEnd() && childrenIt->value.IsArray()) {
    const auto& childrenJson = childrenIt->value;
    tile.createChildTiles(childrenJson.Size());
    const gsl::span<Tile> childTiles = tile.getChildren();

    for (rapidjson::SizeType i = 0; i < childrenJson.Size(); ++i) {
      const auto& childJson = childrenJson[i];
      Tile& child = childTiles[i];
      child.setParent(&tile);
      Tileset::_createTile(
          child,
          childJson,
          transform,
          tile.getRefine(),
          context,
          pLogger);
    }
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
static std::string createExtensionsQueryParameter(
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
static BoundingVolume createDefaultLooseEarthBoundingVolume(
    const CesiumGeospatial::GlobeRectangle& globeRectangle) {
  return BoundingRegionWithLooseFittingHeights(
      BoundingRegion(globeRectangle, -1000.0, -9000.0));
}

/*static*/ void Tileset::_createTerrainTile(
    Tile& tile,
    const rapidjson::Value& layerJson,
    TileContext& context,
    const std::shared_ptr<spdlog::logger>& pLogger,
    bool useWaterMask) {
  context.requestHeaders.push_back(std::make_pair(
      "Accept",
      "application/vnd.quantized-mesh,application/octet-stream;q=0.9,*/"
      "*;q=0.01"));

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
    return;
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
      std::make_optional<CesiumGeometry::QuadtreeTileAvailability>(
          *tilingScheme,
          maxZoom),
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

  tile.setContext(&context);
  tile.setBoundingVolume(boundingVolume);
  tile.setGeometricError(999999999.0);
  tile.createChildTiles(quadtreeXTiles);

  for (uint32_t i = 0; i < quadtreeXTiles; ++i) {
    Tile& childTile = tile.getChildren()[i];
    QuadtreeTileID id(0, i, 0);

    childTile.setContext(&context);
    childTile.setParent(&tile);
    childTile.setTileID(id);
    const CesiumGeospatial::GlobeRectangle childGlobeRectangle =
        unprojectRectangleSimple(projection, tilingScheme->tileToRectangle(id));
    childTile.setBoundingVolume(
        createDefaultLooseEarthBoundingVolume(childGlobeRectangle));
    childTile.setGeometricError(
        8.0 * calcQuadtreeMaxGeometricError(Ellipsoid::WGS84) *
        childGlobeRectangle.computeWidth());
  }
}

/**
 * @brief Tries to update the context request headers with a new token.
 *
 * This will try to obtain the `accessToken` from the JSON of the
 * given response, and set it as the `Bearer ...` value of the
 * `Authorization` header of the request headers of the given
 * context.
 *
 * @param pContext The context
 * @param pIonResponse The response
 * @return Whether the update succeeded
 */
static bool updateContextWithNewToken(
    TileContext* pContext,
    const IAssetResponse* pIonResponse,
    const std::shared_ptr<spdlog::logger>& pLogger) {
  const gsl::span<const std::byte> data = pIonResponse->data();

  rapidjson::Document ionResponse;
  ionResponse.Parse(reinterpret_cast<const char*>(data.data()), data.size());
  if (ionResponse.HasParseError()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Error when parsing Cesium ion response, error code {} at byte offset "
        "{}",
        ionResponse.GetParseError(),
        ionResponse.GetErrorOffset());
    return false;
  }
  std::string accessToken =
      JsonHelpers::getStringOrDefault(ionResponse, "accessToken", "");

  auto authIt = std::find_if(
      pContext->requestHeaders.begin(),
      pContext->requestHeaders.end(),
      [](auto& headerPair) { return headerPair.first == "Authorization"; });
  if (authIt != pContext->requestHeaders.end()) {
    authIt->second = "Bearer " + accessToken;
  } else {
    pContext->requestHeaders.push_back(
        std::make_pair("Authorization", "Bearer " + accessToken));
  }
  return true;
}

void Tileset::_handleTokenRefreshResponse(
    std::shared_ptr<IAssetRequest>&& pIonRequest,
    TileContext* pContext,
    const std::shared_ptr<spdlog::logger>& pLogger) {
  const IAssetResponse* pIonResponse = pIonRequest->response();

  bool failed = true;
  if (pIonResponse && pIonResponse->statusCode() >= 200 &&
      pIonResponse->statusCode() < 300) {
    failed = !updateContextWithNewToken(pContext, pIonResponse, pLogger);
  }

  // Put all auth-failed tiles in this context back into the Unloaded state.
  // TODO: the way this is structured, requests already in progress with the old
  // key might complete after the key has been updated, and there's nothing here
  // clever enough to avoid refreshing the key _again_ in that instance.

  Tile* pTile = this->_loadedTiles.head();

  while (pTile) {
    if (pTile->getContext() == pContext &&
        pTile->getState() == Tile::LoadState::FailedTemporarily &&
        pTile->getContent() && pTile->getContent()->httpStatusCode == 401) {
      if (failed) {
        pTile->markPermanentlyFailed();
      } else {
        pTile->unloadContent();
      }
    }

    pTile = this->_loadedTiles.next(*pTile);
  }

  this->_isRefreshingIonToken = false;
  this->notifyTileDoneLoading(nullptr);
}

FailedTileAction Tileset::_onIonTileFailed(Tile& failedTile) {
  TileContentLoadResult* pContent = failedTile.getContent();
  if (!pContent) {
    return FailedTileAction::GiveUp;
  }

  if (pContent->httpStatusCode != 401) {
    return FailedTileAction::GiveUp;
  }

  if (!this->_ionAssetID) {
    return FailedTileAction::GiveUp;
  }

  if (!this->_isRefreshingIonToken) {
    this->_isRefreshingIonToken = true;

    std::string url = "https://api.cesium.com/v1/assets/" +
                      std::to_string(this->_ionAssetID.value()) + "/endpoint";
    if (this->_ionAccessToken) {
      url += "?access_token=" + this->_ionAccessToken.value();
    }

    ++this->_loadsInProgress;

    this->getExternals()
        .pAssetAccessor->requestAsset(this->getAsyncSystem(), url)
        .thenInMainThread([this, pContext = failedTile.getContext()](
                              std::shared_ptr<IAssetRequest>&& pIonRequest) {
          this->_handleTokenRefreshResponse(
              std::move(pIonRequest),
              pContext,
              this->_externals.pLogger);
        })
        .catchInMainThread([this](const std::exception& e) {
          SPDLOG_LOGGER_ERROR(
              this->_externals.pLogger,
              "Unhandled error when retrying request: {}",
              e.what());
          this->_isRefreshingIonToken = false;
          this->notifyTileDoneLoading(nullptr);
        });
  }

  return FailedTileAction::Wait;
}

static void markTileNonRendered(
    TileSelectionState::Result lastResult,
    Tile& tile,
    ViewUpdateResult& result) {
  if (lastResult == TileSelectionState::Result::Rendered) {
    result.tilesToNoLongerRenderThisFrame.push_back(&tile);
  }
}

static void markTileNonRendered(
    int32_t lastFrameNumber,
    Tile& tile,
    ViewUpdateResult& result) {
  const TileSelectionState::Result lastResult =
      tile.getLastSelectionState().getResult(lastFrameNumber);
  markTileNonRendered(lastResult, tile, result);
}

static void markChildrenNonRendered(
    int32_t lastFrameNumber,
    TileSelectionState::Result lastResult,
    Tile& tile,
    ViewUpdateResult& result) {
  if (lastResult == TileSelectionState::Result::Refined) {
    for (Tile& child : tile.getChildren()) {
      const TileSelectionState::Result childLastResult =
          child.getLastSelectionState().getResult(lastFrameNumber);
      markTileNonRendered(childLastResult, child, result);
      markChildrenNonRendered(lastFrameNumber, childLastResult, child, result);
    }
  }
}

static void markChildrenNonRendered(
    int32_t lastFrameNumber,
    Tile& tile,
    ViewUpdateResult& result) {
  const TileSelectionState::Result lastResult =
      tile.getLastSelectionState().getResult(lastFrameNumber);
  markChildrenNonRendered(lastFrameNumber, lastResult, tile, result);
}

static void markTileAndChildrenNonRendered(
    int32_t lastFrameNumber,
    Tile& tile,
    ViewUpdateResult& result) {
  const TileSelectionState::Result lastResult =
      tile.getLastSelectionState().getResult(lastFrameNumber);
  markTileNonRendered(lastResult, tile, result);
  markChildrenNonRendered(lastFrameNumber, lastResult, tile, result);
}

/**
 * @brief Returns whether a tile with the given bounding volume is visible for
 * the camera.
 *
 * @param viewState The {@link ViewState}
 * @param boundingVolume The bounding volume of the tile
 * @param forceRenderTilesUnderCamera Whether tiles under the camera should
 * always be rendered (see {@link Cesium3DTilesSelection::TilesetOptions})
 * @return Whether the tile is visible according to the current camera
 * configuration
 */
static bool isVisibleFromCamera(
    const ViewState& viewState,
    const BoundingVolume& boundingVolume,
    bool forceRenderTilesUnderCamera) {
  if (viewState.isBoundingVolumeVisible(boundingVolume)) {
    return true;
  }
  if (!forceRenderTilesUnderCamera) {
    return false;
  }

  const std::optional<CesiumGeospatial::Cartographic>& position =
      viewState.getPositionCartographic();

  // TODO: it would be better to test a line pointing down (and up?) from the
  // camera against the bounding volume itself, rather than transforming the
  // bounding volume to a region.
  const CesiumGeospatial::GlobeRectangle* pRectangle =
      Impl::obtainGlobeRectangle(&boundingVolume);
  if (position && pRectangle) {
    return pRectangle->contains(position.value());
  }
  return false;
}

/**
 * @brief Returns whether a tile at the given distance is visible in the fog.
 *
 * @param distance The distance of the tile bounding volume to the camera
 * @param fogDensity The fog density
 * @return Whether the tile is visible in the fog
 */
static bool isVisibleInFog(double distance, double fogDensity) noexcept {
  if (fogDensity <= 0.0) {
    return true;
  }

  const double fogScalar = distance * fogDensity;
  return glm::exp(-(fogScalar * fogScalar)) > 0.0;
}

// Visits a tile for possible rendering. When we call this function with a tile:
//   * It is not yet known whether the tile is visible.
//   * Its parent tile does _not_ meet the SSE (unless ancestorMeetsSse=true,
//   see comments below).
//   * The tile may or may not be renderable.
//   * The tile has not yet been added to a load queue.
Tileset::TraversalDetails Tileset::_visitTileIfNeeded(
    const FrameState& frameState,
    uint32_t depth,
    bool ancestorMeetsSse,
    Tile& tile,
    ViewUpdateResult& result) {
  tile.update(frameState.lastFrameNumber, frameState.currentFrameNumber);
  this->_markTileVisited(tile);

  const Tileset* pTileset = tile.getTileset();
  if (!pTileset) {
    return TraversalDetails();
  }

  // whether we should visit this tile
  bool shouldVisit = true;
  // whether this tile was culled (Note: we might still want to visit it)
  bool culled = false;

  for (const std::shared_ptr<ITileExcluder>& pExcluder :
       this->_options.excluders) {
    if (pExcluder->shouldExclude(tile)) {
      culled = true;
      shouldVisit = false;
      break;
    }
  }

  const std::vector<ViewState>& frustums = frameState.frustums;
  const std::vector<double>& fogDensities = frameState.fogDensities;

  const BoundingVolume& boundingVolume = tile.getBoundingVolume();
  if (std::none_of(
          frustums.begin(),
          frustums.end(),
          [boundingVolume,
           renderTilesUnderCamera = this->_options.renderTilesUnderCamera](
              const ViewState& frustum) {
            return isVisibleFromCamera(
                frustum,
                boundingVolume,
                renderTilesUnderCamera);
          })) {
    // this tile is off-screen so it is a culled tile
    culled = true;
    if (this->_options.enableFrustumCulling) {
      // frustum culling is enabled so we shouldn't visit this off-screen tile
      shouldVisit = false;
    }
  }

  if (this->_nextDistancesVector >= this->_distancesStack.size()) {
    this->_distancesStack.resize(this->_nextDistancesVector + 1);
  }

  std::unique_ptr<std::vector<double>>& pDistances =
      this->_distancesStack[this->_nextDistancesVector];
  if (!pDistances) {
    pDistances = std::make_unique<std::vector<double>>();
  }

  std::vector<double>& distances = *pDistances;
  distances.resize(frustums.size());
  ++this->_nextDistancesVector;

  // Use a unique_ptr to ensure the _nextDistancesVector gets decrements when we
  // leave this scope.
  const auto decrementNextDistancesVector = [this](std::vector<double>*) {
    --this->_nextDistancesVector;
  };
  std::unique_ptr<std::vector<double>, decltype(decrementNextDistancesVector)>
      autoDecrement(&distances, decrementNextDistancesVector);

  std::transform(
      frustums.begin(),
      frustums.end(),
      distances.begin(),
      [boundingVolume](const ViewState& frustum) -> double {
        return glm::sqrt(glm::max(
            frustum.computeDistanceSquaredToBoundingVolume(boundingVolume),
            0.0));
      });

  // if we are still considering visiting this tile, check for fog occlusion
  if (shouldVisit) {
    bool isFogCulled = true;

    for (size_t i = 0; i < frustums.size(); ++i) {
      const double distance = distances[i];
      const double fogDensity = fogDensities[i];

      if (isVisibleInFog(distance, fogDensity)) {
        isFogCulled = false;
        break;
      }
    }

    if (isFogCulled) {
      // this tile is occluded by fog so it is a culled tile
      culled = true;
      if (this->_options.enableFogCulling) {
        // fog culling is enabled so we shouldn't visit this tile
        shouldVisit = false;
      }
    }
  }

  if (!shouldVisit) {
    markTileAndChildrenNonRendered(frameState.lastFrameNumber, tile, result);
    tile.setLastSelectionState(TileSelectionState(
        frameState.currentFrameNumber,
        TileSelectionState::Result::Culled));

    // Preload this culled sibling if requested.
    if (this->_options.preloadSiblings) {
      addTileToLoadQueue(this->_loadQueueLow, frustums, tile, distances);
    }

    ++result.tilesCulled;

    return TraversalDetails();
  }

  return this->_visitTile(
      frameState,
      depth,
      ancestorMeetsSse,
      tile,
      distances,
      culled,
      result);
}

static bool isLeaf(const Tile& tile) noexcept {
  return tile.getChildren().empty();
}

Tileset::TraversalDetails Tileset::_renderLeaf(
    const FrameState& frameState,
    Tile& tile,
    const std::vector<double>& distances,
    ViewUpdateResult& result) {

  const TileSelectionState lastFrameSelectionState =
      tile.getLastSelectionState();

  tile.setLastSelectionState(TileSelectionState(
      frameState.currentFrameNumber,
      TileSelectionState::Result::Rendered));
  result.tilesToRenderThisFrame.push_back(&tile);
  addTileToLoadQueue(
      this->_loadQueueMedium,
      frameState.frustums,
      tile,
      distances);

  TraversalDetails traversalDetails;
  traversalDetails.allAreRenderable = tile.isRenderable();
  traversalDetails.anyWereRenderedLastFrame =
      lastFrameSelectionState.getResult(frameState.lastFrameNumber) ==
      TileSelectionState::Result::Rendered;
  traversalDetails.notYetRenderableCount =
      traversalDetails.allAreRenderable ? 0 : 1;
  return traversalDetails;
}

bool Tileset::_queueLoadOfChildrenRequiredForRefinement(
    const FrameState& frameState,
    Tile& tile,
    const std::vector<double>& distances) {
  if (!this->_options.forbidHoles) {
    return false;
  }
  // If we're forbidding holes, don't refine if any children are still loading.
  gsl::span<Tile> children = tile.getChildren();
  bool waitingForChildren = false;
  for (Tile& child : children) {
    if (!child.isRenderable() && !child.isExternalTileset()) {
      waitingForChildren = true;

      // While we are waiting for the child to load, we need to push along the
      // tile and raster loading by continuing to update it.
      child.update(frameState.lastFrameNumber, frameState.currentFrameNumber);
      this->_markTileVisited(child);

      // We're using the distance to the parent tile to compute the load
      // priority. This is fine because the relative priority of the children is
      // irrelevant; we can't display any of them until all are loaded, anyway.
      addTileToLoadQueue(
          this->_loadQueueMedium,
          frameState.frustums,
          child,
          distances);
    }
  }
  return waitingForChildren;
}

bool Tileset::_meetsSse(
    const std::vector<ViewState>& frustums,
    const Tile& tile,
    const std::vector<double>& distances,
    bool culled) const noexcept {

  double largestSse = 0.0;

  for (size_t i = 0; i < frustums.size() && i < distances.size(); ++i) {
    const ViewState& frustum = frustums[i];
    const double distance = distances[i];

    // Does this tile meet the screen-space error?
    const double sse =
        frustum.computeScreenSpaceError(tile.getGeometricError(), distance);
    if (sse > largestSse) {
      largestSse = sse;
    }
  }

  return culled ? !this->_options.enforceCulledScreenSpaceError ||
                      largestSse < this->_options.culledScreenSpaceError
                : largestSse < this->_options.maximumScreenSpaceError;
}

/**
 * We can render it if _any_ of the following are true:
 *  1. We rendered it (or kicked it) last frame.
 *  2. This tile was culled last frame, or it wasn't even visited because an
 * ancestor was culled.
 *  3. The tile is done loading and ready to render.
 *  Note that even if we decide to render a tile here, it may later get "kicked"
 * in favor of an ancestor.
 */
static bool shouldRenderThisTile(
    const Tile& tile,
    const TileSelectionState& lastFrameSelectionState,
    int32_t lastFrameNumber) noexcept {
  const TileSelectionState::Result originalResult =
      lastFrameSelectionState.getOriginalResult(lastFrameNumber);
  if (originalResult == TileSelectionState::Result::Rendered) {
    return true;
  }
  if (originalResult == TileSelectionState::Result::Culled ||
      originalResult == TileSelectionState::Result::None) {
    return true;
  }

  // Tile::isRenderable is actually a pretty complex operation, so only do
  // it when absolutely necessary
  if (tile.isRenderable()) {
    return true;
  }
  return false;
}

Tileset::TraversalDetails Tileset::_renderInnerTile(
    const FrameState& frameState,
    Tile& tile,
    ViewUpdateResult& result) {

  const TileSelectionState lastFrameSelectionState =
      tile.getLastSelectionState();

  markChildrenNonRendered(frameState.lastFrameNumber, tile, result);
  tile.setLastSelectionState(TileSelectionState(
      frameState.currentFrameNumber,
      TileSelectionState::Result::Rendered));
  result.tilesToRenderThisFrame.push_back(&tile);

  TraversalDetails traversalDetails;
  traversalDetails.allAreRenderable = tile.isRenderable();
  traversalDetails.anyWereRenderedLastFrame =
      lastFrameSelectionState.getResult(frameState.lastFrameNumber) ==
      TileSelectionState::Result::Rendered;
  traversalDetails.notYetRenderableCount =
      traversalDetails.allAreRenderable ? 0 : 1;

  return traversalDetails;
}

Tileset::TraversalDetails Tileset::_refineToNothing(
    const FrameState& frameState,
    Tile& tile,
    ViewUpdateResult& result,
    bool areChildrenRenderable) {

  const TileSelectionState lastFrameSelectionState =
      tile.getLastSelectionState();

  // Nothing else to do except mark this tile refined and return.
  TraversalDetails noChildrenTraversalDetails;
  if (tile.getRefine() == TileRefine::Add) {
    noChildrenTraversalDetails.allAreRenderable = tile.isRenderable();
    noChildrenTraversalDetails.anyWereRenderedLastFrame =
        lastFrameSelectionState.getResult(frameState.lastFrameNumber) ==
        TileSelectionState::Result::Rendered;
    noChildrenTraversalDetails.notYetRenderableCount =
        areChildrenRenderable ? 0 : 1;
  } else {
    markTileNonRendered(frameState.lastFrameNumber, tile, result);
  }

  tile.setLastSelectionState(TileSelectionState(
      frameState.currentFrameNumber,
      TileSelectionState::Result::Refined));
  return noChildrenTraversalDetails;
}

bool Tileset::_loadAndRenderAdditiveRefinedTile(
    const FrameState& frameState,
    Tile& tile,
    ViewUpdateResult& result,
    const std::vector<double>& distances) {
  // If this tile uses additive refinement, we need to render this tile in
  // addition to its children.
  if (tile.getRefine() == TileRefine::Add) {
    result.tilesToRenderThisFrame.push_back(&tile);
    addTileToLoadQueue(
        this->_loadQueueMedium,
        frameState.frustums,
        tile,
        distances);
    return true;
  }

  return false;
}

// TODO This function is obviously too complex. The way how the indices are
// used, in order to deal with the queue elements, should be reviewed...
bool Tileset::_kickDescendantsAndRenderTile(
    const FrameState& frameState,
    Tile& tile,
    ViewUpdateResult& result,
    TraversalDetails& traversalDetails,
    size_t firstRenderedDescendantIndex,
    size_t loadIndexLow,
    size_t loadIndexMedium,
    size_t loadIndexHigh,
    bool queuedForLoad,
    const std::vector<double>& distances) {
  const TileSelectionState lastFrameSelectionState =
      tile.getLastSelectionState();

  std::vector<Tile*>& renderList = result.tilesToRenderThisFrame;

  // Mark the rendered descendants and their ancestors - up to this tile - as
  // kicked.
  for (size_t i = firstRenderedDescendantIndex; i < renderList.size(); ++i) {
    Tile* pWorkTile = renderList[i];
    while (pWorkTile != nullptr &&
           !pWorkTile->getLastSelectionState().wasKicked(
               frameState.currentFrameNumber) &&
           pWorkTile != &tile) {
      pWorkTile->getLastSelectionState().kick();
      pWorkTile = pWorkTile->getParent();
    }
  }

  // Remove all descendants from the render list and add this tile.
  renderList.erase(
      renderList.begin() +
          static_cast<std::vector<Tile*>::iterator::difference_type>(
              firstRenderedDescendantIndex),
      renderList.end());

  if (tile.getRefine() != Cesium3DTilesSelection::TileRefine::Add) {
    renderList.push_back(&tile);
  }

  tile.setLastSelectionState(TileSelectionState(
      frameState.currentFrameNumber,
      TileSelectionState::Result::Rendered));

  // If we're waiting on heaps of descendants, the above will take too long. So
  // in that case, load this tile INSTEAD of loading any of the descendants, and
  // tell the up-level we're only waiting on this tile. Keep doing this until we
  // actually manage to render this tile.
  const bool wasRenderedLastFrame =
      lastFrameSelectionState.getResult(frameState.lastFrameNumber) ==
      TileSelectionState::Result::Rendered;
  const bool wasReallyRenderedLastFrame =
      wasRenderedLastFrame && tile.isRenderable();

  if (!wasReallyRenderedLastFrame &&
      traversalDetails.notYetRenderableCount >
          this->_options.loadingDescendantLimit) {
    // Remove all descendants from the load queues.
    this->_loadQueueLow.erase(
        this->_loadQueueLow.begin() +
            static_cast<std::vector<LoadRecord>::iterator::difference_type>(
                loadIndexLow),
        this->_loadQueueLow.end());
    this->_loadQueueMedium.erase(
        this->_loadQueueMedium.begin() +
            static_cast<std::vector<LoadRecord>::iterator::difference_type>(
                loadIndexMedium),
        this->_loadQueueMedium.end());
    this->_loadQueueHigh.erase(
        this->_loadQueueHigh.begin() +
            static_cast<std::vector<LoadRecord>::iterator::difference_type>(
                loadIndexHigh),
        this->_loadQueueHigh.end());

    if (!queuedForLoad) {
      addTileToLoadQueue(
          this->_loadQueueMedium,
          frameState.frustums,
          tile,
          distances);
    }

    traversalDetails.notYetRenderableCount = tile.isRenderable() ? 0 : 1;
    queuedForLoad = true;
  }

  traversalDetails.allAreRenderable = tile.isRenderable();
  traversalDetails.anyWereRenderedLastFrame = wasRenderedLastFrame;

  return queuedForLoad;
}

// Visits a tile for possible rendering. When we call this function with a tile:
//   * The tile has previously been determined to be visible.
//   * Its parent tile does _not_ meet the SSE (unless ancestorMeetsSse=true,
//   see comments below).
//   * The tile may or may not be renderable.
//   * The tile has not yet been added to a load queue.
Tileset::TraversalDetails Tileset::_visitTile(
    const FrameState& frameState,
    uint32_t depth,
    bool ancestorMeetsSse, // Careful: May be modified before being passed to
                           // children!
    Tile& tile,
    const std::vector<double>& distances,
    bool culled,
    ViewUpdateResult& result) {
  ++result.tilesVisited;
  result.maxDepthVisited = glm::max(result.maxDepthVisited, depth);

  if (culled) {
    ++result.culledTilesVisited;
  }

  // If this is a leaf tile, just render it (it's already been deemed visible).
  if (isLeaf(tile)) {
    return _renderLeaf(frameState, tile, distances, result);
  }

  const bool unconditionallyRefine = tile.getUnconditionallyRefine();
  const bool meetsSse = _meetsSse(frameState.frustums, tile, distances, culled);
  const bool waitingForChildren =
      _queueLoadOfChildrenRequiredForRefinement(frameState, tile, distances);

  if (!unconditionallyRefine &&
      (meetsSse || ancestorMeetsSse || waitingForChildren)) {
    // This tile (or an ancestor) is the one we want to render this frame, but
    // we'll do different things depending on the state of this tile and on what
    // we did _last_ frame.

    // We can render it if _any_ of the following are true:
    // 1. We rendered it (or kicked it) last frame.
    // 2. This tile was culled last frame, or it wasn't even visited because an
    // ancestor was culled.
    // 3. The tile is done loading and ready to render.
    //
    // Note that even if we decide to render a tile here, it may later get
    // "kicked" in favor of an ancestor.
    const TileSelectionState lastFrameSelectionState =
        tile.getLastSelectionState();
    const bool renderThisTile = shouldRenderThisTile(
        tile,
        lastFrameSelectionState,
        frameState.lastFrameNumber);
    if (renderThisTile) {
      // Only load this tile if it (not just an ancestor) meets the SSE.
      if (meetsSse && !ancestorMeetsSse) {
        addTileToLoadQueue(
            this->_loadQueueMedium,
            frameState.frustums,
            tile,
            distances);
      }
      return _renderInnerTile(frameState, tile, result);
    }

    // Otherwise, we can't render this tile (or blank space where it would be)
    // because doing so would cause detail to disappear that was visible last
    // frame. Instead, keep rendering any still-visible descendants that were
    // rendered last frame and render nothing for newly-visible descendants.
    // E.g. if we were rendering level 15 last frame but this frame we want
    // level 14 and the closest renderable level <= 14 is 0, rendering level
    // zero would be pretty jarring so instead we keep rendering level 15 even
    // though its SSE is better than required. So fall through to continue
    // traversal...
    ancestorMeetsSse = true;

    // Load this blocker tile with high priority, but only if this tile (not
    // just an ancestor) meets the SSE.
    if (meetsSse) {
      addTileToLoadQueue(
          this->_loadQueueHigh,
          frameState.frustums,
          tile,
          distances);
    }
  }

  // Refine!

  bool queuedForLoad =
      _loadAndRenderAdditiveRefinedTile(frameState, tile, result, distances);

  const size_t firstRenderedDescendantIndex =
      result.tilesToRenderThisFrame.size();
  const size_t loadIndexLow = this->_loadQueueLow.size();
  const size_t loadIndexMedium = this->_loadQueueMedium.size();
  const size_t loadIndexHigh = this->_loadQueueHigh.size();

  TraversalDetails traversalDetails = this->_visitVisibleChildrenNearToFar(
      frameState,
      depth,
      ancestorMeetsSse,
      tile,
      result);

  const bool descendantTilesAdded =
      firstRenderedDescendantIndex != result.tilesToRenderThisFrame.size();
  if (!descendantTilesAdded) {
    // No descendant tiles were added to the render list by the function above,
    // meaning they were all culled even though this tile was deemed visible.
    // That's pretty common.
    return _refineToNothing(
        frameState,
        tile,
        result,
        traversalDetails.allAreRenderable);
  }

  // At least one descendant tile was added to the render list.
  // The traversalDetails tell us what happened while visiting the children.
  if (!traversalDetails.allAreRenderable &&
      !traversalDetails.anyWereRenderedLastFrame) {
    // Some of our descendants aren't ready to render yet, and none were
    // rendered last frame, so kick them all out of the render list and render
    // this tile instead. Continue to load them though!
    queuedForLoad = _kickDescendantsAndRenderTile(
        frameState,
        tile,
        result,
        traversalDetails,
        firstRenderedDescendantIndex,
        loadIndexLow,
        loadIndexMedium,
        loadIndexHigh,
        queuedForLoad,
        distances);
  } else {
    if (tile.getRefine() != TileRefine::Add) {
      markTileNonRendered(frameState.lastFrameNumber, tile, result);
    }
    tile.setLastSelectionState(TileSelectionState(
        frameState.currentFrameNumber,
        TileSelectionState::Result::Refined));
  }

  if (this->_options.preloadAncestors && !queuedForLoad) {
    addTileToLoadQueue(
        this->_loadQueueLow,
        frameState.frustums,
        tile,
        distances);
  }

  return traversalDetails;
}

Tileset::TraversalDetails Tileset::_visitVisibleChildrenNearToFar(
    const FrameState& frameState,
    uint32_t depth,
    bool ancestorMeetsSse,
    Tile& tile,
    ViewUpdateResult& result) {
  TraversalDetails traversalDetails;

  // TODO: actually visit near-to-far, rather than in order of occurrence.
  gsl::span<Tile> children = tile.getChildren();
  for (Tile& child : children) {
    const TraversalDetails childTraversal = this->_visitTileIfNeeded(
        frameState,
        depth + 1,
        ancestorMeetsSse,
        child,
        result);

    traversalDetails.allAreRenderable &= childTraversal.allAreRenderable;
    traversalDetails.anyWereRenderedLastFrame |=
        childTraversal.anyWereRenderedLastFrame;
    traversalDetails.notYetRenderableCount +=
        childTraversal.notYetRenderableCount;
  }

  return traversalDetails;
}

void Tileset::_processLoadQueue() {
  this->processQueue(
      this->_loadQueueHigh,
      this->_loadsInProgress,
      this->_options.maximumSimultaneousTileLoads);
  this->processQueue(
      this->_loadQueueMedium,
      this->_loadsInProgress,
      this->_options.maximumSimultaneousTileLoads);
  this->processQueue(
      this->_loadQueueLow,
      this->_loadsInProgress,
      this->_options.maximumSimultaneousTileLoads);
}

void Tileset::_unloadCachedTiles() noexcept {
  const int64_t maxBytes = this->getOptions().maximumCachedBytes;

  Tile* pTile = this->_loadedTiles.head();

  while (this->getTotalDataBytes() > maxBytes) {
    if (pTile == nullptr || pTile == this->_pRootTile.get()) {
      // We've either removed all tiles or the next tile is the root.
      // The root tile marks the beginning of the tiles that were used
      // for rendering last frame.
      break;
    }

    Tile* pNext = this->_loadedTiles.next(*pTile);

    const bool removed = pTile->unloadContent();
    if (removed) {
      this->_loadedTiles.remove(*pTile);
    }

    pTile = pNext;
  }
}

void Tileset::_markTileVisited(Tile& tile) noexcept {
  this->_loadedTiles.insertAtTail(tile);
}

std::string Tileset::getResolvedContentUrl(const Tile& tile) const {
  struct Operation {
    const TileContext& context;

    std::string operator()(const std::string& url) { return url; }

    std::string operator()(const QuadtreeTileID& quadtreeID) {
      if (!this->context.implicitContext) {
        return std::string();
      }

      return CesiumUtility::Uri::substituteTemplateParameters(
          context.implicitContext.value().tileTemplateUrls[0],
          [this, &quadtreeID](const std::string& placeholder) -> std::string {
            if (placeholder == "level" || placeholder == "z") {
              return std::to_string(quadtreeID.level);
            }
            if (placeholder == "x") {
              return std::to_string(quadtreeID.x);
            }
            if (placeholder == "y") {
              return std::to_string(quadtreeID.y);
            }
            if (placeholder == "version") {
              return this->context.version.value_or(std::string());
            }

            return placeholder;
          });
    }

    std::string operator()(const OctreeTileID& octreeID) {
      if (!this->context.implicitContext) {
        return std::string();
      }

      return CesiumUtility::Uri::substituteTemplateParameters(
          context.implicitContext.value().tileTemplateUrls[0],
          [this, &octreeID](const std::string& placeholder) -> std::string {
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
            if (placeholder == "version") {
              return this->context.version.value_or(std::string());
            }

            return placeholder;
          });
    }

    std::string
    operator()(UpsampledQuadtreeNode /*subdividedParent*/) noexcept {
      return std::string();
    }
  };

  std::string url = std::visit(Operation{*tile.getContext()}, tile.getTileID());
  if (url.empty()) {
    return url;
  }

  return CesiumUtility::Uri::resolve(tile.getContext()->baseUrl, url, true);
}

std::string Tileset::getResolvedSubtreeUrl(const Tile& tile) const {
  struct Operation {
    const TileContext& context;

    std::string operator()(const std::string& url) { return url; }

    std::string operator()(const QuadtreeTileID& quadtreeID) {
      if (!this->context.implicitContext || 
          !this->context.implicitContext->subtreeTemplateUrl) {
        return std::string();
      }

      return CesiumUtility::Uri::substituteTemplateParameters(
          *this->context.implicitContext.value().subtreeTemplateUrl,
          [this, &quadtreeID](const std::string& placeholder) -> std::string {
            if (placeholder == "level" || placeholder == "z") {
              return std::to_string(quadtreeID.level);
            }
            if (placeholder == "x") {
              return std::to_string(quadtreeID.x);
            }
            if (placeholder == "y") {
              return std::to_string(quadtreeID.y);
            }
            if (placeholder == "version") {
              return this->context.version.value_or(std::string());
            }

            return placeholder;
          });
    }

    std::string operator()(const OctreeTileID& octreeID) {
       if (!this->context.implicitContext || 
           !this->context.implicitContext->subtreeTemplateUrl) {
        return std::string();
      }

      return CesiumUtility::Uri::substituteTemplateParameters(
          *this->context.implicitContext.value().subtreeTemplateUrl,
          [this, &octreeID](const std::string& placeholder) -> std::string {
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
            if (placeholder == "version") {
              return this->context.version.value_or(std::string());
            }

            return placeholder;
          });
    }

    std::string operator()(UpsampledQuadtreeNode /*subdividedParent*/) {
      return std::string();
    }
  };

  std::string url = std::visit(Operation{*tile.getContext()}, tile.getTileID());
  if (url.empty()) {
    return url;
  }

  return CesiumUtility::Uri::resolve(tile.getContext()->baseUrl, url, true);
}

static bool anyRasterOverlaysNeedLoading(const Tile& tile) noexcept {
  for (const RasterMappedTo3DTile& mapped : tile.getMappedRasterTiles()) {
    const RasterOverlayTile* pLoading = mapped.getLoadingTile();
    if (pLoading &&
        pLoading->getState() == RasterOverlayTile::LoadState::Unloaded) {
      return true;
    }
  }

  return false;
}

// TODO The viewState is only needed to
// compute the priority from the distance. So maybe this function should
// receive a priority directly and be called with
// addTileToLoadQueue(queue, tile, priorityFor(tile, viewState, distance))
// (or at least, this function could delegate to such a call...)

/*static*/ void Tileset::addTileToLoadQueue(
    std::vector<Tileset::LoadRecord>& loadQueue,
    const std::vector<ViewState>& frustums,
    Tile& tile,
    const std::vector<double>& distances) {
  if (tile.getState() == Tile::LoadState::Unloaded ||
      anyRasterOverlaysNeedLoading(tile)) {

    const glm::dvec3 boundingVolumeCenter =
        getBoundingVolumeCenter(tile.getBoundingVolume());

    double highestLoadPriority = std::numeric_limits<double>::max();
    for (size_t i = 0; i < frustums.size() && i < distances.size(); ++i) {
      const ViewState& frustum = frustums[i];
      const double distance = distances[i];

      glm::dvec3 tileDirection = boundingVolumeCenter - frustum.getPosition();
      const double magnitude = glm::length(tileDirection);

      if (magnitude >= CesiumUtility::Math::EPSILON5) {
        tileDirection /= magnitude;
        const double loadPriority =
            (1.0 - glm::dot(tileDirection, frustum.getDirection())) * distance;
        if (loadPriority < highestLoadPriority) {
          highestLoadPriority = loadPriority;
        }
      }
    }

    loadQueue.push_back({&tile, highestLoadPriority});
  }
}

void Tileset::processQueue(
    std::vector<Tileset::LoadRecord>& queue,
    const std::atomic<uint32_t>& loadsInProgress,
    uint32_t maximumLoadsInProgress) {
  if (loadsInProgress >= maximumLoadsInProgress) {
    return;
  }

  std::sort(queue.begin(), queue.end());

  for (LoadRecord& record : queue) {
    CESIUM_TRACE_USE_TRACK_SET(this->_loadingSlots);
    record.pTile->loadContent();
    if (loadsInProgress >= maximumLoadsInProgress) {
      break;
    }
  }
}
} // namespace Cesium3DTilesSelection
