#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumITwinClient/Connection.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/VectorDocumentRasterOverlay.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumVectorData/Color.h>
#include <CesiumVectorData/VectorDocument.h>
#include <CesiumVectorData/VectorStyle.h>

#include <spdlog/fwd.h>

#include <memory>
#include <string>

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
   * @param pConnection The connection to the iTwin API to use.
   * @param vectorOptions Options to configure the vector overlay.
   * @param overlayOptions Options to use for this RasterOverlay.
   */
  ITwinGeospatialFeaturesRasterOverlay(
      const std::string& name,
      const std::string& iTwinId,
      const std::string& collectionId,
      const CesiumUtility::IntrusivePointer<Connection>& pConnection,
      const CesiumRasterOverlays::VectorDocumentRasterOverlayOptions&
          vectorOptions,
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
  CesiumRasterOverlays::VectorDocumentRasterOverlayOptions _options;
};
} // namespace CesiumITwinClient
