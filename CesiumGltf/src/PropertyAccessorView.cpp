#include "CesiumGltf/PropertyAccessorView.h"
#include "CesiumGltf/BufferView.h"
#include "CesiumGltf/ClassProperty.h"

namespace CesiumGltf {
bool PropertyAccessorView::getBoolean(size_t /*instance*/) const {
  return false;
}

std::string_view PropertyAccessorView::getString(size_t /*isntance*/) const {
  return {};
}

/*static*/ std::optional<PropertyAccessorView> PropertyAccessorView::create(
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
      MetaBuffer{valueBuffer, stride},
      &classProperty,
      type,
      instanceCount);
}

/*static*/ size_t PropertyAccessorView::getNumberPropertyTypeSize(uint32_t type) {
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

/*static*/ uint32_t PropertyAccessorView::getPropertyType(const ClassProperty& property) {
  PropertyType type = convertStringToPropertyType(property.type);
  if (type == PropertyType::Array) {
    if (property.componentType.isString()) {
      PropertyType componentType =
          convertStringToPropertyType(property.componentType.getString());
      if (componentType == PropertyType::Array) {
        return static_cast<uint32_t>(PropertyType::None);
      }

      return static_cast<uint32_t>(type) | static_cast<uint32_t>(componentType);
    }
  }

  return static_cast<uint32_t>(type);
}
} // namespace CesiumGltf