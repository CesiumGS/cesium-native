#pragma once

#include "Cesium3DTiles/CartographicSelection.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/RasterOverlay.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/Projection.h"
#include <memory>
#include <spdlog/fwd.h>
#include <string>
#include <vector>

namespace Cesium3DTiles {

class CESIUM3DTILES_API RasterizedPolygonsOverlay final : public RasterOverlay {

public:
  RasterizedPolygonsOverlay(
      const std::string& textureTargetName,
      const std::vector<CartographicSelection>& polygons,
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

  const std::vector<CartographicSelection>& getPolygons() const {
    return this->_polygons;
  }

  const std::vector<CartographicSelection>& getClippingPolygons() const {
    return this->_clippingPolygons;
  }

private:
  std::string _textureTargetName;
  std::vector<CartographicSelection> _polygons;
  std::vector<CartographicSelection> _clippingPolygons;
  CesiumGeospatial::Ellipsoid _ellipsoid;
  CesiumGeospatial::Projection _projection;
};
} // namespace Cesium3DTiles
