#pragma once

#include "Cesium3DTilesSelection/Library.h"
#include "Cesium3DTilesSelection/RasterOverlay.h"
#include "CesiumAsync/IAssetRequest.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/Projection.h"
#include <functional>
#include <memory>

namespace Cesium3DTilesSelection {

class CreditSystem;

/**
 * @brief Options for tile map service accesses.
 */
struct ArcGisMapServerRasterOverlayOptions {

  /**
   * @brief The ArcGIS token used to authenticate with the ArcGIS MapServer service.
   */
  std::optional<std::string> token;

  /**
   * @brief If true, the server's pre-cached tiles are used if they are available.
   * 
   * If false, any pre-cached tiles are ignored and the 'export' service is used.
   */
  std::optional<bool> usePreCachedTilesIfAvailable;

  /**
   * @brief A comma-separated list of the layers to show, or undefined if all layers should be shown.
   */
  std::optional<std::string> layers;

  /**
   * @brief A credit for the data source, which is displayed on the canvas. 
   *
   * This parameter is ignored when accessing a tiled server.
   */
  std::optional<std::string> credit;

  /**
   * @brief The {@link CesiumGeometry::Rectangle} of the layer. This parameter is ignored when accessing a tiled layer.
   */
  std::optional<CesiumGeometry::Rectangle> coverageRectangle;

  /**
   * @brief The {@link CesiumGeospatial::Projection} that is used.
   */
  std::optional<CesiumGeospatial::Projection> projection;

  /**
   * @brief The {@link CesiumGeometry::QuadtreeTilingScheme} of the layer.
   *
   * This parameter is ignored when accessing a tiled layer.
   */
  std::optional<CesiumGeometry::QuadtreeTilingScheme> tilingScheme;

  /**
   * @brief The {@link CesiumGeospatial::Ellipsoid}.
   *
   * If the `tilingScheme` is specified and used, this parameter is ignored and
   * the tiling scheme's ellipsoid is used instead. If neither parameter
   * is specified, the {@link CesiumGeospatial::Ellipsoid::WGS84} is used.
   */
  std::optional<CesiumGeospatial::Ellipsoid> ellipsoid;

  /**
   * @brief The width of each tile in pixels. This parameter is ignored when accessing a tiled server.
   */
  std::optional<uint32_t> tileWidth;

  /**
   * @brief The height of each tile in pixels. This parameter is ignored when accessing a tiled server.
   */
  std::optional<uint32_t> tileHeight;

  /**
   * @brief The minimum tile level to request, or undefined if there is no minimum.
   *
   * This parameter is ignored when accessing a tiled server.
   */
  std::optional<uint32_t> minimumLevel;

  /**
   * @brief The maximum tile level to request, or undefined if there is no maximum.
   *
   * This parameter is ignored when accessing a tiled server.
   */
  std::optional<uint32_t> maximumLevel;
};

/**
 * @brief A {@link RasterOverlay} based on tile map service imagery.
 */
class CESIUM3DTILESSELECTION_API ArcGisMapServerRasterOverlay final
    : public RasterOverlay {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param url The base URL.
   * @param options The {@link ArcGisMapServerRasterOverlayOptions}.
   */
  ArcGisMapServerRasterOverlay(
      const std::string& url,
      const ArcGisMapServerRasterOverlayOptions& options =
          ArcGisMapServerRasterOverlayOptions());
  virtual ~ArcGisMapServerRasterOverlay() override;

  virtual CesiumAsync::Future<std::unique_ptr<RasterOverlayTileProvider>>
  createTileProvider(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      RasterOverlay* pOwner) override;

private:
  std::string _url;
  ArcGisMapServerRasterOverlayOptions _options;
};

} // namespace Cesium3DTilesSelection
