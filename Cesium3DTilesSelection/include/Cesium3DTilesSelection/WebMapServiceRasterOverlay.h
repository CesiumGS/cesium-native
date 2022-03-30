#pragma once

#include "Library.h"
#include "RasterOverlay.h"

#include <CesiumAsync/IAssetRequest.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>

#include <functional>
#include <memory>

namespace Cesium3DTilesSelection {

class CreditSystem;

/**
 * @brief Options for web map service accesses.
 */
struct WebMapServiceRasterOverlayOptions {

  /**
   * @brief web map service version. 1.3.0 by default
   */
  std::optional<std::string> version = "1.3.0";

  /**
   * @brief comma separated Web Map Service layer names. 
   */
  std::string layers;

  /**
   * @brief The file extension for images on the server.
   */
  std::optional<std::string> fileExtension;

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
  std::optional<uint32_t> minimumLevel = 0;

  /**
   * @brief The maximum level-of-detail supported by the imagery provider.
   *
   * This will be `std::nullopt` if there is no limit.
   */
  std::optional<uint32_t> maximumLevel = 14;

  /**
   * @brief Pixel width of image tiles.
   */
  std::optional<uint32_t> tileWidth = 256;

  /**
   * @brief Pixel height of image tiles.
   */
  std::optional<uint32_t> tileHeight = 256;
};

/**
 * @brief A {@link RasterOverlay} based on tile map service imagery.
 */
class CESIUM3DTILESSELECTION_API WebMapServiceRasterOverlay final
    : public RasterOverlay {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param name The user-given name of this overlay layer.
   * @param url The base URL.
   * @param headers The headers. This is a list of pairs of strings of the
   * form (Key,Value) that will be inserted as request headers internally.
   * @param tmsOptions The {@link WebMapServiceRasterOverlayOptions}.
   * @param overlayOptions The {@link RasterOverlayOptions} for this instance.
   */
  WebMapServiceRasterOverlay(
      const std::string& name,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers = {},
      const WebMapServiceRasterOverlayOptions& wmsOptions = {},
      const RasterOverlayOptions& overlayOptions = {});
  virtual ~WebMapServiceRasterOverlay() override;

  virtual CesiumAsync::Future<std::unique_ptr<RasterOverlayTileProvider>>
  createTileProvider(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      RasterOverlay* pOwner) override;

private:
  std::string _baseUrl;
  std::vector<CesiumAsync::IAssetAccessor::THeader> _headers;
  WebMapServiceRasterOverlayOptions _options;
};

} // namespace Cesium3DTilesSelection
