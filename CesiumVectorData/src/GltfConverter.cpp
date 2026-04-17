#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/GlobeTransforms.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Material.h>
#include <CesiumGltf/MaterialPBRMetallicRoughness.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
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

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumVectorData;

namespace CesiumVectorData {
namespace {
struct GltfConverterImpl {
  const GeoJsonDocument& geoJson;
  Model model;
  glm::dmat4 enuToFixedFrame;
  GltfConverterImpl(const GeoJsonDocument& inGeoJson)
      : geoJson(inGeoJson), enuToFixedFrame(1.0) {
  }
  static double heightOffset;
  // Gather a feature type from GeoJson into a mesh in a node.
  int32_t gatherLines();
  int32_t gatherPolygons();
  int32_t gatherPoints();
};

double GltfConverterImpl::heightOffset = 10.0;

glm::dvec3 positionMin(const std::span<glm::dvec3> coords) {
  if (coords.empty()) {
    return {0.0, 0.0, 0.0};
  }
  glm::dvec3 minCoord = coords[0];
  for (size_t i = 1; i < coords.size(); ++i) {
    minCoord = min(minCoord, coords[i]);
  }
  return minCoord;
}

std::vector<double> positionMinVector(const std::span<glm::dvec3> coords) {
  glm::dvec3 result = positionMin(coords);
  return {result[0], result[1], result[2]};
}

glm::dvec3 positionMax(const std::span<glm::dvec3> coords) {
  if (coords.empty()) {
    return {0.0, 0.0, 0.0};
  }
  glm::dvec3 maxCoord = coords[0];
  for (size_t i = 1; i < coords.size(); ++i) {
    maxCoord = max(maxCoord, coords[i]);
  }
  return maxCoord;
}

std::vector<double> positionMaxVector(const std::span<glm::dvec3> coords) {
  glm::dvec3 result = positionMax(coords);
  return {result[0], result[1], result[2]};
}

// This finds the average of geographic coordinates, but it's not a great way to
// find the actual average coordinate of the data.

glm::dvec3 computeCentroid(const GeoJsonObject& root) {
  size_t numCoords = 0;
  glm::dvec3 sum(0.0, 0.0, 0.0);
  auto pointsProvider = root.points();
  for (auto pointsItr = pointsProvider.begin();
       pointsItr != pointsProvider.end();
       ++pointsItr) {
    numCoords++;
    sum += *pointsItr;
  }
  auto linesProvider = root.lines();
  for (auto linesItr = linesProvider.begin();
       linesItr != linesProvider.end();
       ++linesItr) {
    for (const auto& coord : *linesItr) {
      numCoords++;
      sum += coord;
    }
  }
  auto polysProvider = root.polygons();
  for (auto polysItr = polysProvider.begin();
       polysItr != polysProvider.end();
       ++polysItr) {
    const auto& polyRings = *polysItr;
    for (const auto& ring : polyRings) {
      for (const auto& coord : ring) {
        numCoords++;
        sum += coord;
      }
    }
  }
  if (numCoords > 0) {
    sum /= static_cast<double>(numCoords);
  }
  return sum;
}

void transformIntoFrame(
    const glm::dmat4& localFrame,
    const std::vector<glm::dvec3>& positions,
    std::vector<glm::dvec3>& outPositions) {
  glm::dmat4 toLocal = inverse(localFrame);
  outPositions.resize(positions.size());
  std::transform(
      positions.begin(),
      positions.end(),
      outPositions.begin(),
      [&toLocal](const glm::dvec3& cartoDegrees) {
        CesiumGeospatial::Cartographic cartographic(Cartographic::fromDegrees(
            cartoDegrees.x,
            cartoDegrees.y,
            cartoDegrees.z + GltfConverterImpl::heightOffset));
        auto cartesian =
            CesiumGeospatial::Ellipsoid::WGS84.cartographicToCartesian(
                cartographic);
        return glm::dvec3(toLocal * glm::dvec4(cartesian, 1.0));
      });
}

int32_t GltfConverterImpl::gatherLines() {
  const GeoJsonObject& root = geoJson.rootObject;
  // Look for linestrings, count objects and coordinates
  std::vector<glm::dvec3> cartoCoordinates;
  for (auto lineStringItr = root.allOfType<GeoJsonLineString>().begin();
       lineStringItr != root.allOfType<GeoJsonLineString>().end();
       ++lineStringItr) {
    cartoCoordinates.insert(
        cartoCoordinates.end(),
        lineStringItr->coordinates.begin(),
        lineStringItr->coordinates.end());
  }
  for (auto multiLineItr = root.allOfType<GeoJsonMultiLineString>().begin();
       multiLineItr != root.allOfType<GeoJsonMultiLineString>().end();
       ++multiLineItr) {
    for (const auto& lineStringCoords : multiLineItr->coordinates) {
      cartoCoordinates.insert(
          cartoCoordinates.end(),
          lineStringCoords.begin(),
          lineStringCoords.end());
    }
  }
  if (cartoCoordinates.empty()) {
    return -1;
  }
  std::vector<glm::dvec3> localPositions(cartoCoordinates.size());
  transformIntoFrame(enuToFixedFrame, cartoCoordinates, localPositions);
  int32_t bufferIndex = static_cast<int32_t>(model.buffers.size());
  model.buffers.emplace_back();
  std::vector<std::byte>& bytes = model.buffers.back().cesium.data;
  bytes.resize(localPositions.size() * sizeof(glm::vec3));
  glm::vec3* position32 = reinterpret_cast<glm::vec3*>(bytes.data());
  for (const auto& localPosition : localPositions) {
    *position32++ =
        glm::vec3(localPosition.x, localPosition.y, localPosition.z);
  }
  int32_t bufferViewIndex = static_cast<int32_t>(model.bufferViews.size());
  BufferView& linesBufferView = model.bufferViews.emplace_back();
  linesBufferView.buffer = bufferIndex;
  linesBufferView.byteOffset = 0;
  linesBufferView.byteLength = static_cast<int64_t>(bytes.size());
  linesBufferView.target = BufferView::Target::ARRAY_BUFFER;
  int64_t accessorByteOffset = 0;
  size_t elementCount = 0;
  int32_t meshIndex = static_cast<int32_t>(model.meshes.size());
  Mesh& linesMesh = model.meshes.emplace_back();
  for (auto lineStringItr = root.allOfType<GeoJsonLineString>().begin();
       lineStringItr != root.allOfType<GeoJsonLineString>().end();
       ++lineStringItr) {
    int32_t accessorIndex = static_cast<int32_t>(model.accessors.size());
    Accessor& linesAccessor = model.accessors.emplace_back();
    linesAccessor.bufferView = bufferViewIndex;
    linesAccessor.byteOffset = accessorByteOffset;
    linesAccessor.componentType = Accessor::ComponentType::FLOAT;
    linesAccessor.count =
        static_cast<int64_t>(lineStringItr->coordinates.size());
    linesAccessor.type = Accessor::Type::VEC3;
    std::span<glm::dvec3> stringCoords{
        &localPositions[elementCount],
        lineStringItr->coordinates.size()};
    linesAccessor.min = positionMinVector(stringCoords);
    linesAccessor.max = positionMaxVector(stringCoords);
    accessorByteOffset += linesAccessor.count * 4 * 3;
    elementCount += lineStringItr->coordinates.size();
    linesMesh.primitives.emplace_back();
    linesMesh.primitives.back().attributes["POSITION"] = accessorIndex;
    linesMesh.primitives.back().mode = MeshPrimitive::Mode::LINE_STRIP;
    linesMesh.primitives.back().material = 0;
  }
  for (auto multiLineItr = root.allOfType<GeoJsonMultiLineString>().begin();
       multiLineItr != root.allOfType<GeoJsonMultiLineString>().end();
       ++multiLineItr) {
    for (const auto& lineStringCoords : multiLineItr->coordinates) {
      int32_t accessorIndex = static_cast<int32_t>(model.accessors.size());
      Accessor& linesAccessor = model.accessors.emplace_back();
      linesAccessor.bufferView = bufferViewIndex;
      linesAccessor.byteOffset = accessorByteOffset;
      linesAccessor.componentType = Accessor::ComponentType::FLOAT;
      linesAccessor.count = static_cast<int64_t>(lineStringCoords.size());
      linesAccessor.type = Accessor::Type::VEC3;
      std::span<glm::dvec3> stringCoords{
          &localPositions[elementCount],
          lineStringCoords.size()};
      linesAccessor.min = positionMinVector(stringCoords);
      linesAccessor.max = positionMaxVector(stringCoords);
      accessorByteOffset += linesAccessor.count * 4 * 3;
      elementCount += lineStringCoords.size();
      linesMesh.primitives.emplace_back();
      linesMesh.primitives.back().attributes["POSITION"] = accessorIndex;
      linesMesh.primitives.back().mode = MeshPrimitive::Mode::LINE_STRIP;
      linesMesh.primitives.back().material = 0;
    }
  }
  int32_t nodeIndex = static_cast<int32_t>(model.nodes.size());
  model.nodes.emplace_back();
  model.nodes.back().mesh = meshIndex;
  CesiumGltfContent::GltfUtilities::setNodeTransform(
      model.nodes.back(),
      enuToFixedFrame);
  return nodeIndex;
}

std::vector<uint64_t>
triangulatePolygon(const std::vector<std::vector<glm::dvec3>>& polygonIn) {
  using N = uint64_t;
  using Point = std::array<double, 2>;
  std::vector<std::vector<Point>> polygon;

  for (const auto& ring : polygonIn) {
    polygon.emplace_back();
    for (const auto& coord : ring) {
      polygon.back().push_back(Point{{coord[0], coord[1]}});
    }
  }

  std::vector<N> indices = mapbox::earcut<N>(polygon);

  return indices;
}

std::vector<uint64_t>
triangulatePolygon(const CesiumVectorData::GeoJsonPolygon& polygonIn) {
  return triangulatePolygon(polygonIn.coordinates);
}

int32_t GltfConverterImpl::gatherPolygons() {
  const GeoJsonObject& root = geoJson.rootObject;
  std::vector<glm::dvec3> cartoCoordinates;
  // GeoJson polygons contain an outer counter and possible inner contours. The
  // first and last coordinates must be identical. An optimization would be to
  // only include that coordinate once, but for now we will put both values into
  // the big array of coordinates.
  for (auto polyItr = root.allOfType<GeoJsonPolygon>().begin();
       polyItr != root.allOfType<GeoJsonPolygon>().end();
       ++polyItr) {
    for (const auto& contour : polyItr->coordinates) {
      cartoCoordinates.insert(
          cartoCoordinates.end(),
          contour.begin(),
          contour.end());
    }
  }
  for (auto multiPolyItr = root.allOfType<GeoJsonMultiPolygon>().begin();
       multiPolyItr != root.allOfType<GeoJsonMultiPolygon>().end();
       ++multiPolyItr) {
    for (const auto& polygon : multiPolyItr->coordinates) {
      for (const auto& contour : polygon) {
        cartoCoordinates.insert(
            cartoCoordinates.end(),
            contour.begin(),
            contour.end());
      }
    }
  }
  if (cartoCoordinates.empty()) {
    return -1;
  }
  std::vector<glm::dvec3> localPositions(cartoCoordinates.size());
  transformIntoFrame(enuToFixedFrame, cartoCoordinates, localPositions);
  int32_t bufferIndex = static_cast<int32_t>(model.buffers.size());
  model.buffers.emplace_back();
  std::vector<std::byte>& bytes = model.buffers.back().cesium.data;
  bytes.resize(localPositions.size() * sizeof(glm::vec3));
  glm::vec3* position32 = reinterpret_cast<glm::vec3*>(bytes.data());
  for (const auto& localPosition : localPositions) {
    *position32++ =
        glm::vec3(localPosition.x, localPosition.y, localPosition.z);
  }
  int32_t bufferViewIndex = static_cast<int32_t>(model.bufferViews.size());
  model.bufferViews.emplace_back();
  model.bufferViews.back().buffer = bufferIndex;
  model.bufferViews.back().byteOffset = 0;
  model.bufferViews.back().byteLength = static_cast<int64_t>(bytes.size());
  model.bufferViews.back().target = BufferView::Target::ARRAY_BUFFER;
  // The polygons are now triangulated, which returns indices to
  // vertices. Create an additional buffer for the indices.
  std::vector<uint32_t> allIndices;
  size_t allVertexCounter = 0;
  for (auto polyItr = root.allOfType<GeoJsonPolygon>().begin();
       polyItr != root.allOfType<GeoJsonPolygon>().end();
       ++polyItr) {
    std::vector<uint64_t> triangulated = triangulatePolygon(*polyItr);
    for (auto index : triangulated) {
      allIndices.push_back(static_cast<uint32_t>(allVertexCounter + index));
    }
    allVertexCounter += polyItr->coordinates.size();
  }
  for (auto multiPolyItr = root.allOfType<GeoJsonMultiPolygon>().begin();
       multiPolyItr != root.allOfType<GeoJsonMultiPolygon>().end();
       ++multiPolyItr) {
    for (const auto& polygon : multiPolyItr->coordinates) {
      std::vector<uint64_t> triangulated = triangulatePolygon(polygon);
      for (auto index : triangulated) {
        allIndices.push_back(static_cast<uint32_t>(allVertexCounter + index));
      }
      allVertexCounter += polygon.size();
    }
  }

  int32_t indexBufferIndex = static_cast<int32_t>(model.buffers.size());
  model.buffers.emplace_back();
  std::vector<std::byte>& indexBytes = model.buffers.back().cesium.data;
  indexBytes.resize(sizeof(uint32_t) * allIndices.size());
  std::memcpy(
      indexBytes.data(),
      allIndices.data(),
      sizeof(uint32_t) * allIndices.size());
  int32_t indexBufferViewIndex = static_cast<int32_t>(model.bufferViews.size());
  BufferView& polyBufferView = model.bufferViews.emplace_back();
  polyBufferView.buffer = indexBufferIndex;
  polyBufferView.byteOffset = 0;
  polyBufferView.byteLength = static_cast<int64_t>(indexBytes.size());
  polyBufferView.target = BufferView::Target::ELEMENT_ARRAY_BUFFER;

  int32_t accessorIndex = static_cast<int32_t>(model.accessors.size());
  Accessor& coordAccessor = model.accessors.emplace_back();
  coordAccessor.bufferView = bufferViewIndex;
  coordAccessor.byteOffset = 0;
  coordAccessor.componentType = Accessor::ComponentType::FLOAT;
  coordAccessor.count = static_cast<int64_t>(localPositions.size());
  coordAccessor.min = positionMinVector(localPositions);
  coordAccessor.max = positionMaxVector(localPositions);
  coordAccessor.type = Accessor::Type::VEC3;

  int32_t indexAccessorIndex = static_cast<int32_t>(model.accessors.size());
  Accessor& indexAccessor = model.accessors.emplace_back();
  indexAccessor.bufferView = indexBufferViewIndex;
  indexAccessor.byteOffset = 0;
  indexAccessor.componentType = Accessor::ComponentType::UNSIGNED_INT;
  indexAccessor.count = static_cast<int64_t>(allIndices.size());
  indexAccessor.type = Accessor::Type::SCALAR;

  int32_t meshIndex = static_cast<int32_t>(model.meshes.size());
  Mesh& polyMesh = model.meshes.emplace_back();
  polyMesh.primitives.emplace_back();
  polyMesh.primitives.back().attributes["POSITION"] = accessorIndex;
  polyMesh.primitives.back().indices = indexAccessorIndex;
  polyMesh.primitives.back().mode = MeshPrimitive::Mode::TRIANGLES;
  polyMesh.primitives.back().material = 0;

  int32_t nodeIndex = static_cast<int32_t>(model.nodes.size());
  model.nodes.emplace_back();
  model.nodes.back().mesh = meshIndex;
  CesiumGltfContent::GltfUtilities::setNodeTransform(
      model.nodes.back(),
      enuToFixedFrame);
  return nodeIndex;
}

int32_t GltfConverterImpl::gatherPoints() {
  const GeoJsonObject& root = geoJson.rootObject;
  // Look for points, count objects and coordinates
  std::vector<glm::dvec3> cartoCoordinates;
  for (auto pointsItr = root.allOfType<GeoJsonPoint>().begin();
       pointsItr != root.allOfType<GeoJsonPoint>().end();
       ++pointsItr) {
    cartoCoordinates.push_back(pointsItr->coordinates);
  }
  for (auto multiPointItr = root.allOfType<GeoJsonMultiPoint>().begin();
       multiPointItr != root.allOfType<GeoJsonMultiPoint>().end();
       ++multiPointItr) {
    cartoCoordinates.insert(
        cartoCoordinates.end(),
        multiPointItr->coordinates.begin(),
        multiPointItr->coordinates.end());
  }
  if (cartoCoordinates.empty()) {
    return -1;
  }
  std::vector<glm::dvec3> localPositions(cartoCoordinates.size());
  transformIntoFrame(enuToFixedFrame, cartoCoordinates, localPositions);
  int32_t bufferIndex = static_cast<int32_t>(model.buffers.size());
  model.buffers.emplace_back();
  std::vector<std::byte>& bytes = model.buffers.back().cesium.data;
  bytes.resize(localPositions.size() * sizeof(glm::vec3));
  glm::vec3* position32 = reinterpret_cast<glm::vec3*>(bytes.data());
  for (const auto& localPosition : localPositions) {
    *position32++ =
        glm::vec3(localPosition.x, localPosition.y, localPosition.z);
  }
  int32_t bufferViewIndex = static_cast<int32_t>(model.bufferViews.size());
  BufferView& pointsBufferView = model.bufferViews.emplace_back();
  pointsBufferView.buffer = bufferIndex;
  pointsBufferView.byteOffset = 0;
  pointsBufferView.byteLength = static_cast<int64_t>(bytes.size());
  pointsBufferView.target = BufferView::Target::ARRAY_BUFFER;
  int32_t accessorIndex = static_cast<int32_t>(model.accessors.size());
  Accessor& pointsAccessor = model.accessors.emplace_back();
  pointsAccessor.bufferView = bufferViewIndex;
  pointsAccessor.byteOffset = 0;
  pointsAccessor.componentType = Accessor::ComponentType::FLOAT;
  pointsAccessor.count = static_cast<int64_t>(localPositions.size());
  pointsAccessor.min = positionMinVector(localPositions);
  pointsAccessor.max = positionMaxVector(localPositions);
  pointsAccessor.type = Accessor::Type::VEC3;

  int32_t meshIndex = static_cast<int32_t>(model.meshes.size());
  Mesh& pointsMesh = model.meshes.emplace_back();
  pointsMesh.primitives.emplace_back();
  pointsMesh.primitives.back().attributes["POSITION"] = accessorIndex;
  pointsMesh.primitives.back().mode = MeshPrimitive::Mode::POINTS;
  pointsMesh.primitives.back().material = 0;

  int32_t nodeIndex = static_cast<int32_t>(model.nodes.size());
  model.nodes.emplace_back();
  model.nodes.back().mesh = meshIndex;
  CesiumGltfContent::GltfUtilities::setNodeTransform(
      model.nodes.back(),
      enuToFixedFrame);
  return nodeIndex;
}
} // namespace

ConverterResult GltfConverter::convert(const GeoJsonDocument& geoJson) {
  ConverterResult result;
  GltfConverterImpl converter(geoJson);
  const GeoJsonObject& root = converter.geoJson.rootObject;
  glm::dvec3 centroid = computeCentroid(root);
  //  local to global cartesian
  converter.enuToFixedFrame = GlobeTransforms::eastNorthUpToFixedFrame(
      Ellipsoid::WGS84.cartographicToCartesian(
          Cartographic::fromDegrees(centroid.x, centroid.y, centroid.z)));
  result.model.emplace();
  converter.model.asset.version = "2.0";
  CesiumGltf::Material& material = converter.model.materials.emplace_back();
  CesiumGltf::MaterialPBRMetallicRoughness& pbr =
      material.pbrMetallicRoughness.emplace();
  pbr.metallicFactor = 0.0;
  pbr.roughnessFactor = 1.0;
  pbr.baseColorFactor[0] = 0.0;
  pbr.baseColorFactor[1] = 0.0;
  pbr.baseColorFactor[2] = 0.0;
  material.doubleSided = true;
  // International orange
  material.emissiveFactor = {0xff / 255.0, 0x4f / 255.0, 0.0};
  size_t rootNodeIndex = converter.model.nodes.size();
  converter.model.nodes.emplace_back();
  auto maybeAddNode = [&model = converter.model,
                       rootNodeIndex](int32_t featureNode) {
    if (featureNode >= 0) {
      model.nodes[rootNodeIndex].children.push_back(featureNode);
    }
  };
  maybeAddNode(converter.gatherLines());
  maybeAddNode(converter.gatherPolygons());
  maybeAddNode(converter.gatherPoints());
  result.model = std::move(converter.model);
  return result;
}

} // namespace CesiumVectorData
