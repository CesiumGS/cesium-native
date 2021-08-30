#pragma once

#include "Cesium3DTilesSelection/Library.h"
#include "Cesium3DTilesSelection/RasterOverlay.h"
#include "Cesium3DTilesSelection/RasterOverlayTileProvider.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumGeospatial/CartographicPolygon.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/Projection.h"
#include <memory>
#include <spdlog/fwd.h>
#include <string>
#include <vector>

namespace Cesium3DTilesSelection {

class CESIUM3DTILESSELECTION_API RasterizedPolygonsOverlay final
    : public RasterOverlay {

public:
  RasterizedPolygonsOverlay(
      const std::string& name,
      const std::vector<CesiumGeospatial::CartographicPolygon>& polygons,
      const CesiumGeospatial::Ellipsoid& ellipsoid,
      const CesiumGeospatial::Projection& projection);
  virtual ~RasterizedPolygonsOverlay() override;

  virtual CesiumAsync::Future<std::unique_ptr<RasterOverlayTileProvider>>
  createTileProvider(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      RasterOverlay* pOwner) override;

  const std::vector<CesiumGeospatial::CartographicPolygon>&
  getPolygons() const {
    return this->_polygons;
  }

  const std::vector<CesiumGeospatial::CartographicPolygon>&
  getClippingPolygons() const {
    return this->_clippingPolygons;
  }

private:
  std::vector<CesiumGeospatial::CartographicPolygon> _polygons;
  std::vector<CesiumGeospatial::CartographicPolygon> _clippingPolygons;
  CesiumGeospatial::Ellipsoid _ellipsoid;
  CesiumGeospatial::Projection _projection;
};
} // namespace Cesium3DTilesSelection
