#pragma once

#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/NetworkAssetDescriptor.h>
#include <CesiumAsync/SharedAssetDepot.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumUtility/SharedAsset.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace CesiumRasterOverlays {

/**
 * @brief A {@link RasterOverlay} that obtains imagery data from Cesium ion.
 */
class CESIUMRASTEROVERLAYS_API IonRasterOverlay : public RasterOverlay {
public:
  /**
   * @brief Creates a new instance.
   *
   * The tiles that are provided by this instance will contain
   * imagery data that was obtained from the Cesium ion asset
   * with the given ID, accessed with the given access token.
   *
   * @param name The user-given name of this overlay layer.
   * @param ionAssetID The asset ID.
   * @param ionAccessToken The access token.
   * @param overlayOptions The {@link RasterOverlayOptions} for this instance.
   * @param ionAssetEndpointUrl The URL of the ion endpoint to make our requests
   * to.
   */
  IonRasterOverlay(
      const std::string& name,
      int64_t ionAssetID,
      const std::string& ionAccessToken,
      const RasterOverlayOptions& overlayOptions = {},
      const std::string& ionAssetEndpointUrl = "https://api.cesium.com/");
  virtual ~IonRasterOverlay() override;

  /**
   * @brief Gets the additional `options` to be passed to the asset endpoint.
   *
   * @returns An optional JSON string describing parameters that are specific to
   * the asset.
   */
  const std::optional<std::string>& getAssetOptions() const noexcept;

  /**
   * @brief Sets the additional `options` to be passed to the asset endpoint.
   *
   * @param options An optional JSON string describing parameters that are
   * specific to the asset.
   */
  void setAssetOptions(const std::optional<std::string>& options) noexcept;

  virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner)
      const override;

protected:
  /**
   * @brief Creates a new instance.
   *
   * The tiles that are provided by this instance will contain
   * imagery data that was obtained from the Cesium ion asset
   * with the given ID, accessed with the given access token.
   *
   * @param name The user-given name of this overlay layer.
   * @param overlayUrl The URL that the raster overlay requests will be made to.
   * @param ionAccessToken The access token.
   * @param needsAuthHeader If true, the access token will be passed through the
   * Authorization header. If false, it will be assumed to be in the provided
   * `overlayUrl`.
   * @param overlayOptions The {@link RasterOverlayOptions} for this instance.
   */
  IonRasterOverlay(
      const std::string& name,
      const std::string& overlayUrl,
      const std::string& ionAccessToken,
      bool needsAuthHeader,
      const RasterOverlayOptions& overlayOptions = {});

private:
  std::string _overlayUrl;
  std::string _ionAccessToken;
  bool _needsAuthHeader;
  std::optional<std::string> _assetOptions;

  class TileProvider;

  struct AssetEndpointAttribution {
    std::string html;
    bool collapsible = true;
  };

  struct ExternalAssetEndpoint
      : public CesiumUtility::SharedAsset<ExternalAssetEndpoint> {
    ExternalAssetEndpoint() noexcept = default;
    ~ExternalAssetEndpoint() noexcept = default;
    ExternalAssetEndpoint(const ExternalAssetEndpoint&) noexcept = default;
    ExternalAssetEndpoint(ExternalAssetEndpoint&&) noexcept = default;

    std::chrono::steady_clock::time_point requestTime{};
    std::string externalType{};
    std::vector<AssetEndpointAttribution> attributions{};
    std::shared_ptr<CesiumAsync::IAssetRequest> pRequestThatFailed{};

    /** @private */
    struct TileMapService {
      std::string url;
      std::string accessToken;
    };

    /** @private */
    struct Bing {
      std::string key;
      std::string url;
      std::string mapStyle;
      std::string culture;
    };

    /** @private */
    struct Google2D {
      std::string url;
      std::string key;
      std::string session;
      std::string expiry;
      std::string imageFormat;
      uint32_t tileWidth;
      uint32_t tileHeight;
    };

    std::variant<std::monostate, TileMapService, Bing, Google2D> options{};
  };

  static std::unordered_map<std::string, ExternalAssetEndpoint> endpointCache;

  using EndpointDepot = CesiumAsync::SharedAssetDepot<
      ExternalAssetEndpoint,
      CesiumAsync::NetworkAssetDescriptor,
      RasterOverlayExternals>;

  static CesiumUtility::IntrusivePointer<EndpointDepot> getEndpointCache();
};

} // namespace CesiumRasterOverlays
