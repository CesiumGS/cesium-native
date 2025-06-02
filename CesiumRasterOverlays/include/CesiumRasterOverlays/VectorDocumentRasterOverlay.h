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
using VectorDocumentRasterOverlayStyleCallback =
    std::function<std::optional<CesiumVectorData::VectorStyle>(
        const std::shared_ptr<CesiumVectorData::GeoJsonDocument>&,
        CesiumVectorData::GeoJsonObject*)>;

/**
 * @brief A set of options for configuring a VectorDocumentRasterOverlay.
 */
struct VectorDocumentRasterOverlayOptions {
  /**
   * @brief The default style to use when no style is otherwise specified on a
   * \ref GeoJsonObject.
   */
  CesiumVectorData::VectorStyle defaultStyle;

  /**
   * @brief If specified, this callback will be run for every node in the
   * document and can be used to set new styles for the nodes.
   */
  std::optional<VectorDocumentRasterOverlayStyleCallback> styleCallback;

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
 * @brief Information required to load a vector document from Cesium ion.
 */
struct IonVectorDocumentRasterOverlaySource {
  /** @brief The ion Asset ID to load. */
  int64_t ionAssetID;
  /** @brief The ion access token to use to access the asset. */
  std::string ionAccessToken;
  /** @brief The URL of the Cesium ion endpoint. */
  std::string ionAssetEndpointUrl = "https://api.cesium.com";
};

/**
 * @brief Possible sources for a VectorDocumentRasterOverlay's vector data.
 */
using VectorDocumentRasterOverlaySource = std::variant<
    std::shared_ptr<CesiumVectorData::GeoJsonDocument>,
    IonVectorDocumentRasterOverlaySource>;

/**
 * @brief A raster overlay made from rasterizing a \ref
 * CesiumVectorData::GeoJsonDocument.
 */
class CESIUMRASTEROVERLAYS_API VectorDocumentRasterOverlay final
    : public RasterOverlay {

public:
  /**
   * @brief Creates a new RasterizedPolygonsOverlay.
   *
   * @param name The user-given name of this polygon layer.
   * @param source The source of the vector data to use for the overlay.
   * @param vectorOverlayOptions Options to configure this
   * VectorDocumentRasterOverlay.
   * @param overlayOptions Options to use for this RasterOverlay.
   */
  VectorDocumentRasterOverlay(
      const std::string& name,
      const VectorDocumentRasterOverlaySource& source,
      const VectorDocumentRasterOverlayOptions& vectorOverlayOptions,
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
  VectorDocumentRasterOverlaySource _source;
  VectorDocumentRasterOverlayOptions _options;
};
} // namespace CesiumRasterOverlays
