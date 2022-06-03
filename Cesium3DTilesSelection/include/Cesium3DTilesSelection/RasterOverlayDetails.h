#pragma once

#include <CesiumGeospatial/Projection.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeometry/Rectangle.h>
#include <vector>

namespace Cesium3DTilesSelection {
struct RasterOverlayDetails {
  std::vector<CesiumGeospatial::Projection> rasterOverlayProjections;

  std::vector<CesiumGeometry::Rectangle> rasterOverlayRectangles;

  CesiumGeospatial::BoundingRegion boundingRegion;

  const CesiumGeometry::Rectangle* findRectangleForOverlayProjection(
      const CesiumGeospatial::Projection& projection) const;
};
} // namespace Cesium3DTilesSelection
