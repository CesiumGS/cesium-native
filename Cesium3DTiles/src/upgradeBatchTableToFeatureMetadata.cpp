#include "upgradeBatchTableToFeatureMetadata.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "CesiumGltf/MeshPrimitiveEXT_feature_metadata.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/ModelEXT_feature_metadata.h"
#include "rapidjson/writer.h"
#include <map>
#include <rapidjson/document.h>

using namespace CesiumGltf;

namespace {
struct CompatibleTypes {
  bool isInt8 = true;
  bool isUint8 = true;
  bool isInt16 = true;
  bool isUint16 = true;
  bool isInt32 = true;
  bool isUint32 = true;
  bool isInt64 = true;
  bool isUint64 = true;
  bool isFloat32 = true;
  bool isFloat64 = true;
  bool isBool = true;
};

struct BinaryProperty {
  int64_t b3dmByteOffset;
  int64_t gltfByteOffset;
  int64_t byteLength;
};

struct GltfFeatureTableType {
  std::string typeName;
  size_t typeSize;
};

const std::map<std::string, GltfFeatureTableType> b3dmComponentTypeToGltfType =
    {
        {"BYTE", GltfFeatureTableType{"INT8", sizeof(int8_t)}},
        {"UNSIGNED_BYTE", GltfFeatureTableType{"UINT8", sizeof(uint8_t)}},
        {"SHORT", GltfFeatureTableType{"INT16", sizeof(int16_t)}},
        {"UNSIGNED_SHORT", GltfFeatureTableType{"UINT16", sizeof(uint16_t)}},
        {"INT", GltfFeatureTableType{"INT32", sizeof(int32_t)}},
        {"UNSIGNED_INT", GltfFeatureTableType{"UINT32", sizeof(uint32_t)}},
        {"FLOAT", GltfFeatureTableType{"FLOAT32", sizeof(float)}},
        {"DOUBLE", GltfFeatureTableType{"FLOAT64", sizeof(double)}},
};

int64_t roundUp(int64_t num, int64_t multiple) {
  return ((num + multiple - 1) / multiple) * multiple;
}

void updateExtensionWithJsonStringProperty(
    Model& gltf,
    ClassProperty& /* classProperty */,
    FeatureTable& /* featureTable */,
    FeatureTableProperty& /*featureTableProperty*/,
    const rapidjson::Value& propertyValue) {
  size_t totalSize = 0;
  size_t maxOffset = 0;
  std::vector<rapidjson::StringBuffer> rapidjsonStrBuffers;
  rapidjsonStrBuffers.reserve(propertyValue.Size());
  for (const auto& v : propertyValue.GetArray()) {
    rapidjson::StringBuffer& buffer = rapidjsonStrBuffers.emplace_back();
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    v.Accept(writer);
    maxOffset = totalSize;
    totalSize += buffer.GetSize();
  }

  size_t offset = 0;
  size_t stringOffset = 0;
  std::vector<std::byte> buffer(
      totalSize + sizeof(size_t) * rapidjsonStrBuffers.size());
  for (const rapidjson::StringBuffer& rapidjsonBuffer : rapidjsonStrBuffers) {
    size_t bufferLength = rapidjsonBuffer.GetSize();
    if (bufferLength != 0) {
      std::memcpy(
          buffer.data() + stringOffset,
          rapidjsonBuffer.GetString(),
          bufferLength);
      std::memcpy(buffer.data() + totalSize + offset, &offset, sizeof(size_t));
      stringOffset += bufferLength;
      offset += sizeof(size_t);
    }
  }

  Buffer& gltfBuffer = gltf.buffers.emplace_back();
  gltfBuffer.cesium.data = std::move(buffer);
}

template <typename T> bool isInRange(int64_t value) {
  return value >= std::numeric_limits<int8_t>::lowest() &&
         value <= std::numeric_limits<int8_t>::max();
}

CompatibleTypes findCompatibleTypes(const rapidjson::Value& propertyValue) {
  CompatibleTypes result{};

  for (auto it = propertyValue.Begin(); it != propertyValue.End(); ++it) {
    if (it->IsBool()) {
      // Should we allow conversion of bools to numeric 0 or 1? Nah.
      result.isInt8 = result.isUint8 = false;
      result.isInt16 = result.isUint16 = false;
      result.isInt32 = result.isUint32 = false;
      result.isInt64 = result.isUint64 = false;
      result.isFloat32 = false;
      result.isFloat64 = false;
    } else if (it->IsInt64()) {
      int64_t value = it->GetInt64();
      result.isInt8 &= isInRange<int8_t>(value);
      result.isUint8 &= isInRange<uint8_t>(value);
      result.isInt16 &= isInRange<int16_t>(value);
      result.isUint16 &= isInRange<uint16_t>(value);
      result.isInt32 &= isInRange<int32_t>(value);
      result.isUint32 &= isInRange<uint32_t>(value);
      result.isInt64 &= isInRange<int64_t>(value);
      result.isUint64 &= isInRange<uint64_t>(value);
      result.isFloat32 &= value >= -2e24 && value <= 2e24;
      result.isFloat64 &= value >= -2e53 && value <= 2e53;
      result.isBool = false;
    } else if (it->IsUint64()) {
      // Only uint64_t can represent a value that fits in a uint64_t but not in
      // an int64_t.
      result.isInt8 = result.isUint8 = false;
      result.isInt16 = result.isUint16 = false;
      result.isInt32 = result.isUint32 = false;
      result.isInt64 = false;
      result.isFloat32 = false;
      result.isFloat64 = false;
      result.isBool = false;
    } else if (it->IsLosslessFloat()) {
      result.isInt8 = result.isUint8 = false;
      result.isInt16 = result.isUint16 = false;
      result.isInt32 = result.isUint32 = false;
      result.isInt64 = result.isUint64 = false;
      result.isFloat32 = true;
      result.isFloat64 = true;
      result.isBool = false;
    } else if (it->IsDouble()) {
      result.isInt8 = result.isUint8 = false;
      result.isInt16 = result.isUint16 = false;
      result.isInt32 = result.isUint32 = false;
      result.isInt64 = result.isUint64 = false;
      result.isFloat32 = false;
      result.isFloat64 = true;
      result.isBool = false;
    } else {
      // A string, null, or something else.
      result.isInt8 = result.isUint8 = false;
      result.isInt16 = result.isUint16 = false;
      result.isInt32 = result.isUint32 = false;
      result.isInt64 = result.isUint64 = false;
      result.isFloat32 = false;
      result.isFloat64 = false;
      result.isBool = false;
    }
  }

  return result;
}

template <typename T, typename TRapidJson = T>
void updateExtensionWithJsonNumericProperty(
    Model& gltf,
    ClassProperty& classProperty,
    FeatureTable& featureTable,
    FeatureTableProperty& featureTableProperty,
    const rapidjson::Value& propertyValue,
    const std::string& typeName) {

  classProperty.type = typeName;

  // Create a new buffer for this property.
  size_t bufferIndex = gltf.buffers.size();
  Buffer& buffer = gltf.buffers.emplace_back();
  buffer.byteLength = sizeof(T) * featureTable.count;
  buffer.cesium.data.resize(buffer.byteLength);

  size_t bufferViewIndex = gltf.bufferViews.size();
  BufferView& bufferView = gltf.bufferViews.emplace_back();
  bufferView.buffer = int32_t(bufferIndex);
  bufferView.byteOffset = 0;
  bufferView.byteStride = sizeof(T);
  bufferView.byteLength = buffer.byteLength;

  featureTableProperty.bufferView = int32_t(bufferViewIndex);

  assert(propertyValue.Size() == featureTable.count);
  T* p = reinterpret_cast<T*>(buffer.cesium.data.data());

  for (auto it = propertyValue.Begin(); it != propertyValue.End(); ++it) {
    *p = static_cast<T>(it->Get<TRapidJson>());
    ++p;
  }
}

void updateExtensionWithJsonBoolProperty(
    Model& /* gltf */,
    ClassProperty& /* classProperty */,
    FeatureTable& /* featureTable */,
    FeatureTableProperty& /* featureTableProperty */,
    const rapidjson::Value& /* propertyValue */) {
  // TODO
}

void updateExtensionWithJsonProperty(
    Model& gltf,
    ClassProperty& classProperty,
    FeatureTable& featureTable,
    FeatureTableProperty& featureTableProperty,
    const rapidjson::Value& propertyValue) {

  // Guess at the type of the property from the type of the first JSON element.
  // If we later find nulls or a different type, we'll start over and convert
  // everything to strings.
  if (propertyValue.Empty() || propertyValue.Size() < featureTable.count) {
    // No property to infer the type from, so assume string.
    updateExtensionWithJsonStringProperty(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue);
    return;
  }

  const rapidjson::Value& firstElement = propertyValue[0];
  if (firstElement.IsArray() || firstElement.IsNull() ||
      firstElement.IsObject() || firstElement.IsString()) {
    // Strings, nulls, objects, and arrays are all represented as strings in
    // EXT_feature_metadata.
    // TODO: arrays of consistent type don't need to be represented as strings!
    updateExtensionWithJsonStringProperty(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue);
    return;
  }

  // Figure out which types we can use for this data.
  CompatibleTypes compatibleTypes = findCompatibleTypes(propertyValue);

  // Use the smallest type we can, and prefer signed to unsigned.
  std::string type = "STRING";
  if (compatibleTypes.isBool) {
    updateExtensionWithJsonBoolProperty(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue);
  } else if (compatibleTypes.isInt8) {
    updateExtensionWithJsonNumericProperty<int8_t, int32_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "INT8");
  } else if (compatibleTypes.isUint8) {
    updateExtensionWithJsonNumericProperty<uint8_t, uint32_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "UINT8");
  } else if (compatibleTypes.isInt16) {
    updateExtensionWithJsonNumericProperty<int16_t, int32_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "INT16");
  } else if (compatibleTypes.isUint16) {
    updateExtensionWithJsonNumericProperty<uint16_t, uint32_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "UINT16");
  } else if (compatibleTypes.isInt32) {
    updateExtensionWithJsonNumericProperty<int32_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "INT32");
  } else if (compatibleTypes.isUint32) {
    updateExtensionWithJsonNumericProperty<uint32_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "UINT32");
  } else if (compatibleTypes.isInt64) {
    updateExtensionWithJsonNumericProperty<int64_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "INT64");
  } else if (compatibleTypes.isUint64) {
    updateExtensionWithJsonNumericProperty<uint64_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "UINT64");
  } else if (compatibleTypes.isFloat32) {
    updateExtensionWithJsonNumericProperty<float>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "FLOAT32");
  } else if (compatibleTypes.isFloat64) {
    updateExtensionWithJsonNumericProperty<double>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "FLOAT64");
  }
}

void updateExtensionWithBinaryProperty(
    Model& gltf,
    int32_t gltfBufferIndex,
    int64_t gltfBufferOffset,
    BinaryProperty& binaryProperty,
    ClassProperty& classProperty,
    FeatureTable& featureTable,
    FeatureTableProperty& featureTableProperty,
    const rapidjson::Value& propertyValue) {
  if (gltfBufferIndex < 0) {
    return;
  }

  const auto& byteOffsetIt = propertyValue.FindMember("byteOffset");
  if (byteOffsetIt == propertyValue.MemberEnd() ||
      !byteOffsetIt->value.IsInt64()) {
    return;
  }

  const auto& componentTypeIt = propertyValue.FindMember("componentType");
  if (componentTypeIt == propertyValue.MemberEnd() ||
      !componentTypeIt->value.IsString()) {
    return;
  }

  const auto& typeIt = propertyValue.FindMember("type");
  if (typeIt == propertyValue.MemberEnd() || !typeIt->value.IsString()) {
    return;
  }

  // convert class property
  int64_t byteOffset = byteOffsetIt->value.GetInt64();
  const std::string& componentType = componentTypeIt->value.GetString();
  const std::string& type = typeIt->value.GetString();

  auto convertedTypeIt = b3dmComponentTypeToGltfType.find(componentType);
  if (convertedTypeIt == b3dmComponentTypeToGltfType.end()) {
    return;
  }
  const GltfFeatureTableType& gltfType = convertedTypeIt->second;

  if (type == "SCALAR") {
    classProperty.type = gltfType.typeName;
  } else if (type == "VEC2") {
    classProperty.type = "ARRAY";
    classProperty.componentCount = 2;
    classProperty.componentType = gltfType.typeName;
  } else if (type == "VEC3") {
    classProperty.type = "ARRAY";
    classProperty.componentCount = 3;
    classProperty.componentType = gltfType.typeName;
  } else if (type == "VEC4") {
    classProperty.type = "ARRAY";
    classProperty.componentCount = 4;
    classProperty.componentType = gltfType.typeName;
  } else {
    return;
  }

  // convert feature table
  size_t typeSize = gltfType.typeSize;
  auto& bufferView = gltf.bufferViews.emplace_back();
  bufferView.buffer = gltfBufferIndex;
  bufferView.byteOffset = gltfBufferOffset;
  bufferView.byteStride = typeSize;
  bufferView.byteLength = typeSize * featureTable.count;

  featureTableProperty.bufferView =
      static_cast<int32_t>(gltf.bufferViews.size() - 1);

  binaryProperty.b3dmByteOffset = byteOffset;
  binaryProperty.gltfByteOffset = gltfBufferOffset;
  binaryProperty.byteLength = static_cast<int64_t>(bufferView.byteLength);
}
} // namespace

namespace Cesium3DTiles {

void upgradeBatchTableToFeatureMetadata(
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumGltf::Model& gltf,
    const rapidjson::Document& featureTableJson,
    const gsl::span<const std::byte>& batchTableJsonData,
    const gsl::span<const std::byte>& batchTableBinaryData) {

  // Parse the b3dm batch table and convert it to the EXT_feature_metadata
  // extension.

  // If the feature table is missing the BATCH_LENGTH semantic, ignore the batch
  // table completely.
  auto batchLengthIt = featureTableJson.FindMember("BATCH_LENGTH");
  if (batchLengthIt == featureTableJson.MemberEnd() ||
      !batchLengthIt->value.IsInt64()) {
    SPDLOG_LOGGER_WARN(
        pLogger,
        "The B3DM has a batch table, but it is being ignored because there is "
        "no BATCH_LENGTH semantic in the feature table or it is not an "
        "integer.");
    return;
  }

  int64_t batchLength = batchLengthIt->value.GetInt64();

  rapidjson::Document document;
  document.Parse(
      reinterpret_cast<const char*>(batchTableJsonData.data()),
      batchTableJsonData.size());
  if (document.HasParseError()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Error when parsing batch table JSON, error code {} at byte offset "
        "{}",
        document.GetParseError(),
        document.GetErrorOffset());
    return;
  }

  // Add the binary part of the batch table - if any - to the glTF as a buffer.
  // We will reallign this buffer later on
  int32_t gltfBufferIndex = -1;
  int64_t gltfBufferOffset = -1;
  std::vector<BinaryProperty> binaryProperties;
  if (!batchTableBinaryData.empty()) {
    gltfBufferIndex = static_cast<int32_t>(gltf.buffers.size());
    gltfBufferOffset = 0;
    gltf.buffers.emplace_back();
  }

  ModelEXT_feature_metadata& modelExtension =
      gltf.addExtension<ModelEXT_feature_metadata>();
  Schema& schema = modelExtension.schema.emplace();
  Class& classDefinition =
      schema.classes.emplace("default", Class()).first->second;

  FeatureTable& featureTable =
      modelExtension.featureTables.emplace("default", FeatureTable())
          .first->second;
  featureTable.count = batchLength;
  featureTable.classProperty = "default";

  // Convert each regular property in the batch table
  for (auto propertyIt = document.MemberBegin();
       propertyIt != document.MemberEnd();
       ++propertyIt) {
    std::string name = propertyIt->name.GetString();
    ClassProperty& classProperty =
        classDefinition.properties.emplace(name, ClassProperty()).first->second;
    classProperty.name = name;

    FeatureTableProperty& featureTableProperty =
        featureTable.properties.emplace(name, FeatureTableProperty())
            .first->second;
    const rapidjson::Value& propertyValue = propertyIt->value;
    if (propertyValue.IsArray()) {
      updateExtensionWithJsonProperty(
          gltf,
          classProperty,
          featureTable,
          featureTableProperty,
          propertyValue);
    } else {
      BinaryProperty& binaryProperty = binaryProperties.emplace_back();
      updateExtensionWithBinaryProperty(
          gltf,
          gltfBufferIndex,
          gltfBufferOffset,
          binaryProperty,
          classProperty,
          featureTable,
          featureTableProperty,
          propertyValue);
      gltfBufferOffset += roundUp(binaryProperty.byteLength, 8);
    }
  }

  // re-arrange binary property buffer
  if (!batchTableBinaryData.empty()) {
    Buffer& buffer = gltf.buffers[gltfBufferIndex];
    buffer.byteLength = gltfBufferOffset;
    buffer.cesium.data.resize(gltfBufferOffset);
    for (const BinaryProperty& binaryProperty : binaryProperties) {
      std::memcpy(
          buffer.cesium.data.data() + binaryProperty.gltfByteOffset,
          batchTableBinaryData.data() + binaryProperty.b3dmByteOffset,
          binaryProperty.byteLength);
    }
  }

  // Create an EXT_feature_metadata extension for each primitive with a _BATCHID
  // attribute.
  for (Mesh& mesh : gltf.meshes) {
    for (MeshPrimitive& primitive : mesh.primitives) {
      auto batchIDIt = primitive.attributes.find("_BATCHID");
      if (batchIDIt == primitive.attributes.end()) {
        // This primitive has no batch ID, ignore it.
        continue;
      }

      // Rename the _BATCHID attribute to _FEATURE_ID_0
      primitive.attributes["_FEATURE_ID_0"] = batchIDIt->second;
      primitive.attributes.erase("_BATCHID");

      // Create a feature extension
      MeshPrimitiveEXT_feature_metadata& extension =
          primitive.addExtension<MeshPrimitiveEXT_feature_metadata>();
      FeatureIDAttribute& attribute =
          extension.featureIdAttributes.emplace_back();
      attribute.featureTable = "default";
      attribute.featureIds.attribute = "_FEATURE_ID_0";
    }
  }
}

} // namespace Cesium3DTiles
