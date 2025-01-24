#pragma once

#include "IPrepareRasterOverlayRendererResources.h"
#include "Library.h"
#include "RasterOverlayTileProvider.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/SharedAssetDepot.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/Result.h>
#include <CesiumUtility/SharedAsset.h>

#include <list>
#include <memory>
#include <optional>
#include <string>

namespace CesiumRasterOverlays {

/**
 * @brief Options for URL template overlays.
 */
struct UrlTemplateRasterOverlayOptions {
  /**
   * @brief A credit for the data source, which is displayed on the canvas.
   */
  std::optional<std::string> credit;

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
   * @brief The minimum level-of-detail supported by the imagery provider.
   *
   * Take care when specifying this that the number of tiles at the minimum
   * level is small, such as four or less. A larger number is likely to
   * result in rendering problems.
   */
  uint32_t minimumLevel = 0;

  /**
   * @brief The maximum level-of-detail supported by the imagery provider.
   */
  uint32_t maximumLevel = 25;

  /**
   * @brief Pixel width of image tiles.
   */
  uint32_t tileWidth = 256;

  /**
   * @brief Pixel height of image tiles.
   */
  uint32_t tileHeight = 256;

  /**
   * @brief The {@link CesiumGeometry::Rectangle}, in radians, covered by the
   * image.
   */
  std::optional<CesiumGeometry::Rectangle> coverageRectangle;
};

/**
 * @brief A {@link RasterOverlay} accessing images from a templated URL.
 */
class CESIUMRASTEROVERLAYS_API UrlTemplateRasterOverlay final
    : public RasterOverlay {
public:
  /**
   * @brief Creates a new instance.
   *
   * The following template parameters are supported in `url`:
   * - `{x}` - The tile X coordinate in the tiling scheme, where 0 is the westernmost tile.
   * - `{y}` - The tile Y coordinate in the tiling scheme, where 0 is the nothernmost tile.
   * - `{z}` - The level of the tile in the tiling scheme, where 0 is the root of the quadtree pyramid.
   * - `{reverseX}` - The tile X coordinate in the tiling scheme, where 0 is the easternmost tile.
   * - `{reverseY}` - The tile Y coordinate in the tiling scheme, where 0 is the southernmost tile.
   * - `{reverseZ}` - The tile Z coordinate in the tiling scheme, where 0 is equivalent to `urlTemplateOptions.maximumLevel`.
   * - `{westDegrees}` - The western edge of the tile in geodetic degrees.
   * - `{southDegrees}` - The southern edge of the tile in geodetic degrees.
   * - `{eastDegrees}` - The eastern edge of the tile in geodetic degrees.
   * - `{northDegrees}` - The northern edge of the tile in geodetic degrees.
   * - `{minimumX}` - The minimum X coordinate of the tile's projected coordinates.
   * - `{minimumY}` - The minimum Y coordinate of the tile's projected coordinates.
   * - `{maximumX}` - The maximum X coordinate of the tile's projected coordinates.
   * - `{maximumY}` - The maximum Y coordinate of the tile's projected coordinates.
   * - `{width}` - The width of each tile in pixels.
   * - `{height}` - The height of each tile in pixels.
   *
   * @param name The user-given name of this overlay layer.
   * @param url The URL with template parameters.
   * @param headers The headers. This is a list of pairs of strings of the
   * form (Key,Value) that will be inserted as request headers internally.
   * @param urlTemplateOptions The {@link UrlTemplateRasterOverlayOptions}.
   * @param overlayOptions The {@link RasterOverlayOptions} for this instance.
   */
  UrlTemplateRasterOverlay(
      const std::string& name,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers = {},
      const UrlTemplateRasterOverlayOptions& urlTemplateOptions = {},
      const RasterOverlayOptions& overlayOptions = {})
      : RasterOverlay(name, overlayOptions),
        _url(url),
        _headers(headers),
        _options(urlTemplateOptions) {}

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
  UrlTemplateRasterOverlayOptions _options;
};
} // namespace CesiumRasterOverlays
