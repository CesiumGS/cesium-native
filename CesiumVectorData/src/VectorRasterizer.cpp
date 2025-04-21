#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumVectorData/VectorRasterizer.h>

#include <blend2d/format.h>
#include <blend2d/geometry.h>
#include <blend2d/path.h>
#include <spdlog/spdlog.h>

#include <cstddef>

using namespace CesiumGeospatial;
using namespace CesiumGeometry;
using namespace CesiumGltf;

namespace CesiumVectorData {
uint32_t Color::toRgba32() const {
  return (uint32_t)this->a << 24 | (uint32_t)this->r << 16 |
         (uint32_t)this->g << 8 | (uint32_t)this->b;
}

VectorRasterizer::VectorRasterizer(
    const CesiumGeospatial::GlobeRectangle& bounds,
    CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset>& imageAsset)
    : _bounds(bounds), _image(), _context(), _imageAsset(imageAsset) {
  CESIUM_ASSERT(imageAsset->channels == 1 || imageAsset->channels == 4);
  CESIUM_ASSERT(imageAsset->bytesPerChannel == 1);
  _image.createFromData(
      this->_imageAsset->width,
      this->_imageAsset->height,
      this->_imageAsset->channels == 1 ? BL_FORMAT_A8 : BL_FORMAT_PRGB32,
      reinterpret_cast<void*>(this->_imageAsset->pixelData.data()),
      (int64_t)this->_imageAsset->width * (int64_t)this->_imageAsset->channels);
  _context.begin(this->_image);
  // Initialize the image as all transparent.
  _context.clearAll();
}

void VectorRasterizer::drawPolygon(
    const CesiumGeospatial::CartographicPolygon& polygon,
    const Color& color) {
  BLRgba32 style(color.toRgba32());

  const double boundsWidth = this->_bounds.computeWidth();
  const double boundsHeight = this->_bounds.computeHeight();

  std::vector<BLPoint> points;
  const std::vector<glm::dvec2>& vertices = polygon.getVertices();
  for (const uint32_t& idx : polygon.getIndices()) {
    const glm::dvec2& vertex = vertices[idx];
    points.emplace_back(
        (vertex.x - this->_bounds.getWest()) / boundsWidth *
            this->_image.width(),
        (vertex.y - this->_bounds.getSouth()) / boundsHeight *
            this->_image.height());
  }

  this->_context.fillPolygon(points.data(), points.size(), style);
}

CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset>
VectorRasterizer::finalize() {
  this->_context.end();

  this->_image.writeToFile("triangle.png");

  if (this->_imageAsset->channels == 4) {
    // Blend2D writes in BGRA whereas ImageAsset is RGBA.
    // We need to swap the channels to fix the values.
    // TODO: use BLPixelConverter which has SIMD support
    std::vector<std::byte>& pixelData = this->_imageAsset->pixelData;
    for (size_t i = 0; i < pixelData.size(); i += 4) {
      // We need to turn BGRA to RGBA, but this is little endian so it's really
      // ARGB to ABGR.
      uint32_t* pPixel = reinterpret_cast<uint32_t*>(pixelData.data() + i);
      const uint32_t pixel = *pPixel;
      const uint32_t newPixel =
          (pixel & 0xff000000) | ((pixel & 0x00ff0000) >> 16) |
          (pixel & 0x0000ff00) | ((pixel & 0x000000ff) << 16);
      *pPixel = newPixel;
    }
  }

  return this->_imageAsset;
}

} // namespace CesiumVectorData