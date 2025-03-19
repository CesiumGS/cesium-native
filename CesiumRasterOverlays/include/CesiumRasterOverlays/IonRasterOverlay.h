#pragma once

#include <CesiumAsync/IAssetRequest.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>

#include <functional>
#include <memory>

namespace CesiumRasterOverlays {

/**
 * @brief A class specifying how an endpoint should be accessed.
 */
class EndpointResource {
public:
  /**
   * @brief If this method returns true, the access token should be included in
   * the `Authorization` header of the initial request.
   */
  virtual bool needsAuthHeaderOnInitialRequest() const = 0;

  /**
   * @brief Returns the endpoint URL for the given asset.
   *
   * @param assetID The ID of the asset to obtain.
   * @param accessToken The access token to use to obtain the asset.
   * @param assetEndpointUrl The base URL of the asset endpoint.
   */
  virtual std::string getUrl(
      int64_t assetID,
      const std::string& accessToken,
      const std::string& assetEndpointUrl) const = 0;
  virtual ~EndpointResource() = default;
};

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
   * @param ionAssetID The asset ID.
   * @param ionAccessToken The access token.
   * @param endpointResource The \ref EndpointResource to use to access the ion
   * API or an ion-compatible API.
   * @param overlayOptions The {@link RasterOverlayOptions} for this instance.
   * @param ionAssetEndpointUrl The URL of the ion endpoint to make our requests
   * to.
   */
  IonRasterOverlay(
      const std::string& name,
      int64_t ionAssetID,
      const std::string& ionAccessToken,
      std::unique_ptr<EndpointResource>&& endpointResource,
      const RasterOverlayOptions& overlayOptions = {},
      const std::string& ionAssetEndpointUrl = "https://api.cesium.com/");

private:
  int64_t _ionAssetID;
  std::string _ionAccessToken;
  std::string _ionAssetEndpointUrl;
  std::unique_ptr<EndpointResource> _endpointResource;

  struct AssetEndpointAttribution {
    std::string html;
    bool collapsible = true;
  };

  struct ExternalAssetEndpoint {
    std::string externalType;
    std::string url;
    std::string mapStyle;
    std::string key;
    std::string culture;
    std::string accessToken;
    std::vector<AssetEndpointAttribution> attributions;
  };

  static std::unordered_map<std::string, ExternalAssetEndpoint> endpointCache;

  CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const ExternalAssetEndpoint& endpoint,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner) const;
};

} // namespace CesiumRasterOverlays
