#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/CreditSystem.h"
#include "Cesium3DTiles/ExternalTilesetContent.h"
#include "Cesium3DTiles/RasterMappedTo3DTile.h"
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumAsync/ITaskProcessor.h"
#include "CesiumGeometry/QuadtreeTileAvailability.h"
#include "CesiumGeospatial/GeographicProjection.h"
#include "CesiumUtility/JsonHelpers.h"
#include "CesiumUtility/Math.h"
#include "CesiumUtility/Uri.h"
#include "TileUtilities.h"
#include "calcQuadtreeMaxGeometricError.h"
#include <cstddef>
#include <glm/common.hpp>
#include <optional>
#include <rapidjson/document.h>
#include <unordered_set>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace Cesium3DTiles {

Tileset::Tileset(
    const TilesetExternals& externals,
    const std::string& url,
    const TilesetOptions& options)
    : _contexts(),
      _externals(externals),
      _asyncSystem(externals.pTaskProcessor),
      _userCredit(
          (options.credit && externals.pCreditSystem)
              ? std::optional<Credit>(externals.pCreditSystem->createCredit(
                    options.credit.value()))
              : std::nullopt),
      _tilesetCredits(),
      _url(url),
      _ionAssetID(),
      _ionAccessToken(),
      _isRefreshingIonToken(false),
      _options(options),
      _pRootTile(),
      _previousFrameNumber(0),
      _updateResult(),
      _loadQueueHigh(),
      _loadQueueMedium(),
      _loadQueueLow(),
      _loadsInProgress(0),
      _loadedTiles(),
      _overlays(*this),
      _tileDataBytes(0),
      _supportsRasterOverlays(false) {
  ++this->_loadsInProgress;
  this->_loadTilesetJson(url);
}

Tileset::Tileset(
    const TilesetExternals& externals,
    uint32_t ionAssetID,
    const std::string& ionAccessToken,
    const TilesetOptions& options)
    : _contexts(),
      _externals(externals),
      _asyncSystem(externals.pTaskProcessor),
      _userCredit(
          (options.credit && externals.pCreditSystem)
              ? std::optional<Credit>(externals.pCreditSystem->createCredit(
                    options.credit.value()))
              : std::nullopt),
      _tilesetCredits(),
      _url(),
      _ionAssetID(ionAssetID),
      _ionAccessToken(ionAccessToken),
      _isRefreshingIonToken(false),
      _options(options),
      _pRootTile(),
      _previousFrameNumber(0),
      _updateResult(),
      _loadQueueHigh(),
      _loadQueueMedium(),
      _loadQueueLow(),
      _loadsInProgress(0),
      _loadedTiles(),
      _overlays(*this),
      _tileDataBytes(0),
      _supportsRasterOverlays(false) {
  std::string ionUrl = "https://api.cesium.com/v1/assets/" +
                       std::to_string(ionAssetID) + "/endpoint";
  if (ionAccessToken.size() > 0) {
    ionUrl += "?access_token=" + ionAccessToken;
  }

  ++this->_loadsInProgress;

  this->_externals.pAssetAccessor->requestAsset(this->_asyncSystem, ionUrl)
      .thenInMainThread([this](std::shared_ptr<IAssetRequest>&& pRequest) {
        this->_handleAssetResponse(std::move(pRequest));
      })
      .catchInMainThread([this, &ionAssetID](const std::exception& e) {
        SPDLOG_LOGGER_ERROR(
            this->_externals.pLogger,
            "Unhandled error for asset {}: {}",
            ionAssetID,
            e.what());
        this->notifyTileDoneLoading(nullptr);
      });
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

void Tileset::_handleAssetResponse(std::shared_ptr<IAssetRequest>&& pRequest) {
  const IAssetResponse* pResponse = pRequest->response();
  if (!pResponse) {
    SPDLOG_LOGGER_ERROR(
        this->_externals.pLogger,
        "No response received for asset request {}",
        pRequest->url());
    this->notifyTileDoneLoading(nullptr);
    return;
  }

  if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
    SPDLOG_LOGGER_ERROR(
        this->_externals.pLogger,
        "Received status code {} for asset response {}",
        pResponse->statusCode(),
        pRequest->url());
    this->notifyTileDoneLoading(nullptr);
    return;
  }

  gsl::span<const std::byte> data = pResponse->data();

  rapidjson::Document ionResponse;
  ionResponse.Parse(reinterpret_cast<const char*>(data.data()), data.size());

  if (ionResponse.HasParseError()) {
    SPDLOG_LOGGER_ERROR(
        this->_externals.pLogger,
        "Error when parsing Cesium ion response JSON, error code {} at byte "
        "offset {}",
        ionResponse.GetParseError(),
        ionResponse.GetErrorOffset());
    return;
  }

  if (this->_externals.pCreditSystem) {

    auto attributionsIt = ionResponse.FindMember("attributions");
    if (attributionsIt != ionResponse.MemberEnd() &&
        attributionsIt->value.IsArray()) {

      for (const rapidjson::Value& attribution :
           attributionsIt->value.GetArray()) {

        auto html = attribution.FindMember("html");
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
    return;
  }

  auto pContext = std::make_unique<TileContext>();

  pContext->pTileset = this;
  pContext->baseUrl = url;
  pContext->requestHeaders.push_back(
      std::make_pair("Authorization", "Bearer " + accessToken));
  pContext->failedTileCallback = [this](Tile& failedTile) {
    return this->_onIonTileFailed(failedTile);
  };
  this->_loadTilesetJson(
      pContext->baseUrl,
      pContext->requestHeaders,
      std::move(pContext));
}

static bool operator<(const FogDensityAtHeight& fogDensity, double height) {
  return fogDensity.cameraHeight < height;
}

static double computeFogDensity(
    const std::vector<FogDensityAtHeight>& fogDensityTable,
    const ViewState& viewState) {
  double height = viewState.getPositionCartographic()
                      .value_or(Cartographic(0.0, 0.0, 0.0))
                      .height;

  // Find the entry that is for >= this camera height.
  auto nextIt =
      std::lower_bound(fogDensityTable.begin(), fogDensityTable.end(), height);

  if (nextIt == fogDensityTable.end()) {
    return fogDensityTable.back().fogDensity;
  } else if (nextIt == fogDensityTable.begin()) {
    return nextIt->fogDensity;
  }

  auto prevIt = nextIt - 1;

  double heightA = prevIt->cameraHeight;
  double densityA = prevIt->fogDensity;

  double heightB = nextIt->cameraHeight;
  double densityB = nextIt->fogDensity;

  double t = glm::clamp((height - heightA) / (heightB - heightA), 0.0, 1.0);

  double density = glm::mix(densityA, densityB, t);

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

const ViewUpdateResult& Tileset::updateViewOffline(const ViewState& viewState) {
  std::vector<Tile*> tilesRenderedPrevFrame =
      this->_updateResult.tilesToRenderThisFrame;

  this->updateView(viewState);
  while (this->_loadsInProgress > 0) {
    this->_externals.pAssetAccessor->tick();
    this->updateView(viewState);
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

const ViewUpdateResult& Tileset::updateView(const ViewState& viewState) {
  this->_asyncSystem.dispatchMainThreadTasks();

  int32_t previousFrameNumber = this->_previousFrameNumber;
  int32_t currentFrameNumber = previousFrameNumber + 1;

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

  double fogDensity =
      computeFogDensity(this->_options.fogDensityTable, viewState);

  this->_loadQueueHigh.clear();
  this->_loadQueueMedium.clear();
  this->_loadQueueLow.clear();

  FrameState frameState{
      viewState,
      previousFrameNumber,
      currentFrameNumber,
      fogDensity};

  this->_visitTileIfNeeded(frameState, 0, false, *pRootTile, result);

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
    for (Credit& credit : this->_tilesetCredits) {
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
      for (RasterMappedTo3DTile mappedRasterTile : mappedRasterTiles) {
        RasterOverlayTile* pRasterOverlayTile = mappedRasterTile.getReadyTile();
        if (pRasterOverlayTile != nullptr) {
          for (Credit credit : pRasterOverlayTile->getCredits()) {
            pCreditSystem->addCreditToFrame(credit);
          }
        }
      }
    }
  }

  this->_previousFrameNumber = currentFrameNumber;

  return result;
}

void Tileset::notifyTileStartLoading(Tile* /*pTile*/) noexcept {
  ++this->_loadsInProgress;
}

void Tileset::notifyTileDoneLoading(Tile* pTile) noexcept {
  assert(this->_loadsInProgress > 0);
  --this->_loadsInProgress;

  if (pTile) {
    this->_tileDataBytes += pTile->computeByteSize();
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

std::optional<Future<std::shared_ptr<IAssetRequest>>>
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

void Tileset::_loadTilesetJson(
    const std::string& url,
    const std::vector<std::pair<std::string, std::string>>& headers,
    std::unique_ptr<TileContext>&& pContext) {
  if (!pContext) {
    pContext = std::make_unique<TileContext>();
  }
  pContext->pTileset = this;

  this->getExternals()
      .pAssetAccessor->requestAsset(this->getAsyncSystem(), url, headers)
      .thenInWorkerThread(
          [pLogger = this->_externals.pLogger, pContext = std::move(pContext)](
              std::shared_ptr<IAssetRequest>&& pRequest) mutable {
            return Tileset::_handleTilesetResponse(
                std::move(pRequest),
                std::move(pContext),
                pLogger);
          })
      .thenInMainThread([this](LoadResult&& loadResult) {
        this->_supportsRasterOverlays = loadResult.supportsRasterOverlays;
        this->addContext(std::move(loadResult.pContext));
        this->_pRootTile = std::move(loadResult.pRootTile);
        this->notifyTileDoneLoading(nullptr);
      })
      .catchInMainThread([this, url](const std::exception& e) {
        SPDLOG_LOGGER_ERROR(
            this->_externals.pLogger,
            "Unhandled error for tileset {}: {}",
            url,
            e.what());
        this->_pRootTile.reset();
        this->notifyTileDoneLoading(nullptr);
      });
}

/*static*/ Tileset::LoadResult Tileset::_handleTilesetResponse(
    std::shared_ptr<IAssetRequest>&& pRequest,
    std::unique_ptr<TileContext>&& pContext,
    const std::shared_ptr<spdlog::logger>& pLogger) {
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

  gsl::span<const std::byte> data = pResponse->data();

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

  std::unique_ptr<Tile> pRootTile = std::make_unique<Tile>();
  pRootTile->setContext(pContext.get());

  auto rootIt = tileset.FindMember("root");
  auto formatIt = tileset.FindMember("format");

  bool supportsRasterOverlays = false;

  if (rootIt != tileset.MemberEnd()) {
    rapidjson::Value& rootJson = rootIt->value;
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
    Tileset::_createTerrainTile(*pRootTile, tileset, *pContext, pLogger);
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
  auto bvIt = tileJson.FindMember(key.c_str());
  if (bvIt == tileJson.MemberEnd() || !bvIt->value.IsObject()) {
    return std::nullopt;
  }

  auto boxIt = bvIt->value.FindMember("box");
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

  auto regionIt = bvIt->value.FindMember("region");
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

  auto sphereIt = bvIt->value.FindMember("sphere");
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

  std::optional<glm::dmat4x4> tileTransform =
      JsonHelpers::getTransformProperty(tileJson, "transform");
  glm::dmat4x4 transform =
      parentTransform * tileTransform.value_or(glm::dmat4x4(1.0));
  tile.setTransform(transform);

  auto contentIt = tileJson.FindMember("content");
  auto childrenIt = tileJson.FindMember("children");

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
  // tile->setBoundingVolume(boundingVolume.value());
  tile.setGeometricError(geometricError.value());

  std::optional<BoundingVolume> viewerRequestVolume =
      getBoundingVolumeProperty(tileJson, "viewerRequestVolume");
  if (viewerRequestVolume) {
    tile.setViewerRequestVolume(
        transformBoundingVolume(transform, viewerRequestVolume.value()));
  }

  auto refineIt = tileJson.FindMember("refine");
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
    gsl::span<Tile> childTiles = tile.getChildren();

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
    const std::vector<std::string>& extensions) noexcept {

  std::vector<std::string> knownExtensions = {"octvertexnormals", "metadata"};
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
    const std::shared_ptr<spdlog::logger>& pLogger) {
  context.requestHeaders.push_back(std::make_pair(
      "Accept",
      "application/vnd.quantized-mesh,application/octet-stream;q=0.9,*/"
      "*;q=0.01"));

  auto tilesetVersionIt = layerJson.FindMember("version");
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

  CesiumGeometry::QuadtreeTilingScheme tilingScheme(
      quadtreeRectangleProjected,
      quadtreeXTiles,
      1);

  std::vector<std::string> urls = JsonHelpers::getStrings(layerJson, "tiles");
  uint32_t maxZoom = JsonHelpers::getUint32OrDefault(layerJson, "maxzoom", 30);

  context.implicitContext = {
      urls,
      tilingScheme,
      projection,
      CesiumGeometry::QuadtreeTileAvailability(tilingScheme, maxZoom)};

  std::vector<std::string> extensions =
      JsonHelpers::getStrings(layerJson, "extensions");

  // Request normals and metadata if they're available
  std::string extensionsToRequest = createExtensionsQueryParameter(extensions);

  if (!extensionsToRequest.empty()) {
    for (std::string& url : context.implicitContext.value().tileTemplateUrls) {
      url =
          CesiumUtility::Uri::addQuery(url, "extensions", extensionsToRequest);
    }
  }

  tile.setContext(&context);
  tile.setBoundingVolume(
      createDefaultLooseEarthBoundingVolume(quadtreeRectangleGlobe));
  tile.setGeometricError(999999999.0);
  tile.createChildTiles(quadtreeXTiles);

  for (uint32_t i = 0; i < quadtreeXTiles; ++i) {
    Tile& childTile = tile.getChildren()[i];
    QuadtreeTileID id(0, i, 0);

    childTile.setContext(&context);
    childTile.setParent(&tile);
    childTile.setTileID(id);
    CesiumGeospatial::GlobeRectangle childGlobeRectangle =
        unprojectRectangleSimple(projection, tilingScheme.tileToRectangle(id));
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
  gsl::span<const std::byte> data = pIonResponse->data();

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
  TileSelectionState::Result lastResult =
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
      TileSelectionState::Result childLastResult =
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
  TileSelectionState::Result lastResult =
      tile.getLastSelectionState().getResult(lastFrameNumber);
  markChildrenNonRendered(lastFrameNumber, lastResult, tile, result);
}

static void markTileAndChildrenNonRendered(
    int32_t lastFrameNumber,
    Tile& tile,
    ViewUpdateResult& result) {
  TileSelectionState::Result lastResult =
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
 * always be rendered (see {@link Cesium3DTiles::TilesetOptions})
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
  const CesiumGeospatial::GlobeRectangle* pRectangle =
      Cesium3DTiles::Impl::obtainGlobeRectangle(&boundingVolume);
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
static bool isVisibleInFog(double distance, double fogDensity) {
  if (fogDensity <= 0.0) {
    return true;
  }
  double fogScalar = distance * fogDensity;
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

  // whether we should visit this tile
  bool shouldVisit = true;
  // whether this tile was culled (Note: we might still want to visit it)
  bool culled = false;

  const BoundingVolume& boundingVolume = tile.getBoundingVolume();
  const ViewState& viewState = frameState.viewState;

  if (!isVisibleFromCamera(
          frameState.viewState,
          boundingVolume,
          this->_options.renderTilesUnderCamera)) {
    // this tile is off-screen so it is a culled tile
    culled = true;
    if (this->_options.enableFrustumCulling) {
      // frustum culling is enabled so we shouldn't visit this off-screen tile
      shouldVisit = false;
    }
  }

  double distance = sqrt(glm::max(
      viewState.computeDistanceSquaredToBoundingVolume(boundingVolume),
      0.0));

  // if we are still considering visiting this tile, check for fog occlusion
  if (shouldVisit && !isVisibleInFog(distance, frameState.fogDensity)) {
    // this tile is occluded by fog so it is a culled tile
    culled = true;
    if (this->_options.enableFogCulling) {
      // fog culling is enabled so we shouldn't visit this tile
      shouldVisit = false;
    }
  }

  if (!shouldVisit) {
    markTileAndChildrenNonRendered(frameState.lastFrameNumber, tile, result);
    tile.setLastSelectionState(TileSelectionState(
        frameState.currentFrameNumber,
        TileSelectionState::Result::Culled));

    // Preload this culled sibling if requested.
    if (this->_options.preloadSiblings) {
      addTileToLoadQueue(
          this->_loadQueueLow,
          frameState.viewState,
          tile,
          distance);
    }

    ++result.tilesCulled;

    return TraversalDetails();
  }

  return this->_visitTile(
      frameState,
      depth,
      ancestorMeetsSse,
      tile,
      distance,
      culled,
      result);
}

static bool isLeaf(const Tile& tile) { return tile.getChildren().empty(); }

Tileset::TraversalDetails Tileset::_renderLeaf(
    const FrameState& frameState,
    Tile& tile,
    double distance,
    ViewUpdateResult& result) {

  TileSelectionState lastFrameSelectionState = tile.getLastSelectionState();

  tile.setLastSelectionState(TileSelectionState(
      frameState.currentFrameNumber,
      TileSelectionState::Result::Rendered));
  result.tilesToRenderThisFrame.push_back(&tile);
  addTileToLoadQueue(
      this->_loadQueueMedium,
      frameState.viewState,
      tile,
      distance);

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
    double distance) {
  if (!this->_options.forbidHoles) {
    return false;
  }
  // If we're forbidding holes, don't refine if any children are still loading.
  gsl::span<Tile> children = tile.getChildren();
  bool waitingForChildren = false;
  for (Tile& child : children) {
    if (!child.isRenderable()) {
      waitingForChildren = true;

      this->_markTileVisited(child);

      // We're using the distance to the parent tile to compute the load
      // priority. This is fine because the relative priority of the children is
      // irrelevant; we can't display any of them until all are loaded, anyway.
      addTileToLoadQueue(
          this->_loadQueueMedium,
          frameState.viewState,
          child,
          distance);
    }
  }
  return waitingForChildren;
}

bool Tileset::_meetsSse(
    const ViewState& viewState,
    const Tile& tile,
    double distance,
    bool culled) const {
  // Does this tile meet the screen-space error?
  double sse =
      viewState.computeScreenSpaceError(tile.getGeometricError(), distance);
  return culled ? !this->_options.enforceCulledScreenSpaceError ||
                      sse < this->_options.culledScreenSpaceError
                : sse < this->_options.maximumScreenSpaceError;
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
    int32_t lastFrameNumber) {
  TileSelectionState::Result originalResult =
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

  TileSelectionState lastFrameSelectionState = tile.getLastSelectionState();

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

  TileSelectionState lastFrameSelectionState = tile.getLastSelectionState();

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
    double distance) {
  // If this tile uses additive refinement, we need to render this tile in
  // addition to its children.
  if (tile.getRefine() == TileRefine::Add) {
    result.tilesToRenderThisFrame.push_back(&tile);
    addTileToLoadQueue(
        this->_loadQueueMedium,
        frameState.viewState,
        tile,
        distance);
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
    double distance) {
  TileSelectionState lastFrameSelectionState = tile.getLastSelectionState();

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

  if (tile.getRefine() != Cesium3DTiles::TileRefine::Add) {
    renderList.push_back(&tile);
  }

  tile.setLastSelectionState(TileSelectionState(
      frameState.currentFrameNumber,
      TileSelectionState::Result::Rendered));

  // If we're waiting on heaps of descendants, the above will take too long. So
  // in that case, load this tile INSTEAD of loading any of the descendants, and
  // tell the up-level we're only waiting on this tile. Keep doing this until we
  // actually manage to render this tile.
  bool wasRenderedLastFrame =
      lastFrameSelectionState.getResult(frameState.lastFrameNumber) ==
      TileSelectionState::Result::Rendered;
  bool wasReallyRenderedLastFrame = wasRenderedLastFrame && tile.isRenderable();

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
          frameState.viewState,
          tile,
          distance);
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
    double distance,
    bool culled,
    ViewUpdateResult& result) {
  ++result.tilesVisited;
  result.maxDepthVisited = glm::max(result.maxDepthVisited, depth);

  if (culled) {
    ++result.culledTilesVisited;
  }

  // If this is a leaf tile, just render it (it's already been deemed visible).
  if (isLeaf(tile)) {
    return _renderLeaf(frameState, tile, distance, result);
  }

  bool meetsSse = _meetsSse(frameState.viewState, tile, distance, culled);
  bool waitingForChildren =
      _queueLoadOfChildrenRequiredForRefinement(frameState, tile, distance);

  if (meetsSse || ancestorMeetsSse || waitingForChildren) {
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
    TileSelectionState lastFrameSelectionState = tile.getLastSelectionState();
    bool renderThisTile = shouldRenderThisTile(
        tile,
        lastFrameSelectionState,
        frameState.lastFrameNumber);
    if (renderThisTile) {
      // Only load this tile if it (not just an ancestor) meets the SSE.
      if (meetsSse) {
        addTileToLoadQueue(
            this->_loadQueueMedium,
            frameState.viewState,
            tile,
            distance);
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
          frameState.viewState,
          tile,
          distance);
    }
  }

  // Refine!

  bool queuedForLoad =
      _loadAndRenderAdditiveRefinedTile(frameState, tile, result, distance);

  size_t firstRenderedDescendantIndex = result.tilesToRenderThisFrame.size();
  size_t loadIndexLow = this->_loadQueueLow.size();
  size_t loadIndexMedium = this->_loadQueueMedium.size();
  size_t loadIndexHigh = this->_loadQueueHigh.size();

  TraversalDetails traversalDetails = this->_visitVisibleChildrenNearToFar(
      frameState,
      depth,
      ancestorMeetsSse,
      tile,
      result);

  bool descendantTilesAdded =
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
        distance);
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
        frameState.viewState,
        tile,
        distance);
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
    TraversalDetails childTraversal = this->_visitTileIfNeeded(
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
  Tileset::processQueue(
      this->_loadQueueHigh,
      this->_loadsInProgress,
      this->_options.maximumSimultaneousTileLoads);
  Tileset::processQueue(
      this->_loadQueueMedium,
      this->_loadsInProgress,
      this->_options.maximumSimultaneousTileLoads);
  Tileset::processQueue(
      this->_loadQueueLow,
      this->_loadsInProgress,
      this->_options.maximumSimultaneousTileLoads);
}

void Tileset::_unloadCachedTiles() {
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

    bool removed = pTile->unloadContent();
    if (removed) {
      this->_loadedTiles.remove(*pTile);
    }

    pTile = pNext;
  }
}

void Tileset::_markTileVisited(Tile& tile) {
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
            } else if (placeholder == "x") {
              return std::to_string(quadtreeID.x);
            } else if (placeholder == "y") {
              return std::to_string(quadtreeID.y);
            } else if (placeholder == "version") {
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
            } else if (placeholder == "x") {
              return std::to_string(octreeID.x);
            } else if (placeholder == "y") {
              return std::to_string(octreeID.y);
            } else if (placeholder == "z") {
              return std::to_string(octreeID.z);
            } else if (placeholder == "version") {
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

static bool anyRasterOverlaysNeedLoading(const Tile& tile) {
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
    const ViewState& viewState,
    Tile& tile,
    double distance) {
  if (tile.getState() == Tile::LoadState::Unloaded ||
      anyRasterOverlaysNeedLoading(tile)) {
    double loadPriority = 0.0;

    glm::dvec3 tileDirection =
        getBoundingVolumeCenter(tile.getBoundingVolume()) -
        viewState.getPosition();
    double magnitude = glm::length(tileDirection);
    if (magnitude >= CesiumUtility::Math::EPSILON5) {
      tileDirection /= magnitude;
      loadPriority =
          (1.0 - glm::dot(tileDirection, viewState.getDirection())) * distance;
    }

    loadQueue.push_back({&tile, loadPriority});
  }
}

/*static*/ void Tileset::processQueue(
    std::vector<Tileset::LoadRecord>& queue,
    std::atomic<uint32_t>& loadsInProgress,
    uint32_t maximumLoadsInProgress) {
  if (loadsInProgress >= maximumLoadsInProgress) {
    return;
  }

  std::sort(queue.begin(), queue.end());

  for (LoadRecord& record : queue) {
    record.pTile->loadContent();

    if (loadsInProgress >= maximumLoadsInProgress) {
      break;
    }
  }
}
} // namespace Cesium3DTiles
