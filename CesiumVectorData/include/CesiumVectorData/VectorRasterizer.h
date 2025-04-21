#pragma once

#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/CompositeCartographicPolygon.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <blend2d/context.h>
#include <blend2d/image.h>

#include <cstddef>

namespace CesiumVectorData {

struct Color {
  std::byte r;
  std::byte g;
  std::byte b;
  std::byte a;

  uint32_t toRgba32() const;
};

class VectorRasterizer {
public:
  VectorRasterizer(
      const CesiumGeospatial::GlobeRectangle& bounds,
      CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset>& imageAsset);

  void drawPolygon(
      const CesiumGeospatial::CartographicPolygon& polygon,
      const Color& drawColor);

  CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> finalize();

private:
  CesiumGeospatial::GlobeRectangle _bounds;
  BLImage _image;
  BLContext _context;
  CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> _imageAsset;
};
} // namespace CesiumVectorData