#pragma once

#include "CesiumGltf/Model.h"
#include "CesiumGltf/FeatureTableProperty.h"
#include "CesiumGltf/PropertyType.h"
#include <cassert>
#include <cstddef>
#include <gsl/span>
#include <optional>
#include <string_view>
#include <type_traits>

namespace CesiumGltf {
struct ClassProperty;

class PropertyAccessorView {
  struct MetaBuffer {
    gsl::span<const std::byte> buffer;
    size_t stride;
  };

public:
  PropertyAccessorView(
      MetaBuffer valueBuffer,
      const ClassProperty* property,
      uint32_t type,
      size_t instanceCount)
      : _valueBuffer{valueBuffer},
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
        TypeToPropertyType<T>::value &
        static_cast<uint32_t>(PropertyType::Array)) {
      return getArray<T>(instance);
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

  template <typename T> gsl::span<const T> getArray(size_t /*instance*/) const {
    return gsl::span<const T>();
  }

  bool getBoolean(size_t instance) const;

  std::string_view getString(size_t isntance) const;

  static size_t getNumberPropertyTypeSize(uint32_t type);

  static uint32_t getPropertyType(const ClassProperty& property);

  MetaBuffer _valueBuffer;
  size_t _instanceCount;
  uint32_t _type;
  const ClassProperty* _property;
};
} // namespace CesiumGltf