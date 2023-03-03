#include "BatchTableToGltfFeatureMetadata.h"

#include "BatchTableHierarchyPropertyValues.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"

#include <CesiumGltf/ExtensionMeshPrimitiveExtFeatureMetadata.h>
#include <CesiumGltf/ExtensionModelExtFeatureMetadata.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/PropertyType.h>
#include <CesiumGltf/PropertyTypeTraits.h>

#include <glm/glm.hpp>
#include <rapidjson/writer.h>
#include <spdlog/fmt/fmt.h>

#include <map>
#include <type_traits>
#include <unordered_set>

using namespace CesiumGltf;
using namespace Cesium3DTilesSelection::CesiumImpl;

namespace Cesium3DTilesSelection {
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
  int64_t batchTableByteOffset;
  int64_t gltfByteOffset;
  int64_t byteLength;
};

struct GltfFeatureTableType {
  std::string typeName;
  size_t typeSize;
};

const std::map<std::string, GltfFeatureTableType>
    batchTableComponentTypeToGltfType = {
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

class ArrayOfPropertyValues {
public:
  class const_iterator {
  public:
    const_iterator(const rapidjson::Value* p) : _p(p) {}

    const_iterator& operator++() {
      ++this->_p;
      return *this;
    }

    bool operator==(const const_iterator& rhs) const {
      return this->_p == rhs._p;
    }

    bool operator!=(const const_iterator& rhs) const {
      return this->_p != rhs._p;
    }

    const rapidjson::Value& operator*() const { return *this->_p; }

    const rapidjson::Value* operator->() const { return this->_p; }

  private:
    const rapidjson::Value* _p;
  };

  ArrayOfPropertyValues(const rapidjson::Value& propertyValues)
      : _propertyValues(propertyValues) {}

  const_iterator begin() const {
    return const_iterator(this->_propertyValues.Begin());
  }

  const_iterator end() const {
    return const_iterator(this->_propertyValues.End());
  }

  int64_t size() const { return this->_propertyValues.Size(); }

private:
  const rapidjson::Value& _propertyValues;
};

template <typename TValueGetter>
CompatibleTypes findCompatibleTypes(const TValueGetter& propertyValue) {
  MaskedType type;
  std::optional<MaskedType> componentType;
  std::optional<uint32_t> minComponentCount;
  std::optional<uint32_t> maxComponentCount;
  for (auto it = propertyValue.begin(); it != propertyValue.end(); ++it) {
    if (it->IsBool()) {
      // Should we allow conversion of bools to numeric 0 or 1? Nah.
      type.isInt8 = type.isUint8 = false;
      type.isInt16 = type.isUint16 = false;
      type.isInt32 = type.isUint32 = false;
      type.isInt64 = type.isUint64 = false;
      type.isFloat32 = false;
      type.isFloat64 = false;
      type.isBool &= true;
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
      type.isUint64 &= true;
      type.isFloat32 = false;
      type.isFloat64 = false;
      type.isBool = false;
      type.isArray = false;
    } else if (it->IsLosslessFloat()) {
      type.isInt8 = type.isUint8 = false;
      type.isInt16 = type.isUint16 = false;
      type.isInt32 = type.isUint32 = false;
      type.isInt64 = type.isUint64 = false;
      type.isFloat32 &= true;
      type.isFloat64 &= true;
      type.isBool = false;
      type.isArray = false;
    } else if (it->IsDouble()) {
      type.isInt8 = type.isUint8 = false;
      type.isInt16 = type.isUint16 = false;
      type.isInt32 = type.isUint32 = false;
      type.isInt64 = type.isUint64 = false;
      type.isFloat32 = false;
      type.isFloat64 &= true;
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
      CompatibleTypes currentComponentType =
          findCompatibleTypes(ArrayOfPropertyValues(*it));
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

template <typename TValueGetter>
void updateExtensionWithJsonStringProperty(
    Model& gltf,
    ClassProperty& classProperty,
    const FeatureTable& featureTable,
    FeatureTableProperty& featureTableProperty,
    const TValueGetter& propertyValue) {

  rapidjson::StringBuffer rapidjsonStrBuffer;
  std::vector<uint64_t> rapidjsonOffsets;
  rapidjsonOffsets.reserve(static_cast<size_t>(featureTable.count + 1));
  rapidjsonOffsets.emplace_back(0);

  auto it = propertyValue.begin();
  for (int64_t i = 0; i < featureTable.count; ++i) {
    if (it == propertyValue.end()) {
      rapidjsonOffsets.emplace_back(rapidjsonStrBuffer.GetLength());
      continue;
    }
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

template <typename T, typename TRapidJson = T, typename TValueGetter>
void updateExtensionWithJsonNumericProperty(
    Model& gltf,
    ClassProperty& classProperty,
    const FeatureTable& featureTable,
    FeatureTableProperty& featureTableProperty,
    const TValueGetter& propertyValue,
    const std::string& typeName) {
  assert(propertyValue.size() >= featureTable.count);

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
  auto it = propertyValue.begin();
  for (int64_t i = 0; i < featureTable.count; ++i) {
    *p = static_cast<T>(it->template Get<TRapidJson>());
    ++p;
    ++it;
  }
}

template <typename TValueGetter>
void updateExtensionWithJsonBoolProperty(
    Model& gltf,
    ClassProperty& classProperty,
    const FeatureTable& featureTable,
    FeatureTableProperty& featureTableProperty,
    const TValueGetter& propertyValue) {
  assert(propertyValue.size() >= featureTable.count);

  std::vector<std::byte> data(static_cast<size_t>(
      glm::ceil(static_cast<double>(featureTable.count) / 8.0)));
  auto it = propertyValue.begin();
  for (rapidjson::SizeType i = 0;
       i < static_cast<rapidjson::SizeType>(featureTable.count);
       ++i) {
    const bool value = it->GetBool();
    const size_t byteIndex = i / 8;
    const size_t bitIndex = i % 8;
    data[byteIndex] =
        static_cast<std::byte>(value << bitIndex) | data[byteIndex];
    ++it;
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

template <
    typename TRapidjson,
    typename ValueType,
    typename OffsetType,
    typename TValueGetter>
void copyNumericDynamicArrayBuffers(
    std::vector<std::byte>& valueBuffer,
    std::vector<std::byte>& offsetBuffer,
    size_t numOfElements,
    const FeatureTable& featureTable,
    const TValueGetter& propertyValue) {
  valueBuffer.resize(sizeof(ValueType) * numOfElements);
  offsetBuffer.resize(
      sizeof(OffsetType) * static_cast<size_t>(featureTable.count + 1));
  ValueType* value = reinterpret_cast<ValueType*>(valueBuffer.data());
  OffsetType* offsetValue = reinterpret_cast<OffsetType*>(offsetBuffer.data());
  OffsetType prevOffset = 0;
  auto it = propertyValue.begin();
  for (int64_t i = 0; i < featureTable.count; ++i) {
    const auto& jsonArrayMember = *it;
    *offsetValue = prevOffset;
    ++offsetValue;
    for (const auto& valueJson : jsonArrayMember.GetArray()) {
      *value = static_cast<ValueType>(valueJson.template Get<TRapidjson>());
      ++value;
    }

    prevOffset = static_cast<OffsetType>(
        prevOffset + jsonArrayMember.Size() * sizeof(ValueType));

    ++it;
  }

  *offsetValue = prevOffset;
}

template <typename TRapidjson, typename ValueType, typename TValueGetter>
void updateNumericArrayProperty(
    Model& gltf,
    ClassProperty& classProperty,
    FeatureTableProperty& featureTableProperty,
    const FeatureTable& featureTable,
    const CompatibleTypes& compatibleTypes,
    const TValueGetter& propertyValue) {
  assert(propertyValue.size() >= featureTable.count);

  // check if it's a fixed array
  if (compatibleTypes.minComponentCount == compatibleTypes.maxComponentCount) {
    const size_t numOfValues = static_cast<size_t>(featureTable.count) *
                               *compatibleTypes.minComponentCount;
    std::vector<std::byte> valueBuffer(sizeof(ValueType) * numOfValues);
    ValueType* value = reinterpret_cast<ValueType*>(valueBuffer.data());
    auto it = propertyValue.begin();
    for (int64_t i = 0; i < featureTable.count; ++i) {
      const auto& jsonArrayMember = *it;
      for (const auto& valueJson : jsonArrayMember.GetArray()) {
        *value = static_cast<ValueType>(valueJson.template Get<TRapidjson>());
        ++value;
      }
      ++it;
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
  auto it = propertyValue.begin();
  for (int64_t i = 0; i < featureTable.count; ++i) {
    const auto& jsonArrayMember = *it;
    numOfElements += jsonArrayMember.Size();
    ++it;
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

template <typename OffsetType, typename TValueGetter>
void copyStringArrayBuffers(
    std::vector<std::byte>& valueBuffer,
    std::vector<std::byte>& offsetBuffer,
    size_t totalByteLength,
    size_t numOfString,
    const FeatureTable& featureTable,
    const TValueGetter& propertyValue) {
  valueBuffer.resize(totalByteLength);
  offsetBuffer.resize((numOfString + 1) * sizeof(OffsetType));
  OffsetType offset = 0;
  size_t offsetIndex = 0;
  auto it = propertyValue.begin();
  for (int64_t i = 0; i < featureTable.count; ++i) {
    const auto& arrayMember = *it;
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
    ++it;
  }

  std::memcpy(
      offsetBuffer.data() + offsetIndex * sizeof(OffsetType),
      &offset,
      sizeof(OffsetType));
}

template <typename OffsetType, typename TValueGetter>
void copyArrayOffsetBufferForStringArrayProperty(
    std::vector<std::byte>& offsetBuffer,
    const FeatureTable& featureTable,
    const TValueGetter& propertyValue) {
  OffsetType prevOffset = 0;
  offsetBuffer.resize(
      static_cast<size_t>(featureTable.count + 1) * sizeof(OffsetType));
  OffsetType* offset = reinterpret_cast<OffsetType*>(offsetBuffer.data());
  auto it = propertyValue.begin();
  for (int64_t i = 0; i < featureTable.count; ++i) {
    const auto& arrayMember = *it;
    *offset = prevOffset;
    prevOffset = static_cast<OffsetType>(
        prevOffset + arrayMember.Size() * sizeof(OffsetType));
    ++offset;
    ++it;
  }

  *offset = prevOffset;
}

template <typename TValueGetter>
void updateStringArrayProperty(
    Model& gltf,
    ClassProperty& classProperty,
    FeatureTableProperty& featureTableProperty,
    const FeatureTable& featureTable,
    const CompatibleTypes& compatibleTypes,
    const TValueGetter& propertyValue) {
  assert(propertyValue.size() >= featureTable.count);

  size_t numOfString = 0;
  size_t totalChars = 0;
  auto it = propertyValue.begin();
  for (int64_t i = 0; i < featureTable.count; ++i) {
    const auto& arrayMember = *it;
    numOfString += arrayMember.Size();
    for (const auto& str : arrayMember.GetArray()) {
      totalChars += str.GetStringLength();
    }
    ++it;
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

template <typename OffsetType, typename TValueGetter>
void copyBooleanArrayBuffers(
    std::vector<std::byte>& valueBuffer,
    std::vector<std::byte>& offsetBuffer,
    size_t numOfElements,
    const FeatureTable& featureTable,
    const TValueGetter& propertyValue) {
  size_t currentIndex = 0;
  const size_t totalByteLength =
      static_cast<size_t>(glm::ceil(static_cast<double>(numOfElements) / 8.0));
  valueBuffer.resize(totalByteLength);
  offsetBuffer.resize(
      static_cast<size_t>(featureTable.count + 1) * sizeof(OffsetType));
  OffsetType* offset = reinterpret_cast<OffsetType*>(offsetBuffer.data());
  OffsetType prevOffset = 0;
  auto it = propertyValue.begin();
  for (int64_t i = 0; i < featureTable.count; ++i) {
    const auto& arrayMember = *it;

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

    ++it;
  }

  *offset = prevOffset;
}

template <typename TValueGetter>
void updateBooleanArrayProperty(
    Model& gltf,
    ClassProperty& classProperty,
    FeatureTableProperty& featureTableProperty,
    const FeatureTable& featureTable,
    const CompatibleTypes& compatibleTypes,
    const TValueGetter& propertyValue) {
  assert(propertyValue.size() >= featureTable.count);

  // fixed array of boolean
  if (compatibleTypes.minComponentCount == compatibleTypes.maxComponentCount) {
    const size_t numOfElements = static_cast<size_t>(
        featureTable.count * compatibleTypes.minComponentCount.value());
    const size_t totalByteLength = static_cast<size_t>(
        glm::ceil(static_cast<double>(numOfElements) / 8.0));
    std::vector<std::byte> valueBuffer(totalByteLength);
    size_t currentIndex = 0;
    auto it = propertyValue.begin();
    for (int64_t i = 0; i < featureTable.count; ++i) {
      const auto& arrayMember = *it;
      for (const auto& data : arrayMember.GetArray()) {
        const bool value = data.GetBool();
        const size_t byteIndex = currentIndex / 8;
        const size_t bitIndex = currentIndex % 8;
        valueBuffer[byteIndex] =
            static_cast<std::byte>(value << bitIndex) | valueBuffer[byteIndex];
        ++currentIndex;
      }
      ++it;
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
  auto it = propertyValue.begin();
  for (int64_t i = 0; i < featureTable.count; ++i) {
    const auto& arrayMember = *it;
    numOfElements += arrayMember.Size();
    ++it;
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

template <typename TValueGetter>
void updateExtensionWithArrayProperty(
    Model& gltf,
    ClassProperty& classProperty,
    const FeatureTable& featureTable,
    FeatureTableProperty& featureTableProperty,
    const CompatibleTypes& compatibleTypes,
    const TValueGetter& propertyValue) {
  assert(propertyValue.size() >= featureTable.count);

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

template <typename TValueGetter>
void updateExtensionWithJsonProperty(
    Model& gltf,
    ClassProperty& classProperty,
    const FeatureTable& featureTable,
    FeatureTableProperty& featureTableProperty,
    const TValueGetter& propertyValue) {

  if (propertyValue.size() == 0 || propertyValue.size() < featureTable.count) {
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
    FeatureTableProperty& featureTableProperty,
    ErrorList& result,
    const FeatureTable& featureTable,
    const std::string& propertyName,
    const rapidjson::Value& propertyValue) {
  assert(
      gltfBufferIndex >= 0 &&
      "gltfBufferIndex is negative. Need to allocate buffer before "
      "converting the binary property");

  const auto& byteOffsetIt = propertyValue.FindMember("byteOffset");
  if (byteOffsetIt == propertyValue.MemberEnd() ||
      !byteOffsetIt->value.IsInt64()) {
    result.emplaceWarning(fmt::format(
        "Skip converting {}. The binary property doesn't have required "
        "byteOffset.",
        propertyName));
    return;
  }

  const auto& componentTypeIt = propertyValue.FindMember("componentType");
  if (componentTypeIt == propertyValue.MemberEnd() ||
      !componentTypeIt->value.IsString()) {
    result.emplaceWarning(fmt::format(
        "Skip converting {}. The binary property doesn't have required "
        "componentType.",
        propertyName));
    return;
  }

  const auto& typeIt = propertyValue.FindMember("type");
  if (typeIt == propertyValue.MemberEnd() || !typeIt->value.IsString()) {
    result.emplaceWarning(fmt::format(
        "Skip convert {}. The binary property doesn't have required type.",
        propertyName));
    return;
  }

  // convert class property
  const int64_t byteOffset = byteOffsetIt->value.GetInt64();
  const std::string& componentType = componentTypeIt->value.GetString();
  const std::string& type = typeIt->value.GetString();

  auto convertedTypeIt = batchTableComponentTypeToGltfType.find(componentType);
  if (convertedTypeIt == batchTableComponentTypeToGltfType.end()) {
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

  binaryProperty.batchTableByteOffset = byteOffset;
  binaryProperty.gltfByteOffset = gltfBufferOffset;
  binaryProperty.byteLength = static_cast<int64_t>(bufferView.byteLength);
}

void updateExtensionWithBatchTableHierarchy(
    Model& gltf,
    Class& classDefinition,
    FeatureTable& featureTable,
    ErrorList& result,
    const rapidjson::Value& batchTableHierarchy) {
  // EXT_feature_metadata can't support hierarchy, so we need to flatten it.
  // It also can't support multiple classes with a single set of feature IDs.
  // So essentially every property of every class gets added to the one class
  // definition.
  auto classesIt = batchTableHierarchy.FindMember("classes");
  if (classesIt == batchTableHierarchy.MemberEnd()) {
    result.emplaceWarning(
        "3DTILES_batch_table_hierarchy does not contain required \"classes\" "
        "property.");
    return;
  }

  auto parentCountsIt = batchTableHierarchy.FindMember("parentCounts");
  if (parentCountsIt != batchTableHierarchy.MemberEnd() &&
      parentCountsIt->value.IsArray()) {
    for (const auto& element : parentCountsIt->value.GetArray()) {
      if (!element.IsInt64() || element.GetInt64() != 1LL) {
        result.emplaceWarning(
            "3DTILES_batch_table_hierarchy with a \"parentCounts\" property is "
            "not currently supported. All instances must have at most one "
            "parent.");
        return;
      }
    }
  }

  // Find all the properties.
  std::unordered_set<std::string> properties;

  for (auto classIt = classesIt->value.Begin();
       classIt != classesIt->value.End();
       ++classIt) {
    auto instancesIt = classIt->FindMember("instances");
    for (auto propertyIt = instancesIt->value.MemberBegin();
         propertyIt != instancesIt->value.MemberEnd();
         ++propertyIt) {
      if (propertyIt->value.IsObject()) {
        result.emplaceWarning(fmt::format(
            "Property {} uses binary values. Only JSON-based "
            "3DTILES_batch_table_hierarchy properties are currently supported.",
            propertyIt->name.GetString()));
      } else {
        properties.insert(propertyIt->name.GetString());
      }
    }
  }

  BatchTableHierarchyPropertyValues batchTableHierarchyValues(
      batchTableHierarchy,
      featureTable.count);

  for (const std::string& name : properties) {
    ClassProperty& classProperty =
        classDefinition.properties.emplace(name, ClassProperty()).first->second;
    classProperty.name = name;

    FeatureTableProperty& featureTableProperty =
        featureTable.properties.emplace(name, FeatureTableProperty())
            .first->second;

    batchTableHierarchyValues.setProperty(name);

    updateExtensionWithJsonProperty(
        gltf,
        classProperty,
        featureTable,
        featureTableProperty,
        batchTableHierarchyValues);
  }
}

void convertBatchTableToGltfFeatureMetadataExtension(
    const rapidjson::Document& batchTableJson,
    const gsl::span<const std::byte>& batchTableBinaryData,
    CesiumGltf::Model& gltf,
    const int64_t featureCount,
    ErrorList& result) {
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

  ExtensionModelExtFeatureMetadata& modelExtension =
      gltf.addExtension<ExtensionModelExtFeatureMetadata>();
  Schema& schema = modelExtension.schema.emplace();
  Class& classDefinition =
      schema.classes.emplace("default", Class()).first->second;

  FeatureTable& featureTable =
      modelExtension.featureTables.emplace("default", FeatureTable())
          .first->second;
  featureTable.count = featureCount;
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
          ArrayOfPropertyValues(propertyValue));
    } else {
      BinaryProperty& binaryProperty = binaryProperties.emplace_back();
      updateExtensionWithBinaryProperty(
          gltf,
          gltfBufferIndex,
          gltfBufferOffset,
          binaryProperty,
          classProperty,
          featureTableProperty,
          result,
          featureTable,
          name,
          propertyValue);
      gltfBufferOffset += roundUp(binaryProperty.byteLength, 8);
    }
  }

  // Convert 3DTILES_batch_table_hierarchy
  auto extensionsIt = batchTableJson.FindMember("extensions");
  if (extensionsIt != batchTableJson.MemberEnd()) {
    auto bthIt =
        extensionsIt->value.FindMember("3DTILES_batch_table_hierarchy");
    if (bthIt != extensionsIt->value.MemberEnd()) {
      updateExtensionWithBatchTableHierarchy(
          gltf,
          classDefinition,
          featureTable,
          result,
          bthIt->value);
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
          batchTableBinaryData.data() + binaryProperty.batchTableByteOffset,
          static_cast<size_t>(binaryProperty.byteLength));
    }
  }
}

} // namespace

ErrorList BatchTableToGltfFeatureMetadata::convertFromB3dm(
    const rapidjson::Document& featureTableJson,
    const rapidjson::Document& batchTableJson,
    const gsl::span<const std::byte>& batchTableBinaryData,
    CesiumGltf::Model& gltf) {
  // Check to make sure a char of rapidjson is 1 byte
  static_assert(
      sizeof(rapidjson::Value::Ch) == 1,
      "RapidJson::Value::Ch is not 1 byte");

  ErrorList result;

  // Parse the b3dm batch table and convert it to the EXT_feature_metadata
  // extension.

  // If the feature table is missing the BATCH_LENGTH semantic, ignore the batch
  // table completely.
  const auto batchLengthIt = featureTableJson.FindMember("BATCH_LENGTH");
  if (batchLengthIt == featureTableJson.MemberEnd() ||
      !batchLengthIt->value.IsInt64()) {
    result.emplaceWarning(
        "The B3DM has a batch table, but it is being ignored because there is "
        "no BATCH_LENGTH semantic in the feature table or it is not an "
        "integer.");
    return result;
  }

  const int64_t batchLength = batchLengthIt->value.GetInt64();

  convertBatchTableToGltfFeatureMetadataExtension(
      batchTableJson,
      batchTableBinaryData,
      gltf,
      batchLength,
      result);

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
      ExtensionMeshPrimitiveExtFeatureMetadata& extension =
          primitive.addExtension<ExtensionMeshPrimitiveExtFeatureMetadata>();
      FeatureIDAttribute& attribute =
          extension.featureIdAttributes.emplace_back();
      attribute.featureTable = "default";
      attribute.featureIds.attribute = "_FEATURE_ID_0";
    }
  }

  return result;
}

ErrorList BatchTableToGltfFeatureMetadata::convertFromPnts(
    const rapidjson::Document& featureTableJson,
    const rapidjson::Document& batchTableJson,
    const gsl::span<const std::byte>& batchTableBinaryData,
    CesiumGltf::Model& gltf) {
  // Check to make sure a char of rapidjson is 1 byte
  static_assert(
      sizeof(rapidjson::Value::Ch) == 1,
      "RapidJson::Value::Ch is not 1 byte");

  ErrorList result;

  // Parse the pnts batch table and convert it to the EXT_feature_metadata
  // extension.

  const auto pointsLengthIt = featureTableJson.FindMember("POINTS_LENGTH");
  if (pointsLengthIt == featureTableJson.MemberEnd() ||
      !pointsLengthIt->value.IsInt64()) {
    result.emplaceError("The PNTS cannot be parsed because there is no valid "
                        "POINTS_LENGTH semantic.");
    return result;
  }

  int64_t featureCount = 0;
  const auto batchLengthIt = featureTableJson.FindMember("BATCH_LENGTH");
  const auto batchIdIt = featureTableJson.FindMember("BATCH_ID");

  // If the feature table is missing the BATCH_LENGTH semantic, the batch table
  // corresponds to per-point properties.
  if (batchLengthIt != featureTableJson.MemberEnd() &&
      batchLengthIt->value.IsInt64()) {
    featureCount = batchLengthIt->value.GetInt64();
  } else if (
      batchIdIt != featureTableJson.MemberEnd() &&
      batchIdIt->value.IsObject()) {
    result.emplaceWarning(
        "The PNTS has a batch table, but it is being ignored because there "
        "is no valid BATCH_LENGTH in the feature table even though BATCH_ID is "
        "defined.");
    return result;
  } else {
    featureCount = pointsLengthIt->value.GetInt64();
  }

  convertBatchTableToGltfFeatureMetadataExtension(
      batchTableJson,
      batchTableBinaryData,
      gltf,
      featureCount,
      result);

  // Create the EXT_feature_metadata extension for the single mesh primitive.
  assert(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];

  assert(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];

  ExtensionMeshPrimitiveExtFeatureMetadata& extension =
      primitive.addExtension<ExtensionMeshPrimitiveExtFeatureMetadata>();
  FeatureIDAttribute& attribute = extension.featureIdAttributes.emplace_back();
  attribute.featureTable = "default";

  auto primitiveBatchIdIt = primitive.attributes.find("_BATCHID");
  if (primitiveBatchIdIt != primitive.attributes.end()) {
    // If _BATCHID is present, rename the _BATCHID attribute to _FEATURE_ID_0
    primitive.attributes["_FEATURE_ID_0"] = primitiveBatchIdIt->second;
    primitive.attributes.erase("_BATCHID");
    attribute.featureIds.attribute = "_FEATURE_ID_0";
  } else {
    // Otherwise, use implicit feature IDs to indicate the metadata is stored in
    // per-point properties.
    attribute.featureIds.constant = 0;
    attribute.featureIds.divisor = 1;
  }

  return result;
}
} // namespace Cesium3DTilesSelection
