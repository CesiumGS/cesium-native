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
} // namespace Impl

template <typename ElementType> class TPropertyView {
public:
  TPropertyView(
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

  static size_t getOffsetFromOffsetBuffer(
      size_t instance,
      const gsl::span<const std::byte>& offsetBuffer,
      PropertyType offsetType) {
    switch (offsetType) {
    case PropertyType::Uint8: {
      uint8_t offset = *reinterpret_cast<const uint8_t*>(
          offsetBuffer.data() + instance * sizeof(uint8_t));
      return static_cast<size_t>(offset);
    }
    case PropertyType::Uint16: {
      uint16_t offset = *reinterpret_cast<const uint16_t*>(
          offsetBuffer.data() + instance * sizeof(uint16_t));
      return static_cast<size_t>(offset);
    }
    case PropertyType::Uint32: {
      uint32_t offset = *reinterpret_cast<const uint32_t*>(
          offsetBuffer.data() + instance * sizeof(uint32_t));
      return static_cast<size_t>(offset);
    }
    case PropertyType::Uint64: {
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
  size_t _componentCount;
  size_t _instanceCount;
};
} // namespace CesiumGltf