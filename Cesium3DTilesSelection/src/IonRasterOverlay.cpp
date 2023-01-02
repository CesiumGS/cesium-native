#include "Cesium3DTilesSelection/IonRasterOverlay.h"

#include "Cesium3DTilesSelection/BingMapsRasterOverlay.h"
#include "Cesium3DTilesSelection/RasterOverlayLoadFailureDetails.h"
#include "Cesium3DTilesSelection/RasterOverlayTile.h"
#include "Cesium3DTilesSelection/RasterOverlayTileProvider.h"
#include "Cesium3DTilesSelection/TileMapServiceRasterOverlay.h"
#include "Cesium3DTilesSelection/TilesetExternals.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Uri.h>

#include <rapidjson/document.h>

using namespace CesiumAsync;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

IonRasterOverlay::IonRasterOverlay(
    const std::string& name,
    int64_t ionAssetID,
    const std::string& ionAccessToken,
    const RasterOverlayOptions& overlayOptions,
    const std::string& ionAssetEndpointUrl)
    : RasterOverlay(name, overlayOptions),
      _ionAssetID(ionAssetID),
      _ionAccessToken(ionAccessToken),
      _ionAssetEndpointUrl(ionAssetEndpointUrl) {}

IonRasterOverlay::~IonRasterOverlay() {}

std::unordered_map<std::string, IonRasterOverlay::ExternalAssetEndpoint>
    IonRasterOverlay::endpointCache;

Future<RasterOverlay::CreateTileProviderResult>
IonRasterOverlay::createTileProvider(
    const ExternalAssetEndpoint& endpoint,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner) const {
  IntrusivePointer<RasterOverlay> pOverlay = nullptr;
  if (endpoint.externalType == "BING") {
    pOverlay = new BingMapsRasterOverlay(
        this->getName(),
        endpoint.url,
        endpoint.key,
        endpoint.mapStyle,
        endpoint.culture);
  } else {
    pOverlay = new TileMapServiceRasterOverlay(
        this->getName(),
        endpoint.url,
        std::vector<CesiumAsync::IAssetAccessor::THeader>{
            std::make_pair("Authorization", "Bearer " + endpoint.accessToken)});
  }

  if (pCreditSystem) {
    std::vector<Credit>& credits = pOverlay->getCredits();
    for (const auto& attribution : endpoint.attributions) {
      credits.emplace_back(pCreditSystem->createCredit(
          attribution.html,
          !attribution.collapsible || this->getOptions().showCreditsOnScreen));
    }
  }

  return pOverlay->createTileProvider(
      asyncSystem,
      pAssetAccessor,
      pCreditSystem,
      pPrepareRendererResources,
      pLogger,
      pOwner);
}

Future<RasterOverlay::CreateTileProviderResult>
IonRasterOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner) const {
  std::string ionUrl = this->_ionAssetEndpointUrl + "v1/assets/" +
                       std::to_string(this->_ionAssetID) + "/endpoint";
  ionUrl = CesiumUtility::Uri::addQuery(
      ionUrl,
      "access_token",
      this->_ionAccessToken);

  pOwner = pOwner ? pOwner : this;

  auto cacheIt = IonRasterOverlay::endpointCache.find(ionUrl);
  if (cacheIt != IonRasterOverlay::endpointCache.end()) {
    return createTileProvider(
        cacheIt->second,
        asyncSystem,
        pAssetAccessor,
        pCreditSystem,
        pPrepareRendererResources,
        pLogger,
        pOwner);
  }

  return pAssetAccessor->get(asyncSystem, ionUrl)
      .thenImmediately(
          [](std::shared_ptr<IAssetRequest>&& pRequest)
              -> nonstd::expected<
                  ExternalAssetEndpoint,
                  RasterOverlayLoadFailureDetails> {
            const IAssetResponse* pResponse = pRequest->response();

            rapidjson::Document response;
            response.Parse(
                reinterpret_cast<const char*>(pResponse->data().data()),
                pResponse->data().size());

            if (response.HasParseError()) {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  RasterOverlayLoadType::CesiumIon,
                  std::move(pRequest),
                  fmt::format(
                      "Error while parsing Cesium ion raster overlay response, "
                      "error code {} at byte offset {}",
                      response.GetParseError(),
                      response.GetErrorOffset())});
            }

            std::string type =
                JsonHelpers::getStringOrDefault(response, "type", "unknown");
            if (type != "IMAGERY") {
              return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                  RasterOverlayLoadType::CesiumIon,
                  std::move(pRequest),
                  fmt::format(
                      "Assets used with a raster overlay must have type "
                      "'IMAGERY', but instead saw '{}'.",
                      type)});
            }

            ExternalAssetEndpoint endpoint;
            endpoint.externalType = JsonHelpers::getStringOrDefault(
                response,
                "externalType",
                "unknown");
            if (endpoint.externalType == "BING") {
              const auto optionsIt = response.FindMember("options");
              if (optionsIt == response.MemberEnd() ||
                  !optionsIt->value.IsObject()) {
                return nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                    RasterOverlayLoadType::CesiumIon,
                    std::move(pRequest),
                    fmt::format(
                        "Cesium ion Bing Maps raster overlay metadata response "
                        "does not contain 'options' or it is not an object.")});
              }

              const auto attributionsIt = response.FindMember("attributions");
              if (attributionsIt != response.MemberEnd() &&
                  attributionsIt->value.IsArray()) {

                for (const rapidjson::Value& attribution :
                     attributionsIt->value.GetArray()) {
                  AssetEndpointAttribution& endpointAttribution =
                      endpoint.attributions.emplace_back();
                  const auto html = attribution.FindMember("html");
                  if (html != attribution.MemberEnd() &&
                      html->value.IsString()) {
                    endpointAttribution.html = html->value.GetString();
                  }
                  auto collapsible = attribution.FindMember("collapsible");
                  if (collapsible != attribution.MemberEnd() &&
                      collapsible->value.IsBool()) {
                    endpointAttribution.collapsible =
                        collapsible->value.GetBool();
                  }
                }
              }

              const auto& options = optionsIt->value;
              endpoint.url =
                  JsonHelpers::getStringOrDefault(options, "url", "");
              endpoint.key =
                  JsonHelpers::getStringOrDefault(options, "key", "");
              endpoint.mapStyle = JsonHelpers::getStringOrDefault(
                  options,
                  "mapStyle",
                  "AERIAL");
              endpoint.culture =
                  JsonHelpers::getStringOrDefault(options, "culture", "");
            } else {
              endpoint.url =
                  JsonHelpers::getStringOrDefault(response, "url", "");
              endpoint.accessToken =
                  JsonHelpers::getStringOrDefault(response, "accessToken", "");
            }

            return endpoint;
          })
      .thenInMainThread(
          [asyncSystem,
           pOwner,
           pAssetAccessor,
           pCreditSystem,
           pPrepareRendererResources,
           ionUrl,
           this,
           pLogger](nonstd::expected<
                    ExternalAssetEndpoint,
                    RasterOverlayLoadFailureDetails>&& result)
              -> Future<CreateTileProviderResult> {
            if (result) {
              IonRasterOverlay::endpointCache[ionUrl] = *result;
              return this->createTileProvider(
                  *result,
                  asyncSystem,
                  pAssetAccessor,
                  pCreditSystem,
                  pPrepareRendererResources,
                  pLogger,
                  pOwner);
            } else {
              return asyncSystem.createResolvedFuture<CreateTileProviderResult>(
                  nonstd::make_unexpected(std::move(result).error()));
            }
          });
}
} // namespace Cesium3DTilesSelection
