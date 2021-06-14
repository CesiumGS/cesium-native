#pragma once

#include "CesiumGltf/PropertyType.h"
#include <cassert>
#include <cstddef>
#include <gsl/span>

namespace CesiumGltf {
template <typename ElementType> class ArrayView {
public:
  ArrayView(gsl::span<const ElementType> buffer) : _valueBuffer{buffer} {}

  const ElementType& operator[](size_t index) const {
    return _valueBuffer[index];
  }

  size_t size() const { return _valueBuffer.size(); }

private:
  gsl::span<const ElementType> _valueBuffer;
};

template <> class ArrayView<bool> {
public:
  ArrayView(
      gsl::span<const std::byte> buffer,
      size_t bitOffset,
      size_t instanceCount)
      : _valueBuffer{buffer},
        _bitOffset{bitOffset},
        _instanceCount{instanceCount} {}

  bool operator[](size_t index) const {
    index += _bitOffset;
    size_t byteIndex = index / 8;
    size_t bitIndex = index % 8;
    int bitValue = static_cast<int>(_valueBuffer[byteIndex] >> bitIndex) & 1;
    return bitValue == 1;
  }

  size_t size() const { return _instanceCount; }

private:
  gsl::span<const std::byte> _valueBuffer;
  size_t _bitOffset;
  size_t _instanceCount;
};

template <> class ArrayView<std::string_view> {
public:
  ArrayView(
      gsl::span<const std::byte> buffer,
      gsl::span<const std::byte> offsetBuffer,
      PropertyType offsetType,
      size_t size)
      : _valueBuffer{buffer},
        _offsetBuffer{offsetBuffer},
        _offsetType{offsetType},
        _size{size} {}

  std::string_view operator[](size_t index) const {
    size_t currentOffset =
        getOffsetFromOffsetBuffer(index, _offsetBuffer, _offsetType);
    size_t nextOffset =
        getOffsetFromOffsetBuffer(index + 1, _offsetBuffer, _offsetType);
    return std::string_view(
        reinterpret_cast<const char*>(_valueBuffer.data() + currentOffset),
        (nextOffset - currentOffset));
  }

  size_t size() const { return _size; }

private:
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
  gsl::span<const std::byte> _offsetBuffer;
  PropertyType _offsetType;
  size_t _size;
};
} // namespace CesiumGltf
