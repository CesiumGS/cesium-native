#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumJsonReader/JsonObjectJsonHandler.h>
#include <CesiumJsonReader/JsonReader.h>
#include <CesiumRasterOverlays/AzureMapsRasterOverlay.h>
#include <CesiumRasterOverlays/QuadtreeRasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Uri.h>

#include <fmt/format.h>
#include <nonstd/expected.hpp>
#include <rapidjson/document.h>
#include <rapidjson/pointer.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumJsonReader;
using namespace CesiumUtility;

namespace {
struct CoverageArea {
  GlobeRectangle rectangle;
  uint32_t zoomMin;
  uint32_t zoomMax;
};

struct CreditAndCoverageAreas {
  Credit credit;
  std::vector<CoverageArea> coverageAreas;
};

std::unordered_map<std::string, std::vector<std::byte>> sessionCache;
} // namespace

namespace CesiumRasterOverlays {
// TODO replace with Azure Maps
const std::string AzureMapsRasterOverlay::AZURE_MAPS_LOGO_HTML =
    "<a href=\"http://www.bing.com\"><img "
    "src=\"data:image/"
    "png;base64,iVBORw0KGgoAAAANSUhEUgAAAFgAAAATCAMAAAAj1DqpAAAAq1BMVEUAAAD////"
    "//////////////////////////////////////////////////////////////////////////"
    "//////////////////////////////////////////////////////////////////////////"
    "////////////////////////////////////////////////////////////////////////"
    "Nr6iZAAAAOHRSTlMABRRJ0xkgCix/"
    "uOGFYTMmHQh7EANWTQ2s5ZSLaURBzZB0blE6MPLav6SfiD81tJpl68kW98NauovCUzcAAAJxSU"
    "RBVDjLtdTpcpswEADgFQiQzCnAYAMGzGUw+I6P93+yStC0djudZKaJ/"
    "kjaXb7RrADAsazAd4zH4xR7fP4G+HEXR/"
    "4OePYKozQog5oBqKb6pbCUbTKMcwUCXH4tvOxVizYJeJX3nzApZM9/"
    "gtcAl02PUIhAY8gho48UchERsWTMtjxAan0R29DeOXxiSLMtRzheTaQRVqPmTAfpGd7pJqT7iu"
    "WR0eLrwKNGdm6NdhAVpHfjPLKSPN6mAHac910AUCTr3OhMBHUUx3SEpbLhb93e3bER1hemO4sU"
    "kPFKWyzztMz2IdBNUd3Ob7KoqDMT+cd27UEQe0AqBlXngXtPGZBuQIYJSBnhy6V9iLEsJ1g/"
    "42VHJvjsABRLR8UGT82bEbYWPEhmBECJHBFgDi+nvVgeKYrnvy+vbN+"
    "EfJxaQUNFvu7DEd5q3NNVGctC1Ce4D0UHuClFKqAhcfu7Be5N5IYI0oOpvMPgGA1fmb96DMfGH"
    "uHDBJc4fYINARvKCPuJMR92Ww6PB01z8Il7kH/"
    "C6nrzeIWL0wtcbUQuaP6G1S2fw8UOaDK2QjyM5PsI2+bUCX01wb1DTLzQnlsRXlvC7P3bn/"
    "BBtTsJ/"
    "PnVAjpzANTtDjQfnL2AN20j2NOhRu9fXoY7G1bXgW1zBhBkKqzOeDbbTq2oqcYJV8C5g6hRJvR"
    "Qg9vFt1tkIlQUJaUcnsbbPtCe/hU7vpHSi2/"
    "VPoCy4jvbTKr0VIkKjyCAkDAAxuu8eRFIxOOXVydFxTPkWAQenCY3MyX4eFCs/jPnu/"
    "OXfbBoeDOo8wHpy8lKpvoafRoG6YgXFYKP4GSj63gtwWfhHzl7Skq9JTshAAAAAElFTkSuQmCC"
    "\" title=\"Bing Imagery\"/></a>";

namespace {
Rectangle createRectangle(
    const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner) {
  return WebMercatorProjection::computeMaximumProjectedRectangle(
      pOwner->getOptions().ellipsoid);
}

QuadtreeTilingScheme createTilingScheme(
    const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner) {
  return QuadtreeTilingScheme(createRectangle(pOwner), 1, 1);
}
} // namespace

class AzureMapsTileProvider final : public QuadtreeRasterOverlayTileProvider {
public:
  AzureMapsTileProvider(
      const IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CreditSystem>& pCreditSystem,
      std::optional<CesiumUtility::Credit> credit,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& endpoint,
      const std::string& key,
      int32_t minimumLevel,
      int32_t maximumLevel,
      int32_t imageSize)
      : QuadtreeRasterOverlayTileProvider(
            pOwner,
            asyncSystem,
            pAssetAccessor,
            pCreditSystem,
            credit,
            pPrepareRendererResources,
            pLogger,
            WebMercatorProjection(pOwner->getOptions().ellipsoid),
            createTilingScheme(pOwner),
            createRectangle(pOwner),
            minimumLevel,
            maximumLevel,
            imageSize,
            imageSize),
        _endpoint(endpoint),
        _key(key) {}

  virtual ~AzureMapsTileProvider() = default;

  void update(const AzureMapsTileProvider& newProvider) {
    this->_endpoint = newProvider._endpoint;
    this->_key = newProvider._key;
  }

protected:
  virtual CesiumAsync::Future<LoadedRasterOverlayImage> loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override {
    Uri uri(CesiumUtility::Uri::substituteTemplateParameters(
        this->_endpoint,
        [this, &tileID](const std::string& key) {
          if (key == "zoom") {
            return std::to_string(tileID.level);
          }
          if (key == "x") {
            return std::to_string(tileID.x);
          }
          if (key == "y") {
            return std::to_string(tileID.y);
          }
          return key;
        }));

    UriQuery query(uri);
    query.setValue("subscription-key", this->_key);

    std::string url = std::string(uri.toString());

    LoadTileImageFromUrlOptions options;
    options.allowEmptyImages = true;
    options.moreDetailAvailable = tileID.level < this->getMaximumLevel();
    options.rectangle = this->getTilingScheme().tileToRectangle(tileID);

    const CesiumGeospatial::GlobeRectangle tileRectangle =
        CesiumGeospatial::unprojectRectangleSimple(
            this->getProjection(),
            options.rectangle);

    for (const CreditAndCoverageAreas& creditAndCoverageAreas : _credits) {
      for (const CoverageArea& coverageArea :
           creditAndCoverageAreas.coverageAreas) {
        if (coverageArea.zoomMin <= tileID.level &&
            tileID.level <= coverageArea.zoomMax &&
            coverageArea.rectangle.computeIntersection(tileRectangle)
                .has_value()) {
          options.credits.push_back(creditAndCoverageAreas.credit);
          break;
        }
      }
    }

    return this->loadTileImageFromUrl(url, {}, std::move(options));
  }

private:
  std::vector<CreditAndCoverageAreas> _credits;
  std::string _endpoint;
  std::string _key;
};

AzureMapsRasterOverlay::AzureMapsRasterOverlay(
    const std::string& name,
    const AzureMapsSessionParameters& sessionParameters,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _sessionParameters(sessionParameters) {}

AzureMapsRasterOverlay::~AzureMapsRasterOverlay() = default;

Future<RasterOverlay::CreateTileProviderResult>
AzureMapsRasterOverlay::createTileProvider(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    IntrusivePointer<const RasterOverlay> pOwner) const {
  Uri tilesetUri(this->_sessionParameters.apiBaseUrl, "tileset");

  UriQuery tilesetQuery(tilesetUri);
  tilesetQuery.setValue("api-version", this->_sessionParameters.apiVersion);
  tilesetQuery.setValue("tilesetId", this->_sessionParameters.tilesetId);
  tilesetQuery.setValue("subscription-key", this->_sessionParameters.key);
  tilesetQuery.setValue("language", this->_sessionParameters.language);
  tilesetQuery.setValue("view", this->_sessionParameters.view);

  tilesetUri.setQuery(tilesetQuery.toQueryString());

  std::string tilesetUrl = std::string(tilesetUri.toString());

  pOwner = pOwner ? pOwner : this;

  const CesiumGeospatial::Ellipsoid& ellipsoid = this->getOptions().ellipsoid;

  auto handleResponse =
      [pOwner,
       asyncSystem,
       pAssetAccessor,
       pCreditSystem,
       pPrepareRendererResources,
       pLogger,
       &key = this->_sessionParameters.key,
       ellipsoid](
          const std::shared_ptr<IAssetRequest>& pRequest,
          const std::span<const std::byte>& data) -> CreateTileProviderResult {
    JsonObjectJsonHandler handler{};
    ReadJsonResult<JsonValue> response = JsonReader::readJson(data, handler);

    if (!response.value) {
      ErrorList errorList{};
      errorList.errors = std::move(response.errors);
      errorList.warnings = std::move(response.warnings);

      return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
          .type = RasterOverlayLoadType::TileProvider,
          .pRequest = pRequest,
          .message = errorList.format("Failed to parse response from Azure "
                                      "Maps tileset API service:")});
    }

    if (!response.value->isObject()) {
      return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
          .type = RasterOverlayLoadType::TileProvider,
          .pRequest = pRequest,
          .message = "Response from Azure Maps tileset API service was not a "
                     "JSON object."});
    }
    const JsonValue::Object& responseObject = response.value->getObject();

    // "tileSize" is an enum and is thus expected as a string.
    std::string tileSizeEnum = "256";
    int32_t tileSize = 256;

    auto it = responseObject.find("tileSize");
    if (it != responseObject.end() && it->second.isString()) {
      tileSizeEnum = it->second.getString();
    }

    try {
      tileSize = std::stoi(tileSizeEnum);
    } catch (std::exception) {
      return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
          .type = RasterOverlayLoadType::TileProvider,
          .pRequest = pRequest,
          .message = "Response from Azure Maps tileset API service did not "
                     "contain a valid "
                     "'tileSize' property."});
    }

    it = responseObject.find("minzoom");
    if (it == responseObject.end() || !it->second.isNumber()) {
      return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
          .type = RasterOverlayLoadType::TileProvider,
          .pRequest = pRequest,
          .message = "Response from Azure Maps tileset API service did not "
                     "contain a valid "
                     "'minzoom' property."});
    }
    int32_t minimumLevel = it->second.getSafeNumberOrDefault<int32_t>(0);

    it = responseObject.find("maxzoom");
    if (it == responseObject.end() || !it->second.isNumber()) {
      return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
          .type = RasterOverlayLoadType::TileProvider,
          .pRequest = pRequest,
          .message = "Response from Azure Maps tileset API service did not "
                     "contain a valid "
                     "'maxzoom' property."});
    }
    int32_t maximumLevel = it->second.getSafeNumberOrDefault<int32_t>(0);

    std::vector<std::string> tiles;
    it = responseObject.find("tiles");
    if (it != responseObject.end()) {
      tiles = it->second.getArrayOfStrings(std::string());
    }

    if (tiles.empty()) {
      return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
          .type = RasterOverlayLoadType::TileProvider,
          .pRequest = pRequest,
          .message = "Response from Azure Maps tileset API service did not "
                     "contain a valid "
                     "'tiles' property."});
    }

    std::string tileEndpoint;
    for (size_t i = 0; i < tiles.size(); i++) {
      // To quote the Azure Maps documentation: "If multiple endpoints are
      // specified, clients may use any combination of endpoints. All endpoints
      // MUST return the same content for the same URL."
      // Therefore, this just picks the first non-empty string endpoint.
      if (!tiles[i].empty()) {
        tileEndpoint = tiles[i];
        break;
      }
    }

    if (tileEndpoint.empty()) {
      return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
          RasterOverlayLoadType::TileProvider,
          pRequest,
          "Azure Maps returned no valid endpoint for given tilesetId."});
    }

    bool showCredits = pOwner->getOptions().showCreditsOnScreen;
    Credit azureMapsCredit =
        pCreditSystem->createCredit(AZURE_MAPS_LOGO_HTML, showCredits);

    return new AzureMapsTileProvider(
        pOwner,
        asyncSystem,
        pAssetAccessor,
        pCreditSystem,
        azureMapsCredit,
        pPrepareRendererResources,
        pLogger,
        tileEndpoint,
        key,
        minimumLevel,
        maximumLevel,
        tileSize);
  };

  auto cacheResultIt = sessionCache.find(tilesetUrl);
  if (cacheResultIt != sessionCache.end()) {
    return asyncSystem.createResolvedFuture(
        handleResponse(nullptr, std::span<std::byte>(cacheResultIt->second)));
  }

  return pAssetAccessor->get(asyncSystem, tilesetUrl)
      .thenInMainThread(
          [tilesetUrl,
           handleResponse](std::shared_ptr<IAssetRequest>&& pRequest)
              -> CreateTileProviderResult {
            const IAssetResponse* pResponse = pRequest->response();

            if (!pResponse) {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  RasterOverlayLoadType::TileProvider,
                  pRequest,
                  "No response received from Azure Maps imagery tileset "
                  "service."});
            }

            if (pResponse->statusCode() < 200 ||
                pResponse->statusCode() >= 300) {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  .type = RasterOverlayLoadType::TileProvider,
                  .pRequest = pRequest,
                  .message = fmt::format(
                      "Azure Maps API tileset request service returned "
                      "HTTP status code {}.",
                      pResponse->statusCode())});
            }

            CreateTileProviderResult handleResponseResult =
                handleResponse(pRequest, pResponse->data());

            // If the response successfully created a tile provider, cache it.
            if (handleResponseResult) {
              sessionCache[tilesetUrl] = std::vector<std::byte>(
                  pResponse->data().begin(),
                  pResponse->data().end());
            }

            return handleResponseResult;
          });
}

Future<void> AzureMapsRasterOverlay::refreshTileProviderWithNewKey(
    const IntrusivePointer<RasterOverlayTileProvider>& pProvider,
    const std::string& newKey) {
  this->_sessionParameters.key = newKey;

  return this
      ->createTileProvider(
          pProvider->getAsyncSystem(),
          pProvider->getAssetAccessor(),
          pProvider->getCreditSystem(),
          pProvider->getPrepareRendererResources(),
          pProvider->getLogger(),
          &pProvider->getOwner())
      .thenInMainThread([pProvider](CreateTileProviderResult&& result) {
        if (!result) {
          SPDLOG_LOGGER_WARN(
              pProvider->getLogger(),
              "Could not refresh Bing Maps raster overlay with a new key: {}.",
              result.error().message);
          return;
        }

        // Use static_cast instead of dynamic_cast here to avoid the need for
        // RTTI, and because we are certain of the type.
        // NOLINTBEGIN(cppcoreguidelines-pro-type-static-cast-downcast)
        AzureMapsTileProvider* pOld =
            static_cast<AzureMapsTileProvider*>(pProvider.get());
        AzureMapsTileProvider* pNew =
            static_cast<AzureMapsTileProvider*>(result->get());
        // NOLINTEND(cppcoreguidelines-pro-type-static-cast-downcast)
        if (pOld->getCoverageRectangle().getLowerLeft() !=
                pNew->getCoverageRectangle().getLowerLeft() ||
            pOld->getCoverageRectangle().getUpperRight() !=
                pNew->getCoverageRectangle().getUpperRight() ||
            pOld->getHeight() != pNew->getHeight() ||
            pOld->getWidth() != pNew->getWidth() ||
            pOld->getMinimumLevel() != pNew->getMinimumLevel() ||
            pOld->getMaximumLevel() != pNew->getMaximumLevel() ||
            pOld->getProjection() != pNew->getProjection()) {
          SPDLOG_LOGGER_WARN(
              pProvider->getLogger(),
              "Could not refresh Azure Maps raster overlay with a new key "
              "because some metadata properties changed unexpectedly upon "
              "refresh.");
          return;
        }

        pOld->update(*pNew);
      });
}

} // namespace CesiumRasterOverlays
