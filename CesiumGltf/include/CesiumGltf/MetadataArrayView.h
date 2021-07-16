#pragma once

#include "CesiumGltf/PropertyType.h"
#include <cassert>
#include <cstddef>
#include <gsl/span>

namespace CesiumGltf {
template <typename ElementType> class MetadataArrayView {
public:
  MetadataArrayView(const gsl::span<const ElementType>& buffer)
      : _valueBuffer{buffer} {}

  const ElementType& operator[](int64_t index) const {
    return _valueBuffer[index];
  }

  int64_t size() const { return static_cast<int64_t>(_valueBuffer.size()); }

private:
  gsl::span<const ElementType> _valueBuffer;
};

template <> class MetadataArrayView<bool> {
public:
  MetadataArrayView(
      const gsl::span<const std::byte>& buffer,
      int64_t bitOffset,
      int64_t instanceCount)
      : _valueBuffer{buffer},
        _bitOffset{bitOffset},
        _instanceCount{instanceCount} {}

  bool operator[](int64_t index) const {
    index += _bitOffset;
    int64_t byteIndex = index / 8;
    int64_t bitIndex = index % 8;
    int bitValue = static_cast<int>(_valueBuffer[byteIndex] >> bitIndex) & 1;
    return bitValue == 1;
  }

  int64_t size() const { return _instanceCount; }

private:
  gsl::span<const std::byte> _valueBuffer;
  int64_t _bitOffset;
  int64_t _instanceCount;
};

template <> class MetadataArrayView<std::string_view> {
public:
  MetadataArrayView(
      const gsl::span<const std::byte>& buffer,
      const gsl::span<const std::byte>& offsetBuffer,
      PropertyType offsetType,
      int64_t size)
      : _valueBuffer{buffer},
        _offsetBuffer{offsetBuffer},
        _offsetType{offsetType},
        _size{size} {}

  std::string_view operator[](int64_t index) const {
    size_t currentOffset =
        getOffsetFromOffsetBuffer(index, _offsetBuffer, _offsetType);
    size_t nextOffset =
        getOffsetFromOffsetBuffer(index + 1, _offsetBuffer, _offsetType);
    return std::string_view(
        reinterpret_cast<const char*>(_valueBuffer.data() + currentOffset),
        (nextOffset - currentOffset));
  }

  int64_t size() const { return _size; }

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
  int64_t _size;
};
} // namespace CesiumGltf
