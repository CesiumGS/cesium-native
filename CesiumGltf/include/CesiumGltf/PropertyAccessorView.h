#pragma once

#include "CesiumGltf/FeatureTableProperty.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/PropertyType.h"
#include <cassert>
#include <cstddef>
#include <gsl/span>
#include <optional>
#include <string_view>
#include <type_traits>

namespace CesiumGltf {
struct Model;
struct BufferView;
struct ClassProperty;

class PropertyAccessorView {
  struct MetaBuffer {
    gsl::span<const std::byte> buffer;
    size_t stride;
  };

  template <typename T> struct ArrayType;
  template <typename T> struct ArrayType<gsl::span<const T>> {
    using type = T;
  };

public:
  PropertyAccessorView(
      MetaBuffer valueBuffer,
      gsl::span<const std::byte> arrayOffsetBuffer,
      gsl::span<const std::byte> stringOffsetBuffer,
      PropertyType offsetType,
      const ClassProperty* property,
      uint32_t type,
      size_t instanceCount)
      : _valueBuffer{valueBuffer},
        _arrayOffsetBuffer{arrayOffsetBuffer},
        _stringOffsetBuffer{stringOffsetBuffer},
        _offsetType{offsetType},
        _instanceCount{instanceCount},
        _type{type},
        _property{property} {}

  size_t numOfInstances() const noexcept { return _instanceCount; }

  uint32_t getType() const noexcept { return _type; }

  template <typename T> T get(size_t instance) const {
    static_assert(
        TypeToPropertyType<T>::value !=
            static_cast<uint32_t>(PropertyType::None),
        "Encounter unknown property type");

    if constexpr (
        TypeToPropertyType<T>::value <=
        static_cast<uint32_t>(PropertyType::Float64)) {
      return getNumber<T>(instance);
    }

    if constexpr (
        TypeToPropertyType<T>::value ==
        static_cast<uint32_t>(PropertyType::Boolean)) {
      return getBoolean(instance);
    }

    if constexpr (
        (TypeToPropertyType<T>::value &
         static_cast<uint32_t>(PropertyType::Array)) != 0) {
      return getArray<typename ArrayType<T>::type>(instance);
    }

    if constexpr (
        TypeToPropertyType<T>::value ==
        static_cast<uint32_t>(PropertyType::String)) {
      return getString(instance);
    }
  }

  static std::optional<PropertyAccessorView> create(
      const Model& model,
      const ClassProperty& classProperty,
      const FeatureTableProperty& featureTableProperty,
      size_t instanceCount);

private:
  template <typename T> T getNumber(size_t instance) const {
    assert(TypeToPropertyType<T>::value == _type);
    return *reinterpret_cast<const T*>(
        _valueBuffer.buffer.data() + instance * _valueBuffer.stride);
  }

  template <typename T> gsl::span<const T> getArray(size_t instance) const {
    assert(
        _type == (static_cast<uint32_t>(PropertyType::Array) |
                  TypeToPropertyType<T>::value));
    if (_property->componentCount) {
      return gsl::span<const T>(
          reinterpret_cast<const T*>(
              _valueBuffer.buffer.data() + instance * _valueBuffer.stride),
          *_property->componentCount);
    }

    size_t currentOffset =
        getOffsetFromOffsetBuffer(instance, _arrayOffsetBuffer, _offsetType);
    size_t nextOffset = getOffsetFromOffsetBuffer(
        instance + 1,
        _arrayOffsetBuffer,
        _offsetType);
    return gsl::span<const T>(
        reinterpret_cast<const T*>(_valueBuffer.buffer.data() + currentOffset),
        (nextOffset - currentOffset) % sizeof(T));
  }

  bool getBoolean(size_t instance) const;

  std::string_view getString(size_t isntance) const;

  static size_t getOffsetFromOffsetBuffer(
      size_t instance,
      const gsl::span<const std::byte>& offsetBuffer,
      PropertyType offsetType);

  static size_t getNumberPropertyTypeSize(uint32_t type);

  static uint32_t getPropertyType(const ClassProperty& property);

  static bool createMetaBuffer(
      const Model& model,
      const BufferView& bufferView,
      size_t instanceCount,
      size_t componentCount,
      uint32_t type,
      MetaBuffer& metaBuffer);

  MetaBuffer _valueBuffer;
  gsl::span<const std::byte> _arrayOffsetBuffer;
  gsl::span<const std::byte> _stringOffsetBuffer;
  PropertyType _offsetType;
  size_t _instanceCount;
  uint32_t _type;
  const ClassProperty* _property;
};
} // namespace CesiumGltf