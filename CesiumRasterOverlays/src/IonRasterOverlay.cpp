#include <CesiumAsync/CesiumIonAssetAccessor.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/NetworkAssetDescriptor.h>
#include <CesiumAsync/SharedAssetDepot.h>
#include <CesiumRasterOverlays/BingMapsRasterOverlay.h>
#include <CesiumRasterOverlays/IonRasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/TileMapServiceRasterOverlay.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/DerivedValue.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Result.h>

#include <fmt/format.h>
#include <nonstd/expected.hpp>
#include <rapidjson/document.h>
#include <spdlog/logger.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumUtility;

namespace CesiumRasterOverlays {

namespace {

// How long to cache the asset endpoint response. After this duration, the
// `/assets/#/endpoint` service will be re-requested when it is next required.
const std::chrono::steady_clock::duration AssetEndpointRerequestInterval =
    std::chrono::minutes(55);

} // namespace

class IonRasterOverlay::TileProvider : public RasterOverlayTileProvider {
public:
  struct CreateTileProvider {
    IntrusivePointer<const RasterOverlay> pOwner;
    RasterOverlayExternals externals;

    SharedFuture<RasterOverlay::CreateTileProviderResult>
    operator()(const IntrusivePointer<ExternalAssetEndpoint>& pEndpoint);
  };

  using TileProviderFactoryType =
      DerivedValue<IntrusivePointer<ExternalAssetEndpoint>, CreateTileProvider>;

  TileProvider(
      const IntrusivePointer<RasterOverlayTileProvider>& pInitialProvider,
      const NetworkAssetDescriptor& descriptor,
      TileProviderFactoryType&& tileProviderFactory)
      : RasterOverlayTileProvider(
            &pInitialProvider->getOwner(),
            pInitialProvider->getExternals(),
            pInitialProvider->getCredit(),
            pInitialProvider->getProjection(),
            pInitialProvider->getCoverageRectangle()),
        _descriptor(descriptor),
        _factory(std::move(tileProviderFactory)){};

  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(const RasterOverlayTile& overlayTile) override {
    IntrusivePointer<TileProvider> thiz(this);

    return IonRasterOverlay::getEndpointCache()
        ->getOrCreate(
            this->getAsyncSystem(),
            this->getAssetAccessor(),
            this->_descriptor)
        .thenInMainThread(
            [thiz](const ResultPointer<ExternalAssetEndpoint>& assetResult)
                -> Future<IntrusivePointer<ExternalAssetEndpoint>> {
              if (!assetResult.pValue ||
                  std::chrono::steady_clock::now() >
                      assetResult.pValue->requestTime +
                          AssetEndpointRerequestInterval) {
                // Invalidate by asset pointer instead of key. This way, if
                // another `loadTileImage` has already invalidated this asset
                // and started loading a new one, we don't inadvertently
                // invalidate the new one.
                IonRasterOverlay::getEndpointCache()->invalidate(
                    *assetResult.pValue);

                return IonRasterOverlay::getEndpointCache()
                    ->getOrCreate(
                        thiz->getAsyncSystem(),
                        thiz->getAssetAccessor(),
                        thiz->_descriptor)
                    .thenImmediately(
                        [](const ResultPointer<ExternalAssetEndpoint>&
                               assetResult) {
                          // This thenImmediately turns the SharedFuture into a
                          // Future. That's better than turning the Future into
                          // a SharedFuture in the hot path else block below.
                          return assetResult.pValue;
                        });
              } else {
                return thiz->getAsyncSystem().createResolvedFuture(
                    IntrusivePointer<ExternalAssetEndpoint>(
                        assetResult.pValue));
              }
            })
        .thenInMainThread(
            [thiz](ResultPointer<ExternalAssetEndpoint>&& assetResult) {
              return thiz->_factory(assetResult.pValue);
            })
        .thenInMainThread(
            [thiz, &overlayTile](
                const RasterOverlay::CreateTileProviderResult& result) {
              if (result) {
                return result.value()->loadTileImage(overlayTile);
              } else {
                return thiz->getAsyncSystem()
                    .createResolvedFuture<LoadedRasterOverlayImage>(
                        LoadedRasterOverlayImage{});
              }
            });
  }

private:
  NetworkAssetDescriptor _descriptor;
  TileProviderFactoryType _factory;
};

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

  NetworkAssetDescriptor descriptor;
  descriptor.url = this->_overlayUrl;

  if (this->_needsAuthHeader) {
    descriptor.headers.emplace_back(
        "Authorization",
        fmt::format("Bearer {}", this->_ionAccessToken));
  }

  RasterOverlayExternals externals{
      .pAssetAccessor = pAssetAccessor,
      .pPrepareRendererResources = pPrepareRendererResources,
      .asyncSystem = asyncSystem,
      .pCreditSystem = pCreditSystem,
      .pLogger = pLogger,
  };

  return getEndpointCache()
      ->getOrCreate(asyncSystem, pAssetAccessor, descriptor)
      .thenInMainThread(
          [asyncSystem, pOwner, externals = std::move(externals), this](
              const Result<IntrusivePointer<ExternalAssetEndpoint>>&
                  result) mutable {
            TileProvider::TileProviderFactoryType factory(
                TileProvider::CreateTileProvider{
                    .pOwner = pOwner,
                    .externals = std::move(externals)});
            if (result.pValue) {
              return factory(result.pValue).thenPassThrough(std::move(factory));
            } else {
              return asyncSystem
                  .createResolvedFuture<CreateTileProviderResult>(
                      nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                          .type = RasterOverlayLoadType::CesiumIon,
                          .pRequest = nullptr,
                          .message = result.errors.format(
                              "Could not access Cesium ion asset"),
                      }))
                  .thenPassThrough(std::move(factory));
            }
          })
      .thenInMainThread([descriptor](auto&& tuple) -> CreateTileProviderResult {
        auto& [factory, result] = tuple;
        if (result) {
          return IntrusivePointer<RasterOverlayTileProvider>(
              new TileProvider(result.value(), descriptor, std::move(factory)));
        } else {
          return std::move(result);
        }
      });
  ;
}

/* static */ CesiumUtility::IntrusivePointer<IonRasterOverlay::EndpointDepot>
IonRasterOverlay::getEndpointCache() {
  static CesiumUtility::IntrusivePointer<EndpointDepot> pDepot =
      new EndpointDepot(
          [](const AsyncSystem& asyncSystem,
             const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
             const NetworkAssetDescriptor& key)
              -> CesiumAsync::Future<
                  CesiumUtility::ResultPointer<ExternalAssetEndpoint>> {
            std::chrono::steady_clock::time_point requestTime =
                std::chrono::steady_clock::now();

            return key.loadBytesFromNetwork(asyncSystem, pAssetAccessor)
                .thenImmediately([requestTime](
                                     Result<std::vector<std::byte>>&& result) {
                  if (!result.value) {
                    return ResultPointer<ExternalAssetEndpoint>(result.errors);
                  }

                  rapidjson::Document response;
                  response.Parse(
                      reinterpret_cast<const char*>(result.value->data()),
                      result.value->size());

                  if (response.HasParseError()) {
                    return ResultPointer<ExternalAssetEndpoint>(
                        ErrorList::error(fmt::format(
                            "Error while parsing Cesium ion raster overlay "
                            "response, error code {} at byte offset {}.",
                            response.GetParseError(),
                            response.GetErrorOffset())));
                  }

                  std::string type = JsonHelpers::getStringOrDefault(
                      response,
                      "type",
                      "unknown");
                  if (type != "IMAGERY") {
                    return ResultPointer<ExternalAssetEndpoint>(
                        ErrorList::error(fmt::format(
                            "Assets used with a raster overlay must have type "
                            "'IMAGERY', but instead saw '{}'.",
                            type)));
                  }

                  ExternalAssetEndpoint endpoint;
                  endpoint.requestTime = requestTime;
                  endpoint.externalType = JsonHelpers::getStringOrDefault(
                      response,
                      "externalType",
                      "unknown");
                  if (endpoint.externalType == "BING") {
                    const auto optionsIt = response.FindMember("options");
                    if (optionsIt == response.MemberEnd() ||
                        !optionsIt->value.IsObject()) {
                      return ResultPointer<ExternalAssetEndpoint>(
                          ErrorList::error(fmt::format(
                              "Cesium ion Bing Maps raster overlay metadata "
                              "response does not contain 'options' or it is "
                              "not an object.")));
                    }

                    const auto attributionsIt =
                        response.FindMember("attributions");
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
                        auto collapsible =
                            attribution.FindMember("collapsible");
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
                    endpoint.accessToken = JsonHelpers::getStringOrDefault(
                        response,
                        "accessToken",
                        "");
                  }

                  return ResultPointer<ExternalAssetEndpoint>(
                      new ExternalAssetEndpoint(std::move(endpoint)));
                });
          });
  return pDepot;
}

SharedFuture<RasterOverlay::CreateTileProviderResult>
IonRasterOverlay::TileProvider::CreateTileProvider::operator()(
    const IntrusivePointer<ExternalAssetEndpoint>& pEndpoint) {
  IntrusivePointer<RasterOverlay> pOverlay = nullptr;
  bool isBing;
  if (pEndpoint->externalType == "BING") {
    isBing = true;
    pOverlay = new BingMapsRasterOverlay(
        this->pOwner->getName(),
        pEndpoint->url,
        pEndpoint->key,
        pEndpoint->mapStyle,
        pEndpoint->culture,
        this->pOwner->getOptions());
  } else {
    isBing = false;
    pOverlay = new TileMapServiceRasterOverlay(
        this->pOwner->getName(),
        pEndpoint->url,
        std::vector<CesiumAsync::IAssetAccessor::THeader>{std::make_pair(
            "Authorization",
            "Bearer " + pEndpoint->accessToken)},
        TileMapServiceRasterOverlayOptions(),
        this->pOwner->getOptions());
  }

  if (this->externals.pCreditSystem) {
    std::vector<Credit>& credits = pOverlay->getCredits();
    for (const auto& attribution : pEndpoint->attributions) {
      credits.emplace_back(this->externals.pCreditSystem->createCredit(
          attribution.html,
          !attribution.collapsible ||
              this->pOwner->getOptions().showCreditsOnScreen));
    }
  }

  return pOverlay
      ->createTileProvider(
          this->externals.asyncSystem,
          this->externals.pAssetAccessor,
          this->externals.pCreditSystem,
          this->externals.pPrepareRendererResources,
          this->externals.pLogger,
          this->pOwner)
      .share();
}

} // namespace CesiumRasterOverlays
