#pragma once

#include "CesiumITwinClient/Connection.h"

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
#include <CesiumVectorData/VectorStyle.h>

#include <spdlog/fwd.h>

#include <memory>
#include <string>
#include <variant>

namespace CesiumITwinClient {

/**
 * @brief A raster overlay made from rasterizing a \ref
 * CesiumVectorData::VectorDocument.
 */
class CESIUMRASTEROVERLAYS_API ITwinGeospatialFeaturesRasterOverlay final
    : public CesiumRasterOverlays::RasterOverlay {

public:
  /**
   * @brief Creates a new ITwinGeospatialFeaturesRasterOverlay.
   *
   * @param name The user-given name of this polygon layer.
   * @param iTwinId The ID of the iTwin to obtain the features from.
   * @param collectionId The ID of the Geospatial Features Collection to obtain
   * the features from.
   * @param style The style to use for rasterizing vector data.
   * @param projection The projection that this RasterOverlay is being generated
   * for.
   * @param mipLevels The number of mip levels to generate.
   * @param overlayOptions Options to use for this RasterOverlay.
   */
  ITwinGeospatialFeaturesRasterOverlay(
      const std::string& name,
      const std::string& iTwinId,
      const std::string& collectionId,
      const CesiumUtility::IntrusivePointer<Connection>& pConnection,
      const CesiumVectorData::VectorStyle& style,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeospatial::Ellipsoid& ellipsoid,
      uint32_t mipLevels = 0,
      const CesiumRasterOverlays::RasterOverlayOptions& overlayOptions = {});
  virtual ~ITwinGeospatialFeaturesRasterOverlay() override;

  virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
      const std::shared_ptr<
          CesiumRasterOverlays::IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner)
      const override;

private:
  std::string _iTwinId;
  std::string _collectionId;
  CesiumUtility::IntrusivePointer<Connection> _pConnection;
  CesiumVectorData::VectorStyle _style;
  CesiumGeospatial::Ellipsoid _ellipsoid;
  CesiumGeospatial::Projection _projection;
  uint32_t _mipLevels;
};
} // namespace CesiumITwinClient
