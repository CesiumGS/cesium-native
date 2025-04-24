#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumVectorData/Color.h>
#include <CesiumVectorData/VectorDocument.h>

#include <spdlog/fwd.h>

#include <memory>
#include <string>

namespace CesiumRasterOverlays {

/**
 * @brief A raster overlay made from rasterizing a \ref
 * CesiumVectorData::VectorDocument.
 */
class CESIUMRASTEROVERLAYS_API VectorDocumentRasterOverlay final
    : public RasterOverlay {

public:
  /**
   * @brief Creates a new RasterizedPolygonsOverlay.
   *
   * @param name The user-given name of this polygon layer.
   * @param document The \ref CesiumVectorData::VectorDocument to rasterize.
   * @param color The color of the vector data in this raster overlay.
   * @param ellipsoid The ellipsoid that this RasterOverlay is being generated
   * for.
   * @param projection The projection that this RasterOverlay is being generated
   * for.
   * @param overlayOptions Options to use for this RasterOverlay.
   */
  VectorDocumentRasterOverlay(
      const std::string& name,
      const CesiumUtility::IntrusivePointer<CesiumVectorData::VectorDocument>&
          document,
      const CesiumVectorData::Color& color,
      const CesiumGeospatial::Projection& projection,
      const RasterOverlayOptions& overlayOptions = {});
  virtual ~VectorDocumentRasterOverlay() override;

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
  CesiumUtility::IntrusivePointer<CesiumVectorData::VectorDocument> _document;
  CesiumVectorData::Color _color;
  CesiumGeospatial::Projection _projection;
};
} // namespace CesiumRasterOverlays
