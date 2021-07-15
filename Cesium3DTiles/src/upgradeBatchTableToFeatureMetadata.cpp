#include "upgradeBatchTableToFeatureMetadata.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "CesiumGltf/MeshPrimitiveEXT_feature_metadata.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/ModelEXT_feature_metadata.h"
#include "CesiumGltf/PropertyType.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include <glm/glm.hpp>
#include <map>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <type_traits>

using namespace CesiumGltf;

namespace {
struct CompatibleTypes {
  PropertyType type;
  std::optional<PropertyType> componentType;
  std::optional<uint32_t> minComponentCount;
  std::optional<uint32_t> maxComponentCount;
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

template <typename T> bool isInRangeForSignedInteger(int64_t value) {
  // this only work if sizeof(T) is smaller than int64_t
  static_assert(
      !std::is_same_v<T, uint64_t> && !std::is_same_v<T, float> &&
      !std::is_same_v<T, double>);

  return value >= static_cast<int64_t>(std::numeric_limits<T>::lowest()) &&
         value <= static_cast<int64_t>(std::numeric_limits<T>::max());
}

template <typename T> bool isInRangeForUnsignedInteger(uint64_t value) {
  static_assert(!std::is_signed_v<T>);

  return value >= static_cast<uint64_t>(std::numeric_limits<T>::lowest()) &&
         value <= static_cast<uint64_t>(std::numeric_limits<T>::max());
}

template <typename OffsetType>
void copyStringBuffer(
    uint64_t totalSize,
    const std::vector<rapidjson::StringBuffer>& rapidjsonStrBuffers,
    std::vector<std::byte>& buffer,
    std::vector<std::byte>& offsetBuffer) {
  OffsetType stringOffset = 0;
  buffer.resize(totalSize);
  offsetBuffer.resize(sizeof(OffsetType) * (rapidjsonStrBuffers.size() + 1));
  OffsetType* offset = reinterpret_cast<OffsetType*>(offsetBuffer.data());
  for (const rapidjson::StringBuffer& rapidjsonBuffer : rapidjsonStrBuffers) {
    size_t bufferLength = rapidjsonBuffer.GetSize();
    if (bufferLength != 0) {
      std::memcpy(
          buffer.data() + stringOffset,
          rapidjsonBuffer.GetString(),
          bufferLength);
      *offset = stringOffset;
      stringOffset = static_cast<OffsetType>(stringOffset + bufferLength);
      ++offset;
    }
  }

  *offset = stringOffset;
}

CompatibleTypes findCompatibleTypes(const rapidjson::Value& propertyValue) {
  CompatibleTypes result{};

  for (auto it = propertyValue.Begin(); it != propertyValue.End(); ++it) {
    if (it->IsBool()) {
      // Should we allow conversion of bools to numeric 0 or 1? Nah.
      result.type = PropertyType::Boolean;
    } else if (it->IsInt64()) {
      int64_t value = it->GetInt64();
      if (isInRangeForSignedInteger<int8_t>(value) &&
          result.type <= PropertyType::Int8) {
        result.type = PropertyType::Int8;
      } else if (
          isInRangeForSignedInteger<uint8_t>(value) &&
          result.type <= PropertyType::Uint8) {
        result.type = PropertyType::Uint8;
      } else if (
          isInRangeForSignedInteger<int16_t>(value) &&
          result.type <= PropertyType::Int16) {
        result.type = PropertyType::Int16;
      } else if (
          isInRangeForSignedInteger<uint16_t>(value) &&
          result.type <= PropertyType::Uint16) {
        result.type = PropertyType::Uint16;
      } else if (
          isInRangeForSignedInteger<int32_t>(value) &&
          result.type <= PropertyType::Int32) {
        result.type = PropertyType::Int32;
      } else if (
          isInRangeForSignedInteger<uint32_t>(value) &&
          result.type <= PropertyType::Uint32) {
        result.type = PropertyType::Uint32;
      } else if (result.type <= PropertyType::Int64) {
        result.type = PropertyType::Int64;
      }
    } else if (it->IsUint64()) {
      result.type = PropertyType::Uint64;
    } else if (it->IsLosslessFloat()) {
      result.type = PropertyType::Float32;
    } else if (it->IsDouble()) {
      result.type = PropertyType::Float64;
    } else if (
        it->IsArray() && !it->Begin()->IsArray() &&
        (result.type == PropertyType::None ||
         result.type == PropertyType::Array)) {
      auto memberResult = findCompatibleTypes(*it);

      result.type = PropertyType::Array;
      if (!result.componentType || result.componentType < memberResult.type) {
        result.componentType = memberResult.type;
      }

      result.maxComponentCount =
          result.maxComponentCount
              ? glm::max(*result.maxComponentCount, it->Size())
              : it->Size();
      result.minComponentCount =
          result.minComponentCount
              ? glm::min(*result.minComponentCount, it->Size())
              : it->Size();
    } else {
      // A string, null, or something else. So convert to string
      result = CompatibleTypes{};
      result.type = PropertyType::String;
      break;
    }
  }

  return result;
}

void updateExtensionWithJsonStringProperty(
    Model& gltf,
    ClassProperty& classProperty,
    FeatureTable& /*featureTable*/,
    FeatureTableProperty& featureTableProperty,
    const rapidjson::Value& propertyValue) {
  uint64_t totalSize = 0;
  std::vector<rapidjson::StringBuffer> rapidjsonStrBuffers;
  rapidjsonStrBuffers.reserve(propertyValue.Size());
  for (const auto& v : propertyValue.GetArray()) {
    rapidjson::StringBuffer& buffer = rapidjsonStrBuffers.emplace_back();
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    v.Accept(writer);
    totalSize += buffer.GetSize();
  }

  std::vector<std::byte> buffer;
  std::vector<std::byte> offsetBuffer;
  if (isInRangeForUnsignedInteger<uint8_t>(totalSize)) {
    copyStringBuffer<uint8_t>(
        totalSize,
        rapidjsonStrBuffers,
        buffer,
        offsetBuffer);
    featureTableProperty.offsetType = "UINT8";
  } else if (isInRangeForUnsignedInteger<uint16_t>(totalSize)) {
    copyStringBuffer<uint16_t>(
        totalSize,
        rapidjsonStrBuffers,
        buffer,
        offsetBuffer);
    featureTableProperty.offsetType = "UINT16";
  } else if (isInRangeForUnsignedInteger<uint32_t>(totalSize)) {
    copyStringBuffer<uint32_t>(
        totalSize,
        rapidjsonStrBuffers,
        buffer,
        offsetBuffer);
    featureTableProperty.offsetType = "UINT32";
  } else {
    copyStringBuffer<uint64_t>(
        totalSize,
        rapidjsonStrBuffers,
        buffer,
        offsetBuffer);
    featureTableProperty.offsetType = "UINT64";
  }

  Buffer& gltfBuffer = gltf.buffers.emplace_back();
  gltfBuffer.byteLength = static_cast<int64_t>(buffer.size());
  gltfBuffer.cesium.data = std::move(buffer);

  BufferView& gltfBufferView = gltf.bufferViews.emplace_back();
  gltfBufferView.buffer = static_cast<int32_t>(gltf.buffers.size() - 1);
  gltfBufferView.byteOffset = 0;
  gltfBufferView.byteLength = static_cast<int64_t>(totalSize);
  int32_t valueBufferViewIdx =
      static_cast<int32_t>(gltf.bufferViews.size() - 1);

  Buffer& gltfOffsetBuffer = gltf.buffers.emplace_back();
  gltfOffsetBuffer.byteLength = static_cast<int64_t>(offsetBuffer.size());
  gltfOffsetBuffer.cesium.data = std::move(offsetBuffer);

  BufferView& gltfOffsetBufferView = gltf.bufferViews.emplace_back();
  gltfOffsetBufferView.buffer = static_cast<int32_t>(gltf.buffers.size() - 1);
  gltfOffsetBufferView.byteOffset = 0;
  gltfOffsetBufferView.byteLength =
      static_cast<int64_t>(gltfOffsetBuffer.cesium.data.size());
  int32_t offsetBufferViewIdx =
      static_cast<int32_t>(gltf.bufferViews.size() - 1);

  classProperty.type = "STRING";

  featureTableProperty.bufferView = valueBufferViewIdx;
  featureTableProperty.stringOffsetBufferView = offsetBufferViewIdx;
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
  buffer.byteLength =
      static_cast<int64_t>(sizeof(T) * static_cast<size_t>(featureTable.count));
  buffer.cesium.data.resize(static_cast<size_t>(buffer.byteLength));

  size_t bufferViewIndex = gltf.bufferViews.size();
  BufferView& bufferView = gltf.bufferViews.emplace_back();
  bufferView.buffer = int32_t(bufferIndex);
  bufferView.byteOffset = 0;
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
    Model& gltf,
    ClassProperty& classProperty,
    FeatureTable& featureTable,
    FeatureTableProperty& featureTableProperty,
    const rapidjson::Value& propertyValue) {
  std::vector<std::byte> data(static_cast<size_t>(
      glm::ceil(static_cast<double>(featureTable.count) / 8.0)));
  const auto& jsonArray = propertyValue.GetArray();
  for (rapidjson::SizeType i = 0; i < jsonArray.Size(); ++i) {
    bool value = jsonArray[i].GetBool();
    size_t byteIndex = i / 8;
    size_t bitIndex = i % 8;
    data[byteIndex] =
        static_cast<std::byte>(value << bitIndex) | data[byteIndex];
  }

  size_t bufferIndex = gltf.buffers.size();
  Buffer& buffer = gltf.buffers.emplace_back();
  buffer.byteLength = static_cast<int64_t>(data.size());
  buffer.cesium.data = std::move(data);

  BufferView& bufferView = gltf.bufferViews.emplace_back();
  bufferView.buffer = static_cast<int32_t>(bufferIndex);
  bufferView.byteOffset = 0;
  bufferView.byteLength = buffer.byteLength;

  featureTableProperty.bufferView =
      static_cast<int32_t>(gltf.bufferViews.size() - 1);

  classProperty.type = "BOOLEAN";
}

template <typename TRapidjson, typename ValueType, typename OffsetType>
void copyNumericDynamicArrayBuffers(
    std::vector<std::byte>& valueBuffer,
    std::vector<std::byte>& offsetBuffer,
    size_t numOfElements,
    const rapidjson::Value& propertyValue) {
  valueBuffer.resize(sizeof(ValueType) * numOfElements);
  offsetBuffer.resize(sizeof(OffsetType) * (propertyValue.Size() + 1));
  ValueType* value = reinterpret_cast<ValueType*>(valueBuffer.data());
  OffsetType* offsetValue = reinterpret_cast<OffsetType*>(offsetBuffer.data());
  OffsetType prevOffset = 0;
  for (const auto& jsonArrayMember : propertyValue.GetArray()) {
    *offsetValue = prevOffset;
    ++offsetValue;
    for (const auto& valueJson : jsonArrayMember.GetArray()) {
      *value = static_cast<ValueType>(valueJson.Get<TRapidjson>());
      ++value;
    }

    prevOffset = static_cast<OffsetType>(
        prevOffset + jsonArrayMember.Size() * sizeof(ValueType));
  }

  *offsetValue = prevOffset;
}

template <typename TRapidjson, typename ValueType>
void updateNumericArrayProperty(
    Model& gltf,
    ClassProperty& classProperty,
    FeatureTableProperty& featureTableProperty,
    const CompatibleTypes& compatibleTypes,
    const rapidjson::Value& propertyValue) {
  const auto& jsonOuterArray = propertyValue.GetArray();

  // check if it's a fixed array
  if (compatibleTypes.minComponentCount == compatibleTypes.maxComponentCount) {
    size_t numOfValues =
        jsonOuterArray.Size() * *compatibleTypes.minComponentCount;
    std::vector<std::byte> valueBuffer(sizeof(ValueType) * numOfValues);
    ValueType* value = reinterpret_cast<ValueType*>(valueBuffer.data());
    for (const auto& jsonArrayMember : jsonOuterArray) {
      for (const auto& valueJson : jsonArrayMember.GetArray()) {
        *value = static_cast<ValueType>(valueJson.Get<TRapidjson>());
        ++value;
      }
    }

    Buffer& gltfValueBuffer = gltf.buffers.emplace_back();
    gltfValueBuffer.byteLength = static_cast<int64_t>(valueBuffer.size());
    gltfValueBuffer.cesium.data = std::move(valueBuffer);

    BufferView& gltfValueBufferView = gltf.bufferViews.emplace_back();
    gltfValueBufferView.buffer = static_cast<int32_t>(gltf.buffers.size() - 1);
    gltfValueBufferView.byteOffset = 0;
    gltfValueBufferView.byteLength =
        static_cast<int64_t>(gltfValueBuffer.cesium.data.size());

    classProperty.type = "ARRAY";
    classProperty.componentType = convertPropertyTypeToString(
        static_cast<PropertyType>(TypeToPropertyType<ValueType>::value));
    classProperty.componentCount = *compatibleTypes.minComponentCount;

    featureTableProperty.bufferView =
        static_cast<int32_t>(gltf.bufferViews.size() - 1);

    return;
  }

  // total size of value buffer
  size_t numOfElements = 0;
  for (const auto& jsonArrayMember : jsonOuterArray) {
    numOfElements += jsonArrayMember.Size();
  }

  PropertyType offsetType = PropertyType::None;
  std::vector<std::byte> valueBuffer;
  std::vector<std::byte> offsetBuffer;
  uint64_t maxOffsetValue = numOfElements * sizeof(ValueType);
  if (isInRangeForUnsignedInteger<uint8_t>(maxOffsetValue)) {
    copyNumericDynamicArrayBuffers<TRapidjson, ValueType, uint8_t>(
        valueBuffer,
        offsetBuffer,
        numOfElements,
        propertyValue);
    offsetType = PropertyType::Uint8;
  } else if (isInRangeForUnsignedInteger<uint16_t>(maxOffsetValue)) {
    copyNumericDynamicArrayBuffers<TRapidjson, ValueType, uint16_t>(
        valueBuffer,
        offsetBuffer,
        numOfElements,
        propertyValue);
    offsetType = PropertyType::Uint16;
  } else if (isInRangeForUnsignedInteger<uint32_t>(maxOffsetValue)) {
    copyNumericDynamicArrayBuffers<TRapidjson, ValueType, uint32_t>(
        valueBuffer,
        offsetBuffer,
        numOfElements,
        propertyValue);
    offsetType = PropertyType::Uint32;
  } else if (isInRangeForUnsignedInteger<uint64_t>(maxOffsetValue)) {
    copyNumericDynamicArrayBuffers<TRapidjson, ValueType, uint64_t>(
        valueBuffer,
        offsetBuffer,
        numOfElements,
        propertyValue);
    offsetType = PropertyType::Uint64;
  }

  Buffer& gltfValueBuffer = gltf.buffers.emplace_back();
  gltfValueBuffer.byteLength = static_cast<int64_t>(valueBuffer.size());
  gltfValueBuffer.cesium.data = std::move(valueBuffer);

  BufferView& gltfValueBufferView = gltf.bufferViews.emplace_back();
  gltfValueBufferView.buffer = static_cast<int32_t>(gltf.buffers.size() - 1);
  gltfValueBufferView.byteOffset = 0;
  gltfValueBufferView.byteLength =
      static_cast<int64_t>(gltfValueBuffer.cesium.data.size());
  int32_t valueBufferIdx = static_cast<int32_t>(gltf.bufferViews.size() - 1);

  Buffer& gltfOffsetBuffer = gltf.buffers.emplace_back();
  gltfOffsetBuffer.byteLength = static_cast<int64_t>(offsetBuffer.size());
  gltfOffsetBuffer.cesium.data = std::move(offsetBuffer);

  BufferView& gltfOffsetBufferView = gltf.bufferViews.emplace_back();
  gltfOffsetBufferView.buffer = static_cast<int32_t>(gltf.buffers.size() - 1);
  gltfOffsetBufferView.byteOffset = 0;
  gltfOffsetBufferView.byteLength =
      static_cast<int64_t>(gltfOffsetBuffer.cesium.data.size());
  int32_t offsetBufferIdx = static_cast<int32_t>(gltf.bufferViews.size() - 1);

  classProperty.type = "ARRAY";
  classProperty.componentType = convertPropertyTypeToString(
      static_cast<PropertyType>(TypeToPropertyType<ValueType>::value));

  featureTableProperty.bufferView = valueBufferIdx;
  featureTableProperty.arrayOffsetBufferView = offsetBufferIdx;
  featureTableProperty.offsetType = convertPropertyTypeToString(offsetType);
}

template <typename OffsetType>
void copyStringArrayBuffers(
    std::vector<std::byte>& valueBuffer,
    std::vector<std::byte>& offsetBuffer,
    size_t totalByteLength,
    size_t numOfString,
    const rapidjson::Value& propertyValue) {
  valueBuffer.resize(totalByteLength);
  offsetBuffer.resize((numOfString + 1) * sizeof(OffsetType));
  OffsetType offset = 0;
  size_t offsetIndex = 0;
  for (const auto& arrayMember : propertyValue.GetArray()) {
    for (const auto& str : arrayMember.GetArray()) {
      OffsetType byteLength = static_cast<OffsetType>(
          str.GetStringLength() * sizeof(rapidjson::Value::Ch));
      std::memcpy(valueBuffer.data() + offset, str.GetString(), byteLength);
      std::memcpy(
          offsetBuffer.data() + offsetIndex * sizeof(OffsetType),
          &offset,
          sizeof(OffsetType));
      offset = static_cast<OffsetType>(offset + byteLength);
      ++offsetIndex;
    }
  }

  std::memcpy(
      offsetBuffer.data() + offsetIndex * sizeof(OffsetType),
      &offset,
      sizeof(OffsetType));
}

template <typename OffsetType>
void copyArrayOffsetBufferForStringArrayProperty(
    std::vector<std::byte>& offsetBuffer,
    size_t totalInstances,
    const rapidjson::Value& propertyValue) {
  OffsetType prevOffset = 0;
  offsetBuffer.resize((totalInstances + 1) * sizeof(OffsetType));
  OffsetType* offset = reinterpret_cast<OffsetType*>(offsetBuffer.data());
  for (const auto& arrayMember : propertyValue.GetArray()) {
    *offset = prevOffset;
    prevOffset = static_cast<OffsetType>(
        prevOffset + arrayMember.Size() * sizeof(OffsetType));
    ++offset;
  }

  *offset = prevOffset;
}

void updateStringArrayProperty(
    Model& gltf,
    ClassProperty& classProperty,
    FeatureTableProperty& featureTableProperty,
    const CompatibleTypes& compatibleTypes,
    const rapidjson::Value& propertyValue) {
  size_t numOfString = 0;
  size_t totalChars = 0;
  for (const auto& arrayMember : propertyValue.GetArray()) {
    numOfString += arrayMember.Size();
    for (const auto& str : arrayMember.GetArray()) {
      totalChars += str.GetStringLength();
    }
  }

  uint64_t totalByteLength = totalChars * sizeof(rapidjson::Value::Ch);
  std::vector<std::byte> valueBuffer;
  std::vector<std::byte> offsetBuffer;
  PropertyType offsetType = PropertyType::None;
  if (isInRangeForUnsignedInteger<uint8_t>(totalByteLength)) {
    copyStringArrayBuffers<uint8_t>(
        valueBuffer,
        offsetBuffer,
        totalByteLength,
        numOfString,
        propertyValue);
    offsetType = PropertyType::Uint8;
  } else if (isInRangeForUnsignedInteger<uint16_t>(totalByteLength)) {
    copyStringArrayBuffers<uint16_t>(
        valueBuffer,
        offsetBuffer,
        totalByteLength,
        numOfString,
        propertyValue);
    offsetType = PropertyType::Uint16;
  } else if (isInRangeForUnsignedInteger<uint32_t>(totalByteLength)) {
    copyStringArrayBuffers<uint32_t>(
        valueBuffer,
        offsetBuffer,
        totalByteLength,
        numOfString,
        propertyValue);
    offsetType = PropertyType::Uint32;
  } else if (isInRangeForUnsignedInteger<uint64_t>(totalByteLength)) {
    copyStringArrayBuffers<uint64_t>(
        valueBuffer,
        offsetBuffer,
        totalByteLength,
        numOfString,
        propertyValue);
    offsetType = PropertyType::Uint64;
  }

  // create gltf value buffer view
  Buffer& gltfValueBuffer = gltf.buffers.emplace_back();
  gltfValueBuffer.byteLength = static_cast<int64_t>(valueBuffer.size());
  gltfValueBuffer.cesium.data = std::move(valueBuffer);

  BufferView& gltfValueBufferView = gltf.bufferViews.emplace_back();
  gltfValueBufferView.buffer =
      static_cast<std::int32_t>(gltf.buffers.size() - 1);
  gltfValueBufferView.byteOffset = 0;
  gltfValueBufferView.byteLength = gltfValueBuffer.byteLength;
  int32_t valueBufferViewIdx =
      static_cast<int32_t>(gltf.bufferViews.size() - 1);

  // create gltf string offset buffer view
  Buffer& gltfOffsetBuffer = gltf.buffers.emplace_back();
  gltfOffsetBuffer.byteLength = static_cast<int64_t>(offsetBuffer.size());
  gltfOffsetBuffer.cesium.data = std::move(offsetBuffer);

  BufferView& gltfOffsetBufferView = gltf.bufferViews.emplace_back();
  gltfOffsetBufferView.buffer = static_cast<int32_t>(gltf.buffers.size() - 1);
  gltfOffsetBufferView.byteOffset = 0;
  gltfOffsetBufferView.byteLength = gltfOffsetBuffer.byteLength;
  int32_t offsetBufferViewIdx =
      static_cast<int32_t>(gltf.bufferViews.size() - 1);

  // fixed array of string
  if (compatibleTypes.minComponentCount == compatibleTypes.maxComponentCount) {
    classProperty.type = "ARRAY";
    classProperty.componentCount = compatibleTypes.minComponentCount;
    classProperty.componentType = "STRING";

    featureTableProperty.bufferView = valueBufferViewIdx;
    featureTableProperty.stringOffsetBufferView = offsetBufferViewIdx;
    featureTableProperty.offsetType = convertPropertyTypeToString(offsetType);
    return;
  }

  // dynamic array of string needs array offset buffer
  size_t totalInstances = propertyValue.Size();
  std::vector<std::byte> arrayOffsetBuffer;
  switch (offsetType) {
  case PropertyType::Uint8:
    copyArrayOffsetBufferForStringArrayProperty<uint8_t>(
        arrayOffsetBuffer,
        totalInstances,
        propertyValue);
    break;
  case PropertyType::Uint16:
    copyArrayOffsetBufferForStringArrayProperty<uint16_t>(
        arrayOffsetBuffer,
        totalInstances,
        propertyValue);
    break;
  case PropertyType::Uint32:
    copyArrayOffsetBufferForStringArrayProperty<uint32_t>(
        arrayOffsetBuffer,
        totalInstances,
        propertyValue);
    break;
  case PropertyType::Uint64:
    copyArrayOffsetBufferForStringArrayProperty<uint64_t>(
        arrayOffsetBuffer,
        totalInstances,
        propertyValue);
    break;
  default:
    break;
  }

  // create gltf array offset buffer view
  Buffer& gltfArrayOffsetBuffer = gltf.buffers.emplace_back();
  gltfArrayOffsetBuffer.byteLength =
      static_cast<int64_t>(arrayOffsetBuffer.size());
  gltfArrayOffsetBuffer.cesium.data = std::move(arrayOffsetBuffer);

  BufferView& gltfArrayOffsetBufferView = gltf.bufferViews.emplace_back();
  gltfArrayOffsetBufferView.buffer =
      static_cast<int32_t>(gltf.buffers.size() - 1);
  gltfArrayOffsetBufferView.byteOffset = 0;
  gltfArrayOffsetBufferView.byteLength = gltfArrayOffsetBuffer.byteLength;
  int32_t arrayOffsetBufferViewIdx =
      static_cast<int32_t>(gltf.bufferViews.size() - 1);

  classProperty.type = "ARRAY";
  classProperty.componentType = "STRING";

  featureTableProperty.bufferView = valueBufferViewIdx;
  featureTableProperty.arrayOffsetBufferView = arrayOffsetBufferViewIdx;
  featureTableProperty.stringOffsetBufferView = offsetBufferViewIdx;
  featureTableProperty.offsetType = convertPropertyTypeToString(offsetType);
}

template <typename OffsetType>
void copyBooleanArrayBuffers(
    std::vector<std::byte>& valueBuffer,
    std::vector<std::byte>& offsetBuffer,
    size_t numOfElements,
    const rapidjson::Value& propertyValue) {
  size_t currentIndex = 0;
  size_t totalByteLength =
      static_cast<size_t>(glm::ceil(static_cast<double>(numOfElements) / 8.0));
  valueBuffer.resize(totalByteLength);
  offsetBuffer.resize((propertyValue.Size() + 1) * sizeof(OffsetType));
  OffsetType* offset = reinterpret_cast<OffsetType*>(offsetBuffer.data());
  OffsetType prevOffset = 0;
  for (const auto& arrayMember : propertyValue.GetArray()) {
    *offset = prevOffset;
    ++offset;
    prevOffset = static_cast<OffsetType>(prevOffset + arrayMember.Size());
    for (const auto& data : arrayMember.GetArray()) {
      bool value = data.GetBool();
      size_t byteIndex = currentIndex / 8;
      size_t bitIndex = currentIndex % 8;
      valueBuffer[byteIndex] =
          static_cast<std::byte>(value << bitIndex) | valueBuffer[byteIndex];
      ++currentIndex;
    }
  }

  *offset = prevOffset;
}

void updateBooleanArrayProperty(
    Model& gltf,
    ClassProperty& classProperty,
    FeatureTableProperty& featureTableProperty,
    const CompatibleTypes& compatibleTypes,
    const rapidjson::Value& propertyValue) {
  // fixed array of boolean
  if (compatibleTypes.minComponentCount == compatibleTypes.maxComponentCount) {
    size_t numOfElements =
        propertyValue.Size() * compatibleTypes.minComponentCount.value();
    size_t totalByteLength = static_cast<size_t>(
        glm::ceil(static_cast<double>(numOfElements) / 8.0));
    std::vector<std::byte> valueBuffer(totalByteLength);
    size_t currentIndex = 0;
    for (const auto& arrayMember : propertyValue.GetArray()) {
      for (const auto& data : arrayMember.GetArray()) {
        bool value = data.GetBool();
        size_t byteIndex = currentIndex / 8;
        size_t bitIndex = currentIndex % 8;
        valueBuffer[byteIndex] =
            static_cast<std::byte>(value << bitIndex) | valueBuffer[byteIndex];
        ++currentIndex;
      }
    }

    Buffer& gltfValueBuffer = gltf.buffers.emplace_back();
    gltfValueBuffer.byteLength = static_cast<int64_t>(valueBuffer.size());
    gltfValueBuffer.cesium.data = std::move(valueBuffer);

    BufferView& gltfValueBufferView = gltf.bufferViews.emplace_back();
    gltfValueBufferView.buffer = static_cast<int32_t>(gltf.buffers.size() - 1);
    gltfValueBufferView.byteOffset = 0;
    gltfValueBufferView.byteLength = gltfValueBuffer.byteLength;

    classProperty.type = "ARRAY";
    classProperty.componentCount = compatibleTypes.minComponentCount;
    classProperty.componentType = "BOOLEAN";

    featureTableProperty.bufferView =
        static_cast<int32_t>(gltf.bufferViews.size() - 1);

    return;
  }

  // dynamic array of boolean
  size_t numOfElements = 0;
  for (const auto& arrayMember : propertyValue.GetArray()) {
    numOfElements += arrayMember.Size();
  }

  std::vector<std::byte> valueBuffer;
  std::vector<std::byte> offsetBuffer;
  PropertyType offsetType = PropertyType::None;
  if (isInRangeForUnsignedInteger<uint8_t>(numOfElements)) {
    copyBooleanArrayBuffers<uint8_t>(
        valueBuffer,
        offsetBuffer,
        numOfElements,
        propertyValue);
    offsetType = PropertyType::Uint8;
  } else if (isInRangeForUnsignedInteger<uint16_t>(numOfElements)) {
    copyBooleanArrayBuffers<uint16_t>(
        valueBuffer,
        offsetBuffer,
        numOfElements,
        propertyValue);
    offsetType = PropertyType::Uint16;
  } else if (isInRangeForUnsignedInteger<uint32_t>(numOfElements)) {
    copyBooleanArrayBuffers<uint32_t>(
        valueBuffer,
        offsetBuffer,
        numOfElements,
        propertyValue);
    offsetType = PropertyType::Uint32;
  } else {
    copyBooleanArrayBuffers<uint64_t>(
        valueBuffer,
        offsetBuffer,
        numOfElements,
        propertyValue);
    offsetType = PropertyType::Uint64;
  }

  Buffer& gltfValueBuffer = gltf.buffers.emplace_back();
  gltfValueBuffer.byteLength = static_cast<int64_t>(valueBuffer.size());
  gltfValueBuffer.cesium.data = std::move(valueBuffer);

  BufferView& gltfValueBufferView = gltf.bufferViews.emplace_back();
  gltfValueBufferView.buffer = static_cast<int32_t>(gltf.buffers.size() - 1);
  gltfValueBufferView.byteOffset = 0;
  gltfValueBufferView.byteLength =
      static_cast<int64_t>(gltfValueBuffer.cesium.data.size());
  int32_t valueBufferIdx = static_cast<int32_t>(gltf.bufferViews.size() - 1);

  Buffer& gltfOffsetBuffer = gltf.buffers.emplace_back();
  gltfOffsetBuffer.byteLength = static_cast<int64_t>(offsetBuffer.size());
  gltfOffsetBuffer.cesium.data = std::move(offsetBuffer);

  BufferView& gltfOffsetBufferView = gltf.bufferViews.emplace_back();
  gltfOffsetBufferView.buffer = static_cast<int32_t>(gltf.buffers.size() - 1);
  gltfOffsetBufferView.byteOffset = 0;
  gltfOffsetBufferView.byteLength =
      static_cast<int64_t>(gltfOffsetBuffer.cesium.data.size());
  int32_t offsetBufferIdx = static_cast<int32_t>(gltf.bufferViews.size() - 1);

  classProperty.type = "ARRAY";
  classProperty.componentType = "BOOLEAN";

  featureTableProperty.bufferView = valueBufferIdx;
  featureTableProperty.arrayOffsetBufferView = offsetBufferIdx;
  featureTableProperty.offsetType = convertPropertyTypeToString(offsetType);
}

void updateExtensionWithArrayProperty(
    Model& gltf,
    ClassProperty& classProperty,
    FeatureTable& /*featureTable*/,
    FeatureTableProperty& featureTableProperty,
    const CompatibleTypes& compatibleTypes,
    const rapidjson::Value& propertyValue) {
  switch (*compatibleTypes.componentType) {
  case PropertyType::Boolean:
    updateBooleanArrayProperty(
        gltf,
        classProperty,
        featureTableProperty,
        compatibleTypes,
        propertyValue);
    break;
  case PropertyType::Int8:
    updateNumericArrayProperty<int32_t, int8_t>(
        gltf,
        classProperty,
        featureTableProperty,
        compatibleTypes,
        propertyValue);
    break;
  case PropertyType::Uint8:
    updateNumericArrayProperty<uint32_t, uint8_t>(
        gltf,
        classProperty,
        featureTableProperty,
        compatibleTypes,
        propertyValue);
    break;
  case PropertyType::Int16:
    updateNumericArrayProperty<int32_t, int16_t>(
        gltf,
        classProperty,
        featureTableProperty,
        compatibleTypes,
        propertyValue);
    break;
  case PropertyType::Uint16:
    updateNumericArrayProperty<uint32_t, uint16_t>(
        gltf,
        classProperty,
        featureTableProperty,
        compatibleTypes,
        propertyValue);
    break;
  case PropertyType::Int32:
    updateNumericArrayProperty<int32_t, int32_t>(
        gltf,
        classProperty,
        featureTableProperty,
        compatibleTypes,
        propertyValue);
    break;
  case PropertyType::Uint32:
    updateNumericArrayProperty<uint32_t, uint32_t>(
        gltf,
        classProperty,
        featureTableProperty,
        compatibleTypes,
        propertyValue);
    break;
  case PropertyType::Int64:
    updateNumericArrayProperty<int64_t, int64_t>(
        gltf,
        classProperty,
        featureTableProperty,
        compatibleTypes,
        propertyValue);
    break;
  case PropertyType::Uint64:
    updateNumericArrayProperty<uint64_t, uint64_t>(
        gltf,
        classProperty,
        featureTableProperty,
        compatibleTypes,
        propertyValue);
    break;
  case PropertyType::Float32:
    updateNumericArrayProperty<float, float>(
        gltf,
        classProperty,
        featureTableProperty,
        compatibleTypes,
        propertyValue);
    break;
  case PropertyType::Float64:
    updateNumericArrayProperty<double, double>(
        gltf,
        classProperty,
        featureTableProperty,
        compatibleTypes,
        propertyValue);
    break;
  case PropertyType::String:
    updateStringArrayProperty(
        gltf,
        classProperty,
        featureTableProperty,
        compatibleTypes,
        propertyValue);
    break;
  default:
    break;
  }
}

void updateExtensionWithJsonProperty(
    Model& gltf,
    ClassProperty& classProperty,
    FeatureTable& featureTable,
    FeatureTableProperty& featureTableProperty,
    const rapidjson::Value& propertyValue) {

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

  // Figure out which types we can use for this data.
  // Use the smallest type we can, and prefer signed to unsigned.
  CompatibleTypes compatibleTypes = findCompatibleTypes(propertyValue);

  switch (compatibleTypes.type) {
  case PropertyType::Boolean:
    updateExtensionWithJsonBoolProperty(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue);
    break;
  case PropertyType::Int8:
    updateExtensionWithJsonNumericProperty<int8_t, int32_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "INT8");
    break;
  case PropertyType::Uint8:
    updateExtensionWithJsonNumericProperty<uint8_t, uint32_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "UINT8");
    break;
  case PropertyType::Int16:
    updateExtensionWithJsonNumericProperty<int16_t, int32_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "INT16");
    break;
  case PropertyType::Uint16:
    updateExtensionWithJsonNumericProperty<uint16_t, uint32_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "UINT16");
    break;
  case PropertyType::Int32:
    updateExtensionWithJsonNumericProperty<int32_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "INT32");
    break;
  case PropertyType::Uint32:
    updateExtensionWithJsonNumericProperty<uint32_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "UINT32");
    break;
  case PropertyType::Int64:
    updateExtensionWithJsonNumericProperty<int64_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "INT64");
    break;
  case PropertyType::Uint64:
    updateExtensionWithJsonNumericProperty<uint64_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "UINT64");
    break;
  case PropertyType::Float32:
    updateExtensionWithJsonNumericProperty<float>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "FLOAT32");
    break;
  case PropertyType::Float64:
    updateExtensionWithJsonNumericProperty<double>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "FLOAT64");
    break;
  case PropertyType::String:
    updateExtensionWithJsonStringProperty(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue);
    break;
  case PropertyType::Array:
    updateExtensionWithArrayProperty(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        compatibleTypes,
        propertyValue);
    break;
  default:
    break;
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
  assert(
      gltfBufferIndex >= 0 &&
      "gltfBufferIndex is negative. Need to allocate buffer before "
      "convert the binary property");

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

  size_t componentCount = 1;
  if (type == "SCALAR") {
    classProperty.type = gltfType.typeName;
  } else if (type == "VEC2") {
    classProperty.type = "ARRAY";
    classProperty.componentCount = 2;
    classProperty.componentType = gltfType.typeName;
    componentCount = 2;
  } else if (type == "VEC3") {
    classProperty.type = "ARRAY";
    classProperty.componentCount = 3;
    classProperty.componentType = gltfType.typeName;
    componentCount = 3;
  } else if (type == "VEC4") {
    classProperty.type = "ARRAY";
    classProperty.componentCount = 4;
    classProperty.componentType = gltfType.typeName;
    componentCount = 4;
  } else {
    return;
  }

  // convert feature table
  size_t typeSize = gltfType.typeSize;
  auto& bufferView = gltf.bufferViews.emplace_back();
  bufferView.buffer = gltfBufferIndex;
  bufferView.byteOffset = gltfBufferOffset;
  bufferView.byteLength = static_cast<int64_t>(
      typeSize * componentCount * static_cast<size_t>(featureTable.count));

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
    const rapidjson::Document& batchTableJson,
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
  for (auto propertyIt = batchTableJson.MemberBegin();
       propertyIt != batchTableJson.MemberEnd();
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
    Buffer& buffer = gltf.buffers[static_cast<size_t>(gltfBufferIndex)];
    buffer.byteLength = gltfBufferOffset;
    buffer.cesium.data.resize(static_cast<size_t>(gltfBufferOffset));
    for (const BinaryProperty& binaryProperty : binaryProperties) {
      std::memcpy(
          buffer.cesium.data.data() + binaryProperty.gltfByteOffset,
          batchTableBinaryData.data() + binaryProperty.b3dmByteOffset,
          static_cast<size_t>(binaryProperty.byteLength));
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
