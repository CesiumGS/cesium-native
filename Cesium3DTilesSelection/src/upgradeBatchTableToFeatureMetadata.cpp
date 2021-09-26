#include "upgradeBatchTableToFeatureMetadata.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"
#include "CesiumGltf/MeshPrimitiveEXT_feature_metadata.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/ModelEXT_feature_metadata.h"
#include "CesiumGltf/PropertyType.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumUtility/Tracing.h"
#include <glm/glm.hpp>
#include <map>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <type_traits>

using namespace CesiumGltf;

namespace {
struct MaskedType {
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
  bool isArray = true;
};

struct CompatibleTypes {
  MaskedType type;
  std::optional<MaskedType> componentType;
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

int64_t roundUp(int64_t num, int64_t multiple) noexcept {
  return ((num + multiple - 1) / multiple) * multiple;
}

template <typename T> bool isInRangeForSignedInteger(int64_t value) noexcept {
  // this only work if sizeof(T) is smaller than int64_t
  static_assert(
      !std::is_same_v<T, uint64_t> && !std::is_same_v<T, float> &&
      !std::is_same_v<T, double>);

  return value >= static_cast<int64_t>(std::numeric_limits<T>::lowest()) &&
         value <= static_cast<int64_t>(std::numeric_limits<T>::max());
}

template <typename T>
bool isInRangeForUnsignedInteger(uint64_t value) noexcept {
  static_assert(!std::is_signed_v<T>);

  return value >= static_cast<uint64_t>(std::numeric_limits<T>::lowest()) &&
         value <= static_cast<uint64_t>(std::numeric_limits<T>::max());
}

template <typename OffsetType>
void copyStringBuffer(
    const rapidjson::StringBuffer& rapidjsonStrBuffer,
    const std::vector<uint64_t>& rapidjsonOffsets,
    std::vector<std::byte>& buffer,
    std::vector<std::byte>& offsetBuffer) {
  buffer.resize(rapidjsonStrBuffer.GetLength());
  std::memcpy(buffer.data(), rapidjsonStrBuffer.GetString(), buffer.size());

  offsetBuffer.resize(sizeof(OffsetType) * rapidjsonOffsets.size());
  OffsetType* offset = reinterpret_cast<OffsetType*>(offsetBuffer.data());
  for (size_t i = 0; i < rapidjsonOffsets.size(); ++i) {
    offset[i] = static_cast<OffsetType>(rapidjsonOffsets[i]);
  }
}

CompatibleTypes findCompatibleTypes(const rapidjson::Value& propertyValue) {
  MaskedType type;
  std::optional<MaskedType> componentType;
  std::optional<uint32_t> minComponentCount;
  std::optional<uint32_t> maxComponentCount;
  for (auto it = propertyValue.Begin(); it != propertyValue.End(); ++it) {
    if (it->IsBool()) {
      // Should we allow conversion of bools to numeric 0 or 1? Nah.
      type.isInt8 = type.isUint8 = false;
      type.isInt16 = type.isUint16 = false;
      type.isInt32 = type.isUint32 = false;
      type.isInt64 = type.isUint64 = false;
      type.isFloat32 = false;
      type.isFloat64 = false;
      type.isBool = true;
      type.isArray = false;
    } else if (it->IsInt64()) {
      const int64_t value = it->GetInt64();
      type.isInt8 &= isInRangeForSignedInteger<int8_t>(value);
      type.isUint8 &= isInRangeForSignedInteger<uint8_t>(value);
      type.isInt16 &= isInRangeForSignedInteger<int16_t>(value);
      type.isUint16 &= isInRangeForSignedInteger<uint16_t>(value);
      type.isInt32 &= isInRangeForSignedInteger<int32_t>(value);
      type.isUint32 &= isInRangeForSignedInteger<uint32_t>(value);
      type.isInt64 &= true;
      type.isUint64 &= value >= 0;
      type.isFloat32 &= it->IsLosslessFloat();
      type.isFloat64 &= it->IsLosslessDouble();
      type.isBool = false;
      type.isArray = false;
    } else if (it->IsUint64()) {
      // Only uint64_t can represent a value that fits in a uint64_t but not in
      // an int64_t.
      type.isInt8 = type.isUint8 = false;
      type.isInt16 = type.isUint16 = false;
      type.isInt32 = type.isUint32 = false;
      type.isInt64 = false;
      type.isUint64 = true;
      type.isFloat32 = false;
      type.isFloat64 = false;
      type.isBool = false;
      type.isArray = false;
    } else if (it->IsLosslessFloat()) {
      type.isInt8 = type.isUint8 = false;
      type.isInt16 = type.isUint16 = false;
      type.isInt32 = type.isUint32 = false;
      type.isInt64 = type.isUint64 = false;
      type.isFloat32 = true;
      type.isFloat64 = true;
      type.isBool = false;
      type.isArray = false;
    } else if (it->IsDouble()) {
      type.isInt8 = type.isUint8 = false;
      type.isInt16 = type.isUint16 = false;
      type.isInt32 = type.isUint32 = false;
      type.isInt64 = type.isUint64 = false;
      type.isFloat32 = false;
      type.isFloat64 = true;
      type.isBool = false;
      type.isArray = false;
    } else if (it->IsArray()) {
      type.isInt8 = type.isUint8 = false;
      type.isInt16 = type.isUint16 = false;
      type.isInt32 = type.isUint32 = false;
      type.isInt64 = type.isUint64 = false;
      type.isFloat32 = false;
      type.isFloat64 = false;
      type.isBool = false;
      type.isArray &= true;
      CompatibleTypes currentComponentType = findCompatibleTypes(*it);
      if (!componentType) {
        componentType = currentComponentType.type;
      } else {
        componentType->isInt8 &= currentComponentType.type.isInt8;
        componentType->isUint8 &= currentComponentType.type.isUint8;
        componentType->isInt16 &= currentComponentType.type.isInt16;
        componentType->isUint16 &= currentComponentType.type.isUint16;
        componentType->isInt32 &= currentComponentType.type.isInt32;
        componentType->isUint32 &= currentComponentType.type.isUint32;
        componentType->isInt64 &= currentComponentType.type.isInt64;
        componentType->isUint64 &= currentComponentType.type.isUint64;
        componentType->isFloat32 &= currentComponentType.type.isFloat32;
        componentType->isFloat64 &= currentComponentType.type.isFloat64;
        componentType->isBool &= currentComponentType.type.isBool;
        componentType->isArray &= currentComponentType.type.isArray;
      }

      maxComponentCount = maxComponentCount
                              ? glm::max(*maxComponentCount, it->Size())
                              : it->Size();
      minComponentCount = minComponentCount
                              ? glm::min(*minComponentCount, it->Size())
                              : it->Size();
    } else {
      // A string, null, or something else.
      type.isInt8 = type.isUint8 = false;
      type.isInt16 = type.isUint16 = false;
      type.isInt32 = type.isUint32 = false;
      type.isInt64 = type.isUint64 = false;
      type.isFloat32 = false;
      type.isFloat64 = false;
      type.isBool = false;
      type.isArray = false;
    }
  }

  return {type, componentType, minComponentCount, maxComponentCount};
}

void updateExtensionWithJsonStringProperty(
    Model& gltf,
    ClassProperty& classProperty,
    const FeatureTable& featureTable,
    FeatureTableProperty& featureTableProperty,
    const rapidjson::Value& propertyValue) {
  assert(propertyValue.Size() >= featureTable.count);

  rapidjson::StringBuffer rapidjsonStrBuffer;
  std::vector<uint64_t> rapidjsonOffsets;
  rapidjsonOffsets.reserve(static_cast<size_t>(featureTable.count + 1));
  rapidjsonOffsets.emplace_back(0);

  auto it = propertyValue.Begin();
  for (int64_t i = 0; i < featureTable.count; ++i) {
    if (!it->IsString()) {
      // Everything else that is not string will be serialized by json
      rapidjson::Writer<rapidjson::StringBuffer> writer(rapidjsonStrBuffer);
      it->Accept(writer);
    } else {
      // Because serialized string json will add double quotations in the buffer
      // which is not needed by us, we will manually add the string to the
      // buffer
      const auto& rapidjsonStr = it->GetString();
      rapidjsonStrBuffer.Reserve(it->GetStringLength());
      for (rapidjson::SizeType j = 0; j < it->GetStringLength(); ++j) {
        rapidjsonStrBuffer.PutUnsafe(rapidjsonStr[j]);
      }
    }

    rapidjsonOffsets.emplace_back(rapidjsonStrBuffer.GetLength());
    ++it;
  }

  const uint64_t totalSize = rapidjsonOffsets.back();
  std::vector<std::byte> buffer;
  std::vector<std::byte> offsetBuffer;
  if (isInRangeForUnsignedInteger<uint8_t>(totalSize)) {
    copyStringBuffer<uint8_t>(
        rapidjsonStrBuffer,
        rapidjsonOffsets,
        buffer,
        offsetBuffer);
    featureTableProperty.offsetType = "UINT8";
  } else if (isInRangeForUnsignedInteger<uint16_t>(totalSize)) {
    copyStringBuffer<uint16_t>(
        rapidjsonStrBuffer,
        rapidjsonOffsets,
        buffer,
        offsetBuffer);
    featureTableProperty.offsetType = "UINT16";
  } else if (isInRangeForUnsignedInteger<uint32_t>(totalSize)) {
    copyStringBuffer<uint32_t>(
        rapidjsonStrBuffer,
        rapidjsonOffsets,
        buffer,
        offsetBuffer);
    featureTableProperty.offsetType = "UINT32";
  } else {
    copyStringBuffer<uint64_t>(
        rapidjsonStrBuffer,
        rapidjsonOffsets,
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
  const int32_t valueBufferViewIdx =
      static_cast<int32_t>(gltf.bufferViews.size() - 1);

  Buffer& gltfOffsetBuffer = gltf.buffers.emplace_back();
  gltfOffsetBuffer.byteLength = static_cast<int64_t>(offsetBuffer.size());
  gltfOffsetBuffer.cesium.data = std::move(offsetBuffer);

  BufferView& gltfOffsetBufferView = gltf.bufferViews.emplace_back();
  gltfOffsetBufferView.buffer = static_cast<int32_t>(gltf.buffers.size() - 1);
  gltfOffsetBufferView.byteOffset = 0;
  gltfOffsetBufferView.byteLength =
      static_cast<int64_t>(gltfOffsetBuffer.cesium.data.size());
  const int32_t offsetBufferViewIdx =
      static_cast<int32_t>(gltf.bufferViews.size() - 1);

  classProperty.type = "STRING";

  featureTableProperty.bufferView = valueBufferViewIdx;
  featureTableProperty.stringOffsetBufferView = offsetBufferViewIdx;
}

template <typename T, typename TRapidJson = T>
void updateExtensionWithJsonNumericProperty(
    Model& gltf,
    ClassProperty& classProperty,
    const FeatureTable& featureTable,
    FeatureTableProperty& featureTableProperty,
    const rapidjson::Value& propertyValue,
    const std::string& typeName) {
  assert(propertyValue.Size() >= featureTable.count);

  classProperty.type = typeName;

  // Create a new buffer for this property.
  const size_t bufferIndex = gltf.buffers.size();
  Buffer& buffer = gltf.buffers.emplace_back();
  buffer.byteLength =
      static_cast<int64_t>(sizeof(T) * static_cast<size_t>(featureTable.count));
  buffer.cesium.data.resize(static_cast<size_t>(buffer.byteLength));

  const size_t bufferViewIndex = gltf.bufferViews.size();
  BufferView& bufferView = gltf.bufferViews.emplace_back();
  bufferView.buffer = int32_t(bufferIndex);
  bufferView.byteOffset = 0;
  bufferView.byteLength = buffer.byteLength;

  featureTableProperty.bufferView = int32_t(bufferViewIndex);

  T* p = reinterpret_cast<T*>(buffer.cesium.data.data());
  auto it = propertyValue.Begin();
  for (int64_t i = 0; i < featureTable.count; ++i) {
    *p = static_cast<T>(it->Get<TRapidJson>());
    ++p;
    ++it;
  }
}

void updateExtensionWithJsonBoolProperty(
    Model& gltf,
    ClassProperty& classProperty,
    const FeatureTable& featureTable,
    FeatureTableProperty& featureTableProperty,
    const rapidjson::Value& propertyValue) {
  assert(propertyValue.Size() >= featureTable.count);

  std::vector<std::byte> data(static_cast<size_t>(
      glm::ceil(static_cast<double>(featureTable.count) / 8.0)));
  const auto& jsonArray = propertyValue.GetArray();
  for (rapidjson::SizeType i = 0;
       i < static_cast<rapidjson::SizeType>(featureTable.count);
       ++i) {
    const bool value = jsonArray[i].GetBool();
    const size_t byteIndex = i / 8;
    const size_t bitIndex = i % 8;
    data[byteIndex] =
        static_cast<std::byte>(value << bitIndex) | data[byteIndex];
  }

  const size_t bufferIndex = gltf.buffers.size();
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
    const FeatureTable& featureTable,
    const rapidjson::Value& propertyValue) {
  valueBuffer.resize(sizeof(ValueType) * numOfElements);
  offsetBuffer.resize(
      sizeof(OffsetType) * static_cast<size_t>(featureTable.count + 1));
  ValueType* value = reinterpret_cast<ValueType*>(valueBuffer.data());
  OffsetType* offsetValue = reinterpret_cast<OffsetType*>(offsetBuffer.data());
  OffsetType prevOffset = 0;
  const auto& jsonOuterArray = propertyValue.GetArray();
  for (int64_t i = 0; i < featureTable.count; ++i) {
    const auto& jsonArrayMember =
        jsonOuterArray[static_cast<rapidjson::SizeType>(i)];
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
    const FeatureTable& featureTable,
    const CompatibleTypes& compatibleTypes,
    const rapidjson::Value& propertyValue) {
  assert(propertyValue.Size() >= featureTable.count);
  const auto& jsonOuterArray = propertyValue.GetArray();

  // check if it's a fixed array
  if (compatibleTypes.minComponentCount == compatibleTypes.maxComponentCount) {
    const size_t numOfValues = static_cast<size_t>(featureTable.count) *
                               *compatibleTypes.minComponentCount;
    std::vector<std::byte> valueBuffer(sizeof(ValueType) * numOfValues);
    ValueType* value = reinterpret_cast<ValueType*>(valueBuffer.data());
    for (int64_t i = 0; i < featureTable.count; ++i) {
      const auto& jsonArrayMember =
          jsonOuterArray[static_cast<rapidjson::SizeType>(i)];
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
  for (int64_t i = 0; i < featureTable.count; ++i) {
    const auto& jsonArrayMember =
        jsonOuterArray[static_cast<rapidjson::SizeType>(i)];
    numOfElements += jsonArrayMember.Size();
  }

  PropertyType offsetType = PropertyType::None;
  std::vector<std::byte> valueBuffer;
  std::vector<std::byte> offsetBuffer;
  const uint64_t maxOffsetValue = numOfElements * sizeof(ValueType);
  if (isInRangeForUnsignedInteger<uint8_t>(maxOffsetValue)) {
    copyNumericDynamicArrayBuffers<TRapidjson, ValueType, uint8_t>(
        valueBuffer,
        offsetBuffer,
        numOfElements,
        featureTable,
        propertyValue);
    offsetType = PropertyType::Uint8;
  } else if (isInRangeForUnsignedInteger<uint16_t>(maxOffsetValue)) {
    copyNumericDynamicArrayBuffers<TRapidjson, ValueType, uint16_t>(
        valueBuffer,
        offsetBuffer,
        numOfElements,
        featureTable,
        propertyValue);
    offsetType = PropertyType::Uint16;
  } else if (isInRangeForUnsignedInteger<uint32_t>(maxOffsetValue)) {
    copyNumericDynamicArrayBuffers<TRapidjson, ValueType, uint32_t>(
        valueBuffer,
        offsetBuffer,
        numOfElements,
        featureTable,
        propertyValue);
    offsetType = PropertyType::Uint32;
  } else if (isInRangeForUnsignedInteger<uint64_t>(maxOffsetValue)) {
    copyNumericDynamicArrayBuffers<TRapidjson, ValueType, uint64_t>(
        valueBuffer,
        offsetBuffer,
        numOfElements,
        featureTable,
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
  const int32_t valueBufferIdx =
      static_cast<int32_t>(gltf.bufferViews.size() - 1);

  Buffer& gltfOffsetBuffer = gltf.buffers.emplace_back();
  gltfOffsetBuffer.byteLength = static_cast<int64_t>(offsetBuffer.size());
  gltfOffsetBuffer.cesium.data = std::move(offsetBuffer);

  BufferView& gltfOffsetBufferView = gltf.bufferViews.emplace_back();
  gltfOffsetBufferView.buffer = static_cast<int32_t>(gltf.buffers.size() - 1);
  gltfOffsetBufferView.byteOffset = 0;
  gltfOffsetBufferView.byteLength =
      static_cast<int64_t>(gltfOffsetBuffer.cesium.data.size());
  const int32_t offsetBufferIdx =
      static_cast<int32_t>(gltf.bufferViews.size() - 1);

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
    const FeatureTable& featureTable,
    const rapidjson::Value& propertyValue) {
  valueBuffer.resize(totalByteLength);
  offsetBuffer.resize((numOfString + 1) * sizeof(OffsetType));
  OffsetType offset = 0;
  size_t offsetIndex = 0;
  const auto& jsonOuterArray = propertyValue.GetArray();
  for (int64_t i = 0; i < featureTable.count; ++i) {
    const auto& arrayMember =
        jsonOuterArray[static_cast<rapidjson::SizeType>(i)];
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
    const FeatureTable& featureTable,
    const rapidjson::Value& propertyValue) {
  OffsetType prevOffset = 0;
  offsetBuffer.resize(
      static_cast<size_t>(featureTable.count + 1) * sizeof(OffsetType));
  OffsetType* offset = reinterpret_cast<OffsetType*>(offsetBuffer.data());
  const auto& jsonOuterArray = propertyValue.GetArray();
  for (int64_t i = 0; i < featureTable.count; ++i) {
    const auto& arrayMember =
        jsonOuterArray[static_cast<rapidjson::SizeType>(i)];
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
    const FeatureTable& featureTable,
    const CompatibleTypes& compatibleTypes,
    const rapidjson::Value& propertyValue) {
  assert(propertyValue.Size() >= featureTable.count);

  size_t numOfString = 0;
  size_t totalChars = 0;
  const auto& jsonOuterArray = propertyValue.GetArray();
  for (int64_t i = 0; i < featureTable.count; ++i) {
    const auto& arrayMember =
        jsonOuterArray[static_cast<rapidjson::SizeType>(i)];
    numOfString += arrayMember.Size();
    for (const auto& str : arrayMember.GetArray()) {
      totalChars += str.GetStringLength();
    }
  }

  const uint64_t totalByteLength = totalChars * sizeof(rapidjson::Value::Ch);
  std::vector<std::byte> valueBuffer;
  std::vector<std::byte> offsetBuffer;
  PropertyType offsetType = PropertyType::None;
  if (isInRangeForUnsignedInteger<uint8_t>(totalByteLength)) {
    copyStringArrayBuffers<uint8_t>(
        valueBuffer,
        offsetBuffer,
        totalByteLength,
        numOfString,
        featureTable,
        propertyValue);
    offsetType = PropertyType::Uint8;
  } else if (isInRangeForUnsignedInteger<uint16_t>(totalByteLength)) {
    copyStringArrayBuffers<uint16_t>(
        valueBuffer,
        offsetBuffer,
        totalByteLength,
        numOfString,
        featureTable,
        propertyValue);
    offsetType = PropertyType::Uint16;
  } else if (isInRangeForUnsignedInteger<uint32_t>(totalByteLength)) {
    copyStringArrayBuffers<uint32_t>(
        valueBuffer,
        offsetBuffer,
        totalByteLength,
        numOfString,
        featureTable,
        propertyValue);
    offsetType = PropertyType::Uint32;
  } else if (isInRangeForUnsignedInteger<uint64_t>(totalByteLength)) {
    copyStringArrayBuffers<uint64_t>(
        valueBuffer,
        offsetBuffer,
        totalByteLength,
        numOfString,
        featureTable,
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
  const int32_t valueBufferViewIdx =
      static_cast<int32_t>(gltf.bufferViews.size() - 1);

  // create gltf string offset buffer view
  Buffer& gltfOffsetBuffer = gltf.buffers.emplace_back();
  gltfOffsetBuffer.byteLength = static_cast<int64_t>(offsetBuffer.size());
  gltfOffsetBuffer.cesium.data = std::move(offsetBuffer);

  BufferView& gltfOffsetBufferView = gltf.bufferViews.emplace_back();
  gltfOffsetBufferView.buffer = static_cast<int32_t>(gltf.buffers.size() - 1);
  gltfOffsetBufferView.byteOffset = 0;
  gltfOffsetBufferView.byteLength = gltfOffsetBuffer.byteLength;
  const int32_t offsetBufferViewIdx =
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
  std::vector<std::byte> arrayOffsetBuffer;
  switch (offsetType) {
  case PropertyType::Uint8:
    copyArrayOffsetBufferForStringArrayProperty<uint8_t>(
        arrayOffsetBuffer,
        featureTable,
        propertyValue);
    break;
  case PropertyType::Uint16:
    copyArrayOffsetBufferForStringArrayProperty<uint16_t>(
        arrayOffsetBuffer,
        featureTable,
        propertyValue);
    break;
  case PropertyType::Uint32:
    copyArrayOffsetBufferForStringArrayProperty<uint32_t>(
        arrayOffsetBuffer,
        featureTable,
        propertyValue);
    break;
  case PropertyType::Uint64:
    copyArrayOffsetBufferForStringArrayProperty<uint64_t>(
        arrayOffsetBuffer,
        featureTable,
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
  const int32_t arrayOffsetBufferViewIdx =
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
    const FeatureTable& featureTable,
    const rapidjson::Value& propertyValue) {
  size_t currentIndex = 0;
  const size_t totalByteLength =
      static_cast<size_t>(glm::ceil(static_cast<double>(numOfElements) / 8.0));
  valueBuffer.resize(totalByteLength);
  offsetBuffer.resize(
      static_cast<size_t>(featureTable.count + 1) * sizeof(OffsetType));
  OffsetType* offset = reinterpret_cast<OffsetType*>(offsetBuffer.data());
  OffsetType prevOffset = 0;
  const auto& jsonOuterArray = propertyValue.GetArray();
  for (int64_t i = 0; i < featureTable.count; ++i) {
    const auto& arrayMember =
        jsonOuterArray[static_cast<rapidjson::SizeType>(i)];

    *offset = prevOffset;
    ++offset;
    prevOffset = static_cast<OffsetType>(prevOffset + arrayMember.Size());
    for (const auto& data : arrayMember.GetArray()) {
      const bool value = data.GetBool();
      const size_t byteIndex = currentIndex / 8;
      const size_t bitIndex = currentIndex % 8;
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
    const FeatureTable& featureTable,
    const CompatibleTypes& compatibleTypes,
    const rapidjson::Value& propertyValue) {
  assert(propertyValue.Size() >= featureTable.count);

  // fixed array of boolean
  if (compatibleTypes.minComponentCount == compatibleTypes.maxComponentCount) {
    const size_t numOfElements = static_cast<size_t>(
        featureTable.count * compatibleTypes.minComponentCount.value());
    const size_t totalByteLength = static_cast<size_t>(
        glm::ceil(static_cast<double>(numOfElements) / 8.0));
    std::vector<std::byte> valueBuffer(totalByteLength);
    size_t currentIndex = 0;
    const auto& jsonOuterArray = propertyValue.GetArray();
    for (int64_t i = 0; i < featureTable.count; ++i) {
      const auto& arrayMember =
          jsonOuterArray[static_cast<rapidjson::SizeType>(i)];
      for (const auto& data : arrayMember.GetArray()) {
        const bool value = data.GetBool();
        const size_t byteIndex = currentIndex / 8;
        const size_t bitIndex = currentIndex % 8;
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
  const auto& jsonOuterArray = propertyValue.GetArray();
  for (int64_t i = 0; i < featureTable.count; ++i) {
    const auto& arrayMember =
        jsonOuterArray[static_cast<rapidjson::SizeType>(i)];
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
        featureTable,
        propertyValue);
    offsetType = PropertyType::Uint8;
  } else if (isInRangeForUnsignedInteger<uint16_t>(numOfElements)) {
    copyBooleanArrayBuffers<uint16_t>(
        valueBuffer,
        offsetBuffer,
        numOfElements,
        featureTable,
        propertyValue);
    offsetType = PropertyType::Uint16;
  } else if (isInRangeForUnsignedInteger<uint32_t>(numOfElements)) {
    copyBooleanArrayBuffers<uint32_t>(
        valueBuffer,
        offsetBuffer,
        numOfElements,
        featureTable,
        propertyValue);
    offsetType = PropertyType::Uint32;
  } else {
    copyBooleanArrayBuffers<uint64_t>(
        valueBuffer,
        offsetBuffer,
        numOfElements,
        featureTable,
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
  const int32_t valueBufferIdx =
      static_cast<int32_t>(gltf.bufferViews.size() - 1);

  Buffer& gltfOffsetBuffer = gltf.buffers.emplace_back();
  gltfOffsetBuffer.byteLength = static_cast<int64_t>(offsetBuffer.size());
  gltfOffsetBuffer.cesium.data = std::move(offsetBuffer);

  BufferView& gltfOffsetBufferView = gltf.bufferViews.emplace_back();
  gltfOffsetBufferView.buffer = static_cast<int32_t>(gltf.buffers.size() - 1);
  gltfOffsetBufferView.byteOffset = 0;
  gltfOffsetBufferView.byteLength =
      static_cast<int64_t>(gltfOffsetBuffer.cesium.data.size());
  const int32_t offsetBufferIdx =
      static_cast<int32_t>(gltf.bufferViews.size() - 1);

  classProperty.type = "ARRAY";
  classProperty.componentType = "BOOLEAN";

  featureTableProperty.bufferView = valueBufferIdx;
  featureTableProperty.arrayOffsetBufferView = offsetBufferIdx;
  featureTableProperty.offsetType = convertPropertyTypeToString(offsetType);
}

void updateExtensionWithArrayProperty(
    Model& gltf,
    ClassProperty& classProperty,
    const FeatureTable& featureTable,
    FeatureTableProperty& featureTableProperty,
    const CompatibleTypes& compatibleTypes,
    const rapidjson::Value& propertyValue) {
  assert(propertyValue.Size() >= featureTable.count);

  if (compatibleTypes.componentType->isBool) {
    updateBooleanArrayProperty(
        gltf,
        classProperty,
        featureTableProperty,
        featureTable,
        compatibleTypes,
        propertyValue);
  } else if (compatibleTypes.componentType->isInt8) {
    updateNumericArrayProperty<int32_t, int8_t>(
        gltf,
        classProperty,
        featureTableProperty,
        featureTable,
        compatibleTypes,
        propertyValue);
  } else if (compatibleTypes.componentType->isUint8) {
    updateNumericArrayProperty<uint32_t, uint8_t>(
        gltf,
        classProperty,
        featureTableProperty,
        featureTable,
        compatibleTypes,
        propertyValue);
  } else if (compatibleTypes.componentType->isInt16) {
    updateNumericArrayProperty<int32_t, int16_t>(
        gltf,
        classProperty,
        featureTableProperty,
        featureTable,
        compatibleTypes,
        propertyValue);
  } else if (compatibleTypes.componentType->isUint16) {
    updateNumericArrayProperty<uint32_t, uint16_t>(
        gltf,
        classProperty,
        featureTableProperty,
        featureTable,
        compatibleTypes,
        propertyValue);
  } else if (compatibleTypes.componentType->isInt32) {
    updateNumericArrayProperty<int32_t, int32_t>(
        gltf,
        classProperty,
        featureTableProperty,
        featureTable,
        compatibleTypes,
        propertyValue);
  } else if (compatibleTypes.componentType->isUint32) {
    updateNumericArrayProperty<uint32_t, uint32_t>(
        gltf,
        classProperty,
        featureTableProperty,
        featureTable,
        compatibleTypes,
        propertyValue);
  } else if (compatibleTypes.componentType->isInt64) {
    updateNumericArrayProperty<int64_t, int64_t>(
        gltf,
        classProperty,
        featureTableProperty,
        featureTable,
        compatibleTypes,
        propertyValue);
  } else if (compatibleTypes.componentType->isUint64) {
    updateNumericArrayProperty<uint64_t, uint64_t>(
        gltf,
        classProperty,
        featureTableProperty,
        featureTable,
        compatibleTypes,
        propertyValue);
  } else if (compatibleTypes.componentType->isFloat32) {
    updateNumericArrayProperty<float, float>(
        gltf,
        classProperty,
        featureTableProperty,
        featureTable,
        compatibleTypes,
        propertyValue);
  } else if (compatibleTypes.componentType->isFloat64) {
    updateNumericArrayProperty<double, double>(
        gltf,
        classProperty,
        featureTableProperty,
        featureTable,
        compatibleTypes,
        propertyValue);
  } else {
    updateStringArrayProperty(
        gltf,
        classProperty,
        featureTableProperty,
        featureTable,
        compatibleTypes,
        propertyValue);
  }
}

void updateExtensionWithJsonProperty(
    Model& gltf,
    ClassProperty& classProperty,
    const FeatureTable& featureTable,
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
  const CompatibleTypes compatibleTypes = findCompatibleTypes(propertyValue);
  if (compatibleTypes.type.isBool) {
    updateExtensionWithJsonBoolProperty(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue);
  } else if (compatibleTypes.type.isInt8) {
    updateExtensionWithJsonNumericProperty<int8_t, int32_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "INT8");
  } else if (compatibleTypes.type.isUint8) {
    updateExtensionWithJsonNumericProperty<uint8_t, uint32_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "UINT8");
  } else if (compatibleTypes.type.isInt16) {
    updateExtensionWithJsonNumericProperty<int16_t, int32_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "INT16");
  } else if (compatibleTypes.type.isUint16) {
    updateExtensionWithJsonNumericProperty<uint16_t, uint32_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "UINT16");
  } else if (compatibleTypes.type.isInt32) {
    updateExtensionWithJsonNumericProperty<int32_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "INT32");
  } else if (compatibleTypes.type.isUint32) {
    updateExtensionWithJsonNumericProperty<uint32_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "UINT32");
  } else if (compatibleTypes.type.isInt64) {
    updateExtensionWithJsonNumericProperty<int64_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "INT64");
  } else if (compatibleTypes.type.isUint64) {
    updateExtensionWithJsonNumericProperty<uint64_t>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "UINT64");
  } else if (compatibleTypes.type.isFloat32) {
    updateExtensionWithJsonNumericProperty<float>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "FLOAT32");
  } else if (compatibleTypes.type.isFloat64) {
    updateExtensionWithJsonNumericProperty<double>(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue,
        "FLOAT64");
  } else if (compatibleTypes.type.isArray) {
    updateExtensionWithArrayProperty(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        compatibleTypes,
        propertyValue);
  } else {
    updateExtensionWithJsonStringProperty(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        propertyValue);
  }
}

void updateExtensionWithBinaryProperty(
    Model& gltf,
    int32_t gltfBufferIndex,
    int64_t gltfBufferOffset,
    BinaryProperty& binaryProperty,
    ClassProperty& classProperty,
    const FeatureTable& featureTable,
    FeatureTableProperty& featureTableProperty,
    const std::string& propertyName,
    const rapidjson::Value& propertyValue,
    const std::shared_ptr<spdlog::logger>& pLogger) {
  assert(
      gltfBufferIndex >= 0 &&
      "gltfBufferIndex is negative. Need to allocate buffer before "
      "convert the binary property");

  const auto& byteOffsetIt = propertyValue.FindMember("byteOffset");
  if (byteOffsetIt == propertyValue.MemberEnd() ||
      !byteOffsetIt->value.IsInt64()) {
    SPDLOG_LOGGER_WARN(
        pLogger,
        "Skip convert {}. The binary property doesn't have required "
        "byteOffset.",
        propertyName);
    return;
  }

  const auto& componentTypeIt = propertyValue.FindMember("componentType");
  if (componentTypeIt == propertyValue.MemberEnd() ||
      !componentTypeIt->value.IsString()) {
    SPDLOG_LOGGER_WARN(
        pLogger,
        "Skip convert {}. The binary property doesn't have required "
        "componentType.",
        propertyName);
    return;
  }

  const auto& typeIt = propertyValue.FindMember("type");
  if (typeIt == propertyValue.MemberEnd() || !typeIt->value.IsString()) {
    SPDLOG_LOGGER_WARN(
        pLogger,
        "Skip convert {}. The binary property doesn't have required type.",
        propertyName);
    return;
  }

  // convert class property
  const int64_t byteOffset = byteOffsetIt->value.GetInt64();
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
  const size_t typeSize = gltfType.typeSize;
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

namespace Cesium3DTilesSelection {

void upgradeBatchTableToFeatureMetadata(
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumGltf::Model& gltf,
    const rapidjson::Document& featureTableJson,
    const rapidjson::Document& batchTableJson,
    const gsl::span<const std::byte>& batchTableBinaryData) {

  CESIUM_TRACE("upgradeBatchTableToFeatureMetadata");

  // Check to make sure a char of rapidjson is 1 byte
  static_assert(
      sizeof(rapidjson::Value::Ch) == 1,
      "RapidJson::Value::Ch is not 1 byte");

  // Parse the b3dm batch table and convert it to the EXT_feature_metadata
  // extension.

  // If the feature table is missing the BATCH_LENGTH semantic, ignore the batch
  // table completely.
  const auto batchLengthIt = featureTableJson.FindMember("BATCH_LENGTH");
  if (batchLengthIt == featureTableJson.MemberEnd() ||
      !batchLengthIt->value.IsInt64()) {
    SPDLOG_LOGGER_WARN(
        pLogger,
        "The B3DM has a batch table, but it is being ignored because there is "
        "no BATCH_LENGTH semantic in the feature table or it is not an "
        "integer.");
    return;
  }

  const int64_t batchLength = batchLengthIt->value.GetInt64();

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

    // Don't interpret extensions or extras as a property.
    if (name == "extensions" || name == "extras") {
      continue;
    }

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
          name,
          propertyValue,
          pLogger);
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

} // namespace Cesium3DTilesSelection
