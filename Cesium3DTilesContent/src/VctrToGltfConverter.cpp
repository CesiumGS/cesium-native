#include "BatchTableToGltfStructuralMetadata.h"

#include <Cesium3DTilesContent/VctrToGltfConverter.h>
#include <CesiumGeometry/Transforms.h>
#include <CesiumGltf/AccessorUtility.h>
#include <CesiumGltf/ExtensionCesiumRTC.h>
#include <CesiumGltf/ExtensionKhrMaterialsUnlit.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumUtility/Log.h>
#include <CesiumUtility/Math.h>

#ifdef _WIN32
#include <corecrt_math_defines.h>
#else
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <draco/attributes/point_attribute.h>
#include <draco/compression/decode.h>
#include <draco/core/decoder_buffer.h>
#include <draco/point_cloud/point_cloud.h>

#include <Cesium3DTilesContent/TileBoundingVolumes.h>

using namespace CesiumGltf;
using namespace CesiumUtility;

namespace Cesium3DTilesContent {
namespace {
struct VctrHeader {
  unsigned char magic[4];
  uint32_t version;
  uint32_t byteLength;
  uint32_t featureTableJsonByteLength;
  uint32_t featureTableBinaryByteLength;
  uint32_t batchTableJsonByteLength;
  uint32_t batchTableBinaryByteLength;
  uint32_t polygonIndicesByteLength;
  uint32_t polygonPositionsByteLength;
  uint32_t polylinePositionsByteLength;
  uint32_t pointPositionsByteLength;
};

struct MetadataProperty {
public:
  enum ComponentType {
    BYTE,
    UNSIGNED_BYTE,
    SHORT,
    UNSIGNED_SHORT,
    INT,
    UNSIGNED_INT,
    FLOAT,
    DOUBLE
  };

  enum Type { SCALAR, VEC2, VEC3, VEC4 };

  static std::optional<ComponentType>
  getComponentTypeFromDracoDataType(const draco::DataType dataType) {
    switch (dataType) {
    case draco::DT_INT8:
      return ComponentType::BYTE;
    case draco::DT_UINT8:
      return ComponentType::UNSIGNED_BYTE;
    case draco::DT_INT16:
      return ComponentType::SHORT;
    case draco::DT_UINT16:
      return ComponentType::UNSIGNED_SHORT;
    case draco::DT_INT32:
      return ComponentType::INT;
    case draco::DT_UINT32:
      return ComponentType::UNSIGNED_INT;
    case draco::DT_FLOAT32:
      return ComponentType::FLOAT;
    case draco::DT_FLOAT64:
      return ComponentType::DOUBLE;
    default:
      return std::nullopt;
    }
  }

  static size_t getSizeOfComponentType(ComponentType componentType) {
    switch (componentType) {
    case ComponentType::BYTE:
    case ComponentType::UNSIGNED_BYTE:
      return sizeof(uint8_t);
    case ComponentType::SHORT:
    case ComponentType::UNSIGNED_SHORT:
      return sizeof(uint16_t);
    case ComponentType::INT:
    case ComponentType::UNSIGNED_INT:
      return sizeof(uint32_t);
    case ComponentType::FLOAT:
      return sizeof(float);
    case ComponentType::DOUBLE:
      return sizeof(double);
    default:
      return 0;
    }
  };

  static std::optional<Type>
  getTypeFromNumberOfComponents(int8_t numComponents) {
    switch (numComponents) {
    case 1:
      return Type::SCALAR;
    case 2:
      return Type::VEC2;
    case 3:
      return Type::VEC3;
    case 4:
      return Type::VEC4;
    default:
      return std::nullopt;
    }
  }
};

  struct DracoMetadataSemantic {
  int32_t dracoId;
  MetadataProperty::ComponentType componentType;
  MetadataProperty::Type type;
};

enum PntsColorType { CONSTANT, RGBA, RGB, RGB565 };

template <typename TColor> TColor srgbToLinear(const TColor srgb) {
  static_assert(
      std::is_same_v<TColor, glm::vec3> || std::is_same_v<TColor, glm::vec4>);

  glm::vec3 srgbInput = glm::vec3(srgb);
  glm::vec3 linearOutput = glm::pow(srgbInput, glm::vec3(2.2f));

  if constexpr (std::is_same_v<TColor, glm::vec4>) {
    return glm::vec4(linearOutput, srgb.w);
  } else if constexpr (std::is_same_v<TColor, glm::vec3>) {
    return linearOutput;
  }
}

struct PntsSemantic {
  uint32_t byteOffset = 0;
  std::optional<int32_t> dracoId;
  std::vector<std::byte> data;
};
struct PntsContent {
  uint32_t pointsLength = 0;
  std::optional<glm::dvec3> rtcCenter;
  std::optional<glm::dvec3> quantizedVolumeOffset;
  std::optional<glm::dvec3> quantizedVolumeScale;
  std::optional<glm::u8vec4> constantRgba;
  std::optional<uint32_t> batchLength;

  PntsSemantic position;
  bool positionQuantized = false;

  glm::vec3 positionMin = glm::vec3(std::numeric_limits<float>::max());
  glm::vec3 positionMax = glm::vec3(std::numeric_limits<float>::lowest());

  std::optional<PntsSemantic> color;
  PntsColorType colorType = PntsColorType::CONSTANT;

  std::optional<PntsSemantic> normal;
  bool normalOctEncoded = false;

  std::optional<PntsSemantic> batchId;
  std::optional<MetadataProperty::ComponentType> batchIdComponentType;

  std::optional<uint32_t> dracoByteOffset;
  std::optional<uint32_t> dracoByteLength;

  std::map<std::string, DracoMetadataSemantic> dracoMetadataSemantics;
  std::vector<std::byte> dracoBatchTableBinary;

  ErrorList errors;
  bool dracoMetadataHasErrors = false;
};


const double maxShort = 32767.0;
// ZigZag decoding function
int16_t zigZagDecode(uint16_t value) {
  return static_cast<int16_t>((value >> 1) ^ (-(value & 1)));
}

// Align byte offset to 4-byte boundary
uint32_t alignByteOffset(uint32_t byteOffset) {
  return byteOffset + ((4 - (byteOffset % 4)) % 4);
}

// Buffer creation function
int32_t
createBufferInGltf(CesiumGltf::Model& gltf, std::vector<std::byte>&& buffer) {
  size_t bufferId = gltf.buffers.size();
  CesiumGltf::Buffer& gltfBuffer = gltf.buffers.emplace_back();
  gltfBuffer.byteLength = static_cast<int32_t>(buffer.size());
  gltfBuffer.cesium.data = std::move(buffer);

  return static_cast<int32_t>(bufferId);
}

// Buffer view creation function
int32_t createBufferViewInGltf(
    CesiumGltf::Model& gltf,
    const int32_t bufferId,
    const int64_t byteLength,
    const int64_t byteStride,
    const int32_t target =
        static_cast<int32_t>(CesiumGltf::BufferView::Target::ARRAY_BUFFER)) {
  size_t bufferViewId = gltf.bufferViews.size();
  CesiumGltf::BufferView& bufferView = gltf.bufferViews.emplace_back();
  bufferView.buffer = bufferId;
  bufferView.byteLength = byteLength;
  bufferView.byteOffset = 0;
  if (byteStride > 0) {
    bufferView.byteStride = byteStride;
  }
  bufferView.target = target;

  return static_cast<int32_t>(bufferViewId);
}

// Accessor creation function
int32_t createAccessorInGltf(
    CesiumGltf::Model& gltf,
    const int32_t bufferViewId,
    const int32_t componentType,
    const int64_t count,
    const std::string type) {
  size_t accessorId = gltf.accessors.size();
  CesiumGltf::Accessor& accessor = gltf.accessors.emplace_back();
  accessor.bufferView = bufferViewId;
  accessor.byteOffset = 0;
  accessor.componentType = componentType;
  accessor.count = count;
  accessor.type = type;

  return static_cast<int32_t>(accessorId);
}

// Function to add VCTR position data to glTF
void addVctrPositionsToGltf(
    const std::vector<glm::vec3>& positions,
    const glm::dvec3& positionMin,
    const glm::dvec3& positionMax,
    CesiumGltf::Model& gltf,
    CesiumGltf::MeshPrimitive& primitive,
    bool createAccessor) {

  const int64_t count = static_cast<int64_t>(positions.size());
  const int64_t byteStride = static_cast<int64_t>(sizeof(glm::vec3));
  const int64_t byteLength = byteStride * count;

  // Create a buffer
  std::vector<std::byte> positionData(static_cast<size_t>(byteLength));
  std::memcpy(
      positionData.data(),
      positions.data(),
      static_cast<size_t>(byteLength));
  int32_t bufferId = createBufferInGltf(gltf, std::move(positionData));

  // Create a buffer view
  int32_t bufferViewId =
      createBufferViewInGltf(gltf, bufferId, byteLength, byteStride);

   // Create an accessor
  if (createAccessor) {
    int32_t accessorId = createAccessorInGltf(
        gltf,
        bufferViewId,
        CesiumGltf::Accessor::ComponentType::FLOAT,
        count,
        CesiumGltf::Accessor::Type::VEC3);

    // Set min/max values
    CesiumGltf::Accessor& accessor =
        gltf.accessors[static_cast<uint32_t>(accessorId)];
    accessor.min = {
        positionMin.x,
        positionMin.y,
        positionMin.z,
    };
    accessor.max = {
        positionMax.x,
        positionMax.y,
        positionMax.z,
    };

    // Add position property to primitive
    primitive.attributes.emplace("POSITION", accessorId);
  }
}

// Function to add VCTR color data to glTF
void addVctrColorsToGltf(
    const std::vector<glm::vec4>& colors,
    CesiumGltf::Model& gltf,
    CesiumGltf::MeshPrimitive& primitive) {

  const int64_t count = static_cast<int64_t>(colors.size());
  const int64_t byteStride = static_cast<int64_t>(sizeof(glm::vec4));
  const int64_t byteLength = byteStride * count;

  // Create a buffer
  std::vector<std::byte> colorData(static_cast<size_t>(byteLength));
  std::memcpy(colorData.data(), colors.data(), static_cast<size_t>(byteLength));
  int32_t bufferId = createBufferInGltf(gltf, std::move(colorData));

  // Create a buffer view
  int32_t bufferViewId =
      createBufferViewInGltf(gltf, bufferId, byteLength, byteStride);

  // Create an accessor
  int32_t accessorId = createAccessorInGltf(
      gltf,
      bufferViewId,
      CesiumGltf::Accessor::ComponentType::FLOAT,
      count,
      CesiumGltf::Accessor::Type::VEC4);

  // Add color properties to primitives
  primitive.attributes.emplace("COLOR_0", accessorId);

  // Settings for cases where translucency processing is required
  if (primitive.material >= 0 &&
      primitive.material < static_cast<int32_t>(gltf.materials.size())) {
    CesiumGltf::Material& material =
        gltf.materials[static_cast<uint32_t>(primitive.material)];
    material.alphaMode = CesiumGltf::Material::AlphaMode::BLEND;
  }
}

// Function to add VCTR index data to glTF
void addVctrIndicesToGltf(
    const std::vector<uint32_t>& indices,
    CesiumGltf::Model& gltf,
    CesiumGltf::MeshPrimitive& primitive) {

  const int64_t count = static_cast<int64_t>(indices.size());
  const int64_t byteStride = 0;
  const int64_t byteLength = static_cast<int64_t>(sizeof(uint32_t)) * count;

  // Create a buffer
  std::vector<std::byte> indexData(static_cast<size_t>(byteLength));
  std::memcpy(
      indexData.data(),
      indices.data(),
      static_cast<size_t>(byteLength));
  int32_t bufferId = createBufferInGltf(gltf, std::move(indexData));

  // Create a buffer view (use the ELEMENT_ARRAY_BUFFER target for the index)
  int32_t bufferViewId = createBufferViewInGltf(
      gltf,
      bufferId,
      byteLength,
      byteStride,
      static_cast<int32_t>(
          CesiumGltf::BufferView::Target::ELEMENT_ARRAY_BUFFER));

  // Create an accessor
  int32_t accessorId = createAccessorInGltf(
      gltf,
      bufferViewId,
      CesiumGltf::Accessor::ComponentType::UNSIGNED_INT,
      count,
      CesiumGltf::Accessor::Type::SCALAR);

  // Add an index property to a primitive
  primitive.indices = accessorId;
}

// Function to add VCTR normal data to glTF
void addVctrNormalsToGltf(
    const std::vector<glm::vec3>& normals,
    CesiumGltf::Model& gltf,
    CesiumGltf::MeshPrimitive& primitive) {

  const int64_t count = static_cast<int64_t>(normals.size());
  const int64_t byteStride = static_cast<int64_t>(sizeof(glm::vec3));
  const int64_t byteLength = byteStride * count;

  // Create a buffer
  std::vector<std::byte> normalData(static_cast<size_t>(byteLength));
  std::memcpy(
      normalData.data(),
      normals.data(),
      static_cast<size_t>(byteLength));
  int32_t bufferId = createBufferInGltf(gltf, std::move(normalData));

  // Create a buffer view
  int32_t bufferViewId =
      createBufferViewInGltf(gltf, bufferId, byteLength, byteStride);

  // Create an accessor
  int32_t accessorId = createAccessorInGltf(
      gltf,
      bufferViewId,
      CesiumGltf::Accessor::ComponentType::FLOAT,
      count,
      CesiumGltf::Accessor::Type::VEC3);

  // Add normal properties to primitives
  primitive.attributes.emplace("NORMAL", accessorId);
}

// Function to calculate normals using linear interpolation (for polygons)
std::vector<glm::vec3> computeNormalsForPolygons(
    const std::vector<glm::vec3>& positions,
    const std::vector<uint32_t>& indices) {

  std::vector<glm::vec3> normals(positions.size(), glm::vec3(0.0f));

  // Calculate and accumulate normals for each triangle
  for (size_t i = 0; i < indices.size(); i += 3) {
    if (i + 2 >= indices.size())
      break;

    uint32_t idx0 = indices[i];
    uint32_t idx1 = indices[i + 1];
    uint32_t idx2 = indices[i + 2];

    if (idx0 >= positions.size() || idx1 >= positions.size() ||
        idx2 >= positions.size())
      continue;

    glm::vec3 v0 = positions[idx0];
    glm::vec3 v1 = positions[idx1];
    glm::vec3 v2 = positions[idx2];

    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 normal = glm::cross(edge1, edge2);

    // Not normalized to proportional to area음
    normals[idx0] += normal;
    normals[idx1] += normal;
    normals[idx2] += normal;
  }

  // Normalize all normals
  for (auto& normal : normals) {
    if (glm::length(normal) > 0.0f) {
      normal = glm::normalize(normal);
    } else {
      normal = glm::vec3(0.0f, 1.0f, 0.0f); // default
    }
  }

  return normals;
}

void parseVctrHeader(
    const std::span<const std::byte>& vctrBinary,
    VctrHeader& header,
    GltfConverterResult& result) {
  if (vctrBinary.size() < sizeof(VctrHeader)) {
    result.errors.emplaceError("The VCTR is invalid because it is too small to "
                               "include a VCTR header.");
    return;
  }

  const VctrHeader* pHeader =
      reinterpret_cast<const VctrHeader*>(vctrBinary.data());

  header = *pHeader;

  if (static_cast<uint32_t>(vctrBinary.size()) < pHeader->byteLength) {
    result.errors.emplaceError(
        "The VCTR is invalid because the total data available is less than the "
        "size specified in its header.");
    return;
  }

  // Check magic value
  if (header.magic[0] != 'v' || header.magic[1] != 'c' ||
      header.magic[2] != 't' || header.magic[3] != 'r') {
    result.errors.emplaceError(
        "The VCTR is invalid because the magic field is not 'vctr'.");
    return;
  }
}

glm::dvec3 computeRegionCenter(const std::array<double, 6>& region) {
  // region: [west, south, east, north, min_height, max_height]
  double west = region[0];
  double east = region[2];

  // Handle the international date line (if east < west)
  if (east < west) {
    east += 2.0 * M_PI; // TWO_PI
  }

  // Calculate longitude and normalize to the range -PI to PI
  double longitude = (west + east) * 0.5;
  // negativePiToPi function (scales values ​​to the range -PI to PI)
  while (longitude > M_PI)
    longitude -= 2.0 * M_PI;
  while (longitude < -M_PI)
    longitude += 2.0 * M_PI;

  // Calculate latitude (simple median)
  double latitude = (region[1] + region[3]) * 0.5;

  // Calculate height (optional - original code uses 0)
  double height = 0.0; // Use 0 in CesiumJs JavaScript code
  //double height = (region[4] + region[5]) * 0.5; // Use median of min/max heights

  return glm::dvec3(longitude, latitude, height);
}

rapidjson::Document parseFeatureTableJsonData(
    const std::span<const std::byte>& featureTableJsonData,
    GltfConverterResult& result) {

  if (!result.model.has_value())
    result.model.emplace();

  rapidjson::Document document;
  document.Parse(
      reinterpret_cast<const char*>(featureTableJsonData.data()),
      featureTableJsonData.size());
  if (document.HasParseError()) {
    result.errors.emplaceError(fmt::format(
        "Error when parsing feature table JSON, error code {} at byte offset "
        "{}",
        document.GetParseError(),
        document.GetErrorOffset()));
    return document;
  }

  const auto rtcIt = document.FindMember("RTC_CENTER");
  if (rtcIt != document.MemberEnd() && rtcIt->value.IsArray() &&
      rtcIt->value.Size() == 3 && rtcIt->value[0].IsNumber() &&
      rtcIt->value[1].IsNumber() && rtcIt->value[2].IsNumber()) {
    // Add the RTC_CENTER value to the glTF as a CESIUM_RTC extension.
    rapidjson::Value& rtcValue = rtcIt->value;
    auto& cesiumRTC =
        result.model->addExtension<CesiumGltf::ExtensionCesiumRTC>();
    result.model->addExtensionRequired(
        CesiumGltf::ExtensionCesiumRTC::ExtensionName);
    cesiumRTC.center = {
        rtcValue[0].GetDouble(),
        rtcValue[1].GetDouble(),
        rtcValue[2].GetDouble()};
  } else {
    // No RTC_CENTER found
  }

  return document;
}

// VCTR polygon processing function
void processPolygons(
    const std::span<const std::byte>& vctrBinary,
    const VctrHeader& header,
    uint32_t headerSize,
    const rapidjson::Document& featureTableJson,
    GltfConverterResult& result) {

  if (!result.model) {
    result.model.emplace();
  }

  if (header.polygonPositionsByteLength == 0 ||
      header.polygonIndicesByteLength == 0) {
    return;
  }

  CesiumGltf::Model& model = result.model.value();

  model.extras["vctr_polygon"] = static_cast<int>(1);

  //Get polygon counts and other feature table properties
  int32_t polygonsLength = 0;
  std::vector<uint32_t> polygonCounts;
  std::vector<uint32_t> polygonIndexCounts;
  std::array<double, 6> region =
      {0, 0, 0, 0, 0, 0}; // [west, south, east, north, min_height, max_height]

  // Extract POLYGONS_LENGTH
  const auto polygonsLengthIt = featureTableJson.FindMember("POLYGONS_LENGTH");
  if (polygonsLengthIt != featureTableJson.MemberEnd() &&
      polygonsLengthIt->value.IsInt()) {
    polygonsLength = polygonsLengthIt->value.GetInt();
  }

  // Extract REGION
  const auto regionIt = featureTableJson.FindMember("REGION");
  if (regionIt != featureTableJson.MemberEnd() && regionIt->value.IsArray() &&
      regionIt->value.Size() == 6) {
    for (rapidjson::SizeType i = 0; i < 6; i++) {
      region[i] = regionIt->value[i].GetDouble();
    }
  } else {
    result.errors.emplaceWarning("VCTR tile is missing REGION property");
    return;
  }

  const size_t featureTableBinaryOffset =
      headerSize + header.featureTableJsonByteLength;

  // Extract POLYGON_COUNT (binary reference)
  const auto polygonCountIt = featureTableJson.FindMember("POLYGON_COUNT");
  if (polygonCountIt != featureTableJson.MemberEnd() &&
      polygonCountIt->value.IsObject() &&
      polygonCountIt->value.HasMember("byteOffset")) {

    uint32_t byteOffset = polygonCountIt->value["byteOffset"].GetUint();

    // Read polygonsLength uint32 values
    size_t dataSize = static_cast<size_t>(polygonsLength) * sizeof(uint32_t);
    if (byteOffset + dataSize <= header.featureTableBinaryByteLength) {
      const uint32_t* countsData = reinterpret_cast<const uint32_t*>(
          vctrBinary.data() + featureTableBinaryOffset + byteOffset);

      polygonCounts.resize(static_cast<size_t>(polygonsLength));
      for (int32_t i = 0; i < polygonsLength; i++) {
        polygonCounts[static_cast<size_t>(i)] = countsData[i];
      }

    } else {
      result.errors.emplaceError(
          "POLYGON_COUNT data exceeds feature table binary bounds");
      return;
    }
  } else {
    result.errors.emplaceError("POLYGON_COUNT not found or invalid format");
    return;
  }

  // Extract POLYGON_INDEX_COUNT (binary reference)
  const auto polygonIndexCountIt =
      featureTableJson.FindMember("POLYGON_INDEX_COUNT");
  if (polygonIndexCountIt != featureTableJson.MemberEnd() &&
      polygonIndexCountIt->value.IsObject() &&
      polygonIndexCountIt->value.HasMember("byteOffset")) {

    uint32_t byteOffset = polygonIndexCountIt->value["byteOffset"].GetUint();

    // Read polygonsLength uint32 values
    size_t dataSize = static_cast<size_t>(polygonsLength) * sizeof(uint32_t);
    if (byteOffset + dataSize <= header.featureTableBinaryByteLength) {
      const uint32_t* indexCountsData = reinterpret_cast<const uint32_t*>(
          vctrBinary.data() + featureTableBinaryOffset + byteOffset);

      polygonIndexCounts.resize(static_cast<size_t>(polygonsLength));
      for (int32_t i = 0; i < polygonsLength; i++) {
        polygonIndexCounts[static_cast<size_t>(i)] = indexCountsData[i];
      }

    } else {
      result.errors.emplaceError(
          "POLYGON_INDEX_COUNT data exceeds feature table binary bounds");
      return;
    }
  } else {
    result.errors.emplaceError(
        "POLYGON_INDEX_COUNT not found or invalid format");
    return;
  }

  // Extract POLYGON_BATCH_IDS (binary reference)
  std::vector<uint16_t> polygonBatchIds;
  const auto polygonBatchIdsIt =
      featureTableJson.FindMember("POLYGON_BATCH_IDS");
  if (polygonBatchIdsIt != featureTableJson.MemberEnd() &&
      polygonBatchIdsIt->value.IsObject() &&
      polygonBatchIdsIt->value.HasMember("byteOffset")) {

    uint32_t byteOffset = polygonBatchIdsIt->value["byteOffset"].GetUint();

    // Read polygonsLength uint16 values ​​(UNSIGNED_SHORT)
    size_t dataSize = static_cast<size_t>(polygonsLength) * sizeof(uint16_t);
    if (byteOffset + dataSize <= header.featureTableBinaryByteLength) {
      const uint16_t* batchIdsData = reinterpret_cast<const uint16_t*>(
          vctrBinary.data() + featureTableBinaryOffset + byteOffset);

      polygonBatchIds.resize(static_cast<size_t>(polygonsLength));
      for (int32_t i = 0; i < polygonsLength; i++) {
        polygonBatchIds[static_cast<size_t>(i)] = batchIdsData[i];
      }
    } else {
      result.errors.emplaceWarning(
          "POLYGON_BATCH_IDS data exceeds feature table binary bounds");
    }
  }

  // check polygonCounts and polygonIndexCounts sizes
  if (polygonCounts.size() != polygonIndexCounts.size()) {
    result.errors.emplaceError(
        "Polygon counts and index counts array sizes don't match");
    return;
  }

  // extract POLYGON_MINIMUM_HEIGHTS and POLYGON_MAXIMUM_HEIGHTS
  std::vector<float> polygonMinimumHeights;
  std::vector<float> polygonMaximumHeights;

  const auto minHeightsIt =
      featureTableJson.FindMember("POLYGON_MINIMUM_HEIGHTS");
  const auto maxHeightsIt =
      featureTableJson.FindMember("POLYGON_MAXIMUM_HEIGHTS");

  if (minHeightsIt != featureTableJson.MemberEnd() &&
      maxHeightsIt != featureTableJson.MemberEnd() &&
      minHeightsIt->value.IsArray() && maxHeightsIt->value.IsArray()) {

    const auto& minHeightsArray = minHeightsIt->value;
    const auto& maxHeightsArray = maxHeightsIt->value;

    if (minHeightsArray.Size() == polygonCounts.size() &&
        maxHeightsArray.Size() == polygonCounts.size()) {

      polygonMinimumHeights.reserve(minHeightsArray.Size());
      polygonMaximumHeights.reserve(maxHeightsArray.Size());

      for (rapidjson::SizeType i = 0; i < minHeightsArray.Size(); i++) {
        if (minHeightsArray[i].IsNumber() && maxHeightsArray[i].IsNumber()) {
          polygonMinimumHeights.push_back(minHeightsArray[i].GetFloat());
          polygonMaximumHeights.push_back(maxHeightsArray[i].GetFloat());
        } else {
          polygonMinimumHeights.clear();
          polygonMaximumHeights.clear();
          break;
        }
      }
    }
  }

  // Read indices from binary buffer
  const size_t indicesOffset = alignByteOffset(
      headerSize + header.featureTableJsonByteLength +
      header.featureTableBinaryByteLength + header.batchTableJsonByteLength +
      header.batchTableBinaryByteLength);

  std::vector<uint32_t> indices;
  if (header.polygonIndicesByteLength > 0) {
    const uint32_t* rawIndices =
        reinterpret_cast<const uint32_t*>(vctrBinary.data() + indicesOffset);
    size_t indexCount = header.polygonIndicesByteLength / sizeof(uint32_t);
    indices.resize(indexCount);
    for (size_t i = 0; i < indexCount; i++) {
      indices[i] = rawIndices[i];
    }
  }

  // Read and decode positions from binary buffer
  const size_t positionsOffset =
      indicesOffset + header.polygonIndicesByteLength;

  std::vector<glm::dvec3> decodedPositions;

  if (header.polygonPositionsByteLength > 0) {
    const uint16_t* rawPositions =
        reinterpret_cast<const uint16_t*>(vctrBinary.data() + positionsOffset);

    size_t vertexCount =
        header.polygonPositionsByteLength / (sizeof(uint16_t) * 2);
    decodedPositions.resize(vertexCount);

    int16_t u = 0;
    int16_t v = 0;

    const double west = region[0];
    const double south = region[1];
    const double east = region[2];
    const double north = region[3];

    CesiumGeospatial::Ellipsoid ellipsoid = CesiumGeospatial::Ellipsoid::WGS84;

    // Decode ZigZag encodings
    for (size_t i = 0; i < vertexCount; i++) {
      uint16_t uValue = rawPositions[i];
      uint16_t vValue = rawPositions[i + vertexCount];

      u += zigZagDecode(uValue);
      v += zigZagDecode(vValue);

      double uRatio = static_cast<double>(u) / maxShort;
      double vRatio = static_cast<double>(v) / maxShort;
      const double longitude = CesiumUtility::Math::lerp(west, east, uRatio);
      const double latitude = CesiumUtility::Math::lerp(south, north, vRatio);

      double height = 0.0; // default height

      glm::dvec3 position = ellipsoid.cartographicToCartesian(
          CesiumGeospatial::Cartographic(longitude, latitude, height));

      decodedPositions[i] = position;
    }
  }

  // if no positions are decoded, return early
  if (decodedPositions.empty()) {
    return;
  }

  // for RTC
  auto& cesiumRTC =
      result.model->addExtension<CesiumGltf::ExtensionCesiumRTC>();
  result.model->addExtensionRequired(
      CesiumGltf::ExtensionCesiumRTC::ExtensionName);

  // Use the center of the region as the center of the RTC
  glm::dvec3 rtcPos = computeRegionCenter(region);

  CesiumGeospatial::Ellipsoid ellipsoid = CesiumGeospatial::Ellipsoid::WGS84;

  rtcPos = ellipsoid.cartographicToCartesian(
      CesiumGeospatial::Cartographic(rtcPos.x, rtcPos.y, rtcPos.z));

  cesiumRTC.center = {rtcPos.x, rtcPos.y, rtcPos.z};

  // Convert to RTC relative position and calibrate coordinate system
  std::vector<glm::vec3> relativePositions;
  relativePositions.reserve(decodedPositions.size());

  glm::vec3 positionMin(std::numeric_limits<float>::max());
  glm::vec3 positionMax(std::numeric_limits<float>::lowest());

  for (const auto& pos : decodedPositions) {
    glm::vec3 relPos = glm::vec3(
        pos.x - cesiumRTC.center[0],
        pos.y - cesiumRTC.center[1],
        pos.z - cesiumRTC.center[2]);

    // To-Do: investigate this coordinate system correction is necessary. in Unity, result object location was no problem.
    glm::vec3 correctedPos = glm::vec3(relPos.x, relPos.z, -relPos.y);

    relativePositions.push_back(correctedPos);

    // Update min/max values
    positionMin = glm::min(positionMin, correctedPos);
    positionMax = glm::max(positionMax, correctedPos);
  }

  // Create glTF mesh with the helper functions
  if (!relativePositions.empty() && !indices.empty()) {
    model.meshes.emplace_back();
    auto& mesh = model.meshes.back();
    mesh.name = "VCTR_Polygon";

    // Save polygon information to extras
    if (!polygonCounts.empty() && !polygonIndexCounts.empty()) {
      // Save the number of polygons
      model.extras["vctr_polygon_count"] =
          static_cast<int64_t>(polygonCounts.size());

      // Store the vertex count of each polygon as a string (for easy parsing)
      std::string vertexCounts;
      std::string indexCounts;

      for (size_t i = 0; i < polygonCounts.size(); i++) {
        if (i > 0) {
          vertexCounts += ",";
          indexCounts += ",";
        }
        vertexCounts += std::to_string(polygonCounts[i]);
        if (i < polygonIndexCounts.size()) {
          indexCounts += std::to_string(polygonIndexCounts[i]);
        }
      }

      model.extras["vctr_polygon_vertex_counts"] = vertexCounts;
      model.extras["vctr_polygon_index_counts"] = indexCounts;
    }

    // Create a material
    size_t materialId = model.materials.size();
    CesiumGltf::Material& material = model.materials.emplace_back();
    material.pbrMetallicRoughness =
        std::make_optional<CesiumGltf::MaterialPBRMetallicRoughness>();
    material.pbrMetallicRoughness.value().metallicFactor = 0.0;
    material.pbrMetallicRoughness.value().roughnessFactor = 1.0;

    // Primitive Creation
    mesh.primitives.emplace_back();
    auto& primitive = mesh.primitives.back();
    primitive.mode = CesiumGltf::MeshPrimitive::Mode::TRIANGLES;
    primitive.material = static_cast<int32_t>(materialId);

    // Add location data (using RTC relative positioning)
    addVctrPositionsToGltf(
        relativePositions, // Use relative position
        glm::dvec3(positionMin),
        glm::dvec3(positionMax),
        model,
        primitive,
        true);

    // Add index data
    addVctrIndicesToGltf(indices, model, primitive);

    // Batch ID processing
    if (!polygonBatchIds.empty()) {
      // Generate batch ID for each vertex (index-based)
      std::vector<uint16_t> vertexBatchIds;
      vertexBatchIds.reserve(relativePositions.size());

      // Track the current vertex index
      uint32_t currentVertexIndex = 0;

      for (size_t polygonIndex = 0; polygonIndex < polygonCounts.size();
           polygonIndex++) {
        uint32_t vertexCount = polygonCounts[polygonIndex];
        uint16_t batchId = (polygonIndex < polygonBatchIds.size())
                               ? polygonBatchIds[polygonIndex]
                               : 0;

        // Assign the same batch ID to all vertices of this polygon
        for (uint32_t v = 0; v < vertexCount; v++) {
          if (currentVertexIndex < relativePositions.size()) {
            vertexBatchIds.push_back(batchId);
          }
          currentVertexIndex++;
        }
      }

      // Create batch ID buffer
      const int64_t batchIdByteLength =
          static_cast<int64_t>(vertexBatchIds.size()) *
          static_cast<int64_t>(sizeof(uint16_t));
      std::vector<std::byte> batchIdData(
          static_cast<size_t>(batchIdByteLength));
      std::memcpy(
          batchIdData.data(),
          vertexBatchIds.data(),
          static_cast<size_t>(batchIdByteLength));
      int32_t batchIdBufferId =
          createBufferInGltf(model, std::move(batchIdData));

      // Create a batch ID buffer view
      int32_t batchIdBufferViewId = createBufferViewInGltf(
          model,
          batchIdBufferId,
          batchIdByteLength,
          static_cast<int64_t>(sizeof(uint16_t)));

      // Create a batch ID accessor
      int32_t batchIdAccessorId = createAccessorInGltf(
          model,
          batchIdBufferViewId,
          CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT,
          static_cast<int64_t>(vertexBatchIds.size()),
          CesiumGltf::Accessor::Type::SCALAR);

      // Add _BATCHID property to primitive
      primitive.attributes.emplace("_BATCHID", batchIdAccessorId);

      // Save batch table information to extras
      model.extras["vctr_polygon_batch_length"] =
          static_cast<int64_t>(polygonBatchIds.size());
    }

    // Calculate and add normals
    std::vector<glm::vec3> normals = computeNormalsForPolygons(
        relativePositions,
        indices); // Calculate normals based on relative positions
    addVctrNormalsToGltf(normals, model, primitive);
  }
}

// Batch ID processing function for polylines
void addPolylineBatchIds(
    const std::vector<uint16_t>& polylineBatchIds,
    const std::vector<uint32_t>& polylineCounts,
    const size_t totalVertices,
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive) {

  if (polylineBatchIds.empty()) {
    return;
  }

  // Generate a batch ID for each vertex
  std::vector<uint16_t> vertexBatchIds;
  vertexBatchIds.reserve(totalVertices);

  uint32_t currentVertexIndex = 0;
  for (size_t polylineIndex = 0; polylineIndex < polylineCounts.size();
       polylineIndex++) {
    uint32_t vertexCount = polylineCounts[polylineIndex];
    uint16_t batchId = (polylineIndex < polylineBatchIds.size())
                           ? polylineBatchIds[polylineIndex]
                           : 0;

    // Assign the same batch ID to all vertices of this polyline
    for (uint32_t v = 0; v < vertexCount; v++) {
      if (currentVertexIndex < totalVertices) {
        vertexBatchIds.push_back(batchId);
      }
      currentVertexIndex++;
    }
  }

  // Create a batch ID buffer
  const int64_t batchIdByteLength =
      static_cast<int64_t>(vertexBatchIds.size() * sizeof(uint16_t));
  std::vector<std::byte> batchIdData(static_cast<size_t>(batchIdByteLength));
  std::memcpy(
      batchIdData.data(),
      vertexBatchIds.data(),
      static_cast<size_t>(batchIdByteLength));
  int32_t batchIdBufferId = createBufferInGltf(model, std::move(batchIdData));

  // Create a batch ID buffer view
  int32_t batchIdBufferViewId = createBufferViewInGltf(
      model,
      batchIdBufferId,
      batchIdByteLength,
      sizeof(uint16_t));

  // Create a Batch ID Accessor
  int32_t batchIdAccessorId = createAccessorInGltf(
      model,
      batchIdBufferViewId,
      CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT,
      static_cast<int64_t>(vertexBatchIds.size()),
      CesiumGltf::Accessor::Type::SCALAR);

  // Add _BATCHID property to primitives
  primitive.attributes.emplace("_BATCHID", batchIdAccessorId);
}

// VCTR polyline processing function
void processPolylines(
    const std::span<const std::byte>& vctrBinary,
    const VctrHeader& header,
    uint32_t headerSize,
    const rapidjson::Document& featureTableJson,
    GltfConverterResult& result) {

  if (!result.model) {
    result.model.emplace();
  }

  if (header.polylinePositionsByteLength == 0) {
    return;
  }

  CesiumGltf::Model& model = result.model.value();

  model.extras["vctr_polyline"] = static_cast<int>(1);

  // Get polyline counts and other feature table properties
  int32_t polylinesLength = 0;
  std::vector<uint32_t> polylineCounts;
  std::vector<uint16_t> polylineWidths;
  std::array<double, 6> region =
      {0, 0, 0, 0, 0, 0}; // [west, south, east, north, min_height, max_height]

  // extract POLYLINES_LENGTH
  const auto polylinesLengthIt =
      featureTableJson.FindMember("POLYLINES_LENGTH");
  if (polylinesLengthIt != featureTableJson.MemberEnd() &&
      polylinesLengthIt->value.IsInt()) {
    polylinesLength = polylinesLengthIt->value.GetInt();
  }

  // extract REGION
  const auto regionIt = featureTableJson.FindMember("REGION");
  if (regionIt != featureTableJson.MemberEnd() && regionIt->value.IsArray() &&
      regionIt->value.Size() == 6) {
    for (rapidjson::SizeType i = 0; i < 6; i++) {
      region[i] = regionIt->value[i].GetDouble();
    }
  } else {
    result.errors.emplaceWarning("VCTR tile is missing REGION property");
    return;
  }

  // extract POLYLINE_COUNTS and POLYLINE_COUNT
  const auto polylineCountsIt = featureTableJson.FindMember("POLYLINE_COUNTS");
  const auto polylineCountIt = featureTableJson.FindMember("POLYLINE_COUNT");

  if (polylineCountsIt != featureTableJson.MemberEnd() &&
      polylineCountsIt->value.IsArray()) {
    // Use plural attributes
    const auto& countsArray = polylineCountsIt->value;
    polylineCounts.reserve(countsArray.Size());
    for (rapidjson::SizeType i = 0; i < countsArray.Size(); i++) {
      polylineCounts.push_back(countsArray[i].GetUint());
    }
  } else if (
      polylineCountIt != featureTableJson.MemberEnd() &&
      polylineCountIt->value.IsArray()) {
    // Use singular properties (fallback)
    const auto& countsArray = polylineCountIt->value;
    polylineCounts.reserve(countsArray.Size());
    for (rapidjson::SizeType i = 0; i < countsArray.Size(); i++) {
      polylineCounts.push_back(countsArray[i].GetUint());
    }
  }

  // Extract POLYLINE_WIDTHS (optional)
  const auto polylineWidthsIt = featureTableJson.FindMember("POLYLINE_WIDTHS");
  if (polylineWidthsIt != featureTableJson.MemberEnd() &&
      polylineWidthsIt->value.IsArray()) {
    const auto& widthsArray = polylineWidthsIt->value;
    polylineWidths.reserve(widthsArray.Size());
    for (rapidjson::SizeType i = 0; i < widthsArray.Size(); i++) {
      uint32_t width = widthsArray[i].GetUint();
      if (width <= std::numeric_limits<uint16_t>::max()) {
        polylineWidths.push_back(static_cast<uint16_t>(width));
      } else {
        // output a warning message and truncate within the range.
        result.errors.emplaceWarning(fmt::format(
            "Polyline width value {} exceeds uint16_t limit, clamping to "
            "maximum",
            width));
        polylineWidths.push_back(std::numeric_limits<uint16_t>::max());
      }
    }
  } else {
    // If no polyline width is given, default to 2.0 (matching CesiumJs JavaScript implementation).
    polylineWidths.resize(static_cast<size_t>(polylinesLength), 2);
  }

  // extract POLYLINE_BATCH_IDS
  std::vector<uint16_t> polylineBatchIds;
  const auto polylineBatchIdsIt =
      featureTableJson.FindMember("POLYLINE_BATCH_IDS");
  if (polylineBatchIdsIt != featureTableJson.MemberEnd()) {
    if (polylineBatchIdsIt->value.IsObject() &&
        polylineBatchIdsIt->value.HasMember("byteOffset")) {

      uint32_t byteOffset = polylineBatchIdsIt->value["byteOffset"].GetUint();
      const size_t featureTableBinaryOffset =
          headerSize + header.featureTableJsonByteLength;

      // Reading batch IDs from binary data
      size_t dataSize = static_cast<size_t>(polylinesLength) * sizeof(uint16_t);
      if (byteOffset + dataSize <= header.featureTableBinaryByteLength) {
        const uint16_t* batchIdsData = reinterpret_cast<const uint16_t*>(
            vctrBinary.data() + featureTableBinaryOffset + byteOffset);

        polylineBatchIds.resize(static_cast<size_t>(polylinesLength));
        std::memcpy(polylineBatchIds.data(), batchIdsData, dataSize);
      }
    } else if (polylineBatchIdsIt->value.IsArray()) {
      // Reading directly from a JSON array
      const auto& batchIdsArray = polylineBatchIdsIt->value;
      polylineBatchIds.reserve(batchIdsArray.Size());
      for (rapidjson::SizeType i = 0; i < batchIdsArray.Size(); i++) {
        if (batchIdsArray[i].IsUint()) {
          uint32_t batchId = batchIdsArray[i].GetUint();
          if (batchId <= std::numeric_limits<uint16_t>::max()) {
            polylineBatchIds.push_back(static_cast<uint16_t>(batchId));
          }
        }
      }
    }
  }

  // Calculate offsets to binary parts
  const size_t indicesOffset = alignByteOffset(
      headerSize + header.featureTableJsonByteLength +
      header.featureTableBinaryByteLength + header.batchTableJsonByteLength +
      header.batchTableBinaryByteLength);

  const size_t polylinePositionsOffset = indicesOffset +
                                         header.polygonIndicesByteLength +
                                         header.polygonPositionsByteLength;

  // Read and decode positions from binary buffer
  std::vector<glm::vec3> decodedPositions;
  if (header.polylinePositionsByteLength > 0) {
    const uint16_t* rawPositions = reinterpret_cast<const uint16_t*>(
        vctrBinary.data() + polylinePositionsOffset);

    // VCTR format has u, v, height for each position
    size_t vertexCount =
        header.polylinePositionsByteLength / (sizeof(uint16_t) * 3);
    decodedPositions.resize(vertexCount);

    // Track the minimum/maximum values ​​of location data
    glm::dvec3 positionMin;
    glm::dvec3 positionMax;

    int16_t u = 0;
    int16_t v = 0;
    //int16_t h = 0;

    const double west = region[0];
    const double south = region[1];
    const double east = region[2];
    const double north = region[3];
    //const double minimumHeight = region[4];
    //const double maximumHeight = region[5];

    CesiumGeospatial::Ellipsoid ellipsoid = CesiumGeospatial::Ellipsoid::WGS84;

    // Decode ZigZag encodings
    for (size_t i = 0; i < vertexCount; i++) {
      uint16_t uValue = rawPositions[i];
      uint16_t vValue = rawPositions[i + vertexCount];
      //uint16_t hValue = rawPositions[i + 2 * vertexCount];

      u += zigZagDecode(uValue);
      v += zigZagDecode(vValue);
      //h += zigZagDecode(hValue);

      double uRatio = static_cast<double>(u) / maxShort;
      double vRatio = static_cast<double>(v) / maxShort;
      //double hRatio = static_cast<double>(h) / maxShort;
      const double longitude = CesiumUtility::Math::lerp(west, east, uRatio);
      const double latitude = CesiumUtility::Math::lerp(south, north, vRatio);
      const double height = 0.0;
      //const double height = CesiumUtility::Math::lerp(minimumHeight, maximumHeight, hRatio);

      glm::dvec3 position = ellipsoid.cartographicToCartesian(
          CesiumGeospatial::Cartographic(longitude, latitude, height));

      decodedPositions[i] = position;

      // Update min/max values
      positionMin = glm::min(positionMin, position);
      positionMax = glm::max(positionMax, position);
    }

    // Generate line segments based on polyline counts
    std::vector<uint32_t> indices;
    std::vector<glm::vec4> lineColors; // Line color (optional)
    size_t currentPosition = 0;

    for (size_t i = 0; i < polylineCounts.size(); i++) {
      uint32_t count = polylineCounts[i];
      if (count > 0) {
        // For each polyline, create line segments
        for (uint32_t j = 0; j < count - 1; j++) {
          indices.push_back(static_cast<uint32_t>(currentPosition) + j);
          indices.push_back(static_cast<uint32_t>(currentPosition) + j + 1);

          // use same color for all lines
          glm::vec4 color = glm::vec4(
              1.0f,
              1.0f,
              1.0f,
              1.0f); 

          // Apply the same color to each point
          lineColors.push_back(color);
          lineColors.push_back(color);
        }

        currentPosition += count;
      }
    }

    // Create glTF mesh if we have positions
    if (!decodedPositions.empty() && !indices.empty()) {
      model.meshes.emplace_back();
      auto& mesh = model.meshes.back();
      mesh.name = "Polylines";

      // create material
      size_t materialId = model.materials.size();
      CesiumGltf::Material& material = model.materials.emplace_back();
      material.pbrMetallicRoughness =
          std::make_optional<CesiumGltf::MaterialPBRMetallicRoughness>();
      material.pbrMetallicRoughness.value().metallicFactor = 0.0;
      material.pbrMetallicRoughness.value().roughnessFactor = 1.0;

      // create mesh primitive
      mesh.primitives.emplace_back();
      auto& primitive = mesh.primitives.back();
      primitive.mode = CesiumGltf::MeshPrimitive::Mode::LINES;
      primitive.material = static_cast<int32_t>(materialId);

      addVctrPositionsToGltf(
          decodedPositions,
          positionMin,
          positionMax,
          model,
          primitive,
          true);

      // add index data
      addVctrIndicesToGltf(indices, model, primitive);

      // add batch IDs
      addPolylineBatchIds(
          polylineBatchIds,
          polylineCounts,
          decodedPositions.size(),
          model,
          primitive);

      // add colors (optional)
      if (!lineColors.empty()) {
        addVctrColorsToGltf(lineColors, model, primitive);
      }

      // Store line width information (using the extras field)
      // glTF doesn't directly support line width, so store it in extras.
      if (!polylineWidths.empty()) {
        // Calculate average width
        float avgWidth = 0.0f;
        for (const auto& width : polylineWidths) {
          avgWidth += static_cast<float>(width);
        }
        avgWidth /= static_cast<float>(polylineWidths.size());

        // Add width information to extras
        primitive.extras.emplace("lineWidth", avgWidth);
      } else {
        // Set default width
        primitive.extras.emplace("lineWidth", 2.0f);
      }
    }
  }
}

void processPoints(
    const std::span<const std::byte>& vctrBinary,
    const VctrHeader& header,
    uint32_t headerSize,
    const rapidjson::Document& featureTableJson,
    GltfConverterResult& result) {

  if (!result.model) {
    result.model.emplace();
  }

  if (header.pointPositionsByteLength == 0) {
    return;
  }

  CesiumGltf::Model& model = result.model.value();

  model.extras["vctr_point"] = static_cast<int>(1);

  // Get point-related properties
  //int32_t pointsLength = 0;
  std::array<double, 6> region =
      {0, 0, 0, 0, 0, 0}; // [west, south, east, north, min_height, max_height]

  // extract REGION
  const auto regionIt = featureTableJson.FindMember("REGION");
  if (regionIt != featureTableJson.MemberEnd() && regionIt->value.IsArray() &&
      regionIt->value.Size() == 6) {
    for (rapidjson::SizeType i = 0; i < 6; i++) {
      region[i] = regionIt->value[i].GetDouble();
    }
  } else {
    result.errors.emplaceWarning("VCTR tile is missing REGION property");
    return;
  }

  // Calculating offsets for point location data
  const size_t indicesOffset = alignByteOffset(
      headerSize + header.featureTableJsonByteLength +
      header.featureTableBinaryByteLength + header.batchTableJsonByteLength +
      header.batchTableBinaryByteLength);

  const size_t pointPositionsOffset =
      indicesOffset + header.polygonIndicesByteLength +
      header.polygonPositionsByteLength + header.polylinePositionsByteLength;

  // Reading and decoding position data from a binary buffer
  std::vector<glm::dvec3> decodedPositions;
  glm::dvec3 positionMin(std::numeric_limits<double>::max());
  glm::dvec3 positionMax(std::numeric_limits<double>::lowest());

  // Optional: A vector to store the point colors
  std::vector<glm::vec4> pointColors;

  if (header.pointPositionsByteLength > 0) {
    const uint16_t* rawPositions = reinterpret_cast<const uint16_t*>(
        vctrBinary.data() + pointPositionsOffset);

    // VCTR format has u, v, height values ​​for each position
    size_t vertexCount =
        header.pointPositionsByteLength / (sizeof(uint16_t) * 3);
    decodedPositions.resize(vertexCount);

    int16_t u = 0;
    int16_t v = 0;
    int16_t h = 0;

    const double west = region[0];
    const double south = region[1];
    const double east = region[2];
    const double north = region[3];
    const double minimumHeight = region[4];
    const double maximumHeight = region[5];

    CesiumGeospatial::Ellipsoid ellipsoid = CesiumGeospatial::Ellipsoid::WGS84;

    // ZigZag encoding and decoding - processing all points
    for (size_t i = 0; i < vertexCount; i++) {
      uint16_t uValue = rawPositions[i];
      uint16_t vValue = rawPositions[i + vertexCount];
      uint16_t hValue = rawPositions[i + 2 * vertexCount];

      u += zigZagDecode(uValue);
      v += zigZagDecode(vValue);
      h += zigZagDecode(hValue);

      double uRatio = static_cast<double>(u) / maxShort;
      double vRatio = static_cast<double>(v) / maxShort;
      double hRatio = static_cast<double>(h) / maxShort;
      const double longitude = CesiumUtility::Math::lerp(west, east, uRatio);
      const double latitude = CesiumUtility::Math::lerp(south, north, vRatio);
      const double height = CesiumUtility::Math::lerp(minimumHeight, maximumHeight, hRatio);
      //const double height = 0.0;

      glm::dvec3 position = ellipsoid.cartographicToCartesian(
          CesiumGeospatial::Cartographic(longitude, latitude, height));

      decodedPositions[i] = position;

      // Update min/max values
      positionMin = glm::min(positionMin, position);
      positionMax = glm::max(positionMax, position);

      // Optional: Calculate color based on height
      float heightNormalized = static_cast<float>(hRatio);
      glm::vec4 color;
      if (heightNormalized < 0.5f) {
        float t = heightNormalized * 2.0f;
        color = glm::vec4(0.0f, t, 1.0f - t, 1.0f);
      } else {
        float t = (heightNormalized - 0.5f) * 2.0f;
        color = glm::vec4(t, 1.0f - t, 0.0f, 1.0f);
      }
      pointColors.push_back(color);
    }
  }

  // If there are no points, processing ends
  if (decodedPositions.empty()) {
    return;
  }

  // for RTC
  auto& cesiumRTC =
      result.model->addExtension<CesiumGltf::ExtensionCesiumRTC>();
  result.model->addExtensionRequired(
      CesiumGltf::ExtensionCesiumRTC::ExtensionName);

  // Use the first point as the center of the RTC
  glm::dvec3 rtcPos = computeRegionCenter(region);

  CesiumGeospatial::Ellipsoid ellipsoid = CesiumGeospatial::Ellipsoid::WGS84;

  rtcPos = ellipsoid.cartographicToCartesian(
      CesiumGeospatial::Cartographic(rtcPos.x, rtcPos.y, rtcPos.z));

  cesiumRTC.center = {rtcPos.x, rtcPos.y, rtcPos.z};

  std::vector<glm::vec3> pointVertex = {glm::vec3(0.0f, 0.0f, 0.0f)};
  std::vector<uint32_t> vertexIndex = {0,0,0};
  // cube data for debugging (visualization)
  /*
  float cubeSize = 100.0f; // You can adjust the cube size
  std::vector<glm::vec3> pointVertex = {
      glm::vec3(-0.5f, -0.5f, -0.5f) * cubeSize,
      glm::vec3(0.5f, -0.5f, -0.5f) * cubeSize,
      glm::vec3(0.5f, -0.5f, 0.5f) * cubeSize,
      glm::vec3(-0.5f, -0.5f, 0.5f) * cubeSize,
      glm::vec3(-0.5f, 0.5f, -0.5f) * cubeSize,
      glm::vec3(0.5f, 0.5f, -0.5f) * cubeSize,
      glm::vec3(0.5f, 0.5f, 0.5f) * cubeSize,
      glm::vec3(-0.5f, 0.5f, 0.5f) * cubeSize};

  // Define cube indices (12 triangles, 36 indices)
  std::vector<uint32_t> vertexIndex = {
      0, 1, 2, 0, 2, 3, // bottom surface
      4, 6, 5, 4, 7, 6, // top surface
      0, 4, 1, 1, 4, 5, // front side
      1, 5, 2, 2, 5, 6, // right side
      2, 6, 3, 3, 6, 7, // back side
      3, 7, 0, 0, 7, 4  // left side
  };
  */

  // Processing point batch ID
  std::vector<uint16_t> pointBatchIds;
  const auto pointBatchIdsIt = featureTableJson.FindMember("POINT_BATCH_IDS");
  if (pointBatchIdsIt != featureTableJson.MemberEnd()) {
    // If the batch ID is in the binary section
    if (pointBatchIdsIt->value.IsObject() &&
        pointBatchIdsIt->value.HasMember("byteOffset")) {

      uint32_t byteOffset = pointBatchIdsIt->value["byteOffset"].GetUint();
      const size_t featureTableBinaryOffset =
          headerSize + header.featureTableJsonByteLength;

      // Read batch ID from binary data
      if (byteOffset + decodedPositions.size() * sizeof(uint16_t) <=
          header.featureTableBinaryByteLength) {
        const uint16_t* batchIdsData = reinterpret_cast<const uint16_t*>(
            vctrBinary.data() + featureTableBinaryOffset + byteOffset);

        pointBatchIds.resize(decodedPositions.size());
        std::memcpy(
            pointBatchIds.data(),
            batchIdsData,
            decodedPositions.size() * sizeof(uint16_t));
      } else {
        result.errors.emplaceWarning("POINT_BATCH_IDS byteOffset is invalid or "
                                     "data extends beyond binary section");
      }
    }
    // If the batch ID is directly in the JSON as an array
    else if (pointBatchIdsIt->value.IsArray()) {
      const auto& batchIdsArray = pointBatchIdsIt->value;
      pointBatchIds.reserve(batchIdsArray.Size());
      for (rapidjson::SizeType i = 0; i < batchIdsArray.Size(); i++) {
        if (batchIdsArray[i].IsUint()) {
          uint32_t batchId = batchIdsArray[i].GetUint();
          if (batchId <= std::numeric_limits<uint16_t>::max()) {
            pointBatchIds.push_back(static_cast<uint16_t>(batchId));
          } else {
            result.errors.emplaceWarning(fmt::format(
                "Point batch ID {} exceeds uint16_t limit, clamping to maximum",
                batchId));
            pointBatchIds.push_back(std::numeric_limits<uint16_t>::max());
          }
        }
      }
    }
  }

  for (size_t i = 0; i < decodedPositions.size(); i++) {
    // RTC relative position calculation
    glm::dvec3 relPos = glm::dvec3(
        decodedPositions[i].x - cesiumRTC.center[0],
        decodedPositions[i].y - cesiumRTC.center[1],
        decodedPositions[i].z - cesiumRTC.center[2]);

    // Create a cube mesh
    model.meshes.emplace_back();
    auto& mesh = model.meshes.back();
    mesh.name = "VCTR_Point_" + std::to_string(i);

    // Create or reuse a material
    size_t materialId;
    if (i < pointColors.size()) {
      // Create a material with a unique color for each point
      materialId = model.materials.size();
      CesiumGltf::Material& material = model.materials.emplace_back();
      material.pbrMetallicRoughness =
          std::make_optional<CesiumGltf::MaterialPBRMetallicRoughness>();
      material.pbrMetallicRoughness.value().metallicFactor = 0.0;
      material.pbrMetallicRoughness.value().roughnessFactor = 1.0;
      material.pbrMetallicRoughness.value().baseColorFactor = {
          pointColors[i].x,
          pointColors[i].y,
          pointColors[i].z,
          pointColors[i].w};
    } else {
      // or create a basic material
      materialId = model.materials.size();
      CesiumGltf::Material& material = model.materials.emplace_back();
      material.pbrMetallicRoughness =
          std::make_optional<CesiumGltf::MaterialPBRMetallicRoughness>();
      material.pbrMetallicRoughness.value().metallicFactor = 0.0;
      material.pbrMetallicRoughness.value().roughnessFactor = 1.0;
      material.pbrMetallicRoughness.value()
          .baseColorFactor = {1.0, 0.5, 0.0, 1.0}; // Orange
    }

    // Create a primitive
    mesh.primitives.emplace_back();
    auto& primitive = mesh.primitives.back();
    primitive.mode = CesiumGltf::MeshPrimitive::Mode::TRIANGLES;
    primitive.material = static_cast<int32_t>(materialId);

    // Create vertex buffer
    const int64_t vertexByteStride = static_cast<int64_t>(sizeof(glm::vec3));
    const int64_t vertexByteLength =
        static_cast<int64_t>(pointVertex.size() * sizeof(glm::vec3));
    std::vector<std::byte> vertexData(static_cast<size_t>(vertexByteLength));
    std::memcpy(
        vertexData.data(),
        pointVertex.data(),
        static_cast<size_t>(vertexByteLength));
    int32_t vertexBufferId = createBufferInGltf(model, std::move(vertexData));

    // Create a buffer view
    int32_t vertexBufferViewId = createBufferViewInGltf(
        model,
        vertexBufferId,
        vertexByteLength,
        vertexByteStride,
        static_cast<int32_t>(CesiumGltf::BufferView::Target::ARRAY_BUFFER));

    // Create an accessor
    int32_t vertexAccessorId = createAccessorInGltf(
        model,
        vertexBufferViewId,
        CesiumGltf::Accessor::ComponentType::FLOAT,
        static_cast<int64_t>(pointVertex.size()),
        CesiumGltf::Accessor::Type::VEC3);

    // Set min/max values ​​for accessors
    CesiumGltf::Accessor& vertexAccessor =
        model.accessors[static_cast<uint32_t>(vertexAccessorId)];

    //float halfSize = cubeSize / 2.0f;
    vertexAccessor.min = {
        //relPos.x - halfSize,
        //relPos.y - halfSize,
        //relPos.z - halfSize
        relPos.x,
        relPos.y,
        relPos.z
    };
    vertexAccessor.max = {
        //relPos.x + halfSize,
        //relPos.y + halfSize,
        //relPos.z + halfSize
        relPos.x,
        relPos.y,
        relPos.z
    };

    // Create an index buffer
    const int64_t indexByteLength =
        static_cast<int64_t>(pointVertex.size() * sizeof(uint32_t));
    std::vector<std::byte> indexData(static_cast<size_t>(indexByteLength));
    std::memcpy(
        indexData.data(),
        vertexIndex.data(),
        static_cast<size_t>(indexByteLength));
    int32_t indexBufferId = createBufferInGltf(model, std::move(indexData));

    // Index buffer view
    int32_t indexBufferViewId = createBufferViewInGltf(
        model,
        indexBufferId,
        indexByteLength,
        0,
        static_cast<int32_t>(
            CesiumGltf::BufferView::Target::ELEMENT_ARRAY_BUFFER));

    // Index accessor
    int32_t indexAccessorId = createAccessorInGltf(
        model,
        indexBufferViewId,
        CesiumGltf::Accessor::ComponentType::UNSIGNED_INT,
        static_cast<int64_t>(vertexIndex.size()),
        CesiumGltf::Accessor::Type::SCALAR);

     // Add batch ID for this point
    if (i < pointBatchIds.size()) {
      const uint16_t batchId = pointBatchIds[i];

      // Create batch ID buffer
      std::vector<std::byte> batchIdData(sizeof(uint16_t));
      std::memcpy(batchIdData.data(), &batchId, sizeof(uint16_t));
      int32_t batchIdBufferId =
          createBufferInGltf(model, std::move(batchIdData));

      // Create a batch ID buffer view
      int32_t batchIdBufferViewId = createBufferViewInGltf(
          model,
          batchIdBufferId,
          sizeof(uint16_t),
          sizeof(uint16_t));

      // Create a batch ID accessor
      int32_t batchIdAccessorId = createAccessorInGltf(
          model,
          batchIdBufferViewId,
          CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT,
          1,
          CesiumGltf::Accessor::Type::SCALAR);

      // Add _BATCHID property to primitive
      primitive.attributes.emplace("_BATCHID", batchIdAccessorId);
    }
    // Assign properties to primitives
    primitive.attributes.emplace("POSITION", vertexAccessorId);
    primitive.indices = indexAccessorId;
  }

  // Scene and node settings
  if (model.scenes.empty()) {
    // If there is no scene, create a new scene
    model.scenes.emplace_back();
  }

  CesiumGltf::Scene& scene = model.scenes[0];

  // Create a node for each mesh
  for (size_t i = 0; i < decodedPositions.size(); i++) {
    model.nodes.emplace_back();
    CesiumGltf::Node& node = model.nodes.back();

    // Set node name
    node.name = "Point_" + std::to_string(i);

    // Set mesh reference
    node.mesh = static_cast<int32_t>(i);

    // Add node reference to scene
    scene.nodes.push_back(static_cast<int32_t>(model.nodes.size() - 1));
  }

  // Set as default scene
  model.scene = 0;
}

void convertVctrMetadataToGltfStructuralMetadata(
    const std::span<const std::byte>& vctrBinary,
    const VctrHeader& header,
    uint32_t headerSize,
    GltfConverterResult& result) {
  if (result.model && header.featureTableJsonByteLength > 0) {
    CesiumGltf::Model& gltf = result.model.value();

const std::span<const std::byte> featureTableJsonData =
        vctrBinary.subspan(headerSize, header.featureTableJsonByteLength);
    rapidjson::Document featureTableJson =
        parseFeatureTableJsonData(featureTableJsonData, result);

    const int64_t batchTableStart = headerSize +
                                    header.featureTableJsonByteLength +
                                    header.featureTableBinaryByteLength;

    const int64_t batchTableLength = header.batchTableBinaryByteLength + header.batchTableJsonByteLength;

    if (batchTableLength > 0) {
      const std::span<const std::byte> batchTableJsonData = vctrBinary.subspan(
          static_cast<size_t>(batchTableStart),
          header.batchTableJsonByteLength);
      const std::span<const std::byte> batchTableBinaryData =
          vctrBinary.subspan(
              static_cast<size_t>(
                  batchTableStart + header.batchTableJsonByteLength),
              header.batchTableBinaryByteLength);

      rapidjson::Document batchTableJson;
      batchTableJson.Parse(
          reinterpret_cast<const char*>(batchTableJsonData.data()),
          batchTableJsonData.size());
      
      if (batchTableJson.HasParseError()) {
        result.errors.emplaceWarning(fmt::format(
            "Error when parsing batch table JSON, error code {} at byte "
            "offset "
            "{}. Skip parsing metadata",
            batchTableJson.GetParseError(),
            batchTableJson.GetErrorOffset()));
        return;
      }
      
      // Use the VCTR-specific batch table converter
      result.errors.merge(BatchTableToGltfStructuralMetadata::convertFromVctr(
          featureTableJson,
          batchTableJson,
          batchTableBinaryData,
          gltf));
    }
  }
}

void convertVctrToGltf(
    const std::span<const std::byte>& vctrBinary,
    const VctrHeader& header,
    GltfConverterResult& result,
    const AssetFetcher& /*assetFetcher*/) { // Mark unused parameter

  const uint32_t headerSize = sizeof(VctrHeader);

  // Parse feature table
  const std::span<const std::byte> featureTableJsonData =
      vctrBinary.subspan(headerSize, header.featureTableJsonByteLength);

  rapidjson::Document featureTableJson;
  if (header.featureTableJsonByteLength > 0) {
    featureTableJson = parseFeatureTableJsonData(featureTableJsonData, result);
    if (result.errors) {
      return;
    }
  } else {
    // If feature table is empty, there's nothing to render
    result.errors.emplaceWarning(
        "Vector tile has empty feature table, no geometry to render.");
    return;
  }

  // Initialize model
  if (!result.model.has_value())
    result.model.emplace();

  // Determine if the tile contains polygons, polylines, or points
  bool hasPolygons = false;
  bool hasPolylines = false;
  bool hasPoints = false;

  if (featureTableJson.HasMember("POLYGONS_LENGTH") &&
      featureTableJson["POLYGONS_LENGTH"].IsInt() &&
      featureTableJson["POLYGONS_LENGTH"].GetInt() > 0) {
    hasPolygons = true;
  }

  if (featureTableJson.HasMember("POLYLINES_LENGTH") &&
      featureTableJson["POLYLINES_LENGTH"].IsInt() &&
      featureTableJson["POLYLINES_LENGTH"].GetInt() > 0) {
    hasPolylines = true;
  }

  if (featureTableJson.HasMember("POINTS_LENGTH") &&
      featureTableJson["POINTS_LENGTH"].IsInt() &&
      featureTableJson["POINTS_LENGTH"].GetInt() > 0) {
    hasPoints = true;
  }

  if (hasPolygons) {
    processPolygons(vctrBinary, header, headerSize, featureTableJson, result);
  }

  if (hasPolylines) {
    processPolylines(
        vctrBinary,
        header,
        headerSize,
        featureTableJson,
        result);
  }

  if (hasPoints) {
    processPoints(vctrBinary, header, headerSize, featureTableJson, result);
  }

  // Convert metadata
  convertVctrMetadataToGltfStructuralMetadata(
      vctrBinary,
      header,
      headerSize,
      result);

  // Setup scene graph if any meshes were created
  if (result.model && !result.model->meshes.empty()) {
    CesiumGltf::Model& model = result.model.value();

    // Add a simple scene and node structure
    model.scenes.emplace_back();
    model.scenes[0].nodes.push_back(0);

    model.nodes.emplace_back();
    auto& node = model.nodes[0];

    // Set coordinate system information
    model.extras["gltfUpAxis"] = static_cast<int>(CesiumGeometry::Axis::Y);

    model.extras["vctr"] = static_cast<int>(1);

    // Set to TRS mode
    node.translation = {0, 0, 0};
    node.rotation = {0, 0, 0, 1};
    node.scale = {1, 1, 1};

    // Or set it in matrix format
    // If it's already in the Z-up coordinate system, set it as is without any
    // additional transformations.
    node.matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // Set node's mesh to the first mesh
    if (!model.meshes.empty()) {
      node.mesh = 0;
    }

    // Set default scene
    model.scene = 0;
  }
}
} // namespace

CesiumAsync::Future<GltfConverterResult> VctrToGltfConverter::convert(
    const std::span<const std::byte>& vctrBinary,
    const CesiumGltfReader::GltfReaderOptions& options,
    const AssetFetcher& assetFetcher) {
  // Unused parameter 'options' - keep it for API compatibility
  (void)options;

  GltfConverterResult result;
  VctrHeader header;
  parseVctrHeader(vctrBinary, header, result);
  if (result.errors) {
    return assetFetcher.asyncSystem.createResolvedFuture(std::move(result));
  }

  convertVctrToGltf(vctrBinary, header, result, assetFetcher);
  return assetFetcher.asyncSystem.createResolvedFuture(std::move(result));
}
} // namespace Cesium3DTilesContent
