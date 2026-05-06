#include <CesiumGeometry/Transforms.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/GlobeTransforms.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Material.h>
#include <CesiumGltf/MaterialPBRMetallicRoughness.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Scene.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/GeoJsonObjectTypes.h>
#include <CesiumVectorData/GltfConverter.h>

#include <glm/common.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/matrix.hpp>
#include <mapbox/earcut.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumVectorData;

namespace CesiumVectorData {
namespace {
struct GltfConverterImpl {
  const GeoJsonDocument& geoJson;
  Model model;
  // Accumlated as the data is traversed, then used to transform coordinates
  // into a local frame.
  BoundingRegionBuilder regionBuilder;
  glm::dmat4 enuToFixedFrame;
  CesiumGeospatial::Ellipsoid ellipsoid;
  std::vector<glm::dvec3> globalCoordinates;
  std::vector<uint32_t> lineIndices;
  std::vector<uint32_t> polyIndices;
  std::vector<uint32_t> pointIndices;
  int32_t positionBufferIndex = -1;
  int32_t positionAccessorIndex = -1;
  int32_t elementBufferIndex = -1;
  int32_t elementBufferViewIndex = -1;
  GltfConverterImpl(
      const GeoJsonDocument& inGeoJson,
      const CesiumGeospatial::Ellipsoid& inEllipsoid)
      : geoJson(inGeoJson), enuToFixedFrame(1.0), ellipsoid(inEllipsoid) {}
  void addCoordinateDegrees(const glm::dvec3& coord) {
    expandRegion(coord);
    this->globalCoordinates.push_back(this->ellipsoid.cartographicToCartesian(
        Cartographic::fromDegrees(coord.x, coord.y, coord.z)));
  }
  // Gather a feature type from GeoJson into a mesh in a node.
  void gatherLines();
  int32_t finalizeLines();
  void gatherPolygons();
  int32_t finalizePolygons();
  void gatherPoints();
  int32_t finalizePoints();
  int32_t
  finalizePrimitive(const std::vector<uint32_t>& elements, int32_t mode);
  void preparePositions();
  // Create a buffer view from a whole buffer
  int32_t makeBufferView(int32_t buffer, int32_t target);
  // A more nuanced view
  int32_t makeBufferView(
      int32_t buffer,
      int32_t target,
      int64_t byteOffset,
      int64_t byteLength);
  int32_t makeAccessor(
      int32_t bufferView,
      int64_t byteOffset,
      int64_t count,
      int32_t componentType,
      const std::string& accessorType);
  void expandRegion(const glm::dvec3& cartoDegrees) {
    this->regionBuilder.expandToIncludePosition(Cartographic::fromDegrees(
        cartoDegrees.x,
        cartoDegrees.y,
        cartoDegrees.z));
  };
};

int32_t GltfConverterImpl::makeBufferView(int32_t buffer, int32_t target) {
  return makeBufferView(
      buffer,
      target,
      0,
      int64_t(this->model.buffers[size_t(buffer)].cesium.data.size()));
}

int32_t GltfConverterImpl::makeBufferView(
    int32_t buffer,
    int32_t target,
    int64_t byteOffset,
    int64_t byteLength) {
  int32_t bufferViewIndex = int32_t(this->model.bufferViews.size());
  BufferView& bufferView = this->model.bufferViews.emplace_back();
  bufferView.buffer = buffer;
  bufferView.byteOffset = byteOffset;
  bufferView.byteLength = byteLength;
  bufferView.target = target;
  return bufferViewIndex;
}

int32_t GltfConverterImpl::makeAccessor(
    int32_t bufferView,
    int64_t byteOffset,
    int64_t count,
    int32_t componentType,
    const std::string& accessorType) {
  int32_t accessorIndex = int32_t(this->model.accessors.size());
  Accessor& accessor = this->model.accessors.emplace_back();
  accessor.bufferView = bufferView;
  accessor.byteOffset = byteOffset;
  accessor.componentType = componentType;
  accessor.count = count;
  accessor.type = accessorType;
  return accessorIndex;
}

glm::vec3 positionMin(std::span<const glm::vec3> coords) {
  if (coords.empty()) {
    return {0.0, 0.0, 0.0};
  }
  glm::vec3 minCoord = coords[0];
  for (size_t i = 1; i < coords.size(); ++i) {
    minCoord = min(minCoord, coords[i]);
  }
  return minCoord;
}

std::vector<double> positionMinVector(std::span<const glm::vec3> coords) {
  glm::vec3 result = positionMin(coords);
  return {result[0], result[1], result[2]};
}

glm::vec3 positionMax(std::span<const glm::vec3> coords) {
  if (coords.empty()) {
    return {0.0, 0.0, 0.0};
  }
  glm::vec3 maxCoord = coords[0];
  for (size_t i = 1; i < coords.size(); ++i) {
    maxCoord = max(maxCoord, coords[i]);
  }
  return maxCoord;
}

std::vector<double> positionMaxVector(std::span<const glm::vec3> coords) {
  glm::vec3 result = positionMax(coords);
  return {result[0], result[1], result[2]};
}

std::vector<uint32_t> lineStringToLines(size_t startIndex, size_t count) {
  if (count < 2) {
    return {};
  }
  std::vector<uint32_t> result;
  result.reserve((count - 1) * 2);
  for (size_t i = startIndex; i < startIndex + count - 1; ++i) {
    result.push_back(uint32_t(i));
    result.push_back(uint32_t(i + 1));
  }
  return result;
}

void GltfConverterImpl::gatherLines() {
  const GeoJsonObject& root = this->geoJson.rootObject;
  // Look for linestrings, count objects and coordinates
  for (auto lineStringItr = root.allOfType<GeoJsonLineString>().begin();
       lineStringItr != root.allOfType<GeoJsonLineString>().end();
       ++lineStringItr) {
    auto stringIndices = lineStringToLines(
        this->globalCoordinates.size(),
        lineStringItr->coordinates.size());
    this->lineIndices.insert(
        this->lineIndices.end(),
        stringIndices.begin(),
        stringIndices.end());
    for (const glm::dvec3& cartoDegrees : lineStringItr->coordinates) {
      addCoordinateDegrees(cartoDegrees);
    }
  }
  for (auto multiLineItr = root.allOfType<GeoJsonMultiLineString>().begin();
       multiLineItr != root.allOfType<GeoJsonMultiLineString>().end();
       ++multiLineItr) {
    for (const auto& lineStringCoords : multiLineItr->coordinates) {
      auto stringIndices = lineStringToLines(
          this->globalCoordinates.size(),
          lineStringCoords.size());
      this->lineIndices.insert(
          this->lineIndices.end(),
          stringIndices.begin(),
          stringIndices.end());
      for (const glm::dvec3& cartoDegrees : lineStringCoords) {
        addCoordinateDegrees(cartoDegrees);
      }
    }
  }
}

int32_t GltfConverterImpl::finalizePrimitive(
    const std::vector<uint32_t>& elements,
    int32_t mode) {
  if (elements.empty()) {
    return -1;
  }
  int32_t meshIndex = int32_t(this->model.meshes.size());
  Mesh& linesMesh = this->model.meshes.emplace_back();
  std::vector<std::byte>& elementBytes =
      this->model.buffers[size_t(this->elementBufferIndex)].cesium.data;
  size_t elementBase = elementBytes.size();
  elementBytes.resize(elementBase + sizeof(uint32_t) * elements.size());
  std::memcpy(
      elementBytes.data() + elementBase,
      elements.data(),
      sizeof(uint32_t) * elements.size());
  int32_t elementAccessorIndex = makeAccessor(
      this->elementBufferViewIndex,
      int64_t(elementBase),
      int64_t(elements.size()),
      Accessor::ComponentType::UNSIGNED_INT,
      Accessor::Type::SCALAR);
  linesMesh.primitives.emplace_back();
  linesMesh.primitives.back().attributes["POSITION"] =
      this->positionAccessorIndex;
  linesMesh.primitives.back().indices = elementAccessorIndex;
  linesMesh.primitives.back().mode = mode;
  linesMesh.primitives.back().material = 0;

  int32_t nodeIndex = int32_t(this->model.nodes.size());
  this->model.nodes.emplace_back();
  this->model.nodes.back().mesh = meshIndex;
  return nodeIndex;
}

int32_t GltfConverterImpl::finalizeLines() {
  return finalizePrimitive(this->lineIndices, MeshPrimitive::Mode::LINES);
}

std::vector<uint32_t>
triangulatePolygon(const std::vector<std::vector<glm::dvec3>>& polygonIn) {
  using Point = std::array<double, 2>;
  std::vector<std::vector<Point>> polygon;
  polygon.reserve(polygonIn.size());
  for (const auto& ring : polygonIn) {
    auto& ringCopy = polygon.emplace_back();
    ringCopy.reserve(ring.size() - 1);
    // The last coordinate of a GeoJson polygon ring is identical to the first,
    // and earcut should work without it.
    for (size_t i = 0; i < ring.size() - 1; i++) {
      ringCopy.push_back(Point{ring[i][0], ring[i][1]});
    }
  }
  return mapbox::earcut<uint32_t>(polygon);
}

void GltfConverterImpl::gatherPolygons() {
  using PolygonRing = std::vector<glm::dvec3>;
  uint32_t elementBase = uint32_t(this->globalCoordinates.size());
  const GeoJsonObject& root = this->geoJson.rootObject;
  for (auto polygonIt = root.allOfType<GeoJsonPolygon>().begin();
       polygonIt != root.allOfType<GeoJsonPolygon>().end();
       ++polygonIt) {
    const std::vector<PolygonRing>& polygonRings = polygonIt->coordinates;
    for (const auto& contour : polygonRings) {
      // The last coordinate is identical to the first.
      for (size_t i = 0; i < contour.size() - 1; ++i) {
        this->addCoordinateDegrees(contour[i]);
      }
    }
    std::vector<uint32_t> triangulatedIndices =
        triangulatePolygon(polygonRings);
    for (uint32_t index : triangulatedIndices) {
      this->polyIndices.push_back(elementBase + index);
    }
    // Sum up the vertex count from each polygon.
    for (const PolygonRing& polygon : polygonRings) {
      elementBase += uint32_t(polygon.size() - 1);
    }
  }
  for (auto multiPolygonIt = root.allOfType<GeoJsonMultiPolygon>().begin();
       multiPolygonIt != root.allOfType<GeoJsonMultiPolygon>().end();
       ++multiPolygonIt) {
    for (const std::vector<PolygonRing>& polygonRings :
         multiPolygonIt->coordinates) {
      for (const auto& contour : polygonRings) {
        for (size_t i = 0; i < contour.size() - 1; ++i) {
          this->addCoordinateDegrees(contour[i]);
        }
      }
      std::vector<uint32_t> triangulatedIndices =
          triangulatePolygon(polygonRings);
      for (uint32_t index : triangulatedIndices) {
        this->polyIndices.push_back(elementBase + index);
      }
      // Sum up the vertex count from each polygon.
      for (const PolygonRing& polygon : polygonRings) {
        elementBase += uint32_t(polygon.size() - 1);
      }
    }
  }
}

int32_t GltfConverterImpl::finalizePolygons() {
  return finalizePrimitive(this->polyIndices, MeshPrimitive::Mode::TRIANGLES);
}

void GltfConverterImpl::gatherPoints() {
  const GeoJsonObject& root = this->geoJson.rootObject;
  uint32_t element = uint32_t(this->globalCoordinates.size());
  // Look for points, count objects and coordinates
  for (auto pointsItr = root.allOfType<GeoJsonPoint>().begin();
       pointsItr != root.allOfType<GeoJsonPoint>().end();
       ++pointsItr) {
    addCoordinateDegrees(pointsItr->coordinates);
    this->pointIndices.push_back(element++);
  }
  for (auto multiPointItr = root.allOfType<GeoJsonMultiPoint>().begin();
       multiPointItr != root.allOfType<GeoJsonMultiPoint>().end();
       ++multiPointItr) {
    for (const glm::dvec3& cartoDegrees : multiPointItr->coordinates) {
      addCoordinateDegrees(cartoDegrees);
      this->pointIndices.push_back(element++);
    }
  }
}

int32_t GltfConverterImpl::finalizePoints() {
  return finalizePrimitive(this->pointIndices, MeshPrimitive::Mode::POINTS);
}

void GltfConverterImpl::preparePositions() {
  BoundingRegion coordsRegion = this->regionBuilder.toRegion();
  Cartographic centroid = coordsRegion.getRectangle().computeCenter();
  centroid.height =
      (coordsRegion.getMinimumHeight() + coordsRegion.getMaximumHeight()) / 2.0;
  //  local to global cartesian
  this->enuToFixedFrame = GlobeTransforms::eastNorthUpToFixedFrame(
      ellipsoid.cartographicToCartesian(centroid));
  glm::dmat4 toLocal = inverse(this->enuToFixedFrame);
  this->positionBufferIndex = int32_t(this->model.buffers.size());
  Buffer& positionBuffer = this->model.buffers.emplace_back();
  positionBuffer.cesium.data.resize(
      sizeof(glm::vec3) * this->globalCoordinates.size());
  glm::vec3* const position32Base =
      reinterpret_cast<glm::vec3*>(positionBuffer.cesium.data.data());
  glm::vec3* position32 = position32Base;
  for (const auto& position : this->globalCoordinates) {
    glm::dvec3 localPosition = glm::dvec3(toLocal * glm::dvec4(position, 1.0));
    // Convert to float
    *position32++ =
        glm::vec3(localPosition.x, localPosition.y, localPosition.z);
  }
  int32_t positionBufferViewIndex = makeBufferView(
      this->positionBufferIndex,
      BufferView::Target::ARRAY_BUFFER);
  this->positionAccessorIndex = makeAccessor(
      positionBufferViewIndex,
      0,
      int64_t(this->globalCoordinates.size()),
      Accessor::ComponentType::FLOAT,
      Accessor::Type::VEC3);
  Accessor& positionAccessor =
      this->model.accessors[size_t(this->positionAccessorIndex)];
  std::span<const glm::vec3> positionSpan(
      position32Base,
      this->globalCoordinates.size());
  positionAccessor.min = positionMinVector(positionSpan);
  positionAccessor.max = positionMaxVector(positionSpan);
  this->elementBufferIndex = int32_t(this->model.buffers.size());
  Buffer& elementBuffer = this->model.buffers.emplace_back();
  size_t elementByteSize =
      sizeof(uint32_t) * (this->lineIndices.size() + this->polyIndices.size() +
                          this->pointIndices.size());
  elementBuffer.cesium.data.reserve(elementByteSize);
  this->elementBufferViewIndex = this->makeBufferView(
      this->elementBufferIndex,
      BufferView::Target::ELEMENT_ARRAY_BUFFER,
      0,
      int64_t(elementByteSize));
}
} // namespace

ConverterResult GltfConverter::convert(
    const GeoJsonDocument& geoJson,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  GltfConverterImpl converter(geoJson, ellipsoid);
  converter.gatherLines();
  converter.gatherPolygons();
  converter.gatherPoints();
  converter.preparePositions();
  converter.model.asset.version = "2.0";
  Material& material = converter.model.materials.emplace_back();
  MaterialPBRMetallicRoughness& pbr = material.pbrMetallicRoughness.emplace();
  std::array white{255.0, 255.0, 255.0};
  pbr.metallicFactor = 0.0;
  pbr.roughnessFactor = 1.0;
  pbr.baseColorFactor.assign(white.begin(), white.end());
  material.doubleSided = true;
  material.emissiveFactor.assign(white.begin(), white.end());
  size_t rootNodeIndex = converter.model.nodes.size();
  converter.model.nodes.emplace_back();
  CesiumGltfContent::GltfUtilities::setNodeTransform(
      converter.model.nodes[rootNodeIndex],
      CesiumGeometry::Transforms::Z_UP_TO_Y_UP * converter.enuToFixedFrame);

  auto maybeAddNode = [&model = converter.model,
                       rootNodeIndex](int32_t featureNode) {
    if (featureNode >= 0) {
      model.nodes[rootNodeIndex].children.push_back(featureNode);
    }
  };
  maybeAddNode(converter.finalizeLines());
  maybeAddNode(converter.finalizePolygons());
  maybeAddNode(converter.finalizePoints());
  Scene& scene = converter.model.scenes.emplace_back();
  scene.nodes.push_back(int32_t(rootNodeIndex));
  converter.model.scene = int32_t(converter.model.scenes.size() - 1);
  return {std::move(converter.model)};
}

} // namespace CesiumVectorData
