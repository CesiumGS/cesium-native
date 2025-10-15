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
#include <CesiumUtility/CreditReferencer.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Uri.h>
#include <CesiumUtility/joinToString.h>

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
std::unordered_map<std::string, std::vector<std::byte>> sessionCache;
} // namespace

namespace CesiumRasterOverlays {
const std::string AZURE_MAPS_LOGO_HTML =
    "<img alt=\"Microsoft Azure\" "
    "src=\"data:image/"
    "png;base64,"
    "iVBORw0KGgoAAAANSUhEUgAAAGkAAAAUCAMAAAC54OMXAAAAAXNSR0IB2cksfwAAAAlwSFlzAA"
    "ALEwAACxMBAJqcGAAAAsRQTFRFAAAA81Ai81Ai2GsZf7oAf7oA81Ai81Ai2GsZf7oAf7oAc3Nz"
    "c3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3"
    "Nzc3Nzc3NzdHR0c3Nzc3Nzc3Nzc3Nzc3Nzc3NzdHR0c3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nz"
    "c3Nzc3Nzc3NzdHR0dHR0c3Nzc3Nzc3Nzc3Nzc3Nzc3NzdHR0c3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3"
    "Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nz"
    "c3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3"
    "Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nz8lIi8lIi12wYgbkAgbkAc3Nzc3Nzc3Nzc3Nzc3Nzc3Nz"
    "c3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3"
    "Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3NzAKXvAKXvO6e1/bYA/"
    "bYAc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3NzdHR0c3Nzc3Nzc3N"
    "zc3Nzc3Nzc3Nzc3Nzc3Nzc3NzAKXvAKXvPKi1/7gA/"
    "7gAc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3N"
    "zc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc"
    "3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3NzjUES/gAAAOx0Uk5TABUrEisbgP9p/"
    "6UDBhRTTgkNSE/n8AFEPv/"
    "+QV3dCM3LAQTxrT0rewcp7Bj2mAq6EwIRDgIEDBIZY4zZDX7pcQsHD5LfH0Xzop6aNrbi3l9mw"
    "KjhG43Ha9uXRsjglS/"
    "Y7vnaFdJ8ab90fRqQVNW9JlKkRKRqO+htmbuLtdHXVniKahw8UePCSndzW7TmYCRY1M+"
    "Ikx7TaPc4xqlUqUanbCWjxWIqWeogr0vrSVzDCaQjOoD4N1qF+oD/av+l/Iks/"
    "XIWM+"
    "R5QJSCmyJD7xfyV8xsLQU17dC4psQh9UJu9MmwUjFNcHodTChQNGe2vnIEAAAC5klEQVR4nN3R"
    "eyzVYRgH8OdbR2lYiYlUOClxIqVOpnKZM13oakUpUSbNrThhVnRhVlFoK02mUksZRosuM93MZS"
    "4pOpaG0NL10Kg4R6f355TNH2zWWOv7z/P953k/794XNF7BeEpQan+GbOykSUOlb/"
    "+opIbuv5E0AHQSafbzelXlXSOtawEff1edfp7iw2glXSa9JZrVz2MFzcNv8zGQBq6bdmqifvSS"
    "Fl6QBbr7IJ37jBYDbXMgb5GSEHU9RBYTvjaR2kJFhdGMN4ZyFXYr7jKrJGZ4REbarCqgqKRlis"
    "/anwxamxwgrRlBMgMeOAH1EKDAGYMpE06saXRrn40bO4CvU4H7MpN5wJ33RHuBdp3nNfoiotcC"
    "IIU266LWMn9jlWAKkKSUDgyV4jlpJ3AyAsiApgsnfcn1SbVWMc3wPOPyY1Fzqz0SxMxNDi4t19"
    "7FSk8U+7HJ4qNqGq+sXx5jjJUwiFztEXsYGZ4deoCvUkodKnlzUhyQaLRJwkmmh+"
    "I9iKJNsD1R151mTEjEu5nn/"
    "QOnSSg73K5AHBr5KHQL28tN34OQsxuITpni+"
    "Pzf0iXnArEbX111rVK6O1QSDUghWXieb81Jt7p/sseJbiuvd/"
    "1WSM7VvdkJ4uRgd71qymxrGJQsdy6V0AnXrbZPgBVCTlrvhNhsm9KL+"
    "yqJAkeQDtqX51hHctLqdD9H/"
    "nwBk1y0eNUJPY0rhaUldj0qAWHz5KlMSq44yA7LEMCN9p8qCDmbldTt0Bfod0G9hZNqRUUWL6S"
    "2I0hevrLQhQNSbLLjukJZDpP0DY2/"
    "R38K148JOy27nvZUdI93kkk3l1zxMiYP28xicthvceb2LgNac3V3hOxICSdVTT8S05sXNpz0uC"
    "xOapnnVdxqLq/9YMM3jq7NfVNk3kmXRdfO6TRGpS3oeuhQJfHvKJsTYC/"
    "kB3bum2kUNMWPLTtd8bFjoy7buz0F2LbO0Yo0mvqW65cPJ41JxlUap/"
    "yP0i9eZhskyycglQAAAABJRU5ErkJggg==\"/>";

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

class AzureMapsRasterOverlayTileProvider final
    : public QuadtreeRasterOverlayTileProvider {
public:
  AzureMapsRasterOverlayTileProvider(
      const IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CreditSystem>& pCreditSystem,
      std::optional<CesiumUtility::Credit> credit,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& baseUrl,
      const std::string& apiVersion,
      const std::string& tilesetId,
      const std::string& key,
      const std::string& tileEndpoint,
      int32_t minimumLevel,
      int32_t maximumLevel,
      int32_t imageSize,
      bool showLogo);

  virtual ~AzureMapsRasterOverlayTileProvider() = default;

  void update(const AzureMapsRasterOverlayTileProvider& newProvider) {
    this->_baseUrl = newProvider._baseUrl;
    this->_apiVersion = newProvider._apiVersion;
    this->_tilesetId = newProvider._tilesetId;
    this->_key = newProvider._key;
    this->_tileEndpoint = newProvider._tileEndpoint;
  }

  CesiumAsync::Future<void> loadCredits();

  virtual void addCredits(
      CesiumUtility::CreditReferencer& creditReferencer) noexcept override;

protected:
  virtual CesiumAsync::Future<LoadedRasterOverlayImage> loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override;

private:
  std::string _baseUrl;
  std::string _apiVersion;
  std::string _tilesetId;
  std::string _key;
  std::string _tileEndpoint;
  std::optional<Credit> _azureCredit;
  std::optional<Credit> _credits;
};

AzureMapsRasterOverlay::AzureMapsRasterOverlay(
    const std::string& name,
    const AzureMapsSessionParameters& sessionParameters,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _sessionParameters(sessionParameters) {
  Uri::ensureTrailingSlash(this->_sessionParameters.apiBaseUrl);
}

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
  Uri tilesetUri(this->_sessionParameters.apiBaseUrl, "map/tileset");

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
       &sessionParameters = this->_sessionParameters,
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

    // it = responseObject.find("attribution");

    bool showCredits = pOwner->getOptions().showCreditsOnScreen;
    Credit azureMapsCredit =
        pCreditSystem->createCredit(AZURE_MAPS_LOGO_HTML, showCredits);

    auto* pProvider = new AzureMapsRasterOverlayTileProvider(
        pOwner,
        asyncSystem,
        pAssetAccessor,
        pCreditSystem,
        azureMapsCredit,
        pPrepareRendererResources,
        pLogger,
        sessionParameters.apiBaseUrl,
        sessionParameters.apiVersion,
        sessionParameters.tilesetId,
        sessionParameters.key,
        tileEndpoint,
        minimumLevel,
        maximumLevel,
        tileSize,
        sessionParameters.showLogo);

    // Start loading credits, but don't wait for the load to finish.
    pProvider->loadCredits();

    return pProvider;
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
              "Could not refresh Azure Maps raster overlay with a new key: {}.",
              result.error().message);
          return;
        }

        // Use static_cast instead of dynamic_cast here to avoid the need for
        // RTTI, and because we are certain of the type.
        // NOLINTBEGIN(cppcoreguidelines-pro-type-static-cast-downcast)
        AzureMapsRasterOverlayTileProvider* pOld =
            static_cast<AzureMapsRasterOverlayTileProvider*>(pProvider.get());
        AzureMapsRasterOverlayTileProvider* pNew =
            static_cast<AzureMapsRasterOverlayTileProvider*>(result->get());
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

namespace {
CesiumAsync::Future<rapidjson::Document> fetchAttributionData(
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& apiBaseUrl,
    const std::string& apiVersion,
    const std::string& tilesetId,
    const std::string& key,
    uint32_t zoom,
    const Rectangle& rectangleDegrees) {
  std::vector<std::string> bounds(4);
  bounds[0] = std::to_string(rectangleDegrees.minimumX); // west
  bounds[1] = std::to_string(rectangleDegrees.minimumY); // south
  bounds[2] = std::to_string(rectangleDegrees.maximumX); // east
  bounds[3] = std::to_string(rectangleDegrees.maximumY); // north

  Uri attributionUri(apiBaseUrl, "map/attribution", true);

  UriQuery query(attributionUri);
  query.setValue("api-version", apiVersion);
  query.setValue("subscription-key", key);
  query.setValue("tilesetId", tilesetId);
  query.setValue("zoom", std::to_string(zoom));
  query.setValue("bounds", joinToString(bounds, ","));
  attributionUri.setQuery(query.toQueryString());

  return pAssetAccessor
      ->get(asyncSystem, std::string(attributionUri.toString()))
      .thenInMainThread(
          [pLogger](const std::shared_ptr<IAssetRequest>& pRequest)
              -> rapidjson::Document {
            const IAssetResponse* pResponse = pRequest->response();
            rapidjson::Document document;

            if (!pResponse) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "No response received from Azure Maps API attribution "
                  "service.");
              return document;
            }

            if (pResponse->statusCode() < 200 ||
                pResponse->statusCode() >= 300) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Error response {} received from Azure Maps API attribution "
                  "service URL {}.",
                  pResponse->statusCode(),
                  pRequest->url());
              return document;
            }

            document.Parse(
                reinterpret_cast<const char*>(pResponse->data().data()),
                pResponse->data().size());

            if (document.HasParseError()) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Error when parsing Azure Maps API attribution service JSON "
                  "from URL {}, error code {} at byte offset {}.",
                  pRequest->url(),
                  document.GetParseError(),
                  document.GetErrorOffset());
            }

            return document;
          });
}
} // namespace

AzureMapsRasterOverlayTileProvider::AzureMapsRasterOverlayTileProvider(
    const IntrusivePointer<const RasterOverlay>& pOwner,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    std::optional<CesiumUtility::Credit> credit,
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& baseUrl,
    const std::string& apiVersion,
    const std::string& tilesetId,
    const std::string& key,
    const std::string& tileEndpoint,
    int32_t minimumLevel,
    int32_t maximumLevel,
    int32_t imageSize,
    bool showLogo)
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
      _baseUrl(baseUrl),
      _apiVersion(apiVersion),
      _tilesetId(tilesetId),
      _key(key),
      _tileEndpoint(tileEndpoint),
      _azureCredit(),
      _credits() {
  if (pCreditSystem && showLogo) {
    this->_azureCredit =
        pCreditSystem->createCredit(AZURE_MAPS_LOGO_HTML, true);
  }
}

CesiumAsync::Future<LoadedRasterOverlayImage>
AzureMapsRasterOverlayTileProvider::loadQuadtreeTileImage(
    const CesiumGeometry::QuadtreeTileID& tileID) const {
  Uri uri(CesiumUtility::Uri::substituteTemplateParameters(
      this->_tileEndpoint,
      [this, &tileID](const std::string& key) {
        if (key == "z") {
          return std::to_string(tileID.level);
        }
        if (key == "x") {
          return std::to_string(tileID.x);
        }
        if (key == "y") {
          uint32_t invertedY = tileID.computeInvertedY(this->getTilingScheme());
          return std::to_string(invertedY);
        }
        return key;
      }));

  UriQuery query(uri);
  query.setValue("subscription-key", this->_key);
  uri.setQuery(query.toQueryString());

  std::string url = std::string(uri.toString());

  LoadTileImageFromUrlOptions options;
  options.allowEmptyImages = true;
  options.moreDetailAvailable = tileID.level < this->getMaximumLevel();
  options.rectangle = this->getTilingScheme().tileToRectangle(tileID);

  const CesiumGeospatial::GlobeRectangle tileRectangle =
      CesiumGeospatial::unprojectRectangleSimple(
          this->getProjection(),
          options.rectangle);

  return this->loadTileImageFromUrl(url, {}, std::move(options));
}

Future<void> AzureMapsRasterOverlayTileProvider::loadCredits() {
  const uint32_t maximumZoomLevel = this->getMaximumLevel();

  IntrusivePointer thiz = this;

  std::vector<Future<std::string>> creditFutures;
  creditFutures.reserve(maximumZoomLevel + 1);

  for (size_t i = 0; i <= maximumZoomLevel; ++i) {
    creditFutures.emplace_back(
        fetchAttributionData(
            this->getAssetAccessor(),
            this->getAsyncSystem(),
            this->getLogger(),
            this->_baseUrl,
            this->_apiVersion,
            this->_tilesetId,
            this->_key,
            uint32_t(i),
            Rectangle(-180.0, -90.0, 180.0, 90.0))
            .thenInMainThread([](rapidjson::Document&& document) {
              if (document.HasParseError() || !document.IsObject()) {
                return std::string();
              }

              std::vector<std::string> copyrights =
                  JsonHelpers::getStrings(document, "copyrights");

              if (copyrights.empty()) {
                return std::string();
              }

              return joinToString(copyrights, ", ");
            }));
  }

  return this->getAsyncSystem()
      .all(std::move(creditFutures))
      .thenInMainThread([thiz](std::vector<std::string>&& results) {
        std::string joined = joinToString(results, ", ");
        // Create a single credit from this giant string.
        thiz->_credits = thiz->getCreditSystem()->createCredit(joined, false);
      });
}

void AzureMapsRasterOverlayTileProvider::addCredits(
    CesiumUtility::CreditReferencer& creditReferencer) noexcept {
  QuadtreeRasterOverlayTileProvider::addCredits(creditReferencer);

  if (this->_azureCredit) {
    creditReferencer.addCreditReference(*this->_azureCredit);
  }

  if (this->_credits) {
    creditReferencer.addCreditReference(*this->_credits);
  }
}

} // namespace CesiumRasterOverlays
