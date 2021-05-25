#pragma once

#include "CesiumGltf/BufferView.h"
#include "CesiumGltf/FeatureTable.h"
#include "CesiumGltf/FeatureTableProperty.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/Schema.h"
#include <cassert>
#include <cstddef>
#include <gsl/span>
#include <optional>
#include <string_view>

namespace CesiumGltf {
enum PropertyType {
  None = 1 << 0,
  Uint8 = 1 << 1,
  Int8 = 1 << 2,
  Uint16 = 1 << 3,
  Int16 = 1 << 4,
  Uint32 = 1 << 5,
  Int32 = 1 << 6,
  Uint64 = 1 << 7,
  Int64 = 1 << 8,
  Float32 = 1 << 9,
  Float64 = 1 << 10,
  Boolean = 1 << 11,
  String = 1 << 12,
  Enum = 1 << 13,
  Array = 1 << 14,
};

template <typename T> struct TypeToPropertyType;

template <> struct TypeToPropertyType<uint8_t> {
  static constexpr uint32_t value = PropertyType::Uint8;
};

template <> struct TypeToPropertyType<int8_t> {
  static constexpr uint32_t value = PropertyType::Int8;
};

template <> struct TypeToPropertyType<uint16_t> {
  static constexpr uint32_t value = PropertyType::Uint16;
};

template <> struct TypeToPropertyType<int16_t> {
  static constexpr uint32_t value = PropertyType::Int16;
};

template <> struct TypeToPropertyType<uint32_t> {
  static constexpr uint32_t value = PropertyType::Uint32;
};

template <> struct TypeToPropertyType<int32_t> {
  static constexpr uint32_t value = PropertyType::Int32;
};

template <> struct TypeToPropertyType<uint64_t> {
  static constexpr uint32_t value = PropertyType::Uint64;
};

template <> struct TypeToPropertyType<int64_t> {
  static constexpr uint32_t value = PropertyType::Int64;
};

template <> struct TypeToPropertyType<float> {
  static constexpr uint32_t value = PropertyType::Float32;
};

template <> struct TypeToPropertyType<double> {
  static constexpr uint32_t value = PropertyType::Float64;
};

template <> struct TypeToPropertyType<bool> {
  static constexpr uint32_t value = PropertyType::Boolean;
};

template <> struct TypeToPropertyType<std::string_view> {
  static constexpr uint32_t value = PropertyType::String;
};

template <typename T> struct TypeToPropertyType<gsl::span<const T>> {
  static constexpr uint32_t value =
      PropertyType::Array | TypeToPropertyType<T>::value;
};

static PropertyType convertStringToPropertyType(const std::string& str) {
  if (str == "UINT8") {
    return PropertyType::Uint8;
  }

  if (str == "INT8") {
    return PropertyType::Int8;
  }

  if (str == "UINT16") {
    return PropertyType::Uint16;
  }

  if (str == "INT16") {
    return PropertyType::Int16;
  }

  if (str == "UINT32") {
    return PropertyType::Uint32;
  }

  if (str == "INT32") {
    return PropertyType::Int32;
  }

  if (str == "UINT64") {
    return PropertyType::Uint64;
  }

  if (str == "INT64") {
    return PropertyType::Int64;
  }

  if (str == "FLOAT32") {
    return PropertyType::Float32;
  }

  if (str == "FLOAT64") {
    return PropertyType::Float64;
  }

  if (str == "BOOLEAN") {
    return PropertyType::Boolean;
  }

  if (str == "STRING") {
    return PropertyType::String;
  }

  if (str == "ENUM") {
    return PropertyType::Enum;
  }

  if (str == "ARRAY") {
    return PropertyType::Array;
  }

  return PropertyType::None;
}

static size_t getScalarTypeSize(uint32_t type) {
  switch (type) {
  case CesiumGltf::None:
    return 0;
  case CesiumGltf::Uint8:
    return sizeof(uint8_t);
  case CesiumGltf::Int8:
    return sizeof(int8_t);
  case CesiumGltf::Uint16:
    return sizeof(uint16_t);
  case CesiumGltf::Int16:
    return sizeof(int16_t);
  case CesiumGltf::Uint32:
    return sizeof(uint32_t);
  case CesiumGltf::Int32:
    return sizeof(int32_t);
  case CesiumGltf::Uint64:
    return sizeof(uint64_t);
  case CesiumGltf::Int64:
    return sizeof(int64_t);
  case CesiumGltf::Float32:
    return sizeof(float);
  case CesiumGltf::Float64:
    return sizeof(double);
  default:
    return 0;
  }
}

static uint32_t getPropertyType(const ClassProperty* property) {

  PropertyType type = convertStringToPropertyType(property->type);
  if (type == PropertyType::Array) {
    if (property->componentType.isString()) {
      PropertyType componentType =
          convertStringToPropertyType(property->componentType.getString());
      if (componentType == PropertyType::Array) {
        return PropertyType::None;
      }

      return type | componentType;
    }
  }

  return type;
}

class PropertyAccessorView {
public:
  PropertyAccessorView(
      gsl::span<const std::byte> valueBuffer,
      size_t stride,
      const ClassProperty* property,
      uint32_t type,
      size_t instanceCount)
      : _valueBuffer{valueBuffer},
        _stride{stride},
        _instanceCount{instanceCount},
        _type{type},
        _property{property} {}

  size_t numOfInstances() const { return _instanceCount; }

  uint32_t getType() const { return _type; }

  template <typename T> const T* getScalar(size_t instance) const {
    assert(TypeToPropertyType<T>::value == _type);
    return *reinterpret_cast<const T*>(
        _valueBuffer.data() + instance * _stride);
  }

  static std::optional<PropertyAccessorView> create(
      const Model& model,
      const Schema& schema,
      const FeatureTable& table,
      const std::string& propertyName) {
    auto classIter = schema.classes.find(*table.classProperty);
    if (classIter == schema.classes.end()) {
      return std::nullopt;
    }

    const auto& classProperties = classIter->second.properties;
    auto classPropertyIter = classProperties.find(propertyName);
    if (classPropertyIter == classProperties.end()) {
      return std::nullopt;
    }

    auto featureTablePropertyIter = table.properties.find(propertyName);
    if (featureTablePropertyIter == table.properties.end()) {
      return std::nullopt;
    }

    const FeatureTableProperty& featureTableProperty =
        featureTablePropertyIter->second;
    if (featureTableProperty.bufferView >= model.bufferViews.size()) {
      return std::nullopt;
    }

    const BufferView& bufferView =
        model.bufferViews[featureTableProperty.bufferView];
    if (bufferView.buffer >= model.buffers.size()) {
      return std::nullopt;
    }

    const Buffer& buffer = model.buffers[bufferView.buffer];
    if (bufferView.byteOffset + bufferView.byteLength >
        static_cast<int64_t>(buffer.cesium.data.size())) {
      return std::nullopt;
    }

    const ClassProperty* classProperty = &classPropertyIter->second;
    uint32_t type = getPropertyType(classProperty);
    if (!type) {
      return std::nullopt;
    }

    size_t stride{};
    if (bufferView.byteStride) {
      stride = *bufferView.byteStride;
    } else {
      stride = getScalarTypeSize(type);
    }

    gsl::span<const std::byte> valueBuffer(
        reinterpret_cast<const std::byte*>(
            buffer.cesium.data.data() + bufferView.byteOffset),
        bufferView.byteLength);
    return PropertyAccessorView(
        valueBuffer,
        stride,
        classProperty,
        type,
        table.count);
  }

private:
  gsl::span<const std::byte> _valueBuffer;
  size_t _stride;
  size_t _instanceCount;
  uint32_t _type;
  const ClassProperty* _property;
};
} // namespace CesiumGltf