#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumAsync/NetworkAssetDescriptor.h>
#include <CesiumAsync/SharedAssetDepot.h>
#include <CesiumRasterOverlays/BingMapsRasterOverlay.h>
#include <CesiumRasterOverlays/GoogleMapTilesRasterOverlay.h>
#include <CesiumRasterOverlays/IonRasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/TileMapServiceRasterOverlay.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/CreditReferencer.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/DerivedValue.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Result.h>
#include <CesiumUtility/Uri.h>

#include <fmt/format.h>
#include <nonstd/expected.hpp>
#include <rapidjson/document.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>
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
  struct AggregatedTileProviderSuccess {
    CesiumUtility::IntrusivePointer<RasterOverlayTileProvider> pAggregated;
    std::vector<Credit> creditsFromIon;
  };

  using AggregatedTileProviderResult = nonstd::
      expected<AggregatedTileProviderSuccess, RasterOverlayLoadFailureDetails>;

  struct CreateTileProvider {
    IntrusivePointer<const RasterOverlay> pOwner;
    RasterOverlayExternals externals;

    SharedFuture<AggregatedTileProviderResult>
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
            pInitialProvider->getProjection(),
            pInitialProvider->getCoverageRectangle()),
        _descriptor(descriptor),
        _factory(std::move(tileProviderFactory)),
        _credits() {}

  static CesiumAsync::Future<RasterOverlay::CreateTileProviderResult> create(
      const RasterOverlayExternals& externals,
      const NetworkAssetDescriptor& descriptor,
      const IntrusivePointer<const RasterOverlay>& pOwner) {
    auto pFactory = std::make_unique<TileProvider::TileProviderFactoryType>(
        TileProvider::TileProviderFactoryType(TileProvider::CreateTileProvider{
            .pOwner = pOwner,
            .externals = externals}));
    return TileProvider::getTileProvider(externals, descriptor, *pFactory)
        .thenInMainThread(
            [descriptor, pFactory = std::move(pFactory)](
                AggregatedTileProviderResult&& result) mutable
            -> CreateTileProviderResult {
              if (result) {
                IntrusivePointer p = new TileProvider(
                    result.value().pAggregated,
                    descriptor,
                    std::move(*pFactory));
                p->_credits = std::move(result.value().creditsFromIon);
                return IntrusivePointer<RasterOverlayTileProvider>(p);
              } else {
                return nonstd::make_unexpected(result.error());
              }
            });
  }

  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(const RasterOverlayTile& overlayTile) override {
    // This method may only be called from the main thread. Make sure the
    // AsyncSystem knows that.
    auto mainThreadScope = this->getAsyncSystem().enterMainThread();

    IntrusivePointer<TileProvider> thiz(this);
    return TileProvider::getTileProvider(
               this->getExternals(),
               this->_descriptor,
               this->_factory)
        .thenInMainThread(
            [thiz, &overlayTile](const AggregatedTileProviderResult& result) {
              if (result) {
                return result.value().pAggregated->loadTileImage(overlayTile);
              } else {
                return thiz->getAsyncSystem()
                    .createResolvedFuture<LoadedRasterOverlayImage>(
                        LoadedRasterOverlayImage{});
              }
            });
  }

  virtual void
  addCredits(CreditReferencer& creditReferencer) noexcept override {
    RasterOverlayTileProvider::addCredits(creditReferencer);

    // Report the credits we got from ion.
    for (const Credit& credit : this->_credits) {
      creditReferencer.addCreditReference(credit);
    }

    // This method may only be called from the main thread. Make sure the
    // AsyncSystem knows that.
    auto mainThreadScope = this->getAsyncSystem().enterMainThread();

    // If the aggregated provider is ready, report its credits, too.
    Future<AggregatedTileProviderResult> future = TileProvider::getTileProvider(
        this->getExternals(),
        this->_descriptor,
        this->_factory);
    if (future.isReady()) {
      // This is guaranteed not to block, because we just checked `isReady()`.
      const AggregatedTileProviderResult& result = future.wait();
      if (result) {
        result.value().pAggregated->addCredits(creditReferencer);
      }
    }
  }

private:
  static Future<AggregatedTileProviderResult> getTileProvider(
      const RasterOverlayExternals& externals,
      const NetworkAssetDescriptor& descriptor,
      TileProviderFactoryType& factory) {
    return IonRasterOverlay::getEndpointCache()
        ->getOrCreate(externals, descriptor)
        .thenInMainThread(
            [externals, descriptor](
                const ResultPointer<ExternalAssetEndpoint>& assetResult) mutable
            -> Future<IntrusivePointer<ExternalAssetEndpoint>> {
              if (!assetResult.pValue ||
                  std::chrono::steady_clock::now() >
                      assetResult.pValue->requestTime +
                          AssetEndpointRerequestInterval) {
                // Invalidate by asset pointer instead of key. This way, if
                // another `loadTileImage` has already invalidated this asset
                // and started loading a new one, we don't inadvertently
                // invalidate the new one. But if we don't have a valid asset,
                // we must invalidate by key.
                if ((assetResult.pValue &&
                     IonRasterOverlay::getEndpointCache()->invalidate(
                         *assetResult.pValue)) ||
                    (!assetResult.pValue &&
                     IonRasterOverlay::getEndpointCache()->invalidate(
                         descriptor))) {
                  SPDLOG_LOGGER_INFO(
                      externals.pLogger,
                      "Refreshing Cesium ion token for URL {}.",
                      descriptor.url);
                }

                return IonRasterOverlay::getEndpointCache()
                    ->getOrCreate(externals, descriptor)
                    .thenImmediately(
                        [pLogger = externals.pLogger, url = descriptor.url](
                            const ResultPointer<ExternalAssetEndpoint>&
                                assetResult) {
                          // This thenImmediately turns the SharedFuture into a
                          // Future. That's better than turning the Future into
                          // a SharedFuture in the hot path else block below.
                          return assetResult.pValue;
                        });
              } else {
                return externals.asyncSystem.createResolvedFuture(
                    IntrusivePointer<ExternalAssetEndpoint>(
                        assetResult.pValue));
              }
            })
        .thenInMainThread(
            [&factory, asyncSystem = externals.asyncSystem](
                const ResultPointer<ExternalAssetEndpoint>& result) mutable {
              if (result.pValue) {
                return factory(result.pValue);
              } else {
                return asyncSystem
                    .createResolvedFuture<AggregatedTileProviderResult>(
                        nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                            .type = RasterOverlayLoadType::CesiumIon,
                            .pRequest = nullptr,
                            .message = result.errors.format(
                                "Could not access Cesium ion asset"),
                        }))
                    .share();
              }
            });
  }

  NetworkAssetDescriptor _descriptor;
  TileProviderFactoryType _factory;
  std::vector<Credit> _credits;
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
      _needsAuthHeader(needsAuthHeader),
      _assetOptions() {}

IonRasterOverlay::~IonRasterOverlay() = default;

const std::optional<std::string>&
IonRasterOverlay::getAssetOptions() const noexcept {
  return this->_assetOptions;
}

void IonRasterOverlay::setAssetOptions(
    const std::optional<std::string>& options) noexcept {
  this->_assetOptions = options;
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

  NetworkAssetDescriptor descriptor;
  descriptor.url = this->_overlayUrl;

  if (this->_needsAuthHeader) {
    descriptor.headers.emplace_back(
        "Authorization",
        fmt::format("Bearer {}", this->_ionAccessToken));
  }

  if (this->_assetOptions) {
    Uri uri(descriptor.url);
    UriQuery query = uri.getQuery();
    query.setValue("options", *this->_assetOptions);
    uri.setQuery(query.toQueryString());
    descriptor.url = uri.toString();
  }

  RasterOverlayExternals externals{
      .pAssetAccessor = pAssetAccessor,
      .pPrepareRendererResources = pPrepareRendererResources,
      .asyncSystem = asyncSystem,
      .pCreditSystem = pCreditSystem,
      .pLogger = pLogger,
  };

  return TileProvider::create(externals, descriptor, pOwner);
}

/* static */ CesiumUtility::IntrusivePointer<IonRasterOverlay::EndpointDepot>
IonRasterOverlay::getEndpointCache() {
  static CesiumUtility::IntrusivePointer<EndpointDepot> pDepot =
      new EndpointDepot(
          [](const RasterOverlayExternals& context,
             const NetworkAssetDescriptor& key)
              -> CesiumAsync::Future<
                  CesiumUtility::ResultPointer<ExternalAssetEndpoint>> {
            std::chrono::steady_clock::time_point requestTime =
                std::chrono::steady_clock::now();

            return key
                .loadFromNetwork(context.asyncSystem, context.pAssetAccessor)
                .thenImmediately([requestTime](std::shared_ptr<IAssetRequest>&&
                                                   pRequest) {
                  ExternalAssetEndpoint endpoint{};

                  const CesiumAsync::IAssetResponse* pResponse =
                      pRequest->response();
                  if (!pResponse) {
                    endpoint.pRequestThatFailed = pRequest;
                    return ResultPointer<ExternalAssetEndpoint>(
                        new ExternalAssetEndpoint(std::move(endpoint)),
                        ErrorList::error(fmt::format(
                            "Request for {} failed.",
                            pRequest->url())));
                  }

                  uint16_t statusCode = pResponse->statusCode();
                  if (statusCode != 0 &&
                      (statusCode < 200 || statusCode >= 300)) {
                    endpoint.pRequestThatFailed = pRequest;
                    return ResultPointer<ExternalAssetEndpoint>(
                        new ExternalAssetEndpoint(std::move(endpoint)),
                        ErrorList::error(fmt::format(
                            "Request for {} failed with code {}",
                            pRequest->url(),
                            pResponse->statusCode())));
                  }

                  std::span<const std::byte> data = pResponse->data();

                  rapidjson::Document response;
                  response.Parse(
                      reinterpret_cast<const char*>(data.data()),
                      data.size());

                  if (response.HasParseError()) {
                    endpoint.pRequestThatFailed = std::move(pRequest);
                    return ResultPointer<ExternalAssetEndpoint>(
                        new ExternalAssetEndpoint(std::move(endpoint)),
                        ErrorList::error(fmt::format(
                            "Error while parsing Cesium ion raster overlay "
                            "response: error code {} at byte offset {}.",
                            response.GetParseError(),
                            response.GetErrorOffset())));
                  }

                  std::string type = JsonHelpers::getStringOrDefault(
                      response,
                      "type",
                      "unknown");
                  if (type != "IMAGERY") {
                    endpoint.pRequestThatFailed = std::move(pRequest);
                    return ResultPointer<ExternalAssetEndpoint>(
                        new ExternalAssetEndpoint(std::move(endpoint)),
                        ErrorList::error(fmt::format(
                            "Assets used with a raster overlay must have type "
                            "'IMAGERY', but instead saw '{}'.",
                            type)));
                  }

                  endpoint.requestTime = requestTime;
                  endpoint.externalType = JsonHelpers::getStringOrDefault(
                      response,
                      "externalType",
                      "unknown");

                  if (endpoint.externalType == "BING") {
                    const auto optionsIt = response.FindMember("options");
                    if (optionsIt == response.MemberEnd() ||
                        !optionsIt->value.IsObject()) {
                      endpoint.pRequestThatFailed = std::move(pRequest);
                      return ResultPointer<ExternalAssetEndpoint>(
                          new ExternalAssetEndpoint(std::move(endpoint)),
                          ErrorList::error(fmt::format(
                              "Cesium ion Bing Maps raster overlay metadata "
                              "response does not contain 'options' or it is "
                              "not an object.")));
                    }

                    const auto& options = optionsIt->value;
                    ExternalAssetEndpoint::Bing& bing =
                        endpoint.options.emplace<ExternalAssetEndpoint::Bing>();
                    bing.url =
                        JsonHelpers::getStringOrDefault(options, "url", "");
                    bing.key =
                        JsonHelpers::getStringOrDefault(options, "key", "");
                    bing.mapStyle = JsonHelpers::getStringOrDefault(
                        options,
                        "mapStyle",
                        "AERIAL");
                    bing.culture =
                        JsonHelpers::getStringOrDefault(options, "culture", "");
                  } else if (endpoint.externalType == "GOOGLE_2D_MAPS") {
                    const auto optionsIt = response.FindMember("options");
                    if (optionsIt == response.MemberEnd() ||
                        !optionsIt->value.IsObject()) {
                      endpoint.pRequestThatFailed = std::move(pRequest);
                      return ResultPointer<ExternalAssetEndpoint>(
                          new ExternalAssetEndpoint(std::move(endpoint)),
                          ErrorList::error(fmt::format(
                              "Cesium ion Google Map Tiles raster overlay "
                              "metadata response does not contain 'options' or "
                              "it is not an object.")));
                    }

                    const auto& options = optionsIt->value;
                    ExternalAssetEndpoint::Google2D& google2D =
                        endpoint.options
                            .emplace<ExternalAssetEndpoint::Google2D>();
                    google2D.url = JsonHelpers::getStringOrDefault(
                        options,
                        "url",
                        google2D.url);
                    google2D.key =
                        JsonHelpers::getStringOrDefault(options, "key", "");
                    google2D.session =
                        JsonHelpers::getStringOrDefault(options, "session", "");
                    google2D.expiry =
                        JsonHelpers::getStringOrDefault(options, "expiry", "");
                    google2D.tileWidth = JsonHelpers::getUint32OrDefault(
                        options,
                        "tileWidth",
                        256);
                    google2D.tileHeight = JsonHelpers::getUint32OrDefault(
                        options,
                        "tileHeight",
                        256);
                    google2D.imageFormat = JsonHelpers::getStringOrDefault(
                        options,
                        "imageFormat",
                        GoogleMapTilesImageFormat::jpeg);

                  } else {
                    ExternalAssetEndpoint::TileMapService& tileMapService =
                        endpoint.options
                            .emplace<ExternalAssetEndpoint::TileMapService>();
                    tileMapService.url =
                        JsonHelpers::getStringOrDefault(response, "url", "");
                    tileMapService.accessToken =
                        JsonHelpers::getStringOrDefault(
                            response,
                            "accessToken",
                            "");
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
                      auto collapsible = attribution.FindMember("collapsible");
                      if (collapsible != attribution.MemberEnd() &&
                          collapsible->value.IsBool()) {
                        endpointAttribution.collapsible =
                            collapsible->value.GetBool();
                      }
                    }
                  }

                  return ResultPointer<ExternalAssetEndpoint>(
                      new ExternalAssetEndpoint(std::move(endpoint)));
                })
                .catchImmediately([](std::exception&& e) {
                  return ResultPointer<ExternalAssetEndpoint>(
                      new ExternalAssetEndpoint(),
                      ErrorList::error(
                          std::string(
                              "Error while accessing Cesium ion asset: ") +
                          e.what()));
                })
                .thenImmediately(
                    [pLogger = context.pLogger, url = key.url](
                        ResultPointer<ExternalAssetEndpoint>&& assetResult) {
                      if (assetResult.pValue &&
                          !std::holds_alternative<std::monostate>(
                              assetResult.pValue->options)) {
                        SPDLOG_LOGGER_INFO(
                            pLogger,
                            "Successfully refreshed Cesium ion token for "
                            "URL {}.",
                            url);
                      } else {
                        SPDLOG_LOGGER_INFO(
                            pLogger,
                            "Failed to refresh Cesium ion token for URL {}.",
                            url);
                      }

                      assetResult.errors.log(
                          pLogger,
                          "The following messages were reported while "
                          "refreshing the token:");

                      return std::move(assetResult);
                    });
          });
  return pDepot;
}

SharedFuture<IonRasterOverlay::TileProvider::AggregatedTileProviderResult>
IonRasterOverlay::TileProvider::CreateTileProvider::operator()(
    const IntrusivePointer<ExternalAssetEndpoint>& pEndpoint) {
  if (pEndpoint == nullptr ||
      std::holds_alternative<std::monostate>(pEndpoint->options)) {
    return this->externals.asyncSystem
        .createResolvedFuture<AggregatedTileProviderResult>(
            nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                RasterOverlayLoadType::CesiumIon,
                pEndpoint ? pEndpoint->pRequestThatFailed : nullptr,
                "Could not access Cesium ion asset."}))
        .share();
  }

  IntrusivePointer<RasterOverlay> pOverlay = nullptr;
  if (pEndpoint->externalType == "BING") {
    CESIUM_ASSERT(std::holds_alternative<ExternalAssetEndpoint::Bing>(
        pEndpoint->options));
    ExternalAssetEndpoint::Bing& bing =
        std::get<ExternalAssetEndpoint::Bing>(pEndpoint->options);
    pOverlay = new BingMapsRasterOverlay(
        this->pOwner->getName(),
        bing.url,
        bing.key,
        bing.mapStyle,
        bing.culture,
        this->pOwner->getOptions());
  } else if (pEndpoint->externalType == "GOOGLE_2D_MAPS") {
    CESIUM_ASSERT(std::holds_alternative<ExternalAssetEndpoint::Google2D>(
        pEndpoint->options));
    ExternalAssetEndpoint::Google2D& google2D =
        std::get<ExternalAssetEndpoint::Google2D>(pEndpoint->options);
    pOverlay = new GoogleMapTilesRasterOverlay(
        this->pOwner->getName(),
        GoogleMapTilesExistingSession{
            .key = google2D.key,
            .session = google2D.session,
            .expiry = google2D.expiry,
            .tileWidth = google2D.tileWidth,
            .tileHeight = google2D.tileHeight,
            .imageFormat = google2D.imageFormat,
            .showLogo = false,
            .apiBaseUrl = google2D.url,
        },
        this->pOwner->getOptions());
  } else {
    CESIUM_ASSERT(std::holds_alternative<ExternalAssetEndpoint::TileMapService>(
        pEndpoint->options));
    ExternalAssetEndpoint::TileMapService& tileMapService =
        std::get<ExternalAssetEndpoint::TileMapService>(pEndpoint->options);
    pOverlay = new TileMapServiceRasterOverlay(
        this->pOwner->getName(),
        tileMapService.url,
        std::vector<CesiumAsync::IAssetAccessor::THeader>{std::make_pair(
            "Authorization",
            "Bearer " + tileMapService.accessToken)},
        TileMapServiceRasterOverlayOptions(),
        this->pOwner->getOptions());
  }

  std::vector<Credit> credits;

  if (this->externals.pCreditSystem) {
    credits.reserve(pEndpoint->attributions.size());
    for (const AssetEndpointAttribution& attribution :
         pEndpoint->attributions) {
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
      .thenImmediately([credits = std::move(credits)](
                           CreateTileProviderResult&& result) mutable {
        if (result) {
          return AggregatedTileProviderResult(AggregatedTileProviderSuccess{
              .pAggregated = result.value(),
              .creditsFromIon = std::move(credits),
          });
        } else {
          return AggregatedTileProviderResult(
              nonstd::make_unexpected(result.error()));
        }
      })
      .share();
}

} // namespace CesiumRasterOverlays
