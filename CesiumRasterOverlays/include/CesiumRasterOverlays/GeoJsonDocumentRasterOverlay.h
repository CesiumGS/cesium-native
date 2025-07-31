#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/Color.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/VectorStyle.h>

#include <spdlog/fwd.h>

#include <memory>
#include <string>

namespace CesiumRasterOverlays {

/**
 * @brief A set of options for configuring a GeoJsonDocumentRasterOverlay.
 */
struct GeoJsonDocumentRasterOverlayOptions {
  /**
   * @brief The default style to use when no style is otherwise specified on a
   * \ref CesiumVectorData::GeoJsonObject.
   */
  CesiumVectorData::VectorStyle defaultStyle;

  /**
   * @brief The ellipsoid to use for this overlay.
   */
  CesiumGeospatial::Ellipsoid ellipsoid;

  /**
   * @brief The number of mip levels to generate.
   */
  uint32_t mipLevels = 0;
};

/**
 * @brief A raster overlay made from rasterizing a \ref
 * CesiumVectorData::GeoJsonDocument.
 */
class CESIUMRASTEROVERLAYS_API GeoJsonDocumentRasterOverlay final
    : public RasterOverlay {

public:
  /**
   * @brief Creates a new GeoJsonDocumentRasterOverlay.
   *
   * @param asyncSystem The async system to use.
   * @param name The user-given name of this polygon layer.
   * @param document The GeoJSON document to use for the overlay.
   * @param vectorOverlayOptions Options to configure this
   * GeoJsonDocumentRasterOverlay.
   * @param overlayOptions Options to use for this RasterOverlay.
   */
  GeoJsonDocumentRasterOverlay(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& name,
      const std::shared_ptr<CesiumVectorData::GeoJsonDocument>& document,
      const GeoJsonDocumentRasterOverlayOptions& vectorOverlayOptions,
      const RasterOverlayOptions& overlayOptions = {});

  /**
   * @brief Creates a new GeoJsonDocumentRasterOverlay from a future.
   *
   * @param name The user-given name of this polygon layer.
   * @param documentFuture A future resolving into a GeoJSON document to use for
   * the overlay.
   * @param vectorOverlayOptions Options to configure this
   * GeoJsonDocumentRasterOverlay.
   * @param overlayOptions Options to use for this RasterOverlay.
   */
  GeoJsonDocumentRasterOverlay(
      const std::string& name,
      CesiumAsync::Future<std::shared_ptr<CesiumVectorData::GeoJsonDocument>>&&
          documentFuture,
      const GeoJsonDocumentRasterOverlayOptions& vectorOverlayOptions,
      const RasterOverlayOptions& overlayOptions = {});

  virtual ~GeoJsonDocumentRasterOverlay() override;

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
  CesiumAsync::Future<std::shared_ptr<CesiumVectorData::GeoJsonDocument>>
      _documentFuture;
  GeoJsonDocumentRasterOverlayOptions _options;
};
} // namespace CesiumRasterOverlays
