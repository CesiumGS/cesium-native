#pragma once

#include "Library.h"
#include "RasterOverlay.h"
#include "RasterOverlayTileProvider.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/Projection.h>

#include <spdlog/fwd.h>

#include <memory>
#include <string>
#include <vector>

namespace Cesium3DTilesSelection {

class CESIUM3DTILESSELECTION_API RasterizedPolygonsOverlay final
    : public RasterOverlay {

public:
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
      const std::shared_ptr<CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner)
      const override;

  const std::vector<CesiumGeospatial::CartographicPolygon>&
  getPolygons() const noexcept {
    return this->_polygons;
  }

  bool getInvertSelection() const noexcept { return this->_invertSelection; }

private:
  std::vector<CesiumGeospatial::CartographicPolygon> _polygons;
  bool _invertSelection;
  CesiumGeospatial::Ellipsoid _ellipsoid;
  CesiumGeospatial::Projection _projection;
};
} // namespace Cesium3DTilesSelection
