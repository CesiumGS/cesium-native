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
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Uri.h>

#include <rapidjson/document.h>

#include <memory>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumJsonReader;
using namespace CesiumJsonWriter;
using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace {

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

protected:
  virtual CesiumAsync::Future<LoadedRasterOverlayImage> loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override;

private:
  CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImageFromService(const CesiumGeometry::QuadtreeTileID& tileID) const;

  std::string _apiBaseUrl;
  std::string _session;
  std::string _key;
  mutable QuadtreeRectangleAvailability _availableTiles;
  mutable QuadtreeRectangleAvailability _availableAvailability;
};

} // namespace

namespace CesiumRasterOverlays {

GoogleMapTilesRasterOverlay::GoogleMapTilesRasterOverlay(
    const std::string& name,
    const std::string& key,
    const std::string& mapType,
    const std::string& language,
    const std::string& region,
    const GoogleMapTilesOptions& googleOptions,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _key(key),
      _mapType(mapType),
      _language(language),
      _region(region),
      _googleOptions(googleOptions) {}

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

  Uri createSessionUri(
      this->_googleOptions.apiBaseUrl,
      "v1/createSession",
      true);

  if (!this->_key.empty()) {
    UriQuery query(createSessionUri);
    query.setValue("key", this->_key);
    createSessionUri.setQuery(query.toQueryString());
  }

  // Add required fields
  JsonValue::Object requestPayload = JsonValue::Object{
      {"mapType", this->_mapType},
      {"language", this->_language},
      {"region", this->_region}};

  // Add optional fields
  if (this->_googleOptions.imageFormat) {
    requestPayload.emplace("imageFormat", *this->_googleOptions.imageFormat);
  }
  if (this->_googleOptions.scale) {
    requestPayload.emplace("scale", *this->_googleOptions.scale);
  }
  if (this->_googleOptions.highDpi) {
    requestPayload.emplace("highDpi", *this->_googleOptions.highDpi);
  }
  if (this->_googleOptions.layerTypes) {
    requestPayload.emplace("layerTypes", *this->_googleOptions.layerTypes);
  }
  if (this->_googleOptions.styles) {
    requestPayload.emplace("styles", *this->_googleOptions.styles);
  }
  if (this->_googleOptions.overlay) {
    requestPayload.emplace("overlay", *this->_googleOptions.overlay);
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
                    this->_googleOptions.apiBaseUrl,
                    session,
                    this->_key,
                    this->_googleOptions.maximumZoomLevel,
                    static_cast<uint32_t>(tileWidth),
                    static_cast<uint32_t>(tileHeight));

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
      _availableTiles(createTilingScheme(pOwner), maximumLevel),
      _availableAvailability(createTilingScheme(pOwner), maximumLevel) {}

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

  const GlobeRectangle unprojectedRect =
      unprojectRectangleSimple(this->getProjection(), options.rectangle);

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
    // This should never happen, because the input rectangle should always be
    // within the tiling scheme's rectangle.
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

} // namespace

CesiumAsync::Future<void>
GoogleMapTilesRasterOverlayTileProvider::loadAvailability(
    const CesiumGeometry::QuadtreeTileID& tileID) const {
  Uri viewportUri(this->_apiBaseUrl, "tile/v1/viewport", true);

  CesiumGeometry::Rectangle tileRectangle =
      this->getTilingScheme().tileToRectangle(tileID);

  GlobeRectangle tileGlobeRectangle =
      unprojectRectangleSimple(this->getProjection(), tileRectangle);

  UriQuery query(viewportUri);
  query.setValue("session", this->_session);
  query.setValue("key", this->_key);
  query.setValue("zoom", std::to_string(tileID.level));
  query.setValue(
      "west",
      std::to_string(Math::radiansToDegrees(tileGlobeRectangle.getWest())));
  query.setValue(
      "south",
      std::to_string(Math::radiansToDegrees(tileGlobeRectangle.getSouth())));
  query.setValue(
      "east",
      std::to_string(Math::radiansToDegrees(tileGlobeRectangle.getEast())));
  query.setValue(
      "north",
      std::to_string(Math::radiansToDegrees(tileGlobeRectangle.getNorth())));

  viewportUri.setQuery(query.toQueryString());

  return this->getAssetAccessor()
      ->get(this->getAsyncSystem(), std::string(viewportUri.toString()))
      .thenInMainThread([this, tileRectangle](
                            const std::shared_ptr<IAssetRequest>& pRequest) {
        const IAssetResponse* pResponse = pRequest->response();
        if (!pResponse) {
          SPDLOG_LOGGER_ERROR(
              this->getLogger(),
              "No response received from Google Map Tiles API viewport "
              "service.");
          return;
        }

        if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
          SPDLOG_LOGGER_ERROR(
              this->getLogger(),
              "Error response {} received from Google Map Tiles API viewport "
              "service.",
              pResponse->statusCode());
          return;
        }

        rapidjson::Document document;
        document.Parse(
            reinterpret_cast<const char*>(pResponse->data().data()),
            pResponse->data().size());

        if (document.HasParseError()) {
          SPDLOG_LOGGER_ERROR(
              this->getLogger(),
              "Error when parsing Google Map Tiles API viewport service JSON, "
              "error code {} at byte offset {}.",
              document.GetParseError(),
              document.GetErrorOffset());
          return;
        }

        auto maxZoomRectsIt = document.FindMember("maxZoomRects");
        if (maxZoomRectsIt == document.MemberEnd() ||
            !maxZoomRectsIt->value.IsArray()) {
          SPDLOG_LOGGER_ERROR(
              this->getLogger(),
              "Google Map Tiles API viewport service JSON is missing the "
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

          // Clamp the South and North coordinates to the tiling scheme's
          // rectangle, because the viewport service likes to tell us about
          // rectangles that go all the way to the poles when that is not
          // possible in Web Mercator.
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

          // Clamp the projected position, too. A position right on the boundary
          // can end up on the wrong side when projected.
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

          // Availability of this level implies availability of all prior
          // levels, too.
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

        // If the response has fewer than 100 maxZoomRects, we know that this is
        // _all_ of them. If it contains 100, there may be more we don't know
        // about yet because Google truncated the list.
        if (maxZoomRects.Size() < 100) {
          this->_availableAvailability.addAvailableTileRange(
              rectangleForAvailability(
                  this->getTilingScheme(),
                  this->getMaximumLevel(),
                  tileRectangle));
        }
      });
}

} // namespace
