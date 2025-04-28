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
#include <CesiumVectorData/VectorRasterizerStyle.h>

#include <spdlog/fwd.h>

#include <memory>
#include <string>
#include <variant>

namespace CesiumRasterOverlays {

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
    CesiumUtility::IntrusivePointer<CesiumVectorData::VectorDocument>,
    IonVectorDocumentRasterOverlaySource>;

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
   * @param document The source of the vector data to use for the overlay.
   * @param style The style to use for rasterizing vector data.
   * @param projection The projection that this RasterOverlay is being generated
   * for.
   * @param mipLevels The number of mip levels to generate.
   * @param overlayOptions Options to use for this RasterOverlay.
   */
  VectorDocumentRasterOverlay(
      const std::string& name,
      const VectorDocumentRasterOverlaySource& source,
      const CesiumVectorData::VectorRasterizerStyle& style,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeospatial::Ellipsoid& ellipsoid,
      uint32_t mipLevels = 0,
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
  CesiumVectorData::VectorRasterizerStyle _style;
  CesiumGeospatial::Ellipsoid _ellipsoid;
  CesiumGeospatial::Projection _projection;
  uint32_t _mipLevels;
};
} // namespace CesiumRasterOverlays
