#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumJsonReader/JsonObjectJsonHandler.h>
#include <CesiumJsonReader/JsonReader.h>
#include <CesiumJsonWriter/JsonObjectWriter.h>
#include <CesiumJsonWriter/PrettyJsonWriter.h>
#include <CesiumRasterOverlays/GoogleMapTilesRasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/Uri.h>

#include <memory>

using namespace CesiumAsync;
using namespace CesiumJsonReader;
using namespace CesiumJsonWriter;
using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace {

class GoogleMapTilesRasterOverlayTileProvider
    : public RasterOverlayTileProvider {};

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
    const std::shared_ptr<CreditSystem>& /* pCreditSystem */,
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
    /* pPrepareRendererResources */,
    const std::shared_ptr<spdlog::logger>& /* pLogger */,
    IntrusivePointer<const RasterOverlay> /* pOwner */) const {
  Uri createSessionUri(this->_googleOptions.apiBaseUrl, "createSession", true);

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
      .thenImmediately(
          [](std::shared_ptr<IAssetRequest>&& pRequest)
              -> CreateTileProviderResult {
            const IAssetResponse* pResponse = pRequest->response();
            if (!pResponse) {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  .type = RasterOverlayLoadType::TileProvider,
                  .pRequest = pRequest,
                  .message = "No response received from Google Map Tiles API "
                             "createSession service."});
            }

            if (pResponse->statusCode() < 200 ||
                pResponse->statusCode() >= 300) {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  .type = RasterOverlayLoadType::TileProvider,
                  .pRequest = pRequest,
                  .message = fmt::format(
                      "Google Map Tiles API createSession service returned "
                      "HTTP status code {}.",
                      pResponse->statusCode())});
            }

            JsonObjectJsonHandler handler{};
            ReadJsonResult<JsonValue> response =
                JsonReader::readJson(pResponse->data(), handler);

            if (!response.value) {
              ErrorList errorList{};
              errorList.errors = std::move(response.errors);
              errorList.warnings = std::move(response.warnings);

              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  .type = RasterOverlayLoadType::TileProvider,
                  .pRequest = pRequest,
                  .message = errorList.format(
                      "Failed to parse response from Google Map Tiles API "
                      "createSession service:")});
            }
          });
}

} // namespace CesiumRasterOverlays