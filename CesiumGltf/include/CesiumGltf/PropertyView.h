#pragma once

#include "CesiumGltf/MetadataArrayView.h"
#include "CesiumGltf/PropertyType.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <gsl/span>
#include <string_view>
#include <type_traits>

namespace CesiumGltf {
template <typename ElementType> class MetadataPropertyView {
public:
  MetadataPropertyView(
      gsl::span<const std::byte> valueBuffer,
      gsl::span<const std::byte> arrayOffsetBuffer,
      gsl::span<const std::byte> stringOffsetBuffer,
      PropertyType offsetType,
      size_t componentCount,
      size_t instanceCount)
      : _valueBuffer{valueBuffer},
        _arrayOffsetBuffer{arrayOffsetBuffer},
        _stringOffsetBuffer{stringOffsetBuffer},
        _offsetType{offsetType},
        _offsetSize{getOffsetSize(offsetType)},
        _componentCount{componentCount},
        _instanceCount{instanceCount} {}

  ElementType get(size_t instance) const {
    if constexpr (IsNumeric<ElementType>::value) {
      return getNumeric(instance);
    }

    if constexpr (IsBoolean<ElementType>::value) {
      return getBoolean(instance);
    }

    if constexpr (IsString<ElementType>::value) {
      return getString(instance);
    }

    if constexpr (IsNumericArray<ElementType>::value) {
      return getNumericArray<typename ArrayType<ElementType>::type>(instance);
    }

    if constexpr (IsBooleanArray<ElementType>::value) {
      return getBooleanArray(instance);
    }

    if constexpr (IsStringArray<ElementType>::value) {
      return getStringArray(instance);
    }
  }

  size_t size() const { return _instanceCount; }

private:
  ElementType getNumeric(size_t instance) const {
    return reinterpret_cast<const ElementType*>(_valueBuffer.data())[instance];
  }

  bool getBoolean(size_t instance) const {
    size_t byteIndex = instance / 8;
    size_t bitIndex = instance % 8;
    int bitValue = static_cast<int>(_valueBuffer[byteIndex] >> bitIndex) & 1;
    return bitValue == 1;
  }

  std::string_view getString(size_t instance) const {
    size_t currentOffset =
        getOffsetFromOffsetBuffer(instance, _stringOffsetBuffer, _offsetType);
    size_t nextOffset = getOffsetFromOffsetBuffer(
        instance + 1,
        _stringOffsetBuffer,
        _offsetType);
    return std::string_view(
        reinterpret_cast<const char*>(_valueBuffer.data() + currentOffset),
        (nextOffset - currentOffset));
  }

  template <typename T>
  MetadataArrayView<T> getNumericArray(size_t instance) const {
    if (_componentCount > 0) {
      gsl::span<const T> vals(
          reinterpret_cast<const T*>(
              _valueBuffer.data() + instance * _componentCount * sizeof(T)),
          _componentCount);
      return MetadataArrayView{vals};
    }

    size_t currentOffset =
        getOffsetFromOffsetBuffer(instance, _arrayOffsetBuffer, _offsetType);
    size_t nextOffset = getOffsetFromOffsetBuffer(
        instance + 1,
        _arrayOffsetBuffer,
        _offsetType);
    gsl::span<const T> vals(
        reinterpret_cast<const T*>(_valueBuffer.data() + currentOffset),
        (nextOffset - currentOffset) / sizeof(T));
    return MetadataArrayView{vals};
  }

  MetadataArrayView<std::string_view> getStringArray(size_t instance) const {
    if (_componentCount > 0) {
      gsl::span<const std::byte> offsetVals(
          _stringOffsetBuffer.data() + instance * _componentCount * _offsetSize,
          (_componentCount + 1) * _offsetSize);
      return MetadataArrayView<std::string_view>(
          _valueBuffer,
          offsetVals,
          _offsetType,
          _componentCount);
    }

    size_t currentOffset =
        getOffsetFromOffsetBuffer(instance, _arrayOffsetBuffer, _offsetType);
    size_t nextOffset = getOffsetFromOffsetBuffer(
        instance + 1,
        _arrayOffsetBuffer,
        _offsetType);
    gsl::span<const std::byte> offsetVals(
        _stringOffsetBuffer.data() + currentOffset,
        (nextOffset - currentOffset + _offsetSize));
    return MetadataArrayView<std::string_view>(
        _valueBuffer,
        offsetVals,
        _offsetType,
        (nextOffset - currentOffset) / _offsetSize);
  }

  MetadataArrayView<bool> getBooleanArray(size_t instance) const {
    if (_componentCount > 0) {
      size_t offsetBits = _componentCount * instance;
      size_t nextOffsetBits = _componentCount * (instance + 1);
      gsl::span<const std::byte> buffer(
          _valueBuffer.data() + offsetBits / 8,
          (nextOffsetBits / 8 - offsetBits / 8 + 1));
      return MetadataArrayView<bool>(buffer, offsetBits % 8, _componentCount);
    }

    size_t currentOffset =
        getOffsetFromOffsetBuffer(instance, _arrayOffsetBuffer, _offsetType);
    size_t nextOffset = getOffsetFromOffsetBuffer(
        instance + 1,
        _arrayOffsetBuffer,
        _offsetType);

    size_t totalBits = nextOffset - currentOffset;
    gsl::span<const std::byte> buffer(
        _valueBuffer.data() + currentOffset / 8,
        (nextOffset / 8 - currentOffset / 8 + 1));
    return MetadataArrayView<bool>(buffer, currentOffset % 8, totalBits);
  }

  static size_t getOffsetSize(PropertyType offsetType) {
    switch (offsetType) {
    case CesiumGltf::PropertyType::Uint8:
      return sizeof(uint8_t);
    case CesiumGltf::PropertyType::Uint16:
      return sizeof(uint16_t);
    case CesiumGltf::PropertyType::Uint32:
      return sizeof(uint32_t);
    case CesiumGltf::PropertyType::Uint64:
      return sizeof(uint64_t);
    default:
      return 0;
    }
  }

  static size_t getOffsetFromOffsetBuffer(
      size_t instance,
      const gsl::span<const std::byte>& offsetBuffer,
      PropertyType offsetType) {
    switch (offsetType) {
    case PropertyType::Uint8: {
      assert(instance < offsetBuffer.size() / sizeof(uint8_t));
      uint8_t offset = *reinterpret_cast<const uint8_t*>(
          offsetBuffer.data() + instance * sizeof(uint8_t));
      return static_cast<size_t>(offset);
    }
    case PropertyType::Uint16: {
      assert(instance < offsetBuffer.size() / sizeof(uint16_t));
      uint16_t offset = *reinterpret_cast<const uint16_t*>(
          offsetBuffer.data() + instance * sizeof(uint16_t));
      return static_cast<size_t>(offset);
    }
    case PropertyType::Uint32: {
      assert(instance < offsetBuffer.size() / sizeof(uint32_t));
      uint32_t offset = *reinterpret_cast<const uint32_t*>(
          offsetBuffer.data() + instance * sizeof(uint32_t));
      return static_cast<size_t>(offset);
    }
    case PropertyType::Uint64: {
      assert(instance < offsetBuffer.size() / sizeof(uint64_t));
      uint64_t offset = *reinterpret_cast<const uint64_t*>(
          offsetBuffer.data() + instance * sizeof(uint64_t));
      return static_cast<size_t>(offset);
    }
    default:
      assert(false && "Offset type has unknown type");
      return 0;
    }
  }

  gsl::span<const std::byte> _valueBuffer;
  gsl::span<const std::byte> _arrayOffsetBuffer;
  gsl::span<const std::byte> _stringOffsetBuffer;
  PropertyType _offsetType;
  size_t _offsetSize;
  size_t _componentCount;
  size_t _instanceCount;
};
} // namespace CesiumGltf
