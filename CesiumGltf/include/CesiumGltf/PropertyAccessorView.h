#pragma once

#include "CesiumGltf/BufferView.h"
#include "CesiumGltf/FeatureTable.h"
#include "CesiumGltf/FeatureTableProperty.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/PropertyType.h"
#include "CesiumGltf/Schema.h"
#include <cassert>
#include <cstddef>
#include <gsl/span>
#include <optional>
#include <string_view>

namespace CesiumGltf {
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

  template <typename T> T getScalar(size_t instance) const {
    assert(TypeToPropertyType<T>::value == _type);
    return *reinterpret_cast<const T*>(
        _valueBuffer.data() + instance * _stride);
  }

  static std::optional<PropertyAccessorView> create(
      const Model& model,
      const ClassProperty& classProperty,
      const FeatureTableProperty& featureTableProperty,
      size_t instanceCount) {
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

    uint32_t type = getPropertyType(classProperty);
    if (!type) {
      return std::nullopt;
    }

    size_t stride{};
    if (bufferView.byteStride && bufferView.byteStride > 0) {
      stride = *bufferView.byteStride;
    } else {
      stride = getNumberPropertyTypeSize(type);
    }

    gsl::span<const std::byte> valueBuffer(
        reinterpret_cast<const std::byte*>(
            buffer.cesium.data.data() + bufferView.byteOffset),
        bufferView.byteLength);
    return PropertyAccessorView(
        valueBuffer,
        stride,
        &classProperty,
        type,
        instanceCount);
  }

private:
  static size_t getNumberPropertyTypeSize(uint32_t type) {
    switch (type) {
    case static_cast<uint32_t>(PropertyType::None):
      return 0;
    case static_cast<uint32_t>(PropertyType::Uint8):
      return sizeof(uint8_t);
    case static_cast<uint32_t>(PropertyType::Int8):
      return sizeof(int8_t);
    case static_cast<uint32_t>(PropertyType::Uint16):
      return sizeof(uint16_t);
    case static_cast<uint32_t>(PropertyType::Int16):
      return sizeof(int16_t);
    case static_cast<uint32_t>(PropertyType::Uint32):
      return sizeof(uint32_t);
    case static_cast<uint32_t>(PropertyType::Int32):
      return sizeof(int32_t);
    case static_cast<uint32_t>(PropertyType::Uint64):
      return sizeof(uint64_t);
    case static_cast<uint32_t>(PropertyType::Int64):
      return sizeof(int64_t);
    case static_cast<uint32_t>(PropertyType::Float32):
      return sizeof(float);
    case static_cast<uint32_t>(PropertyType::Float64):
      return sizeof(double);
    default:
      return 0;
    }
  }

  static uint32_t getPropertyType(const ClassProperty& property) {
    PropertyType type = convertStringToPropertyType(property.type);
    if (type == PropertyType::Array) {
      if (property.componentType.isString()) {
        PropertyType componentType =
            convertStringToPropertyType(property.componentType.getString());
        if (componentType == PropertyType::Array) {
          return static_cast<uint32_t>(PropertyType::None);
        }

        return static_cast<uint32_t>(type) |
               static_cast<uint32_t>(componentType);
      }
    }

    return static_cast<uint32_t>(type);
  }

  gsl::span<const std::byte> _valueBuffer;
  size_t _stride;
  size_t _instanceCount;
  uint32_t _type;
  const ClassProperty* _property;
};
} // namespace CesiumGltf