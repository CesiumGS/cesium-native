#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/Color.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/VectorStyle.h>

#include <spdlog/fwd.h>

#include <functional>
#include <memory>
#include <string>
#include <variant>

namespace CesiumRasterOverlays {

/**
 * @brief A callback used to set new styles on vector documents.
 */
using GeoJsonDocumentRasterOverlayStyleCallback =
    std::function<std::optional<CesiumVectorData::VectorStyle>(
        const std::shared_ptr<CesiumVectorData::GeoJsonDocument>&,
        CesiumVectorData::GeoJsonObject*)>;

/**
 * @brief A set of options for configuring a GeoJsonDocumentRasterOverlay.
 */
struct GeoJsonDocumentRasterOverlayOptions {
  /**
   * @brief The default style to use when no style is otherwise specified on a
   * \ref GeoJsonObject.
   */
  CesiumVectorData::VectorStyle defaultStyle;

  /**
   * @brief If specified, this callback will be run for every object in the
   * document and can be used to set new styles for the objects.
   */
  std::optional<GeoJsonDocumentRasterOverlayStyleCallback> styleCallback;

  /**
   * @brief The projection to use for this overlay.
   */
  CesiumGeospatial::Projection projection;

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
 * @brief Information required to load a GeoJSON document from Cesium ion.
 */
struct IonGeoJsonDocumentRasterOverlaySource {
  /** @brief The ion Asset ID to load. */
  int64_t ionAssetID;
  /** @brief The ion access token to use to access the asset. */
  std::string ionAccessToken;
  /** @brief The URL of the Cesium ion endpoint. */
  std::string ionAssetEndpointUrl = "https://api.cesium.com";
};

/**
 * @brief Possible sources for a GeoJsonDocumentRasterOverlay's vector data.
 */
using GeoJsonDocumentRasterOverlaySource = std::variant<
    std::shared_ptr<CesiumVectorData::GeoJsonDocument>,
    IonGeoJsonDocumentRasterOverlaySource>;

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
   * @param name The user-given name of this polygon layer.
   * @param source The source of the vector data to use for the overlay.
   * @param vectorOverlayOptions Options to configure this
   * GeoJsonDocumentRasterOverlay.
   * @param overlayOptions Options to use for this RasterOverlay.
   */
  GeoJsonDocumentRasterOverlay(
      const std::string& name,
      const GeoJsonDocumentRasterOverlaySource& source,
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
  GeoJsonDocumentRasterOverlaySource _source;
  GeoJsonDocumentRasterOverlayOptions _options;
};
} // namespace CesiumRasterOverlays
