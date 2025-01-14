#pragma once

#include <CesiumAsync/IAssetRequest.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>

#include <functional>
#include <memory>

namespace CesiumRasterOverlays {

/**
 * @brief A {@link RasterOverlay} that obtains imagery data from Cesium ion.
 */
class CESIUMRASTEROVERLAYS_API IonRasterOverlay final : public RasterOverlay {
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

private:
  int64_t _ionAssetID;
  std::string _ionAccessToken;
  std::string _ionAssetEndpointUrl;

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
