#include "BatchTableToGltfStructuralMetadata.h"

#include "BatchTableHierarchyPropertyValues.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"

#include <CesiumGltf/ExtensionExtMeshFeatures.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/PropertyType.h>
#include <CesiumGltf/PropertyTypeTraits.h>

#include <glm/glm.hpp>
#include <rapidjson/writer.h>
#include <spdlog/fmt/fmt.h>

#include <limits>
#include <map>
#include <type_traits>
#include <unordered_set>

using namespace CesiumGltf;
using namespace Cesium3DTilesSelection::CesiumImpl;

namespace Cesium3DTilesSelection {
namespace {
/**
 * Indicates how a JSON value can be interpreted. Does not correspond one-to-one
 * with types / component types in EXT_structural_metadata.
 */
struct MaskedType {
  bool isInt8;
  bool isUint8;
  bool isInt16;
  bool isUint16;
  bool isInt32;
  bool isUint32;
  bool isInt64;
  bool isUint64;
  bool isFloat32;
  bool isFloat64;
  bool isBool;

  MaskedType() : MaskedType(true){};

  MaskedType(bool defaultValue)
      : isInt8(defaultValue),
        isUint8(defaultValue),
        isInt16(defaultValue),
        isUint16(defaultValue),
        isInt32(defaultValue),
        isUint32(defaultValue),
        isInt64(defaultValue),
        isUint64(defaultValue),
        isFloat32(defaultValue),
        isFloat64(defaultValue),
        isBool(defaultValue) {}

  /**
   * Merges another MaskedType into this one.
   */
  void operator&=(const MaskedType& source) {
    isInt8 &= source.isInt8;
    isUint8 &= source.isUint8;
    isInt16 &= source.isInt16;
    isUint16 &= source.isUint16;
    isInt32 &= source.isInt32;
    isUint32 &= source.isUint32;
    isInt64 &= source.isInt64;
    isUint64 &= source.isUint64;
    isFloat32 &= source.isFloat32;
    isFloat64 &= source.isFloat64;
    isBool &= source.isBool;
  }
};

/**
 * Indicates how the elements of an array JSON value can be interpreted. Does
 * not correspond one-to-one with types / component types in
 * EXT_structural_metadata.
 *
 * To avoid complications while parsing, this implementation disallows array
 * elements that are also arrays. The nested arrays will be treated as strings.
 */
struct MaskedArrayType {
  MaskedType elementType;
  uint32_t minArrayCount;
  uint32_t maxArrayCount;

  MaskedArrayType() : MaskedArrayType(true){};

  MaskedArrayType(bool defaultValue)
      : elementType(defaultValue),
        minArrayCount(std::numeric_limits<uint32_t>::max()),
        maxArrayCount(std::numeric_limits<uint32_t>::min()){};

  MaskedArrayType(
      MaskedType inElementType,
      uint32_t inMinArrayCount,
      uint32_t inMaxArrayCount)
      : elementType(inElementType),
        minArrayCount(inMinArrayCount),
        maxArrayCount(inMaxArrayCount) {}

  /**
   * Merges another MaskedArrayType into this one.
   */
  void operator&=(const MaskedArrayType& source) {
    elementType &= source.elementType;
    minArrayCount = glm::min(minArrayCount, source.minArrayCount);
    maxArrayCount = glm::max(maxArrayCount, source.maxArrayCount);
  }
};

/**
 * Indicates a batch table property's compatibility with C++ types.
 */
struct CompatibleTypes {
  // std::monostate represents "complete" compatibility, in that nothing has
  // been determined to be incompatible yet. Once something is either a
  // MaskedType or MaskedArrayType, they are considered incompatible with the
  // other type.
private:
  std::variant<std::monostate, MaskedType, MaskedArrayType> type;

public:
  CompatibleTypes() : type(){};

  CompatibleTypes(const MaskedType& maskedType) : type(maskedType){};
  CompatibleTypes(const MaskedArrayType& maskedArrayType)
      : type(maskedArrayType){};

  /**
   * Whether this is exclusively compatible with array types.
   */
  bool isExclusivelyArray() const {
    return std::holds_alternative<MaskedArrayType>(type);
  }

  /**
   * Marks as incompatible with every type. Fully-incompatible types will be
   * treated as strings.
   */
  void makeIncompatible() { type = MaskedType(false); }

  /**
   * Merges a MaskedType into this CompatibleTypes.
   */
  void operator&=(const MaskedType& inMaskedType) {
    if (std::holds_alternative<MaskedType>(type)) {
      MaskedType& maskedType = std::get<MaskedType>(type);
      maskedType &= inMaskedType;
      return;
    }

    if (std::holds_alternative<MaskedArrayType>(type)) {
      makeIncompatible();
      return;
    }

    type = inMaskedType;
  }

  /**
   * Merges a MaskedArrayType into this CompatibleTypes.
   */
  void operator&=(const MaskedArrayType& inArrayType) {
    if (std::holds_alternative<MaskedArrayType>(type)) {
      MaskedArrayType& arrayType = std::get<MaskedArrayType>(type);
      arrayType &= inArrayType;
      return;
    }

    if (std::holds_alternative<MaskedType>(type)) {
      makeIncompatible();
      return;
    }

    type = inArrayType;
  }

  /**
   * Merges another CompatibleTypes into this one.
   */
  void operator&=(const CompatibleTypes& inCompatibleTypes) {
    if (std::holds_alternative<std::monostate>(inCompatibleTypes.type)) {
      // The other CompatibleTypes is compatible with everything, so it does not
      // change this one.
      return;
    }

    if (std::holds_alternative<MaskedArrayType>(inCompatibleTypes.type)) {
      const MaskedArrayType& arrayType =
          std::get<MaskedArrayType>(inCompatibleTypes.type);
      operator&=(arrayType);
      return;
    }

    const MaskedType& maskedType = std::get<MaskedType>(inCompatibleTypes.type);
    operator&=(maskedType);
  }

  /**
   * Derives MaskedType info from this CompatibleTypes. If this CompatibleTypes
   * is only compatible with arrays, this will return an incompatible
   * MaskedType.
   */
  MaskedType toMaskedType() const {
    if (std::holds_alternative<MaskedType>(type)) {
      return std::get<MaskedType>(type);
    }

    bool isArray = std::holds_alternative<MaskedArrayType>(type);
    return MaskedType(!isArray);
  }

  /**
   * Derives MaskedArrayType info from this CompatibleTypes. If this
   * CompatibleTypes is not compatible with arrays, this will return an
   * incompatible MaskedArrayType.
   */
  MaskedArrayType toMaskedArrayType() const {
    if (std::holds_alternative<MaskedArrayType>(type)) {
      return std::get<MaskedArrayType>(type);
    }

    bool isNonArray = std::holds_alternative<MaskedType>(type);
    return MaskedArrayType(!isNonArray);
  }
};

struct BinaryProperty {
  int64_t batchTableByteOffset;
  int64_t gltfByteOffset;
  int64_t byteLength;
};

struct GltfPropertyTableType {
  std::string type;
  size_t componentCount;
};

const std::map<std::string, GltfPropertyTableType> batchTableTypeToGltfType = {
    {"SCALAR",
     GltfPropertyTableType{
         ExtensionExtStructuralMetadataClassProperty::Type::SCALAR,
         1}},
    {"VEC2",
     GltfPropertyTableType{
         ExtensionExtStructuralMetadataClassProperty::Type::VEC2,
         2}},
    {"VEC3",
     GltfPropertyTableType{
         ExtensionExtStructuralMetadataClassProperty::Type::VEC3,
         3}},
    {"VEC4",
     GltfPropertyTableType{
         ExtensionExtStructuralMetadataClassProperty::Type::VEC4,
         4}},
};

struct GltfPropertyTableComponentType {
  std::string componentType;
  size_t componentTypeSize;
};

const std::map<std::string, GltfPropertyTableComponentType>
    batchTableComponentTypeToGltfComponentType = {
        {"BYTE",
         GltfPropertyTableComponentType{
             ExtensionExtStructuralMetadataClassProperty::ComponentType::INT8,
             sizeof(int8_t)}},
        {"UNSIGNED_BYTE",
         GltfPropertyTableComponentType{
             ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT8,
             sizeof(uint8_t)}},
        {"SHORT",
         GltfPropertyTableComponentType{
             ExtensionExtStructuralMetadataClassProperty::ComponentType::INT16,
             sizeof(int16_t)}},
        {"UNSIGNED_SHORT",
         GltfPropertyTableComponentType{
             ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT16,
             sizeof(uint16_t)}},
        {"INT",
         GltfPropertyTableComponentType{
             ExtensionExtStructuralMetadataClassProperty::ComponentType::INT32,
             sizeof(int32_t)}},
        {"UNSIGNED_INT",
         GltfPropertyTableComponentType{
             ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT32,
             sizeof(uint32_t)}},
        {"FLOAT",
         GltfPropertyTableComponentType{
             ExtensionExtStructuralMetadataClassProperty::ComponentType::
                 FLOAT32,
             sizeof(float)}},
        {"DOUBLE",
         GltfPropertyTableComponentType{
             ExtensionExtStructuralMetadataClassProperty::ComponentType::
                 FLOAT64,
             sizeof(double)}},
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

template <typename TValueIter>
MaskedType getCompatibleTypesForNumber(const TValueIter& it) {
  MaskedType type(false);

  if (it->IsInt64()) {
    const int64_t value = it->GetInt64();
    type.isInt8 = isInRangeForSignedInteger<int8_t>(value);
    type.isUint8 = isInRangeForSignedInteger<uint8_t>(value);
    type.isInt16 = isInRangeForSignedInteger<int16_t>(value);
    type.isUint16 = isInRangeForSignedInteger<uint16_t>(value);
    type.isInt32 = isInRangeForSignedInteger<int32_t>(value);
    type.isUint32 = isInRangeForSignedInteger<uint32_t>(value);
    type.isInt64 = true;
    type.isUint64 = value >= 0;
    type.isFloat32 = it->IsLosslessFloat();
    type.isFloat64 = it->IsLosslessDouble();
  } else if (it->IsUint64()) {
    // Only uint64_t can represent a value that fits in a uint64_t but not in
    // an int64_t.
    type.isUint64 = true;
  } else if (it->IsLosslessFloat()) {
    type.isFloat32 = true;
    type.isFloat64 = true;
  } else if (it->IsDouble()) {
    type.isFloat64 = true;
  }

  return type;
}

template <typename TValueGetter>
CompatibleTypes findCompatibleTypes(const TValueGetter& propertyValue) {
  CompatibleTypes compatibleTypes;
  for (auto it = propertyValue.begin(); it != propertyValue.end(); ++it) {
    if (it->IsBool()) {
      // Don't allow booleans to be converted to numeric 0 or 1.
      MaskedType booleanType(false);
      booleanType.isBool = true;

      compatibleTypes &= booleanType;
    } else if (it->IsNumber()) {
      compatibleTypes &= getCompatibleTypesForNumber(it);
    } else if (it->IsArray()) {
      // Iterate over all of the elements in the array
      // and determine their compatible type.
      CompatibleTypes arrayElementCompatibleTypes =
          findCompatibleTypes(ArrayOfPropertyValues(*it));

      // If the elements inside the array are also arrays, this will return a
      // completely incompatible MaskedType, which means the elements will be
      // treated like strings.
      MaskedType elementType = arrayElementCompatibleTypes.toMaskedType();
      MaskedArrayType arrayType(elementType, it->Size(), it->Size());

      compatibleTypes &= arrayType;
    } else {
      // A string, null, or something else.
      compatibleTypes.makeIncompatible();
    }
  }

  return compatibleTypes;
}

int32_t addBufferToGltf(Model& gltf, std::vector<std::byte>&& buffer) {
  const size_t gltfBufferIndex = gltf.buffers.size();
  Buffer& gltfBuffer = gltf.buffers.emplace_back();
  gltfBuffer.byteLength = static_cast<int64_t>(buffer.size());
  gltfBuffer.cesium.data = std::move(buffer);

  const size_t bufferViewIndex = gltf.bufferViews.size();
  BufferView& bufferView = gltf.bufferViews.emplace_back();
  bufferView.buffer = static_cast<int32_t>(gltfBufferIndex);
  bufferView.byteOffset = 0;
  bufferView.byteLength = gltfBuffer.byteLength;

  return static_cast<int32_t>(bufferViewIndex);
}

template <typename TValueGetter>
void updateExtensionWithJsonStringProperty(
    Model& gltf,
    ExtensionExtStructuralMetadataClassProperty& classProperty,
    const ExtensionExtStructuralMetadataPropertyTable& propertyTable,
    ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty,
    const TValueGetter& propertyValue) {

  rapidjson::StringBuffer rapidjsonStrBuffer;
  std::vector<uint64_t> rapidjsonOffsets;
  rapidjsonOffsets.reserve(static_cast<size_t>(propertyTable.count + 1));
  rapidjsonOffsets.emplace_back(0);

  auto it = propertyValue.begin();
  for (int64_t i = 0; i < propertyTable.count; ++i) {
    if (it == propertyValue.end()) {
      rapidjsonOffsets.emplace_back(rapidjsonStrBuffer.GetLength());
      continue;
    }
    if (!it->IsString()) {
      // Everything else that is not string will be serialized by json
      rapidjson::Writer<rapidjson::StringBuffer> writer(rapidjsonStrBuffer);
      it->Accept(writer);
    } else {
      // Because serialized string json will add double quotations in the
      // buffer which is not needed by us, we will manually add the string to
      // the buffer
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
    propertyTableProperty.stringOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::StringOffsetType::
            UINT8;
  } else if (isInRangeForUnsignedInteger<uint16_t>(totalSize)) {
    copyStringBuffer<uint16_t>(
        rapidjsonStrBuffer,
        rapidjsonOffsets,
        buffer,
        offsetBuffer);
    propertyTableProperty.stringOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::StringOffsetType::
            UINT16;
  } else if (isInRangeForUnsignedInteger<uint32_t>(totalSize)) {
    copyStringBuffer<uint32_t>(
        rapidjsonStrBuffer,
        rapidjsonOffsets,
        buffer,
        offsetBuffer);
    propertyTableProperty.stringOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::StringOffsetType::
            UINT32;
  } else {
    copyStringBuffer<uint64_t>(
        rapidjsonStrBuffer,
        rapidjsonOffsets,
        buffer,
        offsetBuffer);
    propertyTableProperty.stringOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::StringOffsetType::
            UINT64;
  }

  classProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::STRING;

  propertyTableProperty.values = addBufferToGltf(gltf, std::move(buffer));
  propertyTableProperty.stringOffsets =
      addBufferToGltf(gltf, std::move(offsetBuffer));
}

template <typename T, typename TRapidJson = T, typename TValueGetter>
void updateExtensionWithJsonScalarProperty(
    Model& gltf,
    ExtensionExtStructuralMetadataClassProperty& classProperty,
    const ExtensionExtStructuralMetadataPropertyTable& propertyTable,
    ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty,
    const TValueGetter& propertyValue,
    const std::string& componentTypeName) {
  assert(propertyValue.size() >= propertyTable.count);

  classProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::SCALAR;
  classProperty.componentType = componentTypeName;

  // Create a new buffer for this property.
  const size_t byteLength =
      sizeof(T) * static_cast<size_t>(propertyTable.count);
  std::vector<std::byte> buffer(byteLength);

  T* p = reinterpret_cast<T*>(buffer.data());
  auto it = propertyValue.begin();
  for (int64_t i = 0; i < propertyTable.count; ++i, ++p, ++it) {
    *p = static_cast<T>(it->template Get<TRapidJson>());
  }

  propertyTableProperty.values = addBufferToGltf(gltf, std::move(buffer));
}

template <typename TValueGetter>
void updateExtensionWithJsonBooleanProperty(
    Model& gltf,
    ExtensionExtStructuralMetadataClassProperty& classProperty,
    const ExtensionExtStructuralMetadataPropertyTable& propertyTable,
    ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty,
    const TValueGetter& propertyValue) {
  assert(propertyValue.size() >= propertyTable.count);

  std::vector<std::byte> buffer(static_cast<size_t>(
      glm::ceil(static_cast<double>(propertyTable.count) / 8.0)));
  auto it = propertyValue.begin();
  for (rapidjson::SizeType i = 0;
       i < static_cast<rapidjson::SizeType>(propertyTable.count);
       ++i) {
    const bool value = it->GetBool();
    const size_t byteIndex = i / 8;
    const size_t bitIndex = i % 8;
    buffer[byteIndex] =
        static_cast<std::byte>(value << bitIndex) | buffer[byteIndex];
    ++it;
  }

  classProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::BOOLEAN;
  propertyTableProperty.values = addBufferToGltf(gltf, std::move(buffer));
}

template <
    typename TRapidjson,
    typename ValueType,
    typename OffsetType,
    typename TValueGetter>
void copyVariableLengthScalarArraysToBuffers(
    std::vector<std::byte>& valueBuffer,
    std::vector<std::byte>& offsetBuffer,
    size_t numOfElements,
    const ExtensionExtStructuralMetadataPropertyTable& propertyTable,
    const TValueGetter& propertyValue) {
  valueBuffer.resize(sizeof(ValueType) * numOfElements);
  offsetBuffer.resize(
      sizeof(OffsetType) * static_cast<size_t>(propertyTable.count + 1));
  ValueType* value = reinterpret_cast<ValueType*>(valueBuffer.data());
  OffsetType* offsetValue = reinterpret_cast<OffsetType*>(offsetBuffer.data());
  OffsetType prevOffset = 0;
  auto it = propertyValue.begin();
  for (int64_t i = 0; i < propertyTable.count; ++i) {
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
void updateScalarArrayProperty(
    Model& gltf,
    ExtensionExtStructuralMetadataClassProperty& classProperty,
    ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty,
    const ExtensionExtStructuralMetadataPropertyTable& propertyTable,
    const MaskedArrayType& arrayType,
    const TValueGetter& propertyValue) {
  assert(propertyValue.size() >= propertyTable.count);

  classProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::SCALAR;
  classProperty.componentType =
      convertPropertyComponentTypeToString(static_cast<PropertyComponentType>(
          TypeToPropertyType<ValueType>::component));
  classProperty.array = true;

  // Handle fixed-length arrays.
  if (arrayType.minArrayCount == arrayType.maxArrayCount) {
    const size_t arrayCount = static_cast<size_t>(arrayType.minArrayCount);
    const size_t numOfValues =
        static_cast<size_t>(propertyTable.count) * arrayCount;
    std::vector<std::byte> valueBuffer(sizeof(ValueType) * numOfValues);
    ValueType* value = reinterpret_cast<ValueType*>(valueBuffer.data());
    auto it = propertyValue.begin();
    for (int64_t i = 0; i < propertyTable.count; ++i) {
      const auto& jsonArrayMember = *it;
      for (const auto& valueJson : jsonArrayMember.GetArray()) {
        *value = static_cast<ValueType>(valueJson.template Get<TRapidjson>());
        ++value;
      }
      ++it;
    }

    classProperty.count = arrayCount;
    propertyTableProperty.values =
        addBufferToGltf(gltf, std::move(valueBuffer));

    return;
  }

  // Handle variable-length arrays.
  // Compute total size of the value buffer.
  size_t totalNumElements = 0;
  auto it = propertyValue.begin();
  for (int64_t i = 0; i < propertyTable.count; ++i) {
    const auto& jsonArrayMember = *it;
    totalNumElements += jsonArrayMember.Size();
    ++it;
  }

  PropertyComponentType offsetType = PropertyComponentType::None;
  std::vector<std::byte> valueBuffer;
  std::vector<std::byte> offsetBuffer;
  const uint64_t maxOffsetValue = totalNumElements * sizeof(ValueType);
  if (isInRangeForUnsignedInteger<uint8_t>(maxOffsetValue)) {
    copyVariableLengthScalarArraysToBuffers<TRapidjson, ValueType, uint8_t>(
        valueBuffer,
        offsetBuffer,
        totalNumElements,
        propertyTable,
        propertyValue);
    offsetType = PropertyComponentType::Uint8;
  } else if (isInRangeForUnsignedInteger<uint16_t>(maxOffsetValue)) {
    copyVariableLengthScalarArraysToBuffers<TRapidjson, ValueType, uint16_t>(
        valueBuffer,
        offsetBuffer,
        totalNumElements,
        propertyTable,
        propertyValue);
    offsetType = PropertyComponentType::Uint16;
  } else if (isInRangeForUnsignedInteger<uint32_t>(maxOffsetValue)) {
    copyVariableLengthScalarArraysToBuffers<TRapidjson, ValueType, uint32_t>(
        valueBuffer,
        offsetBuffer,
        totalNumElements,
        propertyTable,
        propertyValue);
    offsetType = PropertyComponentType::Uint32;
  } else if (isInRangeForUnsignedInteger<uint64_t>(maxOffsetValue)) {
    copyVariableLengthScalarArraysToBuffers<TRapidjson, ValueType, uint64_t>(
        valueBuffer,
        offsetBuffer,
        totalNumElements,
        propertyTable,
        propertyValue);
    offsetType = PropertyComponentType::Uint64;
  }

  propertyTableProperty.values = addBufferToGltf(gltf, std::move(valueBuffer));
  propertyTableProperty.arrayOffsets =
      addBufferToGltf(gltf, std::move(offsetBuffer));
  propertyTableProperty.arrayOffsetType =
      convertPropertyComponentTypeToString(offsetType);
}

template <typename OffsetType, typename TValueGetter>
void copyStringsToBuffers(
    std::vector<std::byte>& valueBuffer,
    std::vector<std::byte>& offsetBuffer,
    size_t totalByteLength,
    size_t numOfString,
    const ExtensionExtStructuralMetadataPropertyTable& propertyTable,
    const TValueGetter& propertyValue) {
  valueBuffer.resize(totalByteLength);
  offsetBuffer.resize((numOfString + 1) * sizeof(OffsetType));
  OffsetType offset = 0;
  size_t offsetIndex = 0;
  auto it = propertyValue.begin();
  for (int64_t i = 0; i < propertyTable.count; ++i) {
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
void copyArrayOffsetsForStringArraysToBuffer(
    std::vector<std::byte>& offsetBuffer,
    const ExtensionExtStructuralMetadataPropertyTable& propertyTable,
    const TValueGetter& propertyValue) {
  OffsetType prevOffset = 0;
  offsetBuffer.resize(
      static_cast<size_t>(propertyTable.count + 1) * sizeof(OffsetType));
  OffsetType* offset = reinterpret_cast<OffsetType*>(offsetBuffer.data());
  auto it = propertyValue.begin();
  for (int64_t i = 0; i < propertyTable.count; ++i) {
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
    ExtensionExtStructuralMetadataClassProperty& classProperty,
    ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty,
    const ExtensionExtStructuralMetadataPropertyTable& propertyTable,
    const MaskedArrayType& arrayType,
    const TValueGetter& propertyValue) {
  assert(propertyValue.size() >= propertyTable.count);

  size_t stringCount = 0;
  size_t totalCharCount = 0;
  auto it = propertyValue.begin();
  for (int64_t i = 0; i < propertyTable.count; ++i) {
    const auto& arrayMember = *it;
    stringCount += arrayMember.Size();
    for (const auto& str : arrayMember.GetArray()) {
      totalCharCount += str.GetStringLength();
    }
    ++it;
  }

  const uint64_t totalByteLength =
      totalCharCount * sizeof(rapidjson::Value::Ch);
  std::vector<std::byte> valueBuffer;
  std::vector<std::byte> stringOffsetBuffer;
  PropertyComponentType stringOffsetType = PropertyComponentType::None;
  if (isInRangeForUnsignedInteger<uint8_t>(totalByteLength)) {
    copyStringsToBuffers<uint8_t>(
        valueBuffer,
        stringOffsetBuffer,
        totalByteLength,
        stringCount,
        propertyTable,
        propertyValue);
    stringOffsetType = PropertyComponentType::Uint8;
  } else if (isInRangeForUnsignedInteger<uint16_t>(totalByteLength)) {
    copyStringsToBuffers<uint16_t>(
        valueBuffer,
        stringOffsetBuffer,
        totalByteLength,
        stringCount,
        propertyTable,
        propertyValue);
    stringOffsetType = PropertyComponentType::Uint16;
  } else if (isInRangeForUnsignedInteger<uint32_t>(totalByteLength)) {
    copyStringsToBuffers<uint32_t>(
        valueBuffer,
        stringOffsetBuffer,
        totalByteLength,
        stringCount,
        propertyTable,
        propertyValue);
    stringOffsetType = PropertyComponentType::Uint32;
  } else {
    copyStringsToBuffers<uint64_t>(
        valueBuffer,
        stringOffsetBuffer,
        totalByteLength,
        stringCount,
        propertyTable,
        propertyValue);
    stringOffsetType = PropertyComponentType::Uint64;
  }

  classProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::STRING;
  classProperty.array = true;
  propertyTableProperty.stringOffsetType =
      convertPropertyComponentTypeToString(stringOffsetType);
  propertyTableProperty.values = addBufferToGltf(gltf, std::move(valueBuffer));
  propertyTableProperty.stringOffsets =
      addBufferToGltf(gltf, std::move(stringOffsetBuffer));

  // Handle fixed-length arrays
  if (arrayType.minArrayCount == arrayType.maxArrayCount) {
    classProperty.count = arrayType.minArrayCount;
    return;
  }

  // Handle variable-length arrays.
  // For string arrays, arrayOffsets indexes into the stringOffsets buffer,
  // the size of which is the number of stringElements + 1. This determines the
  // component type of the array offsets.
  std::vector<std::byte> arrayOffsetBuffer;
  PropertyComponentType arrayOffsetType = PropertyComponentType::None;
  if (isInRangeForUnsignedInteger<uint8_t>(stringCount + 1)) {
    copyArrayOffsetsForStringArraysToBuffer<uint8_t>(
        arrayOffsetBuffer,
        propertyTable,
        propertyValue);
    arrayOffsetType = PropertyComponentType::Uint8;
  } else if (isInRangeForUnsignedInteger<uint16_t>(stringCount + 1)) {
    copyArrayOffsetsForStringArraysToBuffer<uint16_t>(
        arrayOffsetBuffer,
        propertyTable,
        propertyValue);
    arrayOffsetType = PropertyComponentType::Uint16;
  } else if (isInRangeForUnsignedInteger<uint32_t>(stringCount + 1)) {
    copyArrayOffsetsForStringArraysToBuffer<uint32_t>(
        arrayOffsetBuffer,
        propertyTable,
        propertyValue);
    arrayOffsetType = PropertyComponentType::Uint32;
  } else {
    copyArrayOffsetsForStringArraysToBuffer<uint64_t>(
        arrayOffsetBuffer,
        propertyTable,
        propertyValue);
    arrayOffsetType = PropertyComponentType::Uint64;
  }

  propertyTableProperty.arrayOffsets =
      addBufferToGltf(gltf, std::move(arrayOffsetBuffer));
  propertyTableProperty.arrayOffsetType =
      convertPropertyComponentTypeToString(arrayOffsetType);
}

template <typename OffsetType, typename TValueGetter>
void copyVariableLengthBooleanArraysToBuffers(
    std::vector<std::byte>& valueBuffer,
    std::vector<std::byte>& offsetBuffer,
    size_t numOfElements,
    const ExtensionExtStructuralMetadataPropertyTable& propertyTable,
    const TValueGetter& propertyValue) {
  size_t currentIndex = 0;
  const size_t totalByteLength =
      static_cast<size_t>(glm::ceil(static_cast<double>(numOfElements) / 8.0));
  valueBuffer.resize(totalByteLength);
  offsetBuffer.resize(
      static_cast<size_t>(propertyTable.count + 1) * sizeof(OffsetType));
  OffsetType* offset = reinterpret_cast<OffsetType*>(offsetBuffer.data());
  OffsetType prevOffset = 0;
  auto it = propertyValue.begin();
  for (int64_t i = 0; i < propertyTable.count; ++i) {
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
    ExtensionExtStructuralMetadataClassProperty& classProperty,
    ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty,
    const ExtensionExtStructuralMetadataPropertyTable& propertyTable,
    const MaskedArrayType& arrayType,
    const TValueGetter& propertyValue) {
  assert(propertyValue.size() >= propertyTable.count);

  classProperty.type = "BOOLEAN";
  classProperty.array = true;

  // Fixed-length array of booleans
  if (arrayType.minArrayCount == arrayType.maxArrayCount) {
    const size_t arrayCount = static_cast<size_t>(arrayType.minArrayCount);
    const size_t numOfElements =
        static_cast<size_t>(propertyTable.count) * arrayCount;
    const size_t totalByteLength = static_cast<size_t>(
        glm::ceil(static_cast<double>(numOfElements) / 8.0));
    std::vector<std::byte> valueBuffer(totalByteLength);
    size_t currentIndex = 0;
    auto it = propertyValue.begin();
    for (int64_t i = 0; i < propertyTable.count; ++i) {
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

    propertyTableProperty.values =
        addBufferToGltf(gltf, std::move(valueBuffer));
    classProperty.count = static_cast<int64_t>(arrayCount);

    return;
  }

  // Variable-length array of booleans
  size_t numOfElements = 0;
  auto it = propertyValue.begin();
  for (int64_t i = 0; i < propertyTable.count; ++i) {
    const auto& arrayMember = *it;
    numOfElements += arrayMember.Size();
    ++it;
  }

  std::vector<std::byte> valueBuffer;
  std::vector<std::byte> offsetBuffer;
  PropertyComponentType offsetType = PropertyComponentType::None;
  if (isInRangeForUnsignedInteger<uint8_t>(numOfElements + 1)) {
    copyVariableLengthBooleanArraysToBuffers<uint8_t>(
        valueBuffer,
        offsetBuffer,
        numOfElements,
        propertyTable,
        propertyValue);
    offsetType = PropertyComponentType::Uint8;
  } else if (isInRangeForUnsignedInteger<uint16_t>(numOfElements + 1)) {
    copyVariableLengthBooleanArraysToBuffers<uint16_t>(
        valueBuffer,
        offsetBuffer,
        numOfElements,
        propertyTable,
        propertyValue);
    offsetType = PropertyComponentType::Uint16;
  } else if (isInRangeForUnsignedInteger<uint32_t>(numOfElements + 1)) {
    copyVariableLengthBooleanArraysToBuffers<uint32_t>(
        valueBuffer,
        offsetBuffer,
        numOfElements,
        propertyTable,
        propertyValue);
    offsetType = PropertyComponentType::Uint32;
  } else {
    copyVariableLengthBooleanArraysToBuffers<uint64_t>(
        valueBuffer,
        offsetBuffer,
        numOfElements,
        propertyTable,
        propertyValue);
    offsetType = PropertyComponentType::Uint64;
  }

  propertyTableProperty.values = addBufferToGltf(gltf, std::move(valueBuffer));
  propertyTableProperty.arrayOffsets =
      addBufferToGltf(gltf, std::move(offsetBuffer));
  propertyTableProperty.arrayOffsetType =
      convertPropertyComponentTypeToString(offsetType);
}

template <typename TValueGetter>
void updateExtensionWithArrayProperty(
    Model& gltf,
    ExtensionExtStructuralMetadataClassProperty& classProperty,
    const ExtensionExtStructuralMetadataPropertyTable& propertyTable,
    ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty,
    const MaskedArrayType& arrayType,
    const TValueGetter& propertyValue) {
  assert(propertyValue.size() >= propertyTable.count);

  const MaskedType& elementType = arrayType.elementType;
  if (elementType.isBool) {
    updateBooleanArrayProperty(
        gltf,
        classProperty,
        propertyTableProperty,
        propertyTable,
        arrayType,
        propertyValue);
  } else if (elementType.isInt8) {
    updateScalarArrayProperty<int32_t, int8_t>(
        gltf,
        classProperty,
        propertyTableProperty,
        propertyTable,
        arrayType,
        propertyValue);
  } else if (elementType.isUint8) {
    updateScalarArrayProperty<uint32_t, uint8_t>(
        gltf,
        classProperty,
        propertyTableProperty,
        propertyTable,
        arrayType,
        propertyValue);
  } else if (elementType.isInt16) {
    updateScalarArrayProperty<int32_t, int16_t>(
        gltf,
        classProperty,
        propertyTableProperty,
        propertyTable,
        arrayType,
        propertyValue);
  } else if (elementType.isUint16) {
    updateScalarArrayProperty<uint32_t, uint16_t>(
        gltf,
        classProperty,
        propertyTableProperty,
        propertyTable,
        arrayType,
        propertyValue);
  } else if (elementType.isInt32) {
    updateScalarArrayProperty<int32_t, int32_t>(
        gltf,
        classProperty,
        propertyTableProperty,
        propertyTable,
        arrayType,
        propertyValue);
  } else if (elementType.isUint32) {
    updateScalarArrayProperty<uint32_t, uint32_t>(
        gltf,
        classProperty,
        propertyTableProperty,
        propertyTable,
        arrayType,
        propertyValue);
  } else if (elementType.isInt64) {
    updateScalarArrayProperty<int64_t, int64_t>(
        gltf,
        classProperty,
        propertyTableProperty,
        propertyTable,
        arrayType,
        propertyValue);
  } else if (elementType.isUint64) {
    updateScalarArrayProperty<uint64_t, uint64_t>(
        gltf,
        classProperty,
        propertyTableProperty,
        propertyTable,
        arrayType,
        propertyValue);
  } else if (elementType.isFloat32) {
    updateScalarArrayProperty<float, float>(
        gltf,
        classProperty,
        propertyTableProperty,
        propertyTable,
        arrayType,
        propertyValue);
  } else if (elementType.isFloat64) {
    updateScalarArrayProperty<double, double>(
        gltf,
        classProperty,
        propertyTableProperty,
        propertyTable,
        arrayType,
        propertyValue);
  } else {
    updateStringArrayProperty(
        gltf,
        classProperty,
        propertyTableProperty,
        propertyTable,
        arrayType,
        propertyValue);
  }
}

// Updates the extension with a property defined as an array of values in the
// batch table JSON.
template <typename TValueGetter>
void updateExtensionWithJsonProperty(
    Model& gltf,
    ExtensionExtStructuralMetadataClassProperty& classProperty,
    const ExtensionExtStructuralMetadataPropertyTable& propertyTable,
    ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty,
    const TValueGetter& propertyValue) {

  if (propertyValue.size() == 0 || propertyValue.size() < propertyTable.count) {
    // No property to infer the type from, so assume string.
    updateExtensionWithJsonStringProperty(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue);
    return;
  }

  // Figure out which types we can use for this data.
  // Use the smallest type we can, and prefer signed to unsigned.
  const CompatibleTypes compatibleTypes = findCompatibleTypes(propertyValue);
  if (compatibleTypes.isExclusivelyArray()) {
    MaskedArrayType arrayType = compatibleTypes.toMaskedArrayType();
    updateExtensionWithArrayProperty(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        arrayType,
        propertyValue);
    return;
  }

  MaskedType type = compatibleTypes.toMaskedType();
  if (type.isBool) {
    updateExtensionWithJsonBooleanProperty(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue);
  } else if (type.isInt8) {
    updateExtensionWithJsonScalarProperty<int8_t, int32_t>(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue,
        ExtensionExtStructuralMetadataClassProperty::ComponentType::INT8);
  } else if (type.isUint8) {
    updateExtensionWithJsonScalarProperty<uint8_t, uint32_t>(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue,
        ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT8);
  } else if (type.isInt16) {
    updateExtensionWithJsonScalarProperty<int16_t, int32_t>(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue,
        ExtensionExtStructuralMetadataClassProperty::ComponentType::INT16);
  } else if (type.isUint16) {
    updateExtensionWithJsonScalarProperty<uint16_t, uint32_t>(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue,
        ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT16);
  } else if (type.isInt32) {
    updateExtensionWithJsonScalarProperty<int32_t>(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue,
        ExtensionExtStructuralMetadataClassProperty::ComponentType::INT32);
  } else if (type.isUint32) {
    updateExtensionWithJsonScalarProperty<uint32_t>(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue,
        ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT32);
  } else if (type.isInt64) {
    updateExtensionWithJsonScalarProperty<int64_t>(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue,
        ExtensionExtStructuralMetadataClassProperty::ComponentType::INT64);
  } else if (type.isUint64) {
    updateExtensionWithJsonScalarProperty<uint64_t>(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue,
        ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT64);
  } else if (type.isFloat32) {
    updateExtensionWithJsonScalarProperty<float>(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue,
        ExtensionExtStructuralMetadataClassProperty::ComponentType::FLOAT32);
  } else if (type.isFloat64) {
    updateExtensionWithJsonScalarProperty<double>(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue,
        ExtensionExtStructuralMetadataClassProperty::ComponentType::FLOAT64);
  } else {
    updateExtensionWithJsonStringProperty(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue);
  }
}

void updateExtensionWithBinaryProperty(
    Model& gltf,
    int32_t gltfBufferIndex,
    int64_t gltfBufferOffset,
    BinaryProperty& binaryProperty,
    ExtensionExtStructuralMetadataClassProperty& classProperty,
    ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty,
    const ExtensionExtStructuralMetadataPropertyTable& propertyTable,
    const std::string& propertyName,
    const rapidjson::Value& propertyValue,
    ErrorList& result) {
  assert(
      gltfBufferIndex >= 0 &&
      "gltfBufferIndex is negative. Need to allocate buffer before "
      "converting the binary property");

  const auto& byteOffsetIt = propertyValue.FindMember("byteOffset");
  if (byteOffsetIt == propertyValue.MemberEnd() ||
      !byteOffsetIt->value.IsInt64()) {
    result.emplaceWarning(fmt::format(
        "Skip converting {}. The binary property doesn't have a valid "
        "byteOffset.",
        propertyName));
    return;
  }

  const auto& componentTypeIt = propertyValue.FindMember("componentType");
  if (componentTypeIt == propertyValue.MemberEnd() ||
      !componentTypeIt->value.IsString()) {
    result.emplaceWarning(fmt::format(
        "Skip converting {}. The binary property doesn't have a valid "
        "componentType.",
        propertyName));
    return;
  }

  const auto& typeIt = propertyValue.FindMember("type");
  if (typeIt == propertyValue.MemberEnd() || !typeIt->value.IsString()) {
    result.emplaceWarning(fmt::format(
        "Skip converting {}. The binary property doesn't have a valid type.",
        propertyName));
    return;
  }

  // Convert batch table property to glTF property table property
  const int64_t byteOffset = byteOffsetIt->value.GetInt64();
  const std::string& componentType = componentTypeIt->value.GetString();
  const std::string& type = typeIt->value.GetString();

  auto convertedTypeIt = batchTableTypeToGltfType.find(type);
  if (convertedTypeIt == batchTableTypeToGltfType.end()) {
    result.emplaceWarning(fmt::format(
        "Skip converting {}. The binary property doesn't have a valid type.",
        propertyName));
    return;
  }
  auto convertedComponentTypeIt =
      batchTableComponentTypeToGltfComponentType.find(componentType);
  if (convertedComponentTypeIt ==
      batchTableComponentTypeToGltfComponentType.end()) {
    result.emplaceWarning(fmt::format(
        "Skip converting {}. The binary property doesn't have a valid "
        "componentType.",
        propertyName));
    return;
  }

  const GltfPropertyTableType& gltfType = convertedTypeIt->second;
  const GltfPropertyTableComponentType& gltfComponentType =
      convertedComponentTypeIt->second;

  classProperty.type = gltfType.type;
  classProperty.componentType = gltfComponentType.componentType;

  // Convert to a buffer view
  const size_t componentCount = gltfType.componentCount;
  const size_t componentTypeSize = gltfComponentType.componentTypeSize;
  auto& bufferView = gltf.bufferViews.emplace_back();
  bufferView.buffer = gltfBufferIndex;
  bufferView.byteOffset = gltfBufferOffset;
  bufferView.byteLength = static_cast<int64_t>(
      componentTypeSize * componentCount *
      static_cast<size_t>(propertyTable.count));

  propertyTableProperty.values =
      static_cast<int32_t>(gltf.bufferViews.size() - 1);

  binaryProperty.batchTableByteOffset = byteOffset;
  binaryProperty.gltfByteOffset = gltfBufferOffset;
  binaryProperty.byteLength = static_cast<int64_t>(bufferView.byteLength);
}

void updateExtensionWithBatchTableHierarchy(
    Model& gltf,
    ExtensionExtStructuralMetadataClass& classDefinition,
    ExtensionExtStructuralMetadataPropertyTable& propertyTable,
    ErrorList& result,
    const rapidjson::Value& batchTableHierarchy) {
  // EXT_structural_metadata can't support hierarchy, so we need to flatten it.
  // It also can't support multiple classes with a single set of feature IDs,
  // because IDs can only specify one property table. So essentially every
  // property of every class gets added to the one class definition.
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
            "3DTILES_batch_table_hierarchy with a \"parentCounts\" property "
            "is not currently supported. All instances must have at most one "
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
            "3DTILES_batch_table_hierarchy properties are currently "
            "supported.",
            propertyIt->name.GetString()));
      } else {
        properties.insert(propertyIt->name.GetString());
      }
    }
  }

  BatchTableHierarchyPropertyValues batchTableHierarchyValues(
      batchTableHierarchy,
      propertyTable.count);

  for (const std::string& name : properties) {
    ExtensionExtStructuralMetadataClassProperty& classProperty =
        classDefinition.properties
            .emplace(name, ExtensionExtStructuralMetadataClassProperty())
            .first->second;
    classProperty.name = name;

    ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty =
        propertyTable.properties
            .emplace(
                name,
                ExtensionExtStructuralMetadataPropertyTableProperty())
            .first->second;

    batchTableHierarchyValues.setProperty(name);

    updateExtensionWithJsonProperty(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        batchTableHierarchyValues);
  }
}

void convertBatchTableToGltfStructuralMetadataExtension(
    const rapidjson::Document& batchTableJson,
    const gsl::span<const std::byte>& batchTableBinaryData,
    CesiumGltf::Model& gltf,
    const int64_t featureCount,
    ErrorList& result) {
  // Add the binary part of the batch table - if any - to the glTF as a
  // buffer. We will reallign this buffer later on
  int32_t gltfBufferIndex = -1;
  int64_t gltfBufferOffset = -1;
  std::vector<BinaryProperty> binaryProperties;
  if (!batchTableBinaryData.empty()) {
    gltfBufferIndex = static_cast<int32_t>(gltf.buffers.size());
    gltfBufferOffset = 0;
    gltf.buffers.emplace_back();
  }

  ExtensionModelExtStructuralMetadata& modelExtension =
      gltf.addExtension<ExtensionModelExtStructuralMetadata>();
  ExtensionExtStructuralMetadataSchema& schema =
      modelExtension.schema.emplace();
  schema.id = "default"; // Required by the spec.

  ExtensionExtStructuralMetadataClass& classDefinition =
      schema.classes.emplace("default", ExtensionExtStructuralMetadataClass())
          .first->second;

  ExtensionExtStructuralMetadataPropertyTable& propertyTable =
      modelExtension.propertyTables.emplace_back();
  propertyTable.count = featureCount;
  propertyTable.classProperty = "default";

  // Convert each regular property in the batch table
  for (auto propertyIt = batchTableJson.MemberBegin();
       propertyIt != batchTableJson.MemberEnd();
       ++propertyIt) {
    std::string name = propertyIt->name.GetString();

    // Don't interpret extensions or extras as a property.
    if (name == "extensions" || name == "extras") {
      continue;
    }

    ExtensionExtStructuralMetadataClassProperty& classProperty =
        classDefinition.properties
            .emplace(name, ExtensionExtStructuralMetadataClassProperty())
            .first->second;
    classProperty.name = name;

    ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty =
        propertyTable.properties
            .emplace(
                name,
                ExtensionExtStructuralMetadataPropertyTableProperty())
            .first->second;
    const rapidjson::Value& propertyValue = propertyIt->value;
    if (propertyValue.IsArray()) {
      updateExtensionWithJsonProperty(
          gltf,
          classProperty,
          propertyTable,
          propertyTableProperty,
          ArrayOfPropertyValues(propertyValue));
    } else {
      BinaryProperty& binaryProperty = binaryProperties.emplace_back();
      updateExtensionWithBinaryProperty(
          gltf,
          gltfBufferIndex,
          gltfBufferOffset,
          binaryProperty,
          classProperty,
          propertyTableProperty,
          propertyTable,
          name,
          propertyValue,
          result);
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
          propertyTable,
          result,
          bthIt->value);
    }
  }

  // Re-arrange binary property buffer
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

ErrorList BatchTableToGltfStructuralMetadata::convertFromB3dm(
    const rapidjson::Document& featureTableJson,
    const rapidjson::Document& batchTableJson,
    const gsl::span<const std::byte>& batchTableBinaryData,
    CesiumGltf::Model& gltf) {
  // Check to make sure a char of rapidjson is 1 byte
  static_assert(
      sizeof(rapidjson::Value::Ch) == 1,
      "RapidJson::Value::Ch is not 1 byte");

  ErrorList result;

  // Parse the b3dm batch table and convert it to the EXT_structural_metadata
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

  convertBatchTableToGltfStructuralMetadataExtension(
      batchTableJson,
      batchTableBinaryData,
      gltf,
      batchLength,
      result);

  // Create an EXT_mesh_features extension for each primitive with a _BATCHID
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

      ExtensionExtMeshFeatures& extension =
          primitive.addExtension<ExtensionExtMeshFeatures>();

      ExtensionExtMeshFeaturesFeatureId& featureID =
          extension.featureIds.emplace_back();
      // No fast way to count the unique feature IDs in this primitive, so
      // subtitute the batch table length.
      featureID.featureCount = batchLength;
      featureID.attribute = 0;
      featureID.label = "_FEATURE_ID_0";
      featureID.propertyTable = 0;
    }
  }

  return result;
}

ErrorList BatchTableToGltfStructuralMetadata::convertFromPnts(
    const rapidjson::Document& featureTableJson,
    const rapidjson::Document& batchTableJson,
    const gsl::span<const std::byte>& batchTableBinaryData,
    CesiumGltf::Model& gltf) {
  // Check to make sure a char of rapidjson is 1 byte
  static_assert(
      sizeof(rapidjson::Value::Ch) == 1,
      "RapidJson::Value::Ch is not 1 byte");

  ErrorList result;

  // Parse the pnts batch table and convert it to the EXT_structural_metadata
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

  convertBatchTableToGltfStructuralMetadataExtension(
      batchTableJson,
      batchTableBinaryData,
      gltf,
      featureCount,
      result);

  // Create the EXT_mesh_features extension for the single mesh primitive.
  assert(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];

  assert(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];

  ExtensionExtMeshFeatures& extension =
      primitive.addExtension<ExtensionExtMeshFeatures>();
  ExtensionExtMeshFeaturesFeatureId& featureID =
      extension.featureIds.emplace_back();

  // Setting the feature count is sufficient for implicit feature IDs.
  featureID.featureCount = featureCount;
  featureID.propertyTable = 0;

  auto primitiveBatchIdIt = primitive.attributes.find("_BATCHID");
  if (primitiveBatchIdIt != primitive.attributes.end()) {
    // If _BATCHID is present, rename the _BATCHID attribute to _FEATURE_ID_0
    primitive.attributes["_FEATURE_ID_0"] = primitiveBatchIdIt->second;
    primitive.attributes.erase("_BATCHID");
    featureID.attribute = 0;
    featureID.label = "_FEATURE_ID_0";
  }

  return result;
}
} // namespace Cesium3DTilesSelection
