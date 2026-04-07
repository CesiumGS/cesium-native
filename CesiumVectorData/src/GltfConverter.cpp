#include <CesiumGeospatial/GlobeTransforms.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumVectorData/GltfConverter.h>

#include <mapbox/earcut.hpp>

#include <algorithm>

using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumVectorData;

namespace CesiumVectorData {

glm::dvec3 positionMin(const std::vector<glm::dvec3>& coords) {
  if (coords.empty()) {
    return {0.0, 0.0, 0.0};
  }
  glm::dvec3 minCoord = coords[0];
  for (size_t i = 1; i < coords.size(); ++i) {
    minCoord = min(minCoord, coords[i]);
  }
  return minCoord;
}

glm::dvec3 positionMax(const std::vector<glm::dvec3>& coords) {
  if (coords.empty()) {
    return {0.0, 0.0, 0.0};
  }
  glm::dvec3 maxCoord = coords[0];
  for (size_t i = 1; i < coords.size(); ++i) {
    maxCoord = max(maxCoord, coords[i]);
  }
  return maxCoord;
}

// This finds the average of geographic coordinates, but it's not a great way to
// find the actual average coordinate of the data.

glm::dvec3 computeCentroid(const GeoJsonObject& root) {
  size_t numCoords = 0;
  glm::dvec3 sum(0.0, 0.0, 0.0);

  auto pointsItr = root.points().begin();
  while (pointsItr != root.points().end()) {
    numCoords++;
    sum += *(pointsItr++);
  }
  auto linesItr = root.lines().begin();
  while (linesItr != root.lines().end()) {
    for (const auto& coord : *linesItr) {
      numCoords++;
      sum += coord;
    }
    linesItr++;
  }
  auto polysItr = root.polygons().begin();
  while (polysItr != root.polygons().end()) {
    const auto& polyRings = *polysItr;
    for (const auto& ring : polyRings) {
      for (const auto& coord : ring) {
        numCoords++;
        sum += coord;
      }
    }
    polysItr++;
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
            cartoDegrees.z));
        auto cartesian =
            CesiumGeospatial::Ellipsoid::WGS84.cartographicToCartesian(
                cartographic);
        return glm::dvec3(toLocal * glm::dvec4(cartesian, 1.0));
      });
}

void gatherLines(
    const GeoJsonDocument& geoJson,
    Model& model,
    const glm::dmat4& enuToFixedFrame) {
  const GeoJsonObject& root = geoJson.rootObject;
  // Look for linestrings, count objects and coordinates
  std::vector<glm::dvec3> cartoCoordinates;
  auto lineStringItr = root.allOfType<GeoJsonLineString>().begin();
  while (lineStringItr != root.allOfType<GeoJsonLineString>().end()) {
    cartoCoordinates.insert(
        cartoCoordinates.end(),
        lineStringItr->coordinates.begin(),
        lineStringItr->coordinates.end());
    lineStringItr++;
  }
  auto multiLineItr = root.allOfType<GeoJsonMultiLineString>().begin();
  while (multiLineItr != root.allOfType<GeoJsonMultiLineString>().end()) {
    for (const auto& lineStringCoords : multiLineItr->coordinates) {
      cartoCoordinates.insert(
          cartoCoordinates.end(),
          lineStringCoords.begin(),
          lineStringCoords.end());
    }
    multiLineItr++;
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
  lineStringItr = root.allOfType<GeoJsonLineString>().begin();
  int64_t accessorByteOffset = 0;
  int32_t meshIndex = static_cast<int32_t>(model.meshes.size());
  model.meshes.emplace_back();
  while (lineStringItr != root.allOfType<GeoJsonLineString>().end()) {
    int32_t accessorIndex = static_cast<int32_t>(model.accessors.size());
    model.accessors.emplace_back();
    model.accessors.back().bufferView = bufferViewIndex;
    model.accessors.back().byteOffset = accessorByteOffset;
    model.accessors.back().componentType = Accessor::ComponentType::FLOAT;
    model.accessors.back().count =
        static_cast<int64_t>(lineStringItr->coordinates.size());
    model.accessors.back().type = Accessor::Type::VEC3;
    accessorByteOffset += model.accessors.back().count * 4 * 3;
    model.meshes.back().primitives.emplace_back();
    model.meshes.back().primitives.back().attributes["POSITION"] =
        accessorIndex;
    model.meshes.back().primitives.back().mode =
        MeshPrimitive::Mode::LINE_STRIP;
    model.meshes.back().primitives.back().material = 0;
  }
  multiLineItr = root.allOfType<GeoJsonMultiLineString>().begin();
  while (multiLineItr != root.allOfType<GeoJsonMultiLineString>().end()) {
    for (const auto& lineStringCoords : multiLineItr->coordinates) {
      int32_t accessorIndex = static_cast<int32_t>(model.accessors.size());
      model.accessors.emplace_back();
      model.accessors.back().bufferView = bufferViewIndex;
      model.accessors.back().byteOffset = accessorByteOffset;
      model.accessors.back().componentType = Accessor::ComponentType::FLOAT;
      model.accessors.back().count =
          static_cast<int64_t>(lineStringCoords.size());
      model.accessors.back().type = Accessor::Type::VEC3;
      accessorByteOffset += model.accessors.back().count * 4 * 3;
      model.meshes.back().primitives.emplace_back();
      model.meshes.back().primitives.back().attributes["POSITION"] =
          accessorIndex;
      model.meshes.back().primitives.back().mode =
          MeshPrimitive::Mode::LINE_STRIP;
      model.meshes.back().primitives.back().material = 0;
    }
  }
  model.nodes.emplace_back();
  model.nodes.back().mesh = meshIndex;
  CesiumGltfContent::GltfUtilities::setNodeTransform(
      model.nodes.back(),
      enuToFixedFrame);
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

  // Run earcut
  std::vector<N> indices = mapbox::earcut<N>(polygon);

  return indices;
}

std::vector<uint64_t>
triangulatePolygon(const CesiumVectorData::GeoJsonPolygon& polygonIn) {
  return triangulatePolygon(polygonIn.coordinates);
}

void gatherPolygons(
    const GeoJsonDocument& geoJson,
    Model& model,
    const glm::dmat4& enuToFixedFrame) {
  const GeoJsonObject& root = geoJson.rootObject;
  std::vector<glm::dvec3> cartoCoordinates;
  auto polyItr = root.allOfType<GeoJsonPolygon>().begin();
  // GeoJson polygons contain an outer counter and possible inner contours. The
  // first and last coordinates must be identical. An optimization would be to
  // only include that coordinate once, but for now we will put both values into
  // the big array of coordinates.
  while (polyItr != root.allOfType<GeoJsonPolygon>().end()) {
    for (const auto& contour : polyItr->coordinates) {
      cartoCoordinates.insert(
          cartoCoordinates.end(),
          contour.begin(),
          contour.end());
    }
    polyItr++;
  }
  auto multiPolyItr = root.allOfType<GeoJsonMultiPolygon>().begin();
  while (multiPolyItr != root.allOfType<GeoJsonMultiPolygon>().end()) {
    for (const auto& polygon : multiPolyItr->coordinates) {
      for (const auto& contour : polygon) {
        cartoCoordinates.insert(
            cartoCoordinates.end(),
            contour.begin(),
            contour.end());
      }
    }
    multiPolyItr++;
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
  // The polygons will all be triangulated, with indices to vertices being
  // available. Create an additional buffer for the indices.
  std::vector<uint32_t> allIndices;
  size_t allVertexCounter = 0;
  polyItr = root.allOfType<GeoJsonPolygon>().begin();
  while (polyItr != root.allOfType<GeoJsonPolygon>().end()) {
    std::vector<uint64_t> triangulated = triangulatePolygon(*polyItr);
    for (auto index : triangulated) {
      allIndices.push_back(static_cast<uint32_t>(allVertexCounter + index));
    }
    allVertexCounter += polyItr->coordinates.size();
    polyItr++;
  }
  multiPolyItr = root.allOfType<GeoJsonMultiPolygon>().begin();
  while (multiPolyItr != root.allOfType<GeoJsonMultiPolygon>().end()) {
    for (const auto& polygon : multiPolyItr->coordinates) {
      std::vector<uint64_t> triangulated = triangulatePolygon(polygon);
      for (auto index : triangulated) {
        allIndices.push_back(static_cast<uint32_t>(allVertexCounter + index));
      }
      allVertexCounter += polygon.size();
    }
    multiPolyItr++;
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
  model.bufferViews.emplace_back();
  model.bufferViews.back().buffer = indexBufferIndex;
  model.bufferViews.back().byteOffset = 0;
  model.bufferViews.back().byteLength = static_cast<int64_t>(indexBytes.size());
  model.bufferViews.back().target = BufferView::Target::ELEMENT_ARRAY_BUFFER;
  ;

  int32_t accessorIndex = static_cast<int32_t>(model.accessors.size());
  model.accessors.emplace_back();
  model.accessors.back().bufferView = bufferViewIndex;
  model.accessors.back().byteOffset = 0;
  model.accessors.back().componentType = Accessor::ComponentType::FLOAT;
  model.accessors.back().count = static_cast<int64_t>(localPositions.size());
  model.accessors.back().type = Accessor::Type::VEC3;

  int32_t indexAccessorIndex = static_cast<int32_t>(model.accessors.size());
  model.accessors.emplace_back();
  model.accessors.back().bufferView = indexBufferViewIndex;
  model.accessors.back().byteOffset = 0;
  model.accessors.back().componentType = Accessor::ComponentType::UNSIGNED_INT;
  model.accessors.back().count = static_cast<int64_t>(allIndices.size());
  model.accessors.back().type = Accessor::Type::SCALAR;

  int32_t meshIndex = static_cast<int32_t>(model.meshes.size());
  model.meshes.back().primitives.emplace_back();
  model.meshes.back().primitives.back().attributes["POSITION"] = accessorIndex;
  model.meshes.back().primitives.back().indices = indexAccessorIndex;
  model.meshes.back().primitives.back().mode = MeshPrimitive::Mode::TRIANGLES;
  model.meshes.back().primitives.back().material = 0;

  model.nodes.emplace_back();
  model.nodes.back().mesh = meshIndex;
  CesiumGltfContent::GltfUtilities::setNodeTransform(
      model.nodes.back(),
      enuToFixedFrame);
}

ConverterResult GltfConverter::operator()(const GeoJsonDocument& geoJson) {
  ConverterResult result;
  const GeoJsonObject& root = geoJson.rootObject;
  glm::dvec3 centroid = computeCentroid(root);
  //  local to global cartesian
  glm::dmat4 enuToFixedFrame = GlobeTransforms::eastNorthUpToFixedFrame(
      Ellipsoid::WGS84.cartographicToCartesian(
          Cartographic::fromDegrees(centroid.x, centroid.y, centroid.z)));
  result.model.emplace();
  result.model->asset.version = "2.0";
  CesiumGltf::Material& material = result.model->materials.emplace_back();
  CesiumGltf::MaterialPBRMetallicRoughness& pbr =
      material.pbrMetallicRoughness.emplace();
  pbr.metallicFactor = 0.0;
  pbr.roughnessFactor = 1.0;
  material.doubleSided = true;

  gatherLines(geoJson, *result.model, enuToFixedFrame);
  gatherPolygons(geoJson, *result.model, enuToFixedFrame);
  return result;
}

} // namespace CesiumVectorData
