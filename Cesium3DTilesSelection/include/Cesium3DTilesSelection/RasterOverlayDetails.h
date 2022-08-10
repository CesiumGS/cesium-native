#pragma once

#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Projection.h>

#include <vector>

namespace Cesium3DTilesSelection {
struct RasterOverlayDetails {
  RasterOverlayDetails();

  RasterOverlayDetails(
      std::vector<CesiumGeospatial::Projection>&& rasterOverlayProjections,
      std::vector<CesiumGeometry::Rectangle>&& rasterOverlayRectangles,
      const CesiumGeospatial::BoundingRegion& boundingRegion);

  const CesiumGeometry::Rectangle* findRectangleForOverlayProjection(
      const CesiumGeospatial::Projection& projection) const;

  void merge(RasterOverlayDetails&& other);

  std::vector<CesiumGeospatial::Projection> rasterOverlayProjections;

  std::vector<CesiumGeometry::Rectangle> rasterOverlayRectangles;

  CesiumGeospatial::BoundingRegion boundingRegion;
};
} // namespace Cesium3DTilesSelection
