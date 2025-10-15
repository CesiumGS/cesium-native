#pragma once

#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumUtility/JsonValue.h>

#include <optional>
#include <string>

namespace CesiumRasterOverlays {

/**
 * @brief Holds the parameters for an existing Google Maps Tiles session.
 */
struct GoogleMapTilesExistingSession {
  /**
   * @brief The Google Map Tiles API key to use.
   */
  std::string key;

  /**
   * @brief The session token value to include in all Map Tiles API requests.
   */
  std::string session;

  /**
   * @brief A string that contains the time (in seconds since the epoch) at
   * which the token expires.
   *
   * Google documents that a session token is valid for two weeks from its
   * creation time, but that this policy might change without notice.
   */
  std::string expiry;

  /**
   * @brief The width of the tiles measured in pixels.
   */
  uint32_t tileWidth;

  /**
   * @brief The height of the tiles measured in pixels.
   */
  uint32_t tileHeight;

  /**
   * @brief The image format, which can be either @ref
   * GoogleMapTilesImageFormat::png or @ref GoogleMapTilesImageFormat::jpeg.
   */
  std::string imageFormat;

  /**
   * @brief Whether or not the @ref GoogleMapTilesRasterOverlay should show the
   * Google Maps logo.
   *
   * Google requires the logo to be shown, so setting this to false is only
   * valid when something else is already showing the logo.
   */
  bool showLogo = true;

  /**
   * @brief The base URL for the Google Maps Tiles API.
   */
  std::string apiBaseUrl{"https://tile.googleapis.com/"};
};

/**
 * @brief Standard values for @ref GoogleMapTilesNewSessionParameters::mapType.
 */
struct GoogleMapTilesMapType {
  /**
   * @brief The standard Google Maps painted map tiles.
   */
  inline static const std::string roadmap = "roadmap";

  /**
   * @brief Satellite imagery.
   */
  inline static const std::string satellite = "satellite";

  /**
   * @brief Terrain imagery. When selecting terrain as the map type, you must
   * also set the @ref GoogleMapTilesNewSessionParameters::layerTypes property
   * to @ref GoogleMapTilesLayerType::layerRoadmap.
   */
  inline static const std::string terrain = "terrain";

  /**
   * @brief Street View panoramas. This is a documented map type supported by
   * the service, but it is unlikely to work well as a raster overlay.
   */
  inline static const std::string streetview = "streetview";
};

/**
 * @brief Standard values for @ref
 * GoogleMapTilesNewSessionParameters::layerTypes.
 */
struct GoogleMapTilesLayerType {
  /**
   * @brief Required if you specify terrain as the map type. Can also be
   * optionally overlaid on the satellite map type. Has no effect on roadmap
   * tiles.
   */
  inline static const std::string layerRoadmap = "layerRoadmap";

  /**
   * @brief Shows Street View-enabled streets and locations using blue outlines
   * on the map.
   */
  inline static const std::string layerStreetview = "layerStreetview";

  /**
   * @brief Displays current traffic conditions.
   */
  inline static const std::string layerTraffic = "layerTraffic";
};

/**
 * @brief Standard values for @ref
 * GoogleMapTilesNewSessionParameters::imageFormat.
 */
struct GoogleMapTilesImageFormat {
  /**
   * @brief Portable Network Graphics format. This format supports transparency.
   */
  inline static const std::string png = "png";

  /**
   * @brief Joint Photographic Experts Group format. This format doesn't support
   * transparency.
   */
  inline static const std::string jpeg = "jpeg";
};

/**
 * @brief Standard values for @ref GoogleMapTilesNewSessionParameters::scale.
 */
struct GoogleMapTilesScale {
  /**
   * @brief The default.
   */
  inline static const std::string scaleFactor1x = "scaleFactor1x";

  /**
   * @brief Doubles label size and removes minor feature labels.
   */
  inline static const std::string scaleFactor2x = "scaleFactor2x";

  /**
   * @brief Quadruples label size and removes minor feature labels.
   */
  inline static const std::string scaleFactor4x = "scaleFactor4x";
};

/**
 * @brief Holds the parameters for starting a new Google Maps Tiles session.
 */
struct GoogleMapTilesNewSessionParameters {
  /**
   * @brief The Google Map Tiles API key to use.
   */
  std::string key;

  /**
   * @brief The type of base map. See @ref GoogleMapTilesMapType for standard
   * values.
   */
  std::string mapType{"satellite"};

  /**
   * @brief An [IETF language
   * tag](https://en.wikipedia.org/wiki/IETF_language_tag) that specifies the
   * language used to display information on the tiles. For example, `en-US`
   * specifies the English language as spoken in the United States.
   */
  std::string language{"en-US"};

  /**
   * @brief A [Common Locale Data Repository](https://cldr.unicode.org/) region
   * identifier (two uppercase letters) that represents the physical location of
   * the user. For example, `US`.
   */
  std::string region{"US"};

  /**
   * @brief Specifies the file format to return. See @ref
   * GoogleMapTilesImageFormat for standard values.
   *
   * If you don't specify an `imageFormat`, then the best format for the tile is
   * chosen automatically.
   */
  std::optional<std::string> imageFormat{};

  /**
   * @brief Scales-up the size of map elements (such as road labels), while
   * retaining the tile size and coverage area of the default tile. See @ref
   * GoogleMapTilesScale for standard value.
   *
   * Increasing the scale also reduces the number of labels on the map, which
   * reduces clutter.
   */
  std::optional<std::string> scale{};

  /**
   * @brief Specifies whether to return high-resolution tiles.
   *
   * If the scale-factor is increased, `highDpi` is used to increase the size of
   * the tile. Normally, increasing the scale factor enlarges the resulting tile
   * into an image of the same size, which lowers quality. With `highDpi`, the
   * resulting size is also increased, preserving quality. DPI stands for Dots
   * per Inch, and High DPI means the tile renders using more dots per inch than
   * normal. If `true`, then the number of pixels in each of the x and y
   * dimensions is multiplied by the scale factor (that is , 2x or 4x). The
   * coverage area of the tile remains unchanged. This parameter works only with
   * @ref scale values of @ref GoogleMapTilesScale::scaleFactor2x or @ref
   * GoogleMapTilesScale::scaleFactor4x. It has no effect on @ref
   * GoogleMapTilesScale::scaleFactor1x scale tiles.
   */
  std::optional<bool> highDpi{};

  /**
   * @brief An array of values that specifies the layer types added to the map.
   * See @ref GoogleMapTilesLayerType for standard values.
   */
  std::optional<std::vector<std::string>> layerTypes{};

  /**
   * @brief An array of JSON style objects that specify the appearance and
   * detail level of map features such as roads, parks, and built-up areas.
   *
   * Styling is used to customize the standard Google base map. The `styles`
   * parameter is valid only if the map type is @ref
   * GoogleMapTilesMapType::roadmap. For the complete style syntax, see the
   * [Style
   * Reference](https://developers.google.com/maps/documentation/tile/style-reference).
   */
  std::optional<CesiumUtility::JsonValue::Array> styles{};

  /**
   * @brief A boolean value that specifies whether @ref layerTypes should be
   * rendered as a separate overlay, or combined with the base imagery.
   *
   * When `true`, the base map isn't displayed. If you haven't defined any
   * `layerTypes`, then this value is ignored.
   */
  std::optional<bool> overlay{};

  /**
   * @brief The base URL for the Google Maps Tiles API.
   */
  std::string apiBaseUrl{"https://tile.googleapis.com/"};
};

/**
 * @brief A `RasterOverlay` that retrieves imagery from the [Google Maps Tiles
 * API](https://developers.google.com/maps/documentation/tile).
 */
class CESIUMRASTEROVERLAYS_API GoogleMapTilesRasterOverlay
    : public RasterOverlay {
public:
  /**
   * @brief Constructs a new overlay that will start a new Google Maps Tiles
   * session with the specified parameters.
   *
   * @param name The user-given name of this overlay layer.
   * @param newSessionParameters The parameters for starting a new session.
   * @param overlayOptions The @ref RasterOverlayOptions for this instance.
   */
  GoogleMapTilesRasterOverlay(
      const std::string& name,
      const GoogleMapTilesNewSessionParameters& newSessionParameters,
      const RasterOverlayOptions& overlayOptions = {});

  /**
   * @brief Constructs a new overlay that will use an existing Google Maps Tiles
   * session that was previously started.
   *
   * @param name The user-given name of this overlay layer.
   * @param existingSession The parameters for the existing session.
   * @param overlayOptions The @ref RasterOverlayOptions for this instance.
   */
  GoogleMapTilesRasterOverlay(
      const std::string& name,
      const GoogleMapTilesExistingSession& existingSession,
      const RasterOverlayOptions& overlayOptions = {});

  /** @inheritdoc */
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
  CesiumAsync::Future<CreateTileProviderResult> createNewSession(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner) const;

  std::optional<GoogleMapTilesNewSessionParameters> _newSessionParameters;
  std::optional<GoogleMapTilesExistingSession> _existingSession;
};

} // namespace CesiumRasterOverlays
