#include <CesiumAsync/CesiumIonAssetAccessor.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumRasterOverlays/BingMapsRasterOverlay.h>
#include <CesiumRasterOverlays/IonRasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/TileMapServiceRasterOverlay.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/JsonHelpers.h>

#include <fmt/format.h>
#include <nonstd/expected.hpp>
#include <rapidjson/document.h>
#include <spdlog/logger.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumUtility;

namespace CesiumRasterOverlays {

IonRasterOverlay::IonRasterOverlay(
    const std::string& name,
    int64_t ionAssetID,
    const std::string& ionAccessToken,
    const RasterOverlayOptions& overlayOptions,
    const std::string& ionAssetEndpointUrl)
    : IonRasterOverlay(
          name,
          fmt::format(
              "{}v1/assets/{}/endpoint?access_token={}",
              ionAssetEndpointUrl,
              ionAssetID,
              ionAccessToken),
          ionAccessToken,
          false,
          overlayOptions) {}

IonRasterOverlay::IonRasterOverlay(
    const std::string& name,
    const std::string& overlayUrl,
    const std::string& ionAccessToken,
    bool needsAuthHeader,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _overlayUrl(overlayUrl),
      _ionAccessToken(ionAccessToken),
      _needsAuthHeader(needsAuthHeader) {}

IonRasterOverlay::~IonRasterOverlay() = default;

std::unordered_map<std::string, IonRasterOverlay::ExternalAssetEndpoint>
    IonRasterOverlay::endpointCache;

namespace {

class IonRasterOverlayTileProvider : public RasterOverlayTileProvider {
public:
  IonRasterOverlayTileProvider(
      const IntrusivePointer<RasterOverlayTileProvider>& pAggregate)
      : RasterOverlayTileProvider(
            &pAggregate->getOwner(),
            pAggregate->getAsyncSystem(),
            pAggregate->getAssetAccessor(),
            pAggregate->getCreditSystem(),
            pAggregate->getCredit(),
            pAggregate->getPrepareRendererResources(),
            pAggregate->getLogger(),
            pAggregate->getProjection(),
            pAggregate->getCoverageRectangle()),
        _pAggregate(pAggregate) {}

  void updateToken(const std::string& /* token */) {}

protected:
  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(RasterOverlayTile& /* overlayTile */) override {
    throw 1;
  }

private:
  IntrusivePointer<RasterOverlayTileProvider> _pAggregate;
};

} // namespace

Future<RasterOverlay::CreateTileProviderResult>
IonRasterOverlay::createTileProvider(
    const ExternalAssetEndpoint& endpoint,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner) const {
  IntrusivePointer<RasterOverlay> pOverlay = nullptr;
  bool isBing;
  if (endpoint.externalType == "BING") {
    isBing = true;
    pOverlay = new BingMapsRasterOverlay(
        this->getName(),
        endpoint.url,
        endpoint.key,
        endpoint.mapStyle,
        endpoint.culture,
        this->getOptions());
  } else {
    isBing = false;
    pOverlay = new TileMapServiceRasterOverlay(
        this->getName(),
        endpoint.url,
        std::vector<CesiumAsync::IAssetAccessor::THeader>{
            std::make_pair("Authorization", "Bearer " + endpoint.accessToken)},
        TileMapServiceRasterOverlayOptions(),
        this->getOptions());
  }

  if (pCreditSystem) {
    std::vector<Credit>& credits = pOverlay->getCredits();
    for (const auto& attribution : endpoint.attributions) {
      credits.emplace_back(pCreditSystem->createCredit(
          attribution.html,
          !attribution.collapsible || this->getOptions().showCreditsOnScreen));
    }
  }

  struct ProviderHolder {
    IntrusivePointer<RasterOverlayTileProvider> pProvider = nullptr;
  };

  std::shared_ptr<ProviderHolder> pHolder = std::make_shared<ProviderHolder>();

  std::shared_ptr<CesiumIonAssetAccessor> pIonAccessor =
      std::make_shared<CesiumIonAssetAccessor>(
          pLogger,
          pAssetAccessor,
          this->_overlayUrl,
          // TODO: only add header if _needsAuthHeader.
          std::vector<IAssetAccessor::THeader>{
              {"Authorization",
               fmt::format("Bearer {}", this->_ionAccessToken)}},
          [asyncSystem, url = this->_overlayUrl, pHolder, isBing, pOverlay](
              const CesiumIonAssetAccessor::UpdatedToken& update) {
            // update cache with new access token
            auto cacheIt = endpointCache.find(url);
            if (cacheIt != endpointCache.end()) {
              cacheIt->second.accessToken = update.token;
            }

            if (pHolder->pProvider) {
              if (isBing) {
                // Use static_cast instead of dynamic_cast here to avoid the
                // need for RTTI, and because we are certain of the type.
                // NOLINTBEGIN(cppcoreguidelines-pro-type-static-cast-downcast)
                BingMapsRasterOverlay* pBing =
                    static_cast<BingMapsRasterOverlay*>(pOverlay.get());
                // NOLINTEND(cppcoreguidelines-pro-type-static-cast-downcast)
                return pBing->refreshTileProviderWithNewKey(
                    pHolder->pProvider,
                    update.token);
              }
            }

            return asyncSystem.createResolvedFuture();
          });

  return pOverlay
      ->createTileProvider(
          asyncSystem,
          pIonAccessor,
          pCreditSystem,
          pPrepareRendererResources,
          pLogger,
          std::move(pOwner))
      .thenImmediately(
          [pHolder](
              CreateTileProviderResult&& result) -> CreateTileProviderResult {
            if (result) {
              pHolder->pProvider = *result;
            }
            return result;
          });
}

Future<RasterOverlay::CreateTileProviderResult>
IonRasterOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner) const {
  pOwner = pOwner ? pOwner : this;

  auto cacheIt = IonRasterOverlay::endpointCache.find(this->_overlayUrl);
  if (cacheIt != IonRasterOverlay::endpointCache.end()) {
    IntrusivePointer<const IonRasterOverlay> pThis = this;
    return createTileProvider(
               cacheIt->second,
               asyncSystem,
               pAssetAccessor,
               pCreditSystem,
               pPrepareRendererResources,
               pLogger,
               pOwner)
        .thenInMainThread(
            [pThis,
             asyncSystem,
             pAssetAccessor,
             pCreditSystem,
             pPrepareRendererResources,
             pLogger,
             pOwner](RasterOverlay::CreateTileProviderResult&& result) {
              if (!result) {
                const RasterOverlayLoadFailureDetails& error = result.error();
                if (error.pRequest && error.pRequest->response() &&
                    error.pRequest->response()->statusCode() == 401) {
                  // Cached endpoint response is no good, so remove it and
                  // retry.
                  endpointCache.erase(pThis->_overlayUrl);
                  return pThis->createTileProvider(
                      asyncSystem,
                      pAssetAccessor,
                      pCreditSystem,
                      pPrepareRendererResources,
                      pLogger,
                      pOwner);
                }
              }

              return asyncSystem.createResolvedFuture(std::move(result));
            });
  }

  std::vector<IAssetAccessor::THeader> headers;

  if (this->_needsAuthHeader) {
    headers.emplace_back(
        "Authorization",
        fmt::format("Bearer {}", this->_ionAccessToken));
  }

  return pAssetAccessor->get(asyncSystem, this->_overlayUrl, headers)
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
                      "Error while parsing Cesium ion raster overlay "
                      "response, "
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
                    fmt::format("Cesium ion Bing Maps raster overlay metadata "
                                "response "
                                "does not contain 'options' or it is not an "
                                "object.")});
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
           this,
           pLogger](nonstd::expected<
                    ExternalAssetEndpoint,
                    RasterOverlayLoadFailureDetails>&& result)
              -> Future<CreateTileProviderResult> {
            if (result) {
              IonRasterOverlay::endpointCache[this->_overlayUrl] = *result;
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
} // namespace CesiumRasterOverlays
