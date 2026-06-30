#include <Cesium3DTilesReader/ExtensionSchemaMaxarContentGeoJsonReader.h>
#include <CesiumGeometry/Transforms.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/GlobeTransforms.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Class.h>
#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/ExtensionExtMeshFeatures.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/FeatureId.h>
#include <CesiumGltf/Material.h>
#include <CesiumGltf/MaterialPBRMetallicRoughness.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/PropertyTable.h>
#include <CesiumGltf/PropertyTableProperty.h>
#include <CesiumGltf/Scene.h>
#include <CesiumGltf/Schema.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/JsonValue.h>
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
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumUtility;
using namespace CesiumVectorData;

namespace CesiumVectorData {
namespace {
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

using PolygonRing = std::vector<glm::dvec3>;
using Polygon = std::vector<PolygonRing>;

std::vector<uint32_t> triangulatePolygon(const Polygon& polygonIn) {
  using Point = std::array<double, 2>;
  std::vector<std::vector<Point>> polygon;
  polygon.reserve(polygonIn.size());
  for (const PolygonRing& ring : polygonIn) {
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

struct GltfConverterImpl {
  const GeoJsonDocument& geoJson;
  Model model;
  // Accumulated as the data is traversed, then used to transform coordinates
  // into a local frame.
  BoundingRegionBuilder regionBuilder;
  glm::dmat4 enuToFixedFrame;
  CesiumGeospatial::Ellipsoid ellipsoid;
  std::vector<glm::dvec3> globalCoordinates;
  // The feature ID that is associated with each vertex in the document. The
  // contents will be stored in the resulting glTF, but the values may be
  // translated to 32 bit floats.
  std::vector<uint32_t> featureIds;

  template <int32_t GltfMode> struct VectorPrimitive {
    std::vector<uint32_t> elements;
    int64_t uniqueFeatures = 0;
    static constexpr int32_t gltfMode = GltfMode;
  };

  VectorPrimitive<MeshPrimitive::Mode::LINES> lines;
  VectorPrimitive<MeshPrimitive::Mode::TRIANGLES> polys;
  VectorPrimitive<MeshPrimitive::Mode::POINTS> points;

  // two-way mapping between GeoJSON features and glTF Feature IDs
  std::unordered_map<const GeoJsonObject*, uint32_t> gltfFeatureIds;
  std::vector<const GeoJsonObject*> geoJsonFeatures;

  int32_t positionBufferIndex = -1;
  int32_t positionAccessorIndex = -1;
  int32_t elementBufferIndex = -1;
  int32_t elementBufferViewIndex = -1;
  int32_t featureIdBufferIndex = -1;
  int32_t featureIdBufferViewIndex = -1;
  int32_t featureIdAccessorIndex = -1;

  // Errors and warnings accumulated during the conversion.
  ErrorList errorList;

  GltfConverterImpl(
      const GeoJsonDocument& inGeoJson,
      const CesiumGeospatial::Ellipsoid& inEllipsoid)
      : geoJson(inGeoJson), enuToFixedFrame(1.0), ellipsoid(inEllipsoid) {
    gltfFeatureIds[nullptr] = 0;
    geoJsonFeatures.push_back(nullptr);
  }
  void addGeoJsonCoordinate(const glm::dvec3& coord, uint32_t featureId) {
    Cartographic cesiumCoord =
        Cartographic::fromDegrees(coord.x, coord.y, coord.z);
    this->regionBuilder.expandToIncludePosition(cesiumCoord);
    this->globalCoordinates.push_back(
        this->ellipsoid.cartographicToCartesian(cesiumCoord));
    this->featureIds.push_back(featureId);
  }
  int32_t finalizeLines();
  int32_t finalizePolygons();
  int32_t finalizePoints();
  template <int32_t GltfMode>
  int32_t finalizePrimitive(const VectorPrimitive<GltfMode>& vectorPrimitive);
  void preparePositions();
  void createFeatureIdAccessor(size_t numCoords);

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
  template <typename GeoJsonType>
  void visitGeometry(const GeoJsonType&, const GeoJsonObject*) {}

  void processLinestring(
      const std::vector<glm::dvec3>& lineStringCoords,
      const GeoJsonObject* pFeature) {
    std::vector<uint32_t> stringIndices = lineStringToLines(
        this->globalCoordinates.size(),
        lineStringCoords.size());
    this->lines.elements.insert(
        this->lines.elements.end(),
        stringIndices.begin(),
        stringIndices.end());
    uint32_t featureId = getFeatureId(pFeature, this->lines);
    for (const glm::dvec3& cartoDegrees : lineStringCoords) {
      addGeoJsonCoordinate(cartoDegrees, featureId);
    }
  }

  void visitGeometry(
      const GeoJsonLineString& object,
      const GeoJsonObject* pFeature) {
    processLinestring(object.coordinates, pFeature);
  }

  void visitGeometry(
      const GeoJsonMultiLineString& object,
      const GeoJsonObject* pFeature) {
    for (const std::vector<glm::dvec3>& lineStringCoords : object.coordinates) {
      processLinestring(lineStringCoords, pFeature);
    }
  }

  void
  processPolygon(const Polygon& polygonRings, const GeoJsonObject* pFeature) {
    uint32_t elementBase = uint32_t(this->globalCoordinates.size());
    uint32_t featureId = getFeatureId(pFeature, this->polys);
    for (const PolygonRing& contour : polygonRings) {
      // The last coordinate is identical to the first.
      for (size_t i = 0; i < contour.size() - 1; ++i) {
        this->addGeoJsonCoordinate(contour[i], featureId);
      }
    }
    std::vector<uint32_t> triangulatedIndices =
        triangulatePolygon(polygonRings);
    for (uint32_t index : triangulatedIndices) {
      this->polys.elements.push_back(elementBase + index);
    }
  }

  void
  visitGeometry(const GeoJsonPolygon& object, const GeoJsonObject* pFeature) {
    processPolygon(object.coordinates, pFeature);
  }

  void visitGeometry(
      const GeoJsonMultiPolygon& object,
      const GeoJsonObject* pFeature) {
    for (const std::vector<PolygonRing>& polygonRings : object.coordinates) {
      processPolygon(polygonRings, pFeature);
    }
  }

  void
  processPoint(const glm::dvec3& cartoDegrees, const GeoJsonObject* pFeature) {
    uint32_t featureId = getFeatureId(pFeature, this->points);
    this->points.elements.push_back(uint32_t(this->globalCoordinates.size()));
    addGeoJsonCoordinate(cartoDegrees, featureId);
  }

  void
  visitGeometry(const GeoJsonPoint& object, const GeoJsonObject* pFeature) {
    processPoint(object.coordinates, pFeature);
  }

  void visitGeometry(
      const GeoJsonMultiPoint& object,
      const GeoJsonObject* pFeature) {
    for (const auto& cartoDegrees : object.coordinates) {
      processPoint(cartoDegrees, pFeature);
    }
  }

  template <int32_t GltfMode>
  uint32_t getFeatureId(
      const GeoJsonObject* pFeatureObject,
      VectorPrimitive<GltfMode>& vectorPrimitive) {
    auto mapIt = this->gltfFeatureIds.find(pFeatureObject);
    if (mapIt == this->gltfFeatureIds.end()) {
      uint32_t featureId = uint32_t(this->geoJsonFeatures.size());
      gltfFeatureIds.insert(std::make_pair(pFeatureObject, featureId));
      this->geoJsonFeatures.push_back(pFeatureObject);
      vectorPrimitive.uniqueFeatures++;
      return featureId;
    }
    return mapIt->second;
  }

  struct StringPropertyRepresentation {
    std::string buffer;
    std::vector<uint64_t> offsets;

    StringPropertyRepresentation(size_t numFeatures = 0)
        : offsets(numFeatures + 1, std::numeric_limits<uint64_t>::max()) {
      // feature ID 0 is reserved for missing data
      offsets[0] = 0;
    }
  };

  template <typename Scalar> struct ScalarRepresentation {
    ScalarRepresentation(size_t numFeatures = 0) : properties(numFeatures) {}
    std::vector<Scalar> properties;
    using ScalarType = Scalar;
  };

  using FloatPropertyRepresentation = ScalarRepresentation<double>;
  using IntegerPropertyRepresentation = ScalarRepresentation<int64_t>;

  using PackedProperty = std::variant<
      StringPropertyRepresentation,
      FloatPropertyRepresentation,
      IntegerPropertyRepresentation>;
  using PackedProperties = std::unordered_map<std::string, PackedProperty>;

  void recordStringProperties(
      ExtensionModelExtStructuralMetadata& modelExtension,
      PackedProperties& packedProps) {
    PropertyTable& propertyTable = modelExtension.propertyTables.back();
    size_t metadataBufferIndex = this->model.buffers.size();
    this->model.buffers.emplace_back();
    size_t metadataSize = 0;
    size_t offsetsBufferIndex = this->model.buffers.size();
    this->model.buffers.emplace_back();
    size_t offsetsSize = 0;
    Buffer& metadataBuffer = this->model.buffers[metadataBufferIndex];
    Buffer& offsetsBuffer = this->model.buffers[offsetsBufferIndex];

    for (auto& [_, propRepVariant] : packedProps) {
      if (auto* pPropRep =
              std::get_if<StringPropertyRepresentation>(&propRepVariant)) {
        pPropRep->offsets.back() = pPropRep->buffer.size();
        // Patch up any missing data
        uint64_t lastValidOffset = 0;
        for (uint64_t& offset : pPropRep->offsets) {
          if (offset == std::numeric_limits<uint64_t>::max()) {
            offset = lastValidOffset;
          } else {
            lastValidOffset = offset;
          }
        }
        metadataSize += pPropRep->buffer.size();
        offsetsSize += pPropRep->offsets.size();
      }
    }
    metadataBuffer.cesium.data.resize(metadataSize);
    size_t offsetsByteSize = offsetsSize * sizeof(uint64_t);
    offsetsBuffer.cesium.data.resize(offsetsByteSize);
    // These offsets are in bytes.
    size_t buffOffset = 0;
    size_t offsetDataOffset = 0;
    for (auto& [propName, propRepVariant] : packedProps) {
      if (auto* pPropRep =
              std::get_if<StringPropertyRepresentation>(&propRepVariant)) {
        size_t propOffsetsByteSize =
            pPropRep->offsets.size() * sizeof(uint64_t);
        int32_t propIndex = makeBufferView(
            int32_t(metadataBufferIndex),
            BufferView::Target::ARRAY_BUFFER,
            int64_t(buffOffset),
            int64_t(pPropRep->buffer.size()));
        std::memcpy(
            metadataBuffer.cesium.data.data() + buffOffset,
            pPropRep->buffer.data(),
            pPropRep->buffer.size());
        int32_t offsetsIndex = makeBufferView(
            int32_t(offsetsBufferIndex),
            BufferView::Target::ARRAY_BUFFER,
            int64_t(offsetDataOffset),
            int64_t(propOffsetsByteSize));
        std::memcpy(
            offsetsBuffer.cesium.data.data() + offsetDataOffset,
            pPropRep->offsets.data(),
            propOffsetsByteSize);
        buffOffset += pPropRep->buffer.size();
        offsetDataOffset += pPropRep->offsets.size() * sizeof(uint64_t);
        PropertyTableProperty& propertyTableProperty =
            propertyTable.properties.emplace(propName, PropertyTableProperty())
                .first->second;
        propertyTableProperty.values = propIndex;
        propertyTableProperty.stringOffsets = offsetsIndex;
        propertyTableProperty.stringOffsetType =
            ClassProperty::ComponentType::UINT64;
      }
    }
  }

  template <typename ScalarProperty>
  void recordScalarProperty(
      ExtensionModelExtStructuralMetadata& modelExtension,
      PackedProperties& packedProps) {
    PropertyTable& propertyTable = modelExtension.propertyTables.back();
    size_t metadataBufferIndex = this->model.buffers.size();
    Buffer& metadataBuffer = this->model.buffers.emplace_back();
    size_t metadataSize = 0;

    for (auto& [_, propRepVariant] : packedProps) {
      if (auto* pPropRep = std::get_if<ScalarRepresentation<ScalarProperty>>(
              &propRepVariant)) {
        metadataSize += pPropRep->properties.size() * sizeof(ScalarProperty);
      }
    }
    metadataBuffer.cesium.data.resize(metadataSize);
    size_t buffOffset = 0;
    for (auto& [propName, propRepVariant] : packedProps) {
      if (auto* pPropRep = std::get_if<ScalarRepresentation<ScalarProperty>>(
              &propRepVariant)) {
        size_t byteSize = pPropRep->properties.size() * sizeof(ScalarProperty);
        int32_t propIndex = makeBufferView(
            int32_t(metadataBufferIndex),
            BufferView::Target::ARRAY_BUFFER,
            int64_t(buffOffset),
            int64_t(byteSize));
        std::memcpy(
            metadataBuffer.cesium.data.data() + buffOffset,
            pPropRep->properties.data(),
            byteSize);
        buffOffset += byteSize;
        PropertyTableProperty& propertyTableProperty =
            propertyTable.properties.emplace(propName, PropertyTableProperty())
                .first->second;
        propertyTableProperty.values = propIndex;
      }
    }
  }

  void recordFeatureProperties(const IntrusivePointer<Schema>& pSchema) {
    size_t numFeatures = this->geoJsonFeatures.size();
    PackedProperties packedProps;
    auto classIt = pSchema->classes.begin();
    if (classIt == pSchema->classes.end()) {
      errorList.emplaceWarning("No schema supplied for GeoJSON content.");
      return;
    }
    const Class& metaClass = classIt->second;
    for (const auto& [propertyName, classProp] : metaClass.properties) {
      if (classProp.type == ClassProperty::Type::STRING) {
        packedProps[propertyName] = StringPropertyRepresentation(numFeatures);
      } else if (
          classProp.type == ClassProperty::Type::SCALAR &&
          classProp.componentType) {
        if (*classProp.componentType == ClassProperty::ComponentType::FLOAT64) {
          packedProps[propertyName] = FloatPropertyRepresentation(numFeatures);
        } else if (
            *classProp.componentType == ClassProperty::ComponentType::INT64) {
          packedProps[propertyName] =
              IntegerPropertyRepresentation(numFeatures);
        }
      }
    }
    for (size_t featureId = 0; featureId < numFeatures; ++featureId) {
      const GeoJsonObject* pGeoJsonObject = this->geoJsonFeatures[featureId];
      if (!pGeoJsonObject) {
        continue;
      }
      const GeoJsonFeature& feature = pGeoJsonObject->get<GeoJsonFeature>();
      if (feature.properties) {
        for (auto propIt = feature.properties->begin();
             propIt != feature.properties->end();
             ++propIt) {
          const auto& [name, value] = *propIt;
          if (auto packedPropIt = packedProps.find(name);
              packedPropIt != packedProps.end()) {
            if (auto* pStringRep = std::get_if<StringPropertyRepresentation>(
                    &packedPropIt->second)) {
              pStringRep->offsets[featureId] = pStringRep->buffer.size();
              if (value.isString()) {
                pStringRep->buffer.append(get<JsonValue::String>(value.value));
              } else if (value.isDouble()) {
                pStringRep->buffer.append(std::to_string(value.getDouble()));
              } else if (value.isInt64()) {
                pStringRep->buffer.append(std::to_string(value.getInt64()));
              }
            } else if (
                auto* pFloatRep = std::get_if<FloatPropertyRepresentation>(
                    &packedPropIt->second)) {
              if (value.isDouble()) {
                pFloatRep->properties[featureId] = value.getDouble();
              } else if (value.isInt64()) {
                pFloatRep->properties[featureId] =
                    static_cast<double>(value.getInt64());
              }
            } else if (
                auto* pIntegerRep = std::get_if<IntegerPropertyRepresentation>(
                    &packedPropIt->second)) {
              if (value.isInt64()) {
                pIntegerRep->properties[featureId] = value.getInt64();
              } else if (value.isDouble()) {
                pIntegerRep->properties[featureId] =
                    static_cast<int64_t>(value.getDouble());
              }
            }
          }
        }
      }
    }
    ExtensionModelExtStructuralMetadata& modelExtension =
        this->model.addExtension<ExtensionModelExtStructuralMetadata>();
    this->model.addExtensionUsed(
        ExtensionModelExtStructuralMetadata::ExtensionName);
    modelExtension.schema = pSchema;
    modelExtension.propertyTables.emplace_back();
    if (pSchema && !pSchema->classes.empty()) {
      // Only expect one class
      auto schemaClassItr = pSchema->classes.begin();
      modelExtension.propertyTables.back().classProperty =
          schemaClassItr->first;
    } else {
      modelExtension.propertyTables.back().classProperty = "default";
    }
    modelExtension.propertyTables.back().count = int64_t(numFeatures);
    recordStringProperties(modelExtension, packedProps);
    recordScalarProperty<double>(modelExtension, packedProps);
    recordScalarProperty<int64_t>(modelExtension, packedProps);
  }
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

template <int32_t GltfMode>
int32_t GltfConverterImpl::finalizePrimitive(
    const VectorPrimitive<GltfMode>& vectorPrimitive) {
  if (vectorPrimitive.elements.empty()) {
    return -1;
  }
  int32_t meshIndex = int32_t(this->model.meshes.size());
  Mesh& primitiveMesh = this->model.meshes.emplace_back();
  std::vector<std::byte>& elementBytes =
      this->model.buffers[size_t(this->elementBufferIndex)].cesium.data;
  size_t elementBase = elementBytes.size();
  elementBytes.resize(
      elementBase + sizeof(uint32_t) * vectorPrimitive.elements.size());
  std::memcpy(
      elementBytes.data() + elementBase,
      vectorPrimitive.elements.data(),
      sizeof(uint32_t) * vectorPrimitive.elements.size());
  int32_t elementAccessorIndex = makeAccessor(
      this->elementBufferViewIndex,
      int64_t(elementBase),
      int64_t(vectorPrimitive.elements.size()),
      Accessor::ComponentType::UNSIGNED_INT,
      Accessor::Type::SCALAR);
  MeshPrimitive& primitive = primitiveMesh.primitives.emplace_back();
  primitive.attributes["POSITION"] = this->positionAccessorIndex;
  primitive.attributes["_FEATURE_ID_0"] = this->featureIdAccessorIndex;
  primitive.indices = elementAccessorIndex;
  primitive.mode = vectorPrimitive.gltfMode;
  primitive.material = 0;
  ExtensionExtMeshFeatures& extension =
      primitive.addExtension<ExtensionExtMeshFeatures>();
  FeatureId& featureId = extension.featureIds.emplace_back();
  featureId.attribute = 0;
  featureId.featureCount = vectorPrimitive.uniqueFeatures;
  featureId.propertyTable = 0;
  featureId.label = "_FEATURE_ID_0";
  featureId.nullFeatureId = 0;
  int32_t nodeIndex = int32_t(this->model.nodes.size());
  this->model.nodes.emplace_back();
  this->model.nodes.back().mesh = meshIndex;
  return nodeIndex;
}

int32_t GltfConverterImpl::finalizeLines() {
  return finalizePrimitive(this->lines);
}

int32_t GltfConverterImpl::finalizePolygons() {
  return finalizePrimitive(this->polys);
}

int32_t GltfConverterImpl::finalizePoints() {
  return finalizePrimitive(this->points);
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
  size_t numCoords = this->globalCoordinates.size();
  positionBuffer.cesium.data.resize(sizeof(glm::vec3) * numCoords);
  glm::vec3* const pPosition32Base =
      reinterpret_cast<glm::vec3*>(positionBuffer.cesium.data.data());
  glm::vec3* pPosition32 = pPosition32Base;
  for (const auto& position : this->globalCoordinates) {
    glm::dvec3 localPosition = glm::dvec3(toLocal * glm::dvec4(position, 1.0));
    // Convert to float
    *pPosition32++ =
        glm::vec3(localPosition.x, localPosition.y, localPosition.z);
  }
  int32_t positionBufferViewIndex = makeBufferView(
      this->positionBufferIndex,
      BufferView::Target::ARRAY_BUFFER);
  this->positionAccessorIndex = makeAccessor(
      positionBufferViewIndex,
      0,
      int64_t(numCoords),
      Accessor::ComponentType::FLOAT,
      Accessor::Type::VEC3);
  Accessor& positionAccessor =
      this->model.accessors[size_t(this->positionAccessorIndex)];
  std::span<const glm::vec3> positionSpan(pPosition32Base, numCoords);
  positionAccessor.min = positionMinVector(positionSpan);
  positionAccessor.max = positionMaxVector(positionSpan);
  this->elementBufferIndex = int32_t(this->model.buffers.size());
  Buffer& elementBuffer = this->model.buffers.emplace_back();
  size_t elementByteSize = sizeof(uint32_t) * (this->lines.elements.size() +
                                               this->polys.elements.size() +
                                               this->points.elements.size());
  elementBuffer.cesium.data.reserve(elementByteSize);
  this->elementBufferViewIndex = this->makeBufferView(
      this->elementBufferIndex,
      BufferView::Target::ELEMENT_ARRAY_BUFFER,
      0,
      int64_t(elementByteSize));
  createFeatureIdAccessor(numCoords);
}

void GltfConverterImpl::createFeatureIdAccessor(size_t numCoords) {
  auto maxFeatureItr =
      std::max_element(this->featureIds.begin(), this->featureIds.end());
  this->featureIdBufferIndex = int32_t(this->model.buffers.size());
  Buffer& featureIdBuffer = this->model.buffers.emplace_back();
  featureIdBuffer.cesium.data.resize(sizeof(uint32_t) * numCoords);
  int32_t componentType = 0;
  if (*maxFeatureItr <= std::numeric_limits<uint16_t>::max()) {
    if (*maxFeatureItr <= std::numeric_limits<uint8_t>::max()) {
      componentType = Accessor::ComponentType::UNSIGNED_BYTE;
    } else {
      componentType = Accessor::ComponentType::UNSIGNED_SHORT;
    }
    // Due to alignment requirements, the memory representation is the same for
    // UNSIGNED_BYTE, UNSIGNED_SHORT, and the source UNSIGNED_INT.
    std::memcpy(
        featureIdBuffer.cesium.data.data(),
        this->featureIds.data(),
        sizeof(uint32_t) * numCoords);
  } else {
    componentType = Accessor::ComponentType::FLOAT;
    float* pFeatureDest =
        reinterpret_cast<float*>(featureIdBuffer.cesium.data.data());
    for (size_t i = 0; i < numCoords; ++i) {
      pFeatureDest[i] = static_cast<float>(this->featureIds[i]);
    }
  }
  this->featureIdBufferViewIndex = makeBufferView(
      this->featureIdBufferIndex,
      BufferView::Target::ARRAY_BUFFER);
  // Again, alignment of vertex attributes is 4 bytes.
  this->model.bufferViews[size_t(this->featureIdBufferViewIndex)].byteStride =
      4;
  this->featureIdAccessorIndex = makeAccessor(
      this->featureIdBufferViewIndex,
      0,
      int64_t(numCoords),
      componentType,
      Accessor::Type::SCALAR);
}
} // namespace

ConverterResult GltfConverter::convert(
    const GeoJsonDocument& geoJson,
    const CesiumGeospatial::Ellipsoid& ellipsoid,
    const IntrusivePointer<Schema>& pSchema) {
  GltfConverterImpl converter(geoJson, ellipsoid);
  const GeoJsonObject& root = geoJson.rootObject;
  for (auto geoJsonIt = root.begin(); geoJsonIt != root.end(); ++geoJsonIt) {
    std::visit(
        [&converter, feature = geoJsonIt.getFeature()](auto&& geometry) {
          converter.visitGeometry(geometry, feature);
        },
        geoJsonIt->value);
  }
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
  if (pSchema) {
    converter.recordFeatureProperties(pSchema);
  }
  return {std::move(converter.model), std::move(converter.errorList)};
}

ConvertSchemaResult GltfConverter::convertSchema(
    const Cesium3DTiles::ExtensionSchemaMaxarContentGeoJson& maxarSchema) {
  IntrusivePointer<Schema> pSchema;
  Schema& schema = pSchema.emplace();
  schema.id = "default";
  std::string classKey;
  if (maxarSchema.semantic.empty()) {
    classKey = "geoJsonClass";
  } else {
    classKey = maxarSchema.semantic;
  }
  Class geoJsonClass;
  geoJsonClass.name = classKey;
  for (auto propsIt = maxarSchema.properties.begin();
       propsIt != maxarSchema.properties.end();
       ++propsIt) {
    ClassProperty metaClass;
    metaClass.name = propsIt->id;
    if (propsIt->type == "String") {
      metaClass.type = ClassProperty::Type::STRING;
    } else if (propsIt->type == "Float") {
      metaClass.componentType = ClassProperty::ComponentType::FLOAT64;
    } else if (propsIt->type == "Integer") {
      metaClass.componentType = ClassProperty::ComponentType::INT64;
    }
    geoJsonClass.properties[propsIt->id] = std::move(metaClass);
  }
  schema.classes[classKey] = std::move(geoJsonClass);
  return pSchema;
}

} // namespace CesiumVectorData
