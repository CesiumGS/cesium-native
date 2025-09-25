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
#include <CesiumUtility/Uri.h>

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

protected:
  virtual CesiumAsync::Future<LoadedRasterOverlayImage> loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override;

private:
  std::string _apiBaseUrl;
  std::string _session;
  std::string _key;
  QuadtreeRectangleAvailability _availableTiles;
  QuadtreeRectangleAvailability _availableAvailability;
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

            return asyncSystem.createResolvedFuture<CreateTileProviderResult>(
                IntrusivePointer(new GoogleMapTilesRasterOverlayTileProvider(
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
                    static_cast<uint32_t>(tileHeight))));
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
    // return UrlTemplateRasterOverlay::loadTileImageFromUrl(
    //     this,
    //     fmt::format(
    //         "https://mt.google.com/vt/lyrs=m&x={}&y={}&z={}",
    //         tileID.x,
    //         tileID.y,
    //         tileID.level));
  }

  // 2. If we have complete availability information here, that means the tile
  // is definitely _not_ available (or we would have returned at (1) above).
  if (this->_availableAvailability.isTileAvailable(tileID)) {
    // return this->createResolvedFuture<LoadedRasterOverlayImage>(
    //     LoadedRasterOverlayImage{});
  }

  // 3. We can't tell if this tile is available. Check with the `viewport`
  // service.

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

} // namespace
