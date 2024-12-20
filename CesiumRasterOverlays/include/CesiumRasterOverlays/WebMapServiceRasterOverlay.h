#pragma once

#include <CesiumAsync/IAssetRequest.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>

#include <memory>

namespace CesiumRasterOverlays {

/**
 * @brief Options for Web Map Service (WMS) overlays.
 */
struct WebMapServiceRasterOverlayOptions {

  /**
   * @brief The Web Map Service version. The default is "1.3.0".
   */
  std::string version = "1.3.0";

  /**
   * @brief Comma separated Web Map Service layer names to request.
   */
  std::string layers;

  /**
   * @brief The image format to request, expressed as a MIME type to be given to
   * the server. The default is "image/png".
   */
  std::string format = "image/png";

  /**
   * @brief A credit for the data source, which is displayed on the canvas.
   */
  std::optional<std::string> credit;

  /**
   * @brief The minimum level-of-detail supported by the imagery provider.
   *
   * Take care when specifying this that the number of tiles at the minimum
   * level is small, such as four or less. A larger number is likely to
   * result in rendering problems.
   */
  int32_t minimumLevel = 0;

  /**
   * @brief The maximum level-of-detail supported by the imagery provider.
   */
  int32_t maximumLevel = 14;

  /**
   * @brief Pixel width of image tiles.
   */
  int32_t tileWidth = 256;

  /**
   * @brief Pixel height of image tiles.
   */
  int32_t tileHeight = 256;
};

/**
 * @brief A {@link RasterOverlay} accessing images from a Web Map Service (WMS) server.
 */
class CESIUMRASTEROVERLAYS_API WebMapServiceRasterOverlay final
    : public RasterOverlay {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param name The user-given name of this overlay layer.
   * @param url The base URL.
   * @param headers The headers. This is a list of pairs of strings of the
   * form (Key,Value) that will be inserted as request headers internally.
   * @param wmsOptions The {@link WebMapServiceRasterOverlayOptions}.
   * @param overlayOptions The {@link RasterOverlayOptions} for this instance.
   */
  WebMapServiceRasterOverlay(
      const std::string& name,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers = {},
      const WebMapServiceRasterOverlayOptions& wmsOptions = {},
      const RasterOverlayOptions& overlayOptions = {});
  virtual ~WebMapServiceRasterOverlay() override;

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
  std::string _baseUrl;
  std::vector<CesiumAsync::IAssetAccessor::THeader> _headers;
  WebMapServiceRasterOverlayOptions _options;
};

} // namespace CesiumRasterOverlays
