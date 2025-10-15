#pragma once

#include <CesiumAsync/IAssetRequest.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>

#include <functional>
#include <memory>

namespace CesiumRasterOverlays {

/**
 * @brief Supported values for @ref AzureMapsParameters::tilesetId. See the
 * [official
 * documentation](https://learn.microsoft.com/en-us/rest/api/maps/render/get-map-tile?tabs=HTTP#tilesetid)
 * for all standard values. Note that some tileset IDs return vector data, which
 * is currently unsupported.
 */
struct AzureMapsTilesetId {
  /**
   * @brief All layers with Azure Maps' main style.
   */
  inline static const std::string baseRoad = "microsoft.base.road";

  /**
   * @brief All layers with Azure Maps' dark grey style.
   */
  inline static const std::string baseDarkGrey = "microsoft.base.darkgrey";

  /**
   * @brief Label data in Azure Maps' main style.
   */
  inline static const std::string baseLabelsRoad = "microsoft.base.labels.road";

  /**
   * @brief Label data in Azure Maps' dark grey style.
   */
  inline static const std::string baseLabelsDarkGrey =
      "microsoft.base.labels.darkgrey";

  /**
   * @brief Road, boundary and label  data in Azure Maps' main style.
   */
  inline static const std::string baseHybridRoad = "microsoft.base.hybrid.road";

  /**
   * @brief Road, boundary and label  data in Azure Maps' dark grey style.
   */
  inline static const std::string baseHybridDarkGrey =
      "microsoft.base.hybrid.darkgrey";

  /**
   * @brief A combination of satellite or aerial imagery. Only available in S1
   * and G2 pricing SKU.
   */
  inline static const std::string imagery = "microsoft.imagery";

  /**
   * @brief Shaded relief and terra layers.
   */
  inline static const std::string terra = "microsoft.terra.main";

  /**
   * @brief Weather radar tiles. Latest weather radar images including areas of
   * rain, snow, ice and mixed conditions. For more information on the Azure
   * Maps Weather service coverage, see [Azure Maps weather services
   * coverage](https://learn.microsoft.com/en-us/azure/azure-maps/weather-coverage).
   * For more information on Radar data, see [Weather services in Azure
   * Maps](https://learn.microsoft.com/en-us/azure/azure-maps/weather-services-concepts#radar-images).
   */
  inline static const std::string weatherRadar = "microsoft.weather.radar.main";

  /**
   * @brief Weather infrared tiles. Latest Infrared Satellite images show
   * clouds by their temperature. For more information on the Azure Maps Weather
   * service coverage, see [Azure Maps weather services
   * coverage](https://learn.microsoft.com/en-us/azure/azure-maps/weather-coverage).
   * For more information on Radar data, see [Weather services in Azure
   * Maps](https://learn.microsoft.com/en-us/azure/azure-maps/weather-services-concepts#radar-images).
   */
  inline static const std::string weatherInfrared =
      "microsoft.weather.infrared.main";

  /**
   * @brief Absolute traffic tiles in Azure Maps' main style.
   */
  inline static const std::string trafficAbsolute =
      "microsoft.traffic.absolute.main";

  /**
   * @brief Relative traffic tiles in Azure Maps' main style.
   */
  inline static const std::string trafficRelativeMain =
      "microsoft.traffic.relative.main";

  /**
   * @brief Relative traffic tiles in Azure Maps' dark style.
   */
  inline static const std::string trafficRelativeDark =
      "microsoft.traffic.relative.dark";

  /**
   * @brief Delay traffic tiles in Azure Maps' main style.
   */
  inline static const std::string trafficDelay = "microsoft.traffic.delay.main";

  /**
   * @brief Reduced traffic tiles in Azure Maps' main style.
   */
  inline static const std::string trafficReduced =
      "microsoft.traffic.reduced.main";
};

/**
 * @brief Session parameters for an Azure Maps overlay.
 */
struct AzureMapsSessionParameters {
  /**
   * @brief The Azure Maps subscription key to use.
   */
  std::string key;

  /**
   * @brief The version number of Azure Maps API.
   */
  std::string apiVersion{"2024-04-01"};

  /**
   * @brief A tileset is a collection of raster or vector data broken up into a
   * uniform grid of square tiles at preset zoom levels. Every tileset has a
   * tilesetId to use when making requests. The tilesetId for tilesets created
   * using [Azure Maps Creator](https://aka.ms/amcreator) are generated through
   * the [Tileset Create
   * API](https://learn.microsoft.com/en-us/rest/api/maps-creator/tileset).
   *
   * The supported ready-to-use tilesets supplied by Azure Maps are listed in
   * below. For example, microsoft.base.road.
   */
  std::string tilesetId{AzureMapsTilesetId::imagery};

  /**
   * @brief The language in which search results should be returned. Should be
   * one of the supported [IETF language
   * tags](https://en.wikipedia.org/wiki/IETF_language_tag), case insensitive.
   * When data in the specified language is not available for a specific field,
   * default language is used.
   *
   * Refer to [Supported
   * Languages](https://learn.microsoft.com/en-us/azure/azure-maps/supported-languages)
   * for details.
   */
  std::string language{"en-US"};

  /**
   * @brief The View parameter (also called the "user region" parameter) allows
   * you to show the correct maps for a certain country/region for
   * geopolitically disputed regions. Different countries/regions have different
   * views of such regions, and the View parameter allows your application to
   * comply with the view required by the country/region your application will
   * be serving. By default, the View parameter is set to "Unified" even if you
   * haven't defined it in the request. It is your responsibility to determine
   * the location of your users, and then set the View parameter correctly for
   * that location. Alternatively, you have the option to set 'View=Auto', which
   * will return the map data based on the IP address of the request. The View
   * parameter in Azure Maps must be used in compliance with applicable laws,
   * including those regarding mapping, of the country/region where maps, images
   * and other data and third party content that you are authorized to access
   * via Azure Maps is made available. Example: view=IN.
   *
   * Refer to [Supported Views](https://aka.ms/AzureMapsLocalizationViews) for
   * details and to see the available Views.
   */
  std::string view{"US"};

  /**
   * @brief Whether or not the @ref AzureMapsRasterOverlay should show the
   * Azure Maps logo.
   *
   * Azure requires the logo to be shown, so setting this to false is only
   * valid when something else is already showing the logo.
   */
  bool showLogo = true;

  /**
   * @brief The base URL for the Azure Maps API.
   */
  std::string apiBaseUrl{"https://atlas.microsoft.com/"};
};

/**
 * @brief A @ref RasterOverlay that retrieves imagery from the Azure Maps API.
 */
class CESIUMRASTEROVERLAYS_API AzureMapsRasterOverlay final
    : public RasterOverlay {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param name The user-given name of this overlay layer.
   * @param sessionParameters The session parameters for this overlay.
   * @param overlayOptions The @ref RasterOverlayOptions for this instance.
   */
  AzureMapsRasterOverlay(
      const std::string& name,
      const AzureMapsSessionParameters& sessionParameters,
      const RasterOverlayOptions& overlayOptions = {});
  virtual ~AzureMapsRasterOverlay() override;

  virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner)
      const override;

  /**
   * @brief Refresh a previously-created tile provider using a new key.
   *
   * Calling this method on a tile provider that was not created by this @ref
   * AzureMapsRasterOverlay will lead to undefined behavior.
   *
   * @param pProvider The previously-created tile provider.
   * @param newKey The new key to use.
   */
  CesiumAsync::Future<void> refreshTileProviderWithNewKey(
      const CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>&
          pProvider,
      const std::string& newKey);

private:
  AzureMapsSessionParameters _sessionParameters;
};

} // namespace CesiumRasterOverlays
