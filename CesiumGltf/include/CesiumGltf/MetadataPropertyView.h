#pragma once

#include "CesiumGltf/MetaArrayView.h"
#include "CesiumGltf/PropertyType.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <gsl/span>
#include <string_view>
#include <type_traits>

namespace CesiumGltf {
namespace Impl {
template <typename... T> struct IsNumeric;
template <typename T> struct IsNumeric<T> : std::false_type {};
template <> struct IsNumeric<uint8_t> : std::true_type {};
template <> struct IsNumeric<int8_t> : std::true_type {};
template <> struct IsNumeric<uint16_t> : std::true_type {};
template <> struct IsNumeric<int16_t> : std::true_type {};
template <> struct IsNumeric<uint32_t> : std::true_type {};
template <> struct IsNumeric<int32_t> : std::true_type {};
template <> struct IsNumeric<uint64_t> : std::true_type {};
template <> struct IsNumeric<int64_t> : std::true_type {};
template <> struct IsNumeric<float> : std::true_type {};
template <> struct IsNumeric<double> : std::true_type {};

template <typename... T> struct IsBoolean;
template <typename T> struct IsBoolean<T> : std::false_type {};
template <> struct IsBoolean<bool> : std::true_type {};

template <typename... T> struct IsString;
template <typename T> struct IsString<T> : std::false_type {};
template <> struct IsString<std::string_view> : std::true_type {};

template <typename... T> struct IsNumericArray;
template <typename T> struct IsNumericArray<T> : std::false_type {};
template <typename T> struct IsNumericArray<MetaArrayView<T>> {
  static constexpr bool value = IsNumeric<T>::value;
};

template <typename... T> struct IsBooleanArray;
template <typename T> struct IsBooleanArray<T> : std::false_type {};
template <> struct IsBooleanArray<MetaArrayView<bool>> : std::true_type {};

template <typename... T> struct IsStringArray;
template <typename T> struct IsStringArray<T> : std::false_type {};
template <>
struct IsStringArray<MetaArrayView<std::string_view>> : std::true_type {};

template <typename T> struct ArrayType;
template <typename T> struct ArrayType<CesiumGltf::MetaArrayView<T>> {
  using type = T;
};
} // namespace Impl

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

  ElementType operator[](size_t instance) const {
    if constexpr (Impl::IsNumeric<ElementType>::value) {
      return getNumeric(instance);
    }

    if constexpr (Impl::IsBoolean<ElementType>::value) {
      return getBoolean(instance);
    }

    if constexpr (Impl::IsString<ElementType>::value) {
      return getString(instance);
    }

    if constexpr (Impl::IsNumericArray<ElementType>::value) {
      return getNumericArray<typename Impl::ArrayType<ElementType>::type>(
          instance);
    }

    if constexpr (Impl::IsBooleanArray<ElementType>::value) {
      return getBooleanArray(instance);
    }

    if constexpr (Impl::IsStringArray<ElementType>::value) {
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
  MetaArrayView<T> getNumericArray(size_t instance) const {
    if (_componentCount > 0) {
      gsl::span<const T> vals(
          reinterpret_cast<const T*>(
              _valueBuffer.data() + instance * _componentCount * sizeof(T)),
          _componentCount);
      return MetaArrayView{vals};
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
    return MetaArrayView{vals};
  }

  MetaArrayView<std::string_view> getStringArray(size_t instance) const {
    if (_componentCount > 0) {
      gsl::span<const std::byte> offsetVals(
          _stringOffsetBuffer.data() + instance * _componentCount * _offsetSize,
          (_componentCount + 1) * _offsetSize);
      return MetaArrayView<std::string_view>(
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
    return MetaArrayView<std::string_view>(
        _valueBuffer,
        offsetVals,
        _offsetType,
        (nextOffset - currentOffset) / _offsetSize);
  }

  MetaArrayView<bool> getBooleanArray(size_t instance) const {
    if (_componentCount > 0) {
      size_t totalBits = _componentCount * instance;
      gsl::span<const std::byte> buffer(
          _valueBuffer.data() + totalBits / 8,
          (_componentCount / 8 + 1));
      return MetaArrayView<bool>(buffer, totalBits % 8, _componentCount);
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
    return MetaArrayView<bool>(buffer, currentOffset % 8, totalBits);
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
