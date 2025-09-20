#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/Color.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Math.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/GeoJsonObjectTypes.h>
#include <CesiumVectorData/VectorRasterizer.h>
#include <CesiumVectorData/VectorStyle.h>

// blend2d.h not used directly, but if we don't include it the other blend2d
// headers will emit a warning NOLINTNEXTLINE(misc-include-cleaner)
#include <blend2d.h>
#include <blend2d/context.h>
#include <blend2d/format.h>
#include <blend2d/geometry.h>
#include <blend2d/rgba.h>
#include <glm/ext/vector_double2.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
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
    context.setStrokeWidth(
        (context.targetWidth() * style.width) /
        (bounds.computeWidth() * ellipsoid.getRadii().x));
  }
}

template <typename T> size_t seedForObject(const T& object, size_t base) {
  return base ^ reinterpret_cast<size_t>(&object);
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
  this->_image.createFromData(
      imageWidth,
      imageHeight,
      BL_FORMAT_PRGB32,
      reinterpret_cast<void*>(pData),
      (int64_t)imageWidth * (int64_t)this->_imageAsset->channels);

  this->_context.begin(this->_image);
  // Initialize the image as all transparent.
  this->_context.clearAll();
}

void VectorRasterizer::drawPolygon(
    const CesiumGeospatial::CartographicPolygon& polygon,
    const PolygonStyle& style) {
  if (this->_finalized || (!style.fill && !style.outline)) {
    return;
  }

  std::vector<BLPoint> vertices;
  vertices.reserve(polygon.getVertices().size());

  for (const glm::dvec2& pos : polygon.getVertices()) {
    vertices.emplace_back(
        radiansToPoint(pos.x, pos.y, this->_bounds, this->_context));
  }

  if (style.fill) {
    this->_context.fillPolygon(
        vertices.data(),
        vertices.size(),
        BLRgba32(style.fill->getColor(seedForObject(polygon, 13)).toRgba32()));
  }

  if (style.outline) {
    setStrokeWidth(
        this->_context,
        *style.outline,
        this->_ellipsoid,
        this->_bounds);
    this->_context.strokePolygon(
        vertices.data(),
        vertices.size(),
        BLRgba32(
            style.outline->getColor(seedForObject(polygon, 31)).toRgba32()));
  }
}

void VectorRasterizer::drawPolygon(
    const std::vector<std::vector<glm::dvec3>>& polygon,
    const PolygonStyle& style) {
  if (this->_finalized || (!style.fill && !style.outline)) {
    return;
  }

  std::vector<BLPoint> vertices;
  vertices.reserve(polygon.size());

  for (const std::vector<glm::dvec3>& ring : polygon) {
    // GeoJSON polygons have the reverse winding order from blend2D
    for (auto it = ring.rbegin(); it != ring.rend(); ++it) {
      vertices.emplace_back(radiansToPoint(
          CesiumUtility::Math::degreesToRadians(it->x),
          CesiumUtility::Math::degreesToRadians(it->y),
          this->_bounds,
          this->_context));
    }
  }

  if (style.fill) {
    this->_context.fillPolygon(
        vertices.data(),
        vertices.size(),
        BLRgba32(style.fill->getColor(seedForObject(polygon, 13)).toRgba32()));
  }

  if (style.outline) {
    setStrokeWidth(
        this->_context,
        *style.outline,
        this->_ellipsoid,
        this->_bounds);
    this->_context.strokePolygon(
        vertices.data(),
        vertices.size(),
        BLRgba32(
            style.outline->getColor(seedForObject(polygon, 31)).toRgba32()));
  }
}

void VectorRasterizer::drawPolyline(
    const std::vector<glm::dvec3>& points,
    const LineStyle& style) {
  if (this->_finalized) {
    return;
  }

  std::vector<BLPoint> vertices;
  vertices.reserve(points.size());

  for (const glm::dvec3& vertex : points) {
    BLPoint point = radiansToPoint(
        CesiumUtility::Math::degreesToRadians(vertex.x),
        CesiumUtility::Math::degreesToRadians(vertex.y),
        this->_bounds,
        this->_context);
    vertices.emplace_back(point);
  }

  setStrokeWidth(this->_context, style, this->_ellipsoid, this->_bounds);

  this->_context.strokePolyline(
      vertices.data(),
      vertices.size(),
      BLRgba32(style.getColor(seedForObject(points, 31)).toRgba32()));
}

void VectorRasterizer::drawGeoJsonObject(
    const GeoJsonObject& geoJsonObject,
    const VectorStyle& style) {
  struct PrimitiveDrawVisitor {
    VectorRasterizer& rasterizer;
    const VectorStyle& style;
    void operator()(const GeoJsonLineString& line) {
      rasterizer.drawPolyline(line.coordinates, style.line);
    }
    void operator()(const GeoJsonMultiLineString& lines) {
      for (const std::vector<glm::dvec3>& line : lines.coordinates) {
        rasterizer.drawPolyline(line, style.line);
      }
    }
    void operator()(const GeoJsonPolygon& polygon) {
      rasterizer.drawPolygon(polygon.coordinates, style.polygon);
    }
    void operator()(const GeoJsonMultiPolygon& polygons) {
      for (const std::vector<std::vector<glm::dvec3>>& polygon :
           polygons.coordinates) {
        rasterizer.drawPolygon(polygon, style.polygon);
      }
    }
    void operator()(const GeoJsonPoint& /*catchAll*/) {}
    void operator()(const GeoJsonMultiPoint& /*catchAll*/) {}
    void operator()(const GeoJsonFeature& /*catchAll*/) {}
    void operator()(const GeoJsonFeatureCollection& /*catchAll*/) {}
    void operator()(const GeoJsonGeometryCollection& /*catchAll*/) {}
  };

  for (const GeoJsonObject& object : geoJsonObject) {
    object.visit(PrimitiveDrawVisitor{*this, style});
  }
}

void VectorRasterizer::clear(const CesiumUtility::Color& clearColor) {
  if (this->_finalized) {
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
  // Blend2D provides BLPixelConverter for these sorts of operations, which
  // should be faster because it has SIMD support. But the current
  // implementation seems to perform well as-is, likely thanks to the compiler's
  // vectorization.
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

  this->_finalized = true;
  return this->_imageAsset;
}

} // namespace CesiumVectorData
