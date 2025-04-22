#include "CesiumGeospatial/CompositeCartographicPolygon.h"

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
  // Set up transformation matrix to transform from LLH to pixel coordinates.
  _context.translate(-bounds.getWest(), -bounds.getSouth());
  _context.scale(
      (double)this->_imageAsset->width / bounds.computeWidth(),
      (double)this->_imageAsset->height / bounds.computeHeight());
  // Sets the meta transform to the user transform and resets the user
  // transform. This lets individual render operations perform transformations
  // without affecting the transform from LLH to pixel coordinates.
  _context.userToMeta();
  // We don't want the stroke to be scaled, so set it to perform the scale
  // before we stroke.
  _context.setStrokeTransformOrder(BL_STROKE_TRANSFORM_ORDER_BEFORE);
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

  const std::vector<glm::dvec2>& vertices = polygon.getVertices();

  // Since glm::dvec2 and BLPoint are both structs containing two doubles in the
  // same order, we can treat one as the other to avoid copying.
  CESIUM_ASSERT(sizeof(BLPoint) == sizeof(glm::dvec2));
  this->_context.fillPolygon(
      reinterpret_cast<const BLPoint*>(vertices.data()),
      vertices.size(),
      style);
}

void VectorRasterizer::drawPolygon(
    const CompositeCartographicPolygon& polygon,
    const Color& color) {
  if (_finalized) {
    return;
  }

  BLRgba32 style(color.toRgba32());

  // TODO: This won't necessarily render correctly with holes at the moment, as
  // Blend2D currently just turns the polygon into a path and fills it in.
  const std::vector<glm::dvec2>& vertices = polygon.getUnindexedVertices();
  CESIUM_ASSERT(sizeof(BLPoint) == sizeof(glm::dvec2));
  this->_context.fillPolygon(
      reinterpret_cast<const BLPoint*>(vertices.data()),
      vertices.size(),
      style);
}

void VectorRasterizer::drawPolyline(
    const std::span<Cartographic>& points,
    const Color& color) {
  if (_finalized) {
    return;
  }

  BLRgba32 style(color.toRgba32());

  // Unfortunately Cartographic has an extra component that BLPoint does not, so
  // we can't use it directly.
  std::vector<BLPoint> vertices;
  vertices.reserve(points.size());

  for (Cartographic& vertex : points) {
    vertices.emplace_back(vertex.longitude, vertex.latitude);
  }

  this->_context.strokePolyline(vertices.data(), vertices.size(), style);
}

void VectorRasterizer::clear(const Color& clearColor) {
  if (_finalized) {
    return;
  }

  this->_context.fillAll(clearColor);
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