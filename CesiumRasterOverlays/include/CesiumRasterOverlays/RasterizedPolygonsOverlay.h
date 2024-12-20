#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>

#include <spdlog/fwd.h>

#include <memory>
#include <string>
#include <vector>

namespace CesiumRasterOverlays {

/**
 * @brief A raster overlay made from rasterizing a set of \ref
 * CesiumGeospatial::CartographicPolygon "CartographicPolygon" objects. The
 * resulting overlay is monochromatic - white where pixels are inside of the
 * polygons, and black where they are not.
 */
class CESIUMRASTEROVERLAYS_API RasterizedPolygonsOverlay final
    : public RasterOverlay {

public:
  /**
   * @brief Creates a new RasterizedPolygonsOverlay.
   *
   * @param name The user-given name of this polygon layer.
   * @param polygons The \ref CesiumGeospatial::CartographicPolygon
   * "CartographicPolygon" objects to rasterize.
   * @param invertSelection If true, the overlay's colors will be inverted. The
   * pixels inside of polygons will be black, and those outside will be white.
   * @param ellipsoid The ellipsoid that this RasterOverlay is being generated
   * for.
   * @param projection The projection that this RasterOverlay is being generated
   * for.
   * @param overlayOptions Options to use for this RasterOverlay.
   */
  RasterizedPolygonsOverlay(
      const std::string& name,
      const std::vector<CesiumGeospatial::CartographicPolygon>& polygons,
      bool invertSelection,
      const CesiumGeospatial::Ellipsoid& ellipsoid,
      const CesiumGeospatial::Projection& projection,
      const RasterOverlayOptions& overlayOptions = {});
  virtual ~RasterizedPolygonsOverlay() override;

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
   * @brief Gets the polygons that are being rasterized to create this overlay.
   */
  const std::vector<CesiumGeospatial::CartographicPolygon>&
  getPolygons() const noexcept {
    return this->_polygons;
  }

  /**
   * @brief Gets the value of the `invertSelection` value passed to the
   * constructor.
   */
  bool getInvertSelection() const noexcept { return this->_invertSelection; }

  /**
   * @brief Gets the ellipsoid that this overlay is being generated for.
   */
  const CesiumGeospatial::Ellipsoid& getEllipsoid() const noexcept {
    return this->_ellipsoid;
  }

private:
  std::vector<CesiumGeospatial::CartographicPolygon> _polygons;
  bool _invertSelection;
  CesiumGeospatial::Ellipsoid _ellipsoid;
  CesiumGeospatial::Projection _projection;
};
} // namespace CesiumRasterOverlays
