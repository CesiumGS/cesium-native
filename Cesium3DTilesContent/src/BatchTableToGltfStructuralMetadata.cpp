#include "BatchTableToGltfStructuralMetadata.h"

#include "BatchTableHierarchyPropertyValues.h"
#include "MetadataProperty.h"

#include <Cesium3DTilesContent/GltfConverterUtility.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Class.h>
#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/ExtensionExtInstanceFeatures.h>
#include <CesiumGltf/ExtensionExtMeshFeatures.h>
#include <CesiumGltf/ExtensionExtMeshGpuInstancing.h>
#include <CesiumGltf/ExtensionKhrDracoMeshCompression.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/FeatureId.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Node.h>
#include <CesiumGltf/PropertyTable.h>
#include <CesiumGltf/PropertyTableProperty.h>
#include <CesiumGltf/PropertyType.h>
#include <CesiumGltf/PropertyTypeTraits.h>
#include <CesiumGltf/Schema.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/JsonValue.h>

#include <fmt/format.h>
#include <glm/common.hpp>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

using namespace CesiumGltf;
using namespace Cesium3DTilesContent::CesiumImpl;
using namespace CesiumUtility;

namespace Cesium3DTilesContent {
namespace {
/**
 * Indicates how a JSON value can be interpreted as a primitive type. Does not
 * correspond one-to-one with types / component types in
 * EXT_structural_metadata.
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

  /**
   * Whether this is incompatible with every primitive type. Fully-incompatible
   * types will be treated as strings.
   */
  bool isIncompatible() const noexcept {
    return !isInt8 && !isUint8 && !isInt16 && !isUint16 && !isInt32 &&
           !isUint32 && !isInt64 && !isUint64 && !isFloat32 && !isFloat64 &&
           !isBool;
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

  /**
   * Whether this is incompatible with every primitive type. Fully-incompatible
   * types will be treated as strings.
   */
  bool isIncompatible() const noexcept { return elementType.isIncompatible(); }
};

/**
 * Represents information about a batch table property, indicating its
 * compatibility with C++ types and whether it has encountered any null values.
 */
struct CompatibleTypes {
private:
  /**
   * std::monostate represents "complete" compatibility, in that nothing has
   * been determined to be incompatible yet. Once something is either a
   * MaskedType or MaskedArrayType, they are considered incompatible with the
   * other type.
   */
  std::variant<std::monostate, MaskedType, MaskedArrayType> _type;

  /**
   * Whether the property has encountered a null value. A
   * property may contain null values even though all other values are of the
   * same non-null type. In this case, it can simply replace the null with a
   * "noData" value in the EXT_structural_metadata property.
   */
  bool _hasNullValue = false;

  /**
   * The following booleans track possible "noData" (sentinel) values for the
   * property.
   *
   * We don't want to spend too much effort finding a "noData" value, because
   * with any given property there can be multiple candidates. Thus, there are
   * only a few values that are reserved as potential sentinel values:
   *
   * - 0, for signed or unsigned integers
   * - -1, for signed integers
   * - "null", for strings
   *
   * If a property does not contain one of these values, then it may be used as
   * the "noData" value in the property. The sentinel value will then be copied
   * to the buffer, instead of the null value.
   */
  bool _canUseZeroSentinel = true;
  bool _canUseNegativeOneSentinel = true;
  bool _canUseNullStringSentinel = true;

public:
  CompatibleTypes() : _type(){};
  CompatibleTypes(const MaskedType& maskedType) : _type(maskedType){};
  CompatibleTypes(const MaskedArrayType& maskedArrayType)
      : _type(maskedArrayType){};

  /**
   * Whether this is exclusively compatible with array types. This indicates an
   * exclusively array property, as opposed to a newly initialized one that is
   * "compatible" with everything.
   */
  bool isExclusivelyArray() const noexcept {
    return std::holds_alternative<MaskedArrayType>(_type);
  }

  /**
   * Whether this property is with at least one unsigned integer type. Does not
   * count arrays.
   */
  bool isCompatibleWithUnsignedInteger() const noexcept {
    if (std::holds_alternative<MaskedArrayType>(_type)) {
      return false;
    }

    if (std::holds_alternative<std::monostate>(_type)) {
      return true;
    }

    MaskedType type = std::get<MaskedType>(_type);
    return type.isUint8 || type.isUint16 || type.isUint32 || type.isUint64;
  }

  /**
   * Whether this property is compatible with at least one signed integer type.
   * Does not count arrays.
   */
  bool isCompatibleWithSignedInteger() const noexcept {
    if (std::holds_alternative<MaskedArrayType>(_type)) {
      return false;
    }

    if (std::holds_alternative<std::monostate>(_type)) {
      return true;
    }

    MaskedType type = std::get<MaskedType>(_type);
    return type.isInt8 || type.isInt16 || type.isInt32 || type.isInt64;
  }

  /**
   * Whether this property is compatible with every type. This only really
   * happens when a CompatibleTypes is initialized and never modified.
   */
  bool isFullyCompatible() const noexcept {
    return std::holds_alternative<std::monostate>(_type);
  }

  /**
   * Whether this property is incompatible with every primitive type.
   * Fully-incompatible properties will be treated as string properties.
   */
  bool isIncompatible() const noexcept {
    if (std::holds_alternative<MaskedType>(_type)) {
      return std::get<MaskedType>(_type).isIncompatible();
    }

    if (std::holds_alternative<MaskedArrayType>(_type)) {
      return std::get<MaskedArrayType>(_type).isIncompatible();
    }

    // std::monostate means compatibility with all types.
    return false;
  }

  /**
   * Marks as incompatible with every primitive type. Fully-incompatible
   * properties will be treated as string properties.
   */
  void makeIncompatible() noexcept { _type = MaskedType(false); }

  /**
   * Merges a MaskedType into this BatchTableProperty.
   */
  void operator&=(const MaskedType& inMaskedType) noexcept {
    if (std::holds_alternative<MaskedType>(_type)) {
      MaskedType& maskedType = std::get<MaskedType>(_type);
      maskedType &= inMaskedType;
      return;
    }

    if (std::holds_alternative<MaskedArrayType>(_type)) {
      makeIncompatible();
      return;
    }

    _type = inMaskedType;
  }

  /**
   * Merges a MaskedArrayType into this CompatibleTypes.
   */
  void operator&=(const MaskedArrayType& inArrayType) noexcept {
    if (std::holds_alternative<MaskedArrayType>(_type)) {
      MaskedArrayType& arrayType = std::get<MaskedArrayType>(_type);
      arrayType &= inArrayType;
      return;
    }

    if (std::holds_alternative<MaskedType>(_type)) {
      makeIncompatible();
      return;
    }

    _type = inArrayType;
  }

  /**
   * Merges another CompatibleTypes into this one.
   */
  void operator&=(const CompatibleTypes& inTypes) noexcept {
    if (std::holds_alternative<std::monostate>(inTypes._type)) {
      // The other CompatibleTypes is compatible with everything, so it does not
      // change this one.
    } else

        if (std::holds_alternative<MaskedArrayType>(inTypes._type)) {
      const MaskedArrayType& arrayType =
          std::get<MaskedArrayType>(inTypes._type);
      operator&=(arrayType);
    } else {
      const MaskedType& maskedType = std::get<MaskedType>(inTypes._type);
      operator&=(maskedType);
    }

    _hasNullValue |= inTypes._hasNullValue;
    _canUseZeroSentinel &= inTypes._canUseZeroSentinel;
    _canUseNegativeOneSentinel &= inTypes._canUseNegativeOneSentinel;
    _canUseNullStringSentinel &= inTypes._canUseNullStringSentinel;
  }

  /**
   * Derives MaskedType info from this CompatibleTypes. If this property
   * is only compatible with arrays, this will return an incompatible
   * MaskedType.
   */
  MaskedType toMaskedType() const noexcept {
    if (std::holds_alternative<MaskedType>(_type)) {
      return std::get<MaskedType>(_type);
    }

    bool isArray = std::holds_alternative<MaskedArrayType>(_type);
    return MaskedType(!isArray);
  }

  /**
   * Derives MaskedArrayType info from this CompatibleTypes. If this
   * property is not compatible with arrays, this will return an incompatible
   * MaskedArrayType.
   */
  MaskedArrayType toMaskedArrayType() const noexcept {
    if (std::holds_alternative<MaskedArrayType>(_type)) {
      return std::get<MaskedArrayType>(_type);
    }

    bool isNonArray = std::holds_alternative<MaskedType>(_type);
    return MaskedArrayType(!isNonArray);
  }

  /**
   * Gets whether the property of this type includes a null value.
   */
  bool hasNullValue() const noexcept { return _hasNullValue; }

  /**
   * Sets whether the property includes a null value. If a null value has been
   * encountered, a sentinel value may potentially be provided.
   */
  void setHasNullValue(bool value) noexcept { _hasNullValue = value; }

  /**
   * Gets a possible sentinel value for this type. If no value can be used, this
   * returns std::nullopt.
   */
  const std::optional<CesiumUtility::JsonValue>
  getSentinelValue() const noexcept {
    if (isCompatibleWithSignedInteger()) {
      if (_canUseZeroSentinel) {
        return 0;
      }

      if (_canUseNegativeOneSentinel) {
        return -1;
      }
    }

    if (isCompatibleWithUnsignedInteger()) {
      return _canUseZeroSentinel
                 ? std::make_optional<CesiumUtility::JsonValue>(0)
                 : std::nullopt;
    }

    if (isIncompatible() && _canUseNullStringSentinel) {
      return "null";
    }

    return std::nullopt;
  }

  /**
   * Removes any sentinel values that are incompatible with the property. This
   * also removes the sentinel values that equal the given value.
   *
   * This is helpful for when a property contains a sentinel value as non-null
   * data; the sentinel value can then be removed from consideration.
   */
  void removeSentinelValues(const CesiumUtility::JsonValue& value) noexcept {
    if (value.isNumber()) {
      // Don't try to use string as sentinels for numbers.
      _canUseNullStringSentinel = false;

      if (value.isInt64()) {
        auto intValue = value.getInt64();
        _canUseZeroSentinel &= (intValue != 0);
        _canUseNegativeOneSentinel &= (intValue != -1);
      } else if (value.isUint64()) {
        _canUseZeroSentinel &= (value.getUint64() != 0);
        // Since the value is truly a uint64, -1 cannot be used.
        _canUseNegativeOneSentinel = false;
      }
    } else if (value.isString()) {
      // Don't try to use numbers as sentinels for strings.
      _canUseZeroSentinel = false;
      _canUseNegativeOneSentinel = false;

      const auto& stringValue = value.getString();
      if (stringValue == "null") {
        _canUseNullStringSentinel = false;
      }
    }
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
    {"SCALAR", GltfPropertyTableType{ClassProperty::Type::SCALAR, 1}},
    {"VEC2", GltfPropertyTableType{ClassProperty::Type::VEC2, 2}},
    {"VEC3", GltfPropertyTableType{ClassProperty::Type::VEC3, 3}},
    {"VEC4", GltfPropertyTableType{ClassProperty::Type::VEC4, 4}},
};

struct GltfPropertyTableComponentType {
  std::string componentType;
  size_t componentTypeSize;
};

const std::map<std::string, GltfPropertyTableComponentType>
    batchTableComponentTypeToGltfComponentType = {
        {"BYTE",
         GltfPropertyTableComponentType{
             ClassProperty::ComponentType::INT8,
             sizeof(int8_t)}},
        {"UNSIGNED_BYTE",
         GltfPropertyTableComponentType{
             ClassProperty::ComponentType::UINT8,
             sizeof(uint8_t)}},
        {"SHORT",
         GltfPropertyTableComponentType{
             ClassProperty::ComponentType::INT16,
             sizeof(int16_t)}},
        {"UNSIGNED_SHORT",
         GltfPropertyTableComponentType{
             ClassProperty::ComponentType::UINT16,
             sizeof(uint16_t)}},
        {"INT",
         GltfPropertyTableComponentType{
             ClassProperty::ComponentType::INT32,
             sizeof(int32_t)}},
        {"UNSIGNED_INT",
         GltfPropertyTableComponentType{
             ClassProperty::ComponentType::UINT32,
             sizeof(uint32_t)}},
        {"FLOAT",
         GltfPropertyTableComponentType{
             ClassProperty::ComponentType::FLOAT32,
             sizeof(float)}},
        {"DOUBLE",
         GltfPropertyTableComponentType{
             ClassProperty::ComponentType::FLOAT64,
             sizeof(double)}},
};

int64_t roundUp(int64_t num, int64_t multiple) noexcept {
  return ((num + multiple - 1) / multiple) * multiple;
}

template <typename T> bool isInRangeForSignedInteger(int64_t value) noexcept {
  // This only works if sizeof(T) is smaller than int64_t
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
      continue;
    }

    if (it->IsNumber()) {
      compatibleTypes &= getCompatibleTypesForNumber(it);

      // Check that the value does not equal one of the possible sentinel
      // values.
      if (it->IsInt64()) {
        compatibleTypes.removeSentinelValues(it->GetInt64());
      } else if (it->IsUint64()) {
        compatibleTypes.removeSentinelValues(it->GetUint64());
      }

      continue;
    }

    if (it->IsArray()) {
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

      continue;
    }

    if (it->IsNull()) {
      compatibleTypes.setHasNullValue(true);

      // If the value is null, check if there is still a possible sentinel
      // values. If none exist, default the type to string.
      if (!compatibleTypes.getSentinelValue()) {
        compatibleTypes.makeIncompatible();
      }

      continue;
    }

    // If this code is reached, the value is a string or something else.
    compatibleTypes.makeIncompatible();

    // If this is a string, check that the value does not equal one of the
    // possible sentinel values.
    if (it->IsString()) {
      compatibleTypes.removeSentinelValues(it->GetString());
    }
  }

  // If no sentinel value is available, then it's not possible to accurately
  // represent the null value of this property. Make it a string property
  // instead.
  if (compatibleTypes.hasNullValue() && !compatibleTypes.getSentinelValue()) {
    compatibleTypes.makeIncompatible();
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
    ClassProperty& classProperty,
    const PropertyTable& propertyTable,
    PropertyTableProperty& propertyTableProperty,
    const TValueGetter& propertyValue) {

  rapidjson::StringBuffer rapidjsonStrBuffer;
  std::vector<uint64_t> rapidjsonOffsets;
  rapidjsonOffsets.reserve(static_cast<size_t>(propertyTable.count + 1));
  rapidjsonOffsets.emplace_back(0);

  std::optional<std::string> noDataValue;
  if (classProperty.noData) {
    noDataValue = classProperty.noData->getString();
  }

  auto it = propertyValue.begin();
  for (int64_t i = 0; i < propertyTable.count; ++i) {
    if (it == propertyValue.end()) {
      rapidjsonOffsets.emplace_back(rapidjsonStrBuffer.GetLength());
      continue;
    }
    if (it->IsString() || (it->IsNull() && noDataValue)) {
      // Because serialized string json will add double quotations in the
      // buffer which is not needed by us, we will manually add the string to
      // the buffer
      std::string_view value;
      if (it->IsString()) {
        value = std::string_view(it->GetString(), it->GetStringLength());
      } else {
        CESIUM_ASSERT(noDataValue);
        value = *noDataValue;
      }
      rapidjsonStrBuffer.Reserve(value.size());
      for (rapidjson::SizeType j = 0; j < value.size(); ++j) {
        rapidjsonStrBuffer.PutUnsafe(value[j]);
      }
    } else {
      // Everything else that is not string will be serialized by json
      rapidjson::Writer<rapidjson::StringBuffer> writer(rapidjsonStrBuffer);
      it->Accept(writer);
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
        PropertyTableProperty::StringOffsetType::UINT8;
  } else if (isInRangeForUnsignedInteger<uint16_t>(totalSize)) {
    copyStringBuffer<uint16_t>(
        rapidjsonStrBuffer,
        rapidjsonOffsets,
        buffer,
        offsetBuffer);
    propertyTableProperty.stringOffsetType =
        PropertyTableProperty::StringOffsetType::UINT16;
  } else if (isInRangeForUnsignedInteger<uint32_t>(totalSize)) {
    copyStringBuffer<uint32_t>(
        rapidjsonStrBuffer,
        rapidjsonOffsets,
        buffer,
        offsetBuffer);
    propertyTableProperty.stringOffsetType =
        PropertyTableProperty::StringOffsetType::UINT32;
  } else {
    copyStringBuffer<uint64_t>(
        rapidjsonStrBuffer,
        rapidjsonOffsets,
        buffer,
        offsetBuffer);
    propertyTableProperty.stringOffsetType =
        PropertyTableProperty::StringOffsetType::UINT64;
  }

  classProperty.type = ClassProperty::Type::STRING;

  propertyTableProperty.values = addBufferToGltf(gltf, std::move(buffer));
  propertyTableProperty.stringOffsets =
      addBufferToGltf(gltf, std::move(offsetBuffer));
}

template <typename T, typename TRapidJson = T, typename TValueGetter>
void updateExtensionWithJsonScalarProperty(
    Model& gltf,
    ClassProperty& classProperty,
    const PropertyTable& propertyTable,
    PropertyTableProperty& propertyTableProperty,
    const TValueGetter& propertyValue,
    const std::string& componentTypeName) {
  CESIUM_ASSERT(propertyValue.size() >= propertyTable.count);

  classProperty.type = ClassProperty::Type::SCALAR;
  classProperty.componentType = componentTypeName;

  // Create a new buffer for this property.
  const size_t byteLength =
      sizeof(T) * static_cast<size_t>(propertyTable.count);
  std::vector<std::byte> buffer(byteLength);

  T* p = reinterpret_cast<T*>(buffer.data());
  auto it = propertyValue.begin();

  std::optional<T> noDataValue;
  if (classProperty.noData) {
    noDataValue = classProperty.noData->getSafeNumber<T>();
  }

  for (int64_t i = 0; i < propertyTable.count; ++i, ++p, ++it) {
    if (it->IsNull()) {
      CESIUM_ASSERT(noDataValue.has_value());
      *p = *noDataValue;
    } else {
      *p = static_cast<T>(it->template Get<TRapidJson>());
    }
  }

  propertyTableProperty.values = addBufferToGltf(gltf, std::move(buffer));
}

template <typename TValueGetter>
void updateExtensionWithJsonBooleanProperty(
    Model& gltf,
    ClassProperty& classProperty,
    const PropertyTable& propertyTable,
    PropertyTableProperty& propertyTableProperty,
    const TValueGetter& propertyValue) {
  CESIUM_ASSERT(propertyValue.size() >= propertyTable.count);

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

  classProperty.type = ClassProperty::Type::BOOLEAN;
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
    const PropertyTable& propertyTable,
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

    prevOffset = static_cast<OffsetType>(prevOffset + jsonArrayMember.Size());

    ++it;
  }

  *offsetValue = prevOffset;
}

template <typename TRapidjson, typename ValueType, typename TValueGetter>
void updateScalarArrayProperty(
    Model& gltf,
    ClassProperty& classProperty,
    PropertyTableProperty& propertyTableProperty,
    const PropertyTable& propertyTable,
    const MaskedArrayType& arrayType,
    const TValueGetter& propertyValue) {
  CESIUM_ASSERT(propertyValue.size() >= propertyTable.count);

  classProperty.type = ClassProperty::Type::SCALAR;
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
    const PropertyTable& propertyTable,
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
    const PropertyTable& propertyTable,
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
    ClassProperty& classProperty,
    PropertyTableProperty& propertyTableProperty,
    const PropertyTable& propertyTable,
    const MaskedArrayType& arrayType,
    const TValueGetter& propertyValue) {
  CESIUM_ASSERT(propertyValue.size() >= propertyTable.count);

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

  classProperty.type = ClassProperty::Type::STRING;
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
  // the size of which is the number of stringElements + 1. This determines
  // the component type of the array offsets.
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
    const PropertyTable& propertyTable,
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
    ClassProperty& classProperty,
    PropertyTableProperty& propertyTableProperty,
    const PropertyTable& propertyTable,
    const MaskedArrayType& arrayType,
    const TValueGetter& propertyValue) {
  CESIUM_ASSERT(propertyValue.size() >= propertyTable.count);

  classProperty.type = ClassProperty::Type::BOOLEAN;
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
    ClassProperty& classProperty,
    const PropertyTable& propertyTable,
    PropertyTableProperty& propertyTableProperty,
    const MaskedArrayType& arrayType,
    const TValueGetter& propertyValue) {
  CESIUM_ASSERT(propertyValue.size() >= propertyTable.count);

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
    ClassProperty& classProperty,
    const PropertyTable& propertyTable,
    PropertyTableProperty& propertyTableProperty,
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
  if (compatibleTypes.isFullyCompatible()) {
    // If this is "fully compatible", then the property contained no values (or
    // rather, no non-null values). Exclude it from the model to avoid errors.
    return;
  }

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
  auto maybeSentinel = compatibleTypes.getSentinelValue();

  // Try to set the "noData" value before copying the property (to avoid copying
  // nulls).
  if (compatibleTypes.hasNullValue() && maybeSentinel) {
    JsonValue sentinelValue = *maybeSentinel;
    // If -1 is the only available sentinel, modify the masked type to only use
    // signed integer types (if possible).
    if (sentinelValue.getInt64OrDefault(0) == -1) {
      type.isUint8 = false;
      type.isUint16 = false;
      type.isUint32 = false;
      type.isUint64 = false;
    }

    classProperty.noData = sentinelValue;
  }

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
        ClassProperty::ComponentType::INT8);
  } else if (type.isUint8) {
    updateExtensionWithJsonScalarProperty<uint8_t, uint32_t>(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue,
        ClassProperty::ComponentType::UINT8);
  } else if (type.isInt16) {
    updateExtensionWithJsonScalarProperty<int16_t, int32_t>(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue,
        ClassProperty::ComponentType::INT16);
  } else if (type.isUint16) {
    updateExtensionWithJsonScalarProperty<uint16_t, uint32_t>(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue,
        ClassProperty::ComponentType::UINT16);
  } else if (type.isInt32) {
    updateExtensionWithJsonScalarProperty<int32_t>(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue,
        ClassProperty::ComponentType::INT32);
  } else if (type.isUint32) {
    updateExtensionWithJsonScalarProperty<uint32_t>(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue,
        ClassProperty::ComponentType::UINT32);
  } else if (type.isInt64) {
    updateExtensionWithJsonScalarProperty<int64_t>(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue,
        ClassProperty::ComponentType::INT64);
  } else if (type.isUint64) {
    updateExtensionWithJsonScalarProperty<uint64_t>(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue,
        ClassProperty::ComponentType::UINT64);
  } else if (type.isFloat32) {
    updateExtensionWithJsonScalarProperty<float>(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue,
        ClassProperty::ComponentType::FLOAT32);
  } else if (type.isFloat64) {
    updateExtensionWithJsonScalarProperty<double>(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        propertyValue,
        ClassProperty::ComponentType::FLOAT64);
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
    ClassProperty& classProperty,
    PropertyTableProperty& propertyTableProperty,
    const PropertyTable& propertyTable,
    const std::string& propertyName,
    const rapidjson::Value& propertyValue,
    ErrorList& result) {
  CESIUM_ASSERT(
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
    Class& classDefinition,
    PropertyTable& propertyTable,
    ErrorList& result,
    const rapidjson::Value& batchTableHierarchy) {
  // EXT_structural_metadata can't support hierarchy, so we need to flatten
  // it. It also can't support multiple classes with a single set of feature
  // IDs, because IDs can only specify one property table. So essentially
  // every property of every class gets added to the one class definition.
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
    ClassProperty& classProperty =
        classDefinition.properties.emplace(name, ClassProperty()).first->second;
    classProperty.name = name;

    PropertyTableProperty& propertyTableProperty =
        propertyTable.properties.emplace(name, PropertyTableProperty())
            .first->second;

    batchTableHierarchyValues.setProperty(name);

    updateExtensionWithJsonProperty(
        gltf,
        classProperty,
        propertyTable,
        propertyTableProperty,
        batchTableHierarchyValues);
    if (propertyTableProperty.values < 0) {
      // Don't include properties without _any_ values.
      propertyTable.properties.erase(name);
    }
  }
}

void convertBatchTableToGltfStructuralMetadataExtension(
    const rapidjson::Document& batchTableJson,
    const std::span<const std::byte>& batchTableBinaryData,
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
  gltf.addExtensionUsed(ExtensionModelExtStructuralMetadata::ExtensionName);

  Schema& schema = modelExtension.schema.emplace();
  schema.id = "default"; // Required by the spec.

  Class& classDefinition =
      schema.classes.emplace("default", Class()).first->second;

  PropertyTable& propertyTable = modelExtension.propertyTables.emplace_back();
  propertyTable.name = "default";
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

    ClassProperty& classProperty =
        classDefinition.properties.emplace(name, ClassProperty()).first->second;
    classProperty.name = name;

    PropertyTableProperty& propertyTableProperty =
        propertyTable.properties.emplace(name, PropertyTableProperty())
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

    if (propertyTableProperty.values < 0) {
      // Don't include properties without _any_ values.
      propertyTable.properties.erase(name);
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
    const std::span<const std::byte>& batchTableBinaryData,
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

      // Also rename the attribute in the Draco extension, if it exists.
      ExtensionKhrDracoMeshCompression* pDraco =
          primitive.getExtension<ExtensionKhrDracoMeshCompression>();
      if (pDraco) {
        auto dracoIt = pDraco->attributes.find("_BATCHID");
        if (dracoIt != pDraco->attributes.end()) {
          pDraco->attributes["_FEATURE_ID_0"] = dracoIt->second;
          pDraco->attributes.erase("_BATCHID");
        }
      }

      ExtensionExtMeshFeatures& extension =
          primitive.addExtension<ExtensionExtMeshFeatures>();
      gltf.addExtensionUsed(ExtensionExtMeshFeatures::ExtensionName);

      FeatureId& featureID = extension.featureIds.emplace_back();

      // No fast way to count the unique feature IDs in this primitive, so
      // substitute the batch table length.
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
    const std::span<const std::byte>& batchTableBinaryData,
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
  CESIUM_ASSERT(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];

  CESIUM_ASSERT(mesh.primitives.size() == 1);
  MeshPrimitive& primitive = mesh.primitives[0];

  ExtensionExtMeshFeatures& extension =
      primitive.addExtension<ExtensionExtMeshFeatures>();
  gltf.addExtensionUsed(ExtensionExtMeshFeatures::ExtensionName);

  FeatureId& featureID = extension.featureIds.emplace_back();

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

namespace {

// Does something like this already exist?

template <typename T> int32_t componentTypeFromCpp();

template <> int32_t componentTypeFromCpp<uint8_t>() {
  return Accessor::ComponentType::UNSIGNED_BYTE;
}

template <> int32_t componentTypeFromCpp<uint16_t>() {
  return Accessor::ComponentType::UNSIGNED_SHORT;
}

template <> int32_t componentTypeFromCpp<uint32_t>() {
  return Accessor::ComponentType::UNSIGNED_INT;
}

// encapsulation of the binary batch id data in an I3dm
struct BatchIdSemantic {
  std::variant<
      std::span<const uint8_t>,
      std::span<const uint16_t>,
      std::span<const uint32_t>>
      batchSpan;
  const std::byte* rawData;
  uint32_t numElements;
  uint32_t byteSize;

  template <typename UintType>
  static std::span<const UintType>
  makeSpan(const std::byte* byteData, uint32_t offset, uint32_t numElements) {
    return std::span<const UintType>(
        reinterpret_cast<const UintType*>(byteData + offset),
        numElements);
  }

  BatchIdSemantic(
      const rapidjson::Document& featureTableJson,
      uint32_t numInstances,
      const std::span<const std::byte>& featureTableJsonData)
      : rawData(nullptr), numElements(0), byteSize(0) {
    const auto batchIdIt = featureTableJson.FindMember("BATCH_ID");
    if (batchIdIt == featureTableJson.MemberEnd() ||
        !batchIdIt->value.IsObject()) {
      return;
    }
    const auto byteOffsetIt = batchIdIt->value.FindMember("byteOffset");
    if (byteOffsetIt == batchIdIt->value.MemberEnd() ||
        !byteOffsetIt->value.IsUint()) {
      // Warning
    }
    uint32_t byteOffset = byteOffsetIt->value.GetUint();
    const auto componentTypeIt = batchIdIt->value.FindMember("componentType");
    if (componentTypeIt != featureTableJson.MemberEnd() &&
        componentTypeIt->value.IsString()) {
      const std::string& componentTypeString =
          componentTypeIt->value.GetString();
      if (MetadataProperty::stringToMetadataComponentType.find(
              componentTypeString) ==
          MetadataProperty::stringToMetadataComponentType.end()) {
        // Warning
      }
      MetadataProperty::ComponentType componentType =
          MetadataProperty::stringToMetadataComponentType.at(
              componentTypeString);
      rawData = featureTableJsonData.data();
      if (componentType == MetadataProperty::ComponentType::UNSIGNED_BYTE) {
        batchSpan = makeSpan<uint8_t>(rawData, byteOffset, numInstances);
        numElements = numInstances;
        byteSize = numElements * sizeof(uint8_t);
      } else if (
          componentType == MetadataProperty::ComponentType::UNSIGNED_SHORT) {
        batchSpan = makeSpan<uint8_t>(rawData, byteOffset, numInstances);
        numElements = numInstances;
        byteSize = numElements * sizeof(uint16_t);
      } else if (
          componentType == MetadataProperty::ComponentType::UNSIGNED_INT) {
        batchSpan = makeSpan<uint32_t>(rawData, byteOffset, numInstances);
        numElements = numInstances;
        byteSize = numElements * sizeof(uint32_t);
      }
    }
  }

  size_t idSize() const {
    return std::visit(
        [](auto&& batchIds) { return sizeof(batchIds[0]); },
        batchSpan);
  }

  uint32_t maxBatchId() const {
    return std::visit(
        [](auto&& batchIds) {
          auto itr = std::max_element(batchIds.begin(), batchIds.end());
          return static_cast<uint32_t>(*itr);
        },
        batchSpan);
  }

  int32_t componentType() const {
    return std::visit(
        [](auto&& batchIds) {
          using span_type = std::remove_reference_t<decltype(batchIds)>;
          return componentTypeFromCpp<typename span_type::value_type>();
        },
        batchSpan);
  }
};

// returns an accessor ID for the added feature IDs
int32_t
addFeatureIdsToGltf(CesiumGltf::Model& gltf, const BatchIdSemantic& batchIds) {
  int32_t featuresBufferId = GltfConverterUtility::createBufferInGltf(gltf);
  auto& featuresBuffer = gltf.buffers[static_cast<uint32_t>(featuresBufferId)];
  featuresBuffer.cesium.data.resize(batchIds.byteSize);
  std::memcpy(
      &featuresBuffer.cesium.data[0],
      batchIds.rawData,
      batchIds.byteSize);
  int32_t featuresBufferViewId = GltfConverterUtility::createBufferViewInGltf(
      gltf,
      featuresBufferId,
      0,
      static_cast<int64_t>(batchIds.idSize()));
  gltf.bufferViews[static_cast<uint32_t>(featuresBufferViewId)].byteLength =
      batchIds.byteSize;

  int32_t accessorId = GltfConverterUtility::createAccessorInGltf(
      gltf,
      featuresBufferViewId,
      batchIds.componentType(),
      batchIds.numElements,
      Accessor::Type::SCALAR);
  return accessorId;
}

} // namespace

ErrorList BatchTableToGltfStructuralMetadata::convertFromI3dm(
    const rapidjson::Document& featureTableJson,
    const rapidjson::Document& batchTableJson,
    const std::span<const std::byte>& featureTableJsonData,
    const std::span<const std::byte>& batchTableBinaryData,
    CesiumGltf::Model& gltf) {
  // Check to make sure a char of rapidjson is 1 byte
  static_assert(
      sizeof(rapidjson::Value::Ch) == 1,
      "RapidJson::Value::Ch is not 1 byte");

  ErrorList result;

  // Parse the batch table and convert it to the EXT_structural_metadata
  // extension.

  // Batch table length is either the max batch ID + 1 or, if there are no batch
  // IDs, the number of instances.
  int64_t featureCount = 0;
  std::optional<BatchIdSemantic> optBatchIds;
  const auto batchIdIt = featureTableJson.FindMember("BATCH_ID");
  std::optional<uint32_t> optInstancesLength =
      GltfConverterUtility::getValue<uint32_t>(
          featureTableJson,
          "INSTANCES_LENGTH");
  if (!optInstancesLength) {
    result.emplaceError("Required INSTANCES_LENGTH semantic is missing");
    return result;
  }
  if (batchIdIt == featureTableJson.MemberEnd()) {
    featureCount = *optInstancesLength;
  } else {
    optBatchIds = BatchIdSemantic(
        featureTableJson,
        *optInstancesLength,
        featureTableJsonData);
    uint32_t maxBatchId = optBatchIds->maxBatchId();
    featureCount = maxBatchId + 1;
  }

  convertBatchTableToGltfStructuralMetadataExtension(
      batchTableJson,
      batchTableBinaryData,
      gltf,
      featureCount,
      result);

  int32_t featureIdAccessor = -1;
  if (optBatchIds.has_value()) {
    featureIdAccessor = addFeatureIdsToGltf(gltf, *optBatchIds);
  }

  // Create an EXT_instance_features extension for node that has an
  // EXT_mesh_gpu_instancing extension
  for (Node& node : gltf.nodes) {
    if (auto* pGpuInstancing =
            node.getExtension<ExtensionExtMeshGpuInstancing>()) {
      auto& instanceFeatureExt =
          node.addExtension<ExtensionExtInstanceFeatures>();
      gltf.addExtensionUsed(ExtensionExtInstanceFeatures::ExtensionName);
      instanceFeatureExt.featureIds.resize(1);
      instanceFeatureExt.featureIds[0].featureCount = featureCount;
      instanceFeatureExt.featureIds[0].propertyTable = 0;
      if (featureIdAccessor >= 0) {
        pGpuInstancing->attributes["_FEATURE_ID_0"] = featureIdAccessor;
        instanceFeatureExt.featureIds[0].attribute = 0;
      }
    }
  }
  return result;
}
} // namespace Cesium3DTilesContent
