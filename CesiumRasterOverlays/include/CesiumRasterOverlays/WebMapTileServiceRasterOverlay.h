#pragma once

#include <CesiumAsync/IAssetRequest.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>

#include <functional>
#include <memory>
#include <vector>

namespace CesiumRasterOverlays {

/**
 * @brief Options for {@link WebMapTileServiceRasterOverlay}.
 */
struct WebMapTileServiceRasterOverlayOptions {

  /**
   * @brief The MIME type for images to retrieve from the server.
   *
   * Default value is "image/jpeg"
   */
  std::optional<std::string> format;

  /**
   * @brief The subdomains to use for the <code>{s}</code> placeholder in the URL template.
   *
   * If this parameter is a single string, each character in the string is a
   * subdomain. If it is an array, each element in the array is a subdomain.
   */
  std::vector<std::string> subdomains;

  /**
   * @brief A credit for the data source, which is displayed on the canvas.
   */
  std::optional<std::string> credit;

  /**
   * @brief The layer name for WMTS requests.
   */
  std::string layer;

  /**
   * @brief The style name for WMTS requests.
   */
  std::string style;

  /**
   * @brief The identifier of the TileMatrixSet to use for WMTS requests.
   */
  std::string tileMatrixSetID;

  /**
   * @brief A list of identifiers in the TileMatrix to use for WMTS requests,
   * one per TileMatrix level.
   */
  std::optional<std::vector<std::string>> tileMatrixLabels;

  /**
   * @brief The minimum level-of-detail supported by the imagery provider.
   *
   * Take care when specifying this that the number of tiles at the minimum
   * level is small, such as four or less. A larger number is likely to
   * result in rendering problems.
   * Default value is 0.
   */
  std::optional<uint32_t> minimumLevel;

  /**
   * @brief The maximum level-of-detail supported by the imagery provider.
   *
   * Default value is 25.
   */
  std::optional<uint32_t> maximumLevel;

  /**
   * @brief The {@link CesiumGeometry::Rectangle}, in radians, covered by the
   * image.
   */
  std::optional<CesiumGeometry::Rectangle> coverageRectangle;

  /**
   * @brief The {@link CesiumGeospatial::Projection} that is used.
   */
  std::optional<CesiumGeospatial::Projection> projection;

  /**
   * @brief The {@link CesiumGeometry::QuadtreeTilingScheme} specifying how
   * the ellipsoidal surface is broken into tiles.
   */
  std::optional<CesiumGeometry::QuadtreeTilingScheme> tilingScheme;

  /**
   * @brief A object containing static dimensions and their values.
   */
  std::optional<std::map<std::string, std::string>> dimensions;

  /**
   * @brief Pixel width of image tiles.
   *
   * Default value is 256
   */
  std::optional<uint32_t> tileWidth;

  /**
   * @brief Pixel height of image tiles.
   *
   * Default value is 256
   */
  std::optional<uint32_t> tileHeight;
};

/**
 * @brief A {@link RasterOverlay} accessing images from a Web Map Tile Service
 * (WMTS) server.
 */
class CESIUMRASTEROVERLAYS_API WebMapTileServiceRasterOverlay final
    : public RasterOverlay {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param name The user-given name of this overlay layer.
   * @param url The base URL.
   * @param headers The headers. This is a list of pairs of strings of the
   * form (Key,Value) that will be inserted as request headers internally.
   * @param tmsOptions The {@link WebMapTileServiceRasterOverlayOptions}.
   * @param overlayOptions The {@link RasterOverlayOptions} for this instance.
   */
  WebMapTileServiceRasterOverlay(
      const std::string& name,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers = {},
      const WebMapTileServiceRasterOverlayOptions& tmsOptions = {},
      const RasterOverlayOptions& overlayOptions = {});
  virtual ~WebMapTileServiceRasterOverlay() override;

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
  std::string _url;
  std::vector<CesiumAsync::IAssetAccessor::THeader> _headers;
  WebMapTileServiceRasterOverlayOptions _options;
};

} // namespace CesiumRasterOverlays
