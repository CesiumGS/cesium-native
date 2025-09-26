#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeometry/QuadtreeRectangleAvailability.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumJsonReader/JsonObjectJsonHandler.h>
#include <CesiumJsonReader/JsonReader.h>
#include <CesiumJsonWriter/JsonObjectWriter.h>
#include <CesiumJsonWriter/PrettyJsonWriter.h>
#include <CesiumRasterOverlays/GoogleMapTilesRasterOverlay.h>
#include <CesiumRasterOverlays/QuadtreeRasterOverlayTileProvider.h>
#include <CesiumUtility/CreditReferencer.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/StringHelpers.h>
#include <CesiumUtility/Uri.h>
#include <CesiumUtility/joinToString.h>

#include <rapidjson/document.h>

#include <memory>
#include <set>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumJsonReader;
using namespace CesiumJsonWriter;
using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace {

// The maximum zoom level supported by Google Maps. The documentation claims
// this is 22, but that's obviously wrong. There's little harm in this being too
// high (except that we can't let tile indices overflow a 32-bit signed integer,
// so 30 is the practical maximum), but if this is too small then we may not
// show some available detail.
const int32_t maximumZoomLevel{28};

class GoogleMapTilesRasterOverlayTileProvider
    : public QuadtreeRasterOverlayTileProvider {
public:
  GoogleMapTilesRasterOverlayTileProvider(
      const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
      std::optional<CesiumUtility::Credit> credit,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& apiBaseUrl,
      const std::string& session,
      const std::string& key,
      uint32_t maximumLevel,
      uint32_t imageWidth,
      uint32_t imageHeight);

  CesiumAsync::Future<void>
  loadAvailability(const CesiumGeometry::QuadtreeTileID& tileID) const;

  CesiumAsync::Future<void> loadCredits();

  virtual void addCredits(
      CesiumUtility::CreditReferencer& creditReferencer) noexcept override;

protected:
  virtual CesiumAsync::Future<LoadedRasterOverlayImage> loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override;

private:
  CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImageFromService(const CesiumGeometry::QuadtreeTileID& tileID) const;

  std::string _apiBaseUrl;
  std::string _session;
  std::string _key;
  std::optional<Credit> _credit;
  mutable QuadtreeRectangleAvailability _availableTiles;
  mutable QuadtreeRectangleAvailability _availableAvailability;
};

void ensureTrailingSlash(std::string& url) {
  Uri uri(url);
  std::string_view path = uri.getPath();
  if (path.empty() || path.back() != '/') {
    uri.setPath(std::string(path) + '/');
    url = uri.toString();
  }
}

} // namespace

namespace CesiumRasterOverlays {

GoogleMapTilesRasterOverlay::GoogleMapTilesRasterOverlay(
    const std::string& name,
    const GoogleMapTilesNewSessionParameters& newSessionParameters,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _newSessionParameters(newSessionParameters),
      _existingSession(std::nullopt) {
  ensureTrailingSlash(this->_newSessionParameters->apiBaseUrl);
}

GoogleMapTilesRasterOverlay::GoogleMapTilesRasterOverlay(
    const std::string& name,
    const GoogleMapTilesExistingSession& existingSession,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _newSessionParameters(std::nullopt),
      _existingSession(existingSession) {
  ensureTrailingSlash(this->_existingSession->apiBaseUrl);
}

Future<RasterOverlay::CreateTileProviderResult>
GoogleMapTilesRasterOverlay::createTileProvider(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    IntrusivePointer<const RasterOverlay> pOwner) const {
  pOwner = pOwner ? pOwner : this;

  if (this->_newSessionParameters) {
    return this->createNewSession(
        asyncSystem,
        pAssetAccessor,
        pCreditSystem,
        pPrepareRendererResources,
        pLogger,
        pOwner);
  } else if (this->_existingSession) {
    const GoogleMapTilesExistingSession& session = *this->_existingSession;

    IntrusivePointer pTileProvider =
        new GoogleMapTilesRasterOverlayTileProvider(
            pOwner,
            asyncSystem,
            pAssetAccessor,
            pCreditSystem,
            std::nullopt,
            pPrepareRendererResources,
            pLogger,
            session.apiBaseUrl,
            session.session,
            session.key,
            maximumZoomLevel,
            session.tileWidth,
            session.tileHeight);

    // Load credits, but don't wait for them to load.
    pTileProvider->loadCredits();

    // Load initial availability information before trying to fulfill
    // tile requests. This should drastically reduce the number of
    // viewport requests we need to do.
    return pTileProvider->loadAvailability(QuadtreeTileID(0, 0, 0))
        .thenImmediately([pTileProvider]() {
          return CreateTileProviderResult(pTileProvider);
        });
  } else {
    return asyncSystem.createResolvedFuture<CreateTileProviderResult>(
        nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
            .type = RasterOverlayLoadType::TileProvider,
            .pRequest = nullptr,
            .message =
                "GoogleMapTilesRasterOverlay is not configured with either "
                "new session parameters or an existing session."}));
  }
}

Future<RasterOverlay::CreateTileProviderResult>
GoogleMapTilesRasterOverlay::createNewSession(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    IntrusivePointer<const RasterOverlay> pOwner) const {
  Uri createSessionUri(
      this->_newSessionParameters->apiBaseUrl,
      "v1/createSession",
      true);

  if (!this->_newSessionParameters->key.empty()) {
    UriQuery query(createSessionUri);
    query.setValue("key", this->_newSessionParameters->key);
    createSessionUri.setQuery(query.toQueryString());
  }

  // Add required fields
  JsonValue::Object requestPayload = JsonValue::Object{
      {"mapType", this->_newSessionParameters->mapType},
      {"language", this->_newSessionParameters->language},
      {"region", this->_newSessionParameters->region}};

  // Add optional fields
  if (this->_newSessionParameters->imageFormat) {
    requestPayload.emplace(
        "imageFormat",
        *this->_newSessionParameters->imageFormat);
  }
  if (this->_newSessionParameters->scale) {
    requestPayload.emplace("scale", *this->_newSessionParameters->scale);
  }
  if (this->_newSessionParameters->highDpi) {
    requestPayload.emplace("highDpi", *this->_newSessionParameters->highDpi);
  }
  if (this->_newSessionParameters->layerTypes) {
    requestPayload.emplace(
        "layerTypes",
        *this->_newSessionParameters->layerTypes);
  }
  if (this->_newSessionParameters->styles) {
    requestPayload.emplace("styles", *this->_newSessionParameters->styles);
  }
  if (this->_newSessionParameters->overlay) {
    requestPayload.emplace("overlay", *this->_newSessionParameters->overlay);
  }

  PrettyJsonWriter writer{};
  writeJsonValue(requestPayload, writer);
  std::vector<std::byte> requestPayloadBytes = writer.toBytes();

  return pAssetAccessor
      ->request(
          asyncSystem,
          "POST",
          std::string(createSessionUri.toString()),
          {{"Content-Type", "application/json"}},
          requestPayloadBytes)
      .thenInMainThread(
          [this,
           asyncSystem,
           pAssetAccessor,
           pCreditSystem,
           pPrepareRendererResources,
           pLogger,
           pOwner](std::shared_ptr<IAssetRequest>&& pRequest)
              -> Future<CreateTileProviderResult> {
            const IAssetResponse* pResponse = pRequest->response();
            if (!pResponse) {
              return asyncSystem.createResolvedFuture<CreateTileProviderResult>(
                  nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                      .type = RasterOverlayLoadType::TileProvider,
                      .pRequest = pRequest,
                      .message =
                          "No response received from Google Map Tiles API "
                          "createSession service."}));
            }

            if (pResponse->statusCode() < 200 ||
                pResponse->statusCode() >= 300) {
              return asyncSystem.createResolvedFuture<CreateTileProviderResult>(
                  nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                      .type = RasterOverlayLoadType::TileProvider,
                      .pRequest = pRequest,
                      .message = fmt::format(
                          "Google Map Tiles API createSession service returned "
                          "HTTP status code {}.",
                          pResponse->statusCode())}));
            }

            JsonObjectJsonHandler handler{};
            ReadJsonResult<JsonValue> response =
                JsonReader::readJson(pResponse->data(), handler);

            if (!response.value) {
              ErrorList errorList{};
              errorList.errors = std::move(response.errors);
              errorList.warnings = std::move(response.warnings);

              return asyncSystem.createResolvedFuture<CreateTileProviderResult>(
                  nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                      .type = RasterOverlayLoadType::TileProvider,
                      .pRequest = pRequest,
                      .message = errorList.format(
                          "Failed to parse response from Google Map Tiles API "
                          "createSession service:")}));
            }

            if (!response.value->isObject()) {
              return asyncSystem.createResolvedFuture<CreateTileProviderResult>(
                  nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                      .type = RasterOverlayLoadType::TileProvider,
                      .pRequest = pRequest,
                      .message =
                          "Response from Google Map Tiles API "
                          "createSession service was not a JSON object."}));
            }

            const JsonValue::Object& responseObject =
                response.value->getObject();

            auto it = responseObject.find("session");
            if (it == responseObject.end() || !it->second.isString()) {
              return asyncSystem.createResolvedFuture<CreateTileProviderResult>(
                  nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                      .type = RasterOverlayLoadType::TileProvider,
                      .pRequest = pRequest,
                      .message =
                          "Response from Google Map Tiles API "
                          "createSession service did not contain a valid "
                          "'session' property."}));
            }

            std::string session = it->second.getString();

            it = responseObject.find("expiry");
            if (it == responseObject.end() || !it->second.isString()) {
              return asyncSystem.createResolvedFuture<CreateTileProviderResult>(
                  nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                      .type = RasterOverlayLoadType::TileProvider,
                      .pRequest = pRequest,
                      .message =
                          "Response from Google Map Tiles API "
                          "createSession service did not contain a valid "
                          "'expiry' property."}));
            }

            std::string expiry = it->second.getString();

            it = responseObject.find("tileWidth");
            if (it == responseObject.end() || !it->second.isNumber()) {
              return asyncSystem.createResolvedFuture<CreateTileProviderResult>(
                  nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                      .type = RasterOverlayLoadType::TileProvider,
                      .pRequest = pRequest,
                      .message =
                          "Response from Google Map Tiles API "
                          "createSession service did not contain a valid "
                          "'tileWidth' property."}));
            }

            int32_t tileWidth = it->second.getSafeNumberOrDefault<int32_t>(256);

            it = responseObject.find("tileHeight");
            if (it == responseObject.end() || !it->second.isNumber()) {
              return asyncSystem.createResolvedFuture<CreateTileProviderResult>(
                  nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                      .type = RasterOverlayLoadType::TileProvider,
                      .pRequest = pRequest,
                      .message =
                          "Response from Google Map Tiles API "
                          "createSession service did not contain a valid "
                          "'tileHeight' property."}));
            }

            int32_t tileHeight =
                it->second.getSafeNumberOrDefault<int32_t>(256);

            it = responseObject.find("imageFormat");
            if (it == responseObject.end() || !it->second.isString()) {
              return asyncSystem.createResolvedFuture<CreateTileProviderResult>(
                  nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                      .type = RasterOverlayLoadType::TileProvider,
                      .pRequest = pRequest,
                      .message =
                          "Response from Google Map Tiles API "
                          "createSession service did not contain a valid "
                          "'imageFormat' property."}));
            }

            std::string imageFormat = it->second.getString();

            IntrusivePointer pTileProvider =
                new GoogleMapTilesRasterOverlayTileProvider(
                    pOwner,
                    asyncSystem,
                    pAssetAccessor,
                    pCreditSystem,
                    std::nullopt,
                    pPrepareRendererResources,
                    pLogger,
                    this->_newSessionParameters->apiBaseUrl,
                    session,
                    this->_newSessionParameters->key,
                    maximumZoomLevel,
                    static_cast<uint32_t>(tileWidth),
                    static_cast<uint32_t>(tileHeight));

            // Load credits, but don't wait for them to load.
            pTileProvider->loadCredits();

            // Load initial availability information before trying to fulfill
            // tile requests. This should drastically reduce the number of
            // viewport requests we need to do.
            return pTileProvider->loadAvailability(QuadtreeTileID(0, 0, 0))
                .thenImmediately([pTileProvider]() {
                  return CreateTileProviderResult(pTileProvider);
                });
          });
}

} // namespace CesiumRasterOverlays

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

GoogleMapTilesRasterOverlayTileProvider::
    GoogleMapTilesRasterOverlayTileProvider(
        const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner,
        const CesiumAsync::AsyncSystem& asyncSystem,
        const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
        const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
        std::optional<CesiumUtility::Credit> credit,
        const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
            pPrepareRendererResources,
        const std::shared_ptr<spdlog::logger>& pLogger,
        const std::string& apiBaseUrl,
        const std::string& session,
        const std::string& key,
        uint32_t maximumLevel,
        uint32_t imageWidth,
        uint32_t imageHeight)
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
          0,
          maximumLevel,
          imageWidth,
          imageHeight),
      _apiBaseUrl(apiBaseUrl),
      _session(session),
      _key(key),
      _credit(),
      _availableTiles(createTilingScheme(pOwner), maximumLevel),
      _availableAvailability(createTilingScheme(pOwner), maximumLevel) {}

void GoogleMapTilesRasterOverlayTileProvider::addCredits(
    CesiumUtility::CreditReferencer& creditReferencer) noexcept {
  QuadtreeRasterOverlayTileProvider::addCredits(creditReferencer);

  if (this->_credit) {
    creditReferencer.addCreditReference(*this->_credit);
  }
}

CesiumAsync::Future<LoadedRasterOverlayImage>
GoogleMapTilesRasterOverlayTileProvider::loadQuadtreeTileImage(
    const CesiumGeometry::QuadtreeTileID& tileID) const {
  // 1. If the tile is known to be available, load it.
  if (this->_availableTiles.isTileAvailable(tileID)) {
    return this->loadTileImageFromService(tileID);
  }

  // 2. If we have complete availability information here, that means the tile
  // is definitely _not_ available (or we would have returned at (1) above).
  //
  // Note that _availableAvailability conceptually only stores level-less
  // rectangles. So, to query it, we use the maximum level tile that contains
  // this tile's center.
  std::optional<QuadtreeTileID> maybeMaxLevelTile =
      this->getTilingScheme().positionToTile(
          this->getTilingScheme().tileToRectangle(tileID).getCenter(),
          this->getMaximumLevel());

  if (!maybeMaxLevelTile ||
      this->_availableAvailability.isTileAvailable(*maybeMaxLevelTile)) {
    return this->getAsyncSystem()
        .createResolvedFuture<LoadedRasterOverlayImage>(
            LoadedRasterOverlayImage{});
  }

  // 3. We can't tell if this tile is available. Check with the `viewport`
  // service.
  return this->loadAvailability(tileID).thenInMainThread([this, tileID]() {
    // We asked for availability information for just this one tile. We can only
    // assume Google gave it to us within the first 100 rectangles included in
    // the response.
    if (this->_availableTiles.isTileAvailable(tileID)) {
      return this->loadTileImageFromService(tileID);
    } else {
      return this->getAsyncSystem()
          .createResolvedFuture<LoadedRasterOverlayImage>(
              LoadedRasterOverlayImage{});
    }
  });
}

CesiumAsync::Future<LoadedRasterOverlayImage>
GoogleMapTilesRasterOverlayTileProvider::loadTileImageFromService(
    const CesiumGeometry::QuadtreeTileID& tileID) const {
  Uri tileUri(
      this->_apiBaseUrl,
      fmt::format(
          "v1/2dtiles/{}/{}/{}",
          tileID.level,
          tileID.x,
          tileID.computeInvertedY(this->getTilingScheme())),
      true);

  UriQuery tileQuery(tileUri);
  tileQuery.setValue("session", this->_session);
  tileQuery.setValue("key", this->_key);

  tileUri.setQuery(tileQuery.toQueryString());

  LoadTileImageFromUrlOptions options;
  options.rectangle = this->getTilingScheme().tileToRectangle(tileID);
  options.moreDetailAvailable = tileID.level < this->getMaximumLevel();

  return this->loadTileImageFromUrl(
      std::string(tileUri.toString()),
      {},
      std::move(options));
}

namespace {
QuadtreeTileRectangularRange rectangleForAvailability(
    const QuadtreeTilingScheme& tilingScheme,
    uint32_t maximumLevel,
    const Rectangle& rectangle) {
  std::optional<QuadtreeTileID> sw =
      tilingScheme.positionToTile(rectangle.getLowerLeft(), maximumLevel);
  std::optional<QuadtreeTileID> ne =
      tilingScheme.positionToTile(rectangle.getUpperRight(), maximumLevel);

  if (!sw || !ne) {
    // This should never happen, because the input rectangle should always
    // be within the tiling scheme's rectangle.
    return QuadtreeTileRectangularRange{
        .level = 0,
        .minimumX = 0,
        .minimumY = 0,
        .maximumX = 0,
        .maximumY = 0};
  }

  return QuadtreeTileRectangularRange{
      .level = maximumLevel,
      .minimumX = sw->x,
      .minimumY = sw->y,
      .maximumX = ne->x,
      .maximumY = ne->y};
}

CesiumAsync::Future<rapidjson::Document> fetchViewportData(
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& apiBaseUrl,
    const std::string& session,
    const std::string& key,
    uint32_t zoom,
    const Rectangle& rectangleDegrees) {
  Uri viewportUri(apiBaseUrl, "tile/v1/viewport", true);

  UriQuery query(viewportUri);
  query.setValue("session", session);
  query.setValue("key", key);
  query.setValue("zoom", std::to_string(zoom));
  query.setValue("west", std::to_string(rectangleDegrees.minimumX));
  query.setValue("south", std::to_string(rectangleDegrees.minimumY));
  query.setValue("east", std::to_string(rectangleDegrees.maximumX));
  query.setValue("north", std::to_string(rectangleDegrees.maximumY));

  viewportUri.setQuery(query.toQueryString());

  return pAssetAccessor->get(asyncSystem, std::string(viewportUri.toString()))
      .thenInMainThread(
          [pLogger](const std::shared_ptr<IAssetRequest>& pRequest)
              -> rapidjson::Document {
            const IAssetResponse* pResponse = pRequest->response();
            rapidjson::Document document;

            if (!pResponse) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "No response received from Google Map Tiles API viewport "
                  "service.");
              return document;
            }

            if (pResponse->statusCode() < 200 ||
                pResponse->statusCode() >= 300) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Error response {} received from Google Map Tiles API "
                  "viewport service.",
                  pResponse->statusCode());
              return document;
            }

            document.Parse(
                reinterpret_cast<const char*>(pResponse->data().data()),
                pResponse->data().size());

            if (document.HasParseError()) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Error when parsing Google Map Tiles API viewport "
                  "service JSON, error code {} at byte offset {}.",
                  document.GetParseError(),
                  document.GetErrorOffset());
              return document;
            }

            return document;
          });
}

} // namespace

CesiumAsync::Future<void>
GoogleMapTilesRasterOverlayTileProvider::loadAvailability(
    const CesiumGeometry::QuadtreeTileID& tileID) const {
  CesiumGeometry::Rectangle tileRectangle =
      this->getTilingScheme().tileToRectangle(tileID);

  GlobeRectangle tileGlobeRectangle =
      unprojectRectangleSimple(this->getProjection(), tileRectangle);

  Rectangle rectangleDegrees(
      Math::radiansToDegrees(tileGlobeRectangle.getWest()),
      Math::radiansToDegrees(tileGlobeRectangle.getSouth()),
      Math::radiansToDegrees(tileGlobeRectangle.getEast()),
      Math::radiansToDegrees(tileGlobeRectangle.getNorth()));

  return fetchViewportData(
             this->getAssetAccessor(),
             this->getAsyncSystem(),
             this->getLogger(),
             this->_apiBaseUrl,
             this->_session,
             this->_key,
             tileID.level,
             rectangleDegrees)
      .thenInMainThread([this, tileRectangle](rapidjson::Document&& document) {
        if (document.HasParseError()) {
          return;
        }

        auto maxZoomRectsIt = document.FindMember("maxZoomRects");
        if (maxZoomRectsIt == document.MemberEnd() ||
            !maxZoomRectsIt->value.IsArray()) {
          SPDLOG_LOGGER_ERROR(
              this->getLogger(),
              "Google Map Tiles API viewport service JSON is missing "
              "the "
              "`maxZoomRects` property.");
          return;
        }

        const rapidjson::Value& maxZoomRects = maxZoomRectsIt->value;
        for (auto it = maxZoomRects.Begin(); it != maxZoomRects.End(); ++it) {
          const auto& rectJson = *it;
          if (!rectJson.IsObject()) {
            continue;
          }

          int32_t maxZoom =
              JsonHelpers::getInt32OrDefault(rectJson, "maxZoom", -1);
          std::optional<double> maybeWest =
              JsonHelpers::getScalarProperty(rectJson, "west");
          std::optional<double> maybeSouth =
              JsonHelpers::getScalarProperty(rectJson, "south");
          std::optional<double> maybeEast =
              JsonHelpers::getScalarProperty(rectJson, "east");
          std::optional<double> maybeNorth =
              JsonHelpers::getScalarProperty(rectJson, "north");

          if (maxZoom < 0 || !maybeWest || !maybeSouth || !maybeEast ||
              !maybeNorth) {
            continue;
          }

          Cartographic southwestRadians =
              Cartographic::fromDegrees(*maybeWest, *maybeSouth);
          Cartographic northeastRadians =
              Cartographic::fromDegrees(*maybeEast, *maybeNorth);

          // Clamp the South and North coordinates to the tiling
          // scheme's rectangle, because the viewport service likes to
          // tell us about rectangles that go all the way to the poles
          // when that is not possible in Web Mercator.
          GlobeRectangle maxRectangle =
              WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;
          southwestRadians.latitude =
              std::max(southwestRadians.latitude, maxRectangle.getSouth());
          northeastRadians.latitude =
              std::min(northeastRadians.latitude, maxRectangle.getNorth());

          glm::dvec2 southwestMercator = glm::dvec2(
              projectPosition(this->getProjection(), southwestRadians));
          glm::dvec2 northeastMercator = glm::dvec2(
              projectPosition(this->getProjection(), northeastRadians));

          // Clamp the projected position, too. A position right on the
          // boundary can end up on the wrong side when projected.
          Rectangle maxProjectedRectangle =
              this->getTilingScheme().getRectangle();
          southwestMercator =
              glm::max(southwestMercator, maxProjectedRectangle.getLowerLeft());
          northeastMercator = glm::min(
              northeastMercator,
              maxProjectedRectangle.getUpperRight());

          // Convert the geographic coordinates to tile coordinates.
          std::optional<QuadtreeTileID> sw =
              this->getTilingScheme().positionToTile(
                  southwestMercator,
                  uint32_t(maxZoom));
          std::optional<QuadtreeTileID> ne =
              this->getTilingScheme().positionToTile(
                  northeastMercator,
                  uint32_t(maxZoom));

          if (!sw || !ne) {
            continue;
          }

          this->_availableTiles.addAvailableTileRange(
              QuadtreeTileRectangularRange{
                  .level = uint32_t(maxZoom),
                  .minimumX = sw->x,
                  .minimumY = sw->y,
                  .maximumX = ne->x,
                  .maximumY = ne->y});

          // Availability of this level implies availability of all
          // prior levels, too.
          while (maxZoom > 0) {
            --maxZoom;
            sw->x >>= 1;
            sw->y >>= 1;
            ne->x >>= 1;
            ne->y >>= 1;

            this->_availableTiles.addAvailableTileRange(
                QuadtreeTileRectangularRange{
                    .level = uint32_t(maxZoom),
                    .minimumX = sw->x,
                    .minimumY = sw->y,
                    .maximumX = ne->x,
                    .maximumY = ne->y});
          }
        }

        // If the response has fewer than 100 maxZoomRects, we know that
        // this is _all_ of them. If it contains 100, there may be more
        // we don't know about yet because Google truncated the list.
        if (maxZoomRects.Size() < 100) {
          this->_availableAvailability.addAvailableTileRange(
              rectangleForAvailability(
                  this->getTilingScheme(),
                  this->getMaximumLevel(),
                  tileRectangle));
        }
      });
}

Future<void> GoogleMapTilesRasterOverlayTileProvider::loadCredits() {
  IntrusivePointer thiz = this;

  std::vector<Future<std::string>> creditFutures;
  creditFutures.reserve(maximumZoomLevel + 1);

  for (size_t i = 0; i <= maximumZoomLevel; ++i) {
    creditFutures.emplace_back(
        fetchViewportData(
            this->getAssetAccessor(),
            this->getAsyncSystem(),
            this->getLogger(),
            this->_apiBaseUrl,
            this->_session,
            this->_key,
            uint32_t(i),
            Rectangle(-180.0, -90.0, 180.0, 90.0))
            .thenInMainThread([](rapidjson::Document&& document) {
              if (document.HasParseError()) {
                return std::string();
              }

              auto attributionsIt = document.FindMember("copyright");
              if (attributionsIt == document.MemberEnd() ||
                  !attributionsIt->value.IsString()) {
                return std::string();
              }

              return std::string(
                  attributionsIt->value.GetString(),
                  attributionsIt->value.GetStringLength());
            }));
  }

  return this->getAsyncSystem()
      .all(std::move(creditFutures))
      .thenInMainThread([thiz](std::vector<std::string>&& results) {
        const std::string copyrightPrefix = "Imagery ©";
        std::set<std::string> uniqueCredits;
        std::vector<std::string> credits;
        std::string preamble;

        for (size_t i = 0; i < results.size(); ++i) {
          // Each copyright starts with "Imagery ©2025" (or some other year
          // in the future, presumably). Remove that preamble, but remember
          // it so we can re-add it later.
          std::string creditString = results[i];
          if (creditString.find(copyrightPrefix) == 0) {
            // Skip any spaces after the copyright symbol.
            size_t firstNonSpace =
                creditString.find_first_not_of(' ', copyrightPrefix.size());

            // Skip the number (year).
            size_t firstNonNumberAfterSpace =
                firstNonSpace == std::string::npos
                    ? std::string::npos
                    : creditString.find_first_not_of(
                          "0123456789",
                          firstNonSpace);
            if (firstNonNumberAfterSpace != std::string::npos) {
              // Use the first non-empty preamble we find. Hopefully they're all
              // the same, but we don't check.
              if (preamble.empty()) {
                preamble = creditString.substr(0, firstNonNumberAfterSpace);
              }

              // The actual credit string starts after the preamble.
              creditString = creditString.substr(firstNonNumberAfterSpace);
            }
          }

          std::vector<std::string_view> parts = StringHelpers::splitOnCharacter(
              creditString,
              ',',
              StringHelpers::SplitOptions{
                  .trimWhitespace = true,
                  .omitEmptyParts = true});

          for (size_t partIndex = 0; partIndex < parts.size(); ++partIndex) {
            // If any of the parts are "Inc.", it should be tacked onto
            // the previous item. It would be nice if comma weren't both a
            // separator and a thing that occurs in individual credits,
            // but alas. This is not an exhaustive list of things that
            // could potentially include commas, but it's the only one
            // we've seen so far.
            std::string credit = std::string(parts[partIndex]);
            if (partIndex != parts.size() - 1 &&
                parts[partIndex + 1] == "Inc.") {
              credit += ", Inc.";
              ++partIndex;
            }

            // `uniqueCredits` ensures uniqueness, `credits` maintains
            // order.
            if (uniqueCredits.insert(credit).second) {
              credits.emplace_back(std::move(credit));
            }
          }
        }

        // Join all the credits back together, and re-add the preamble.
        std::string joined = joinToString(credits, ", ");
        if (!preamble.empty()) {
          joined = preamble + " " + joined;
        }

        // Create a single credit from this giant string.
        thiz->_credit = thiz->getCreditSystem()->createCredit(joined, false);
      });
}

} // namespace
