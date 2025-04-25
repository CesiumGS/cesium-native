#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/CompositeCartographicPolygon.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumVectorData/Color.h>
#include <CesiumVectorData/VectorNode.h>
#include <CesiumVectorData/VectorRasterizer.h>

// blend2d.h not used directly, but if we don't include it the other blend2d
// headers will emit a warning NOLINTNEXTLINE(misc-include-cleaner)
#include <blend2d.h>
#include <blend2d/context.h>
#include <blend2d/format.h>
#include <blend2d/geometry.h>
#include <blend2d/rgba.h>
#include <glm/ext/vector_double2.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <variant>
#include <vector>

using namespace CesiumGeospatial;
using namespace CesiumGeometry;
using namespace CesiumGltf;

namespace CesiumVectorData {
namespace {
BLPoint radiansToPoint(
    double longitude,
    double latitude,
    const GlobeRectangle& rect,
    const BLContext& context) {
  return BLPoint(
      (longitude - rect.getWest()) / rect.computeWidth() *
          context.targetWidth(),
      (1.0 - (latitude - rect.getSouth()) / rect.computeHeight()) *
          context.targetHeight());
}
} // namespace

VectorRasterizer::VectorRasterizer(
    const GlobeRectangle& bounds,
    CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset>& imageAsset)
    : _bounds(bounds), _image(), _context(), _imageAsset(imageAsset) {
  CESIUM_ASSERT(imageAsset->channels == 4);
  CESIUM_ASSERT(imageAsset->bytesPerChannel == 1);
  _image.createFromData(
      this->_imageAsset->width,
      this->_imageAsset->height,
      BL_FORMAT_PRGB32,
      reinterpret_cast<void*>(this->_imageAsset->pixelData.data()),
      (int64_t)this->_imageAsset->width * (int64_t)this->_imageAsset->channels);

  _context.begin(this->_image);
  // Initialize the image as all transparent.
  _context.clearAll();
}

void VectorRasterizer::drawPolygon(
    const CartographicPolygon& polygon,
    const Color& color) {
  if (_finalized) {
    return;
  }

  BLRgba32 style(color.toRgba32());
  std::vector<BLPoint> vertices;
  vertices.reserve(polygon.getVertices().size());

  for (const glm::dvec2& vertex : polygon.getVertices()) {
    vertices.emplace_back(
        radiansToPoint(vertex.x, vertex.y, this->_bounds, this->_context));
  }

  CESIUM_ASSERT(sizeof(BLPoint) == sizeof(glm::dvec2));
  this->_context.fillPolygon(vertices.data(), vertices.size(), style);
}

void VectorRasterizer::drawPolygon(
    const CompositeCartographicPolygon& polygon,
    const Color& color) {
  if (_finalized) {
    return;
  }

  BLRgba32 style(color.toRgba32());

  std::vector<BLPoint> vertices;
  vertices.reserve(polygon.getWoundVertices().size());

  for (const glm::dvec2& vertex : polygon.getWoundVertices()) {
    vertices.emplace_back(
        radiansToPoint(vertex.x, vertex.y, this->_bounds, this->_context));
  }

  this->_context.fillPolygon(vertices.data(), vertices.size(), style);
}

void VectorRasterizer::drawPolyline(
    const std::span<const Cartographic>& points,
    const Color& color) {
  if (_finalized) {
    return;
  }

  BLRgba32 style(color.toRgba32());

  std::vector<BLPoint> vertices;
  vertices.reserve(points.size());

  for (const Cartographic& vertex : points) {
    vertices.emplace_back(radiansToPoint(
        vertex.longitude,
        vertex.latitude,
        this->_bounds,
        this->_context));
  }

  // this->_context.setStrokeWidth(1.0 / this->_context.targetWidth());
  this->_context.strokePolyline(vertices.data(), vertices.size(), style);
}

void VectorRasterizer::drawPrimitive(
    const VectorPrimitive& primitive,
    const Color& drawColor) {
  struct PrimitiveDrawVisitor {
    VectorRasterizer& rasterizer;
    const Color& drawColor;
    void operator()(const Cartographic& /*point*/) {}
    void operator()(const std::vector<Cartographic>& points) {
      rasterizer.drawPolyline(points, drawColor);
    }
    void operator()(const CompositeCartographicPolygon& polygon) {
      rasterizer.drawPolygon(polygon, drawColor);
    }
  };

  std::visit(PrimitiveDrawVisitor{*this, drawColor}, primitive);
}

void VectorRasterizer::clear(const Color& clearColor) {
  if (_finalized) {
    return;
  }

  this->_context.fillAll(BLRgba32(clearColor.toRgba32()));
}

CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset>
VectorRasterizer::finalize() {
  if (_finalized) {
    return this->_imageAsset;
  }

  this->_context.end();

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

  _finalized = true;
  return this->_imageAsset;
}

} // namespace CesiumVectorData