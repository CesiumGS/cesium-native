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
#include <CesiumVectorData/VectorStyle.h>

// blend2d.h not used directly, but if we don't include it the other blend2d
// headers will emit a warning NOLINTNEXTLINE(misc-include-cleaner)
#include <blend2d.h>
#include <blend2d/context.h>
#include <blend2d/format.h>
#include <blend2d/geometry.h>
#include <blend2d/gradient.h>
#include <blend2d/rgba.h>
#include <glm/ext/vector_double2.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
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

void setStrokeWidth(
    BLContext& context,
    const LineStyle& style,
    const Ellipsoid& ellipsoid,
    const GlobeRectangle& bounds) {
  if (style.widthMode == LineWidthMode::Pixels) {
    context.setStrokeWidth(style.width);
  } else if (style.widthMode == LineWidthMode::Meters) {
    const double radians = style.width / ellipsoid.getRadii().x;
    context.setStrokeWidth(radians / bounds.computeWidth());
  }
}
} // namespace

VectorRasterizer::VectorRasterizer(
    const GlobeRectangle& bounds,
    CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset>& imageAsset,
    uint32_t mipLevel,
    const CesiumGeospatial::Ellipsoid& ellipsoid)
    : _bounds(bounds),
      _image(),
      _context(),
      _imageAsset(imageAsset),
      _mipLevel(mipLevel),
      _ellipsoid(ellipsoid) {
  CESIUM_ASSERT(this->_imageAsset->channels == 4);
  CESIUM_ASSERT(this->_imageAsset->bytesPerChannel == 1);
  CESIUM_ASSERT(
      _mipLevel == 0 ||
      this->_imageAsset->mipPositions.size() > this->_mipLevel);
  const int32_t imageWidth =
      std::max(this->_imageAsset->width >> this->_mipLevel, 1);
  const int32_t imageHeight =
      std::max(this->_imageAsset->height >> this->_mipLevel, 1);
  std::byte* pData =
      this->_mipLevel == 0
          ? this->_imageAsset->pixelData.data()
          : this->_imageAsset->pixelData.data() +
                this->_imageAsset->mipPositions[this->_mipLevel].byteOffset;
  _image.createFromData(
      imageWidth,
      imageHeight,
      BL_FORMAT_PRGB32,
      reinterpret_cast<void*>(pData),
      (int64_t)imageWidth * (int64_t)this->_imageAsset->channels);

  _context.begin(this->_image);
  // Initialize the image as all transparent.
  _context.clearAll();
}

void VectorRasterizer::drawPolygon(
    const CartographicPolygon& polygon,
    const VectorStyle& style) {
  if (_finalized) {
    return;
  }

  std::vector<BLPoint> vertices;
  vertices.reserve(polygon.getVertices().size());

  for (const glm::dvec2& vertex : polygon.getVertices()) {
    vertices.emplace_back(
        radiansToPoint(vertex.x, vertex.y, this->_bounds, this->_context));
  }

  if (style.polygon.fill) {
    this->_context.fillPolygon(
        vertices.data(),
        vertices.size(),
        BLRgba32(style.polygon.getColor().toRgba32()));
  }

  if (style.polygon.outline) {
    setStrokeWidth(this->_context, style.line, this->_ellipsoid, this->_bounds);
    this->_context.strokePolygon(
        vertices.data(),
        vertices.size(),
        BLRgba32(style.line.getColor().toRgba32()));
  }
}

void VectorRasterizer::drawPolygon(
    const CompositeCartographicPolygon& polygon,
    const VectorStyle& style) {
  if (_finalized || (!style.polygon.fill && !style.polygon.outline)) {
    return;
  }

  std::vector<BLPoint> vertices;
  vertices.reserve(polygon.getWoundVertices().size());

  for (const glm::dvec2& vertex : polygon.getWoundVertices()) {
    vertices.emplace_back(
        radiansToPoint(vertex.x, vertex.y, this->_bounds, this->_context));
  }

  if (style.polygon.fill) {
    this->_context.fillPolygon(
        vertices.data(),
        vertices.size(),
        BLRgba32(style.polygon.getColor().toRgba32()));
  }

  if (style.polygon.outline) {
    setStrokeWidth(this->_context, style.line, this->_ellipsoid, this->_bounds);
    this->_context.strokePolygon(
        vertices.data(),
        vertices.size(),
        BLRgba32(style.line.getColor().toRgba32()));
  }
}

void VectorRasterizer::drawPolyline(
    const std::span<const Cartographic>& points,
    const VectorStyle& style) {
  if (_finalized) {
    return;
  }

  std::vector<BLPoint> vertices;
  vertices.reserve(points.size());

  glm::dvec2 min(
      std::numeric_limits<double>::max(),
      std::numeric_limits<double>::max());
  glm::dvec2 max(
      std::numeric_limits<double>::min(),
      std::numeric_limits<double>::min());

  for (const Cartographic& vertex : points) {
    BLPoint point = radiansToPoint(
        vertex.longitude,
        vertex.latitude,
        this->_bounds,
        this->_context);
    vertices.emplace_back(point);
    min.x = std::min(min.x, point.x);
    min.y = std::min(min.y, point.y);
    max.x = std::max(max.x, point.x);
    max.y = std::max(max.y, point.y);
  }

  setStrokeWidth(this->_context, style.line, this->_ellipsoid, this->_bounds);

  this->_context.strokePolyline(
      vertices.data(),
      vertices.size(),
      BLRgba32(style.line.getColor().toRgba32()));
}

void VectorRasterizer::drawPrimitive(
    const VectorPrimitive& primitive,
    const VectorStyle& style) {
  struct PrimitiveDrawVisitor {
    VectorRasterizer& rasterizer;
    const VectorStyle& style;
    void operator()(const Cartographic& /*point*/) {}
    void operator()(const std::vector<Cartographic>& points) {
      rasterizer.drawPolyline(points, style);
    }
    void operator()(const CompositeCartographicPolygon& polygon) {
      rasterizer.drawPolygon(polygon, style);
    }
  };

  std::visit(PrimitiveDrawVisitor{*this, style}, primitive);
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
  std::byte* pData =
      this->_mipLevel == 0
          ? this->_imageAsset->pixelData.data()
          : this->_imageAsset->pixelData.data() +
                this->_imageAsset->mipPositions[this->_mipLevel].byteOffset;
  const size_t size =
      this->_mipLevel == 0
          ? this->_imageAsset->pixelData.size()
          : this->_imageAsset->mipPositions[this->_mipLevel].byteSize;
  for (size_t i = 0; i < size; i += 4) {
    // We need to turn BGRA to RGBA, but this is little endian so it's really
    // ARGB to ABGR.
    uint32_t* pPixel = reinterpret_cast<uint32_t*>(pData + i);
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
