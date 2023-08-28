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
 * @brief Options for tile map service accesses.
 */
struct WebMapTileServiceRasterOverlayOptions {

  /**
   * @brief The MIME type for images to retrieve from the server.
   */
  std::optional<std::string> format;

  /**
   * @brief The subdomains to use for the <code>{s}</code> placeholder in the URL template.
   *
   * If this parameter is a single string, each character in the string is a
   * subdomain. If it is an array, each element in the array is a subdomain.
   */
  std::optional<std::variant<std::string, std::vector<std::string>>> subdomains;

  /**
   * @brief A credit for the data source, which is displayed on the canvas.
   */
  std::optional<std::string> credit;

  /**
   * @brief The layer name for WMTS requests.
   */
  std::optional<std::string> layer;

  /**
   * @brief The style name for WMTS requests.
   */
  std::optional<std::string> style;

  /**
   * @brief The identifier of the TileMatrixSet to use for WMTS requests.
   */
  std::optional<std::string> tileMatrixSetID;

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
   */
  std::optional<uint32_t> minimumLevel;

  /**
   * @brief The maximum level-of-detail supported by the imagery provider.
   *
   * This will be `std::nullopt` if there is no limit.
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
   * @brief The {@link CesiumGeospatial::Ellipsoid}.
   *
   * If the `tilingScheme` is specified, this parameter is ignored and
   * the tiling scheme's ellipsoid is used instead. If neither parameter
   * is specified, the {@link CesiumGeospatial::Ellipsoid::WGS84} is used.
   */
  std::optional<CesiumGeospatial::Ellipsoid> ellipsoid;

  /**
   * @brief A object containing static dimensions and their values.
   */
  std::optional<std::map<std::string, std::string>> dimensions;

  /**
   * @brief Pixel width of image tiles.
   */
  std::optional<uint32_t> tileWidth;

  /**
   * @brief Pixel height of image tiles.
   */
  std::optional<uint32_t> tileHeight;
};

/**
 * @brief A {@link RasterOverlay} based on tile map service imagery.
 */
class CESIUM3DTILESSELECTION_API WebMapTileServiceRasterOverlay final
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
      const std::shared_ptr<CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner)
      const override;

private:
  std::string _url;
  std::vector<CesiumAsync::IAssetAccessor::THeader> _headers;
  WebMapTileServiceRasterOverlayOptions _options;
};

} // namespace Cesium3DTilesSelection
