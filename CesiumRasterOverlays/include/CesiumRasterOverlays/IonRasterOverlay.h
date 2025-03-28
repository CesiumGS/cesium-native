#pragma once

#include <CesiumAsync/IAssetRequest.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>

#include <memory>

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
  bool _needsAuthHeader = false;

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
