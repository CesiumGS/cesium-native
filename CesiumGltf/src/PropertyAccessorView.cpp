#include "CesiumGltf/PropertyAccessorView.h"
#include "CesiumGltf/Buffer.h"
#include "CesiumGltf/BufferView.h"
#include "CesiumGltf/ClassProperty.h"

namespace {
CesiumGltf::PropertyType getOffsetType(const std::string& type) {
  if (type == "UINT8") {
    return CesiumGltf::PropertyType::Uint8;
  }

  if (type == "UINT16") {
    return CesiumGltf::PropertyType::Uint16;
  }

  if (type == "UINT32") {
    return CesiumGltf::PropertyType::Uint32;
  }

  if (type == "UINT64") {
    return CesiumGltf::PropertyType::Uint64;
  }

  return CesiumGltf::PropertyType::None;
}
}

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
  // check correct type
  uint32_t type = getPropertyType(classProperty);
  if (!type) {
    return std::nullopt;
  }

  // check buffer view index
  if (featureTableProperty.bufferView >= model.bufferViews.size()) {
    return std::nullopt;
  }

  // check buffer view size is correct
  const BufferView& valueBufferView =
      model.bufferViews[featureTableProperty.bufferView];
  if (valueBufferView.buffer >= model.buffers.size() || valueBufferView.byteOffset < 0) {
    return std::nullopt;
  }

  MetaBuffer valueMetaBuffer;
  if (!createMetaBuffer(
          model,
          valueBufferView,
          instanceCount,
          type,
          valueMetaBuffer)) {
    return std::nullopt;
  }

  // make sure dynamic array will have offset buffer
  bool isFixedArray =
      classProperty.componentCount && classProperty.componentCount > 0;
  bool isDynamicArray =
      type & static_cast<uint32_t>(PropertyType::Array) && !isFixedArray; 
  MetaBuffer arrayOffsetMetaBuffer;
  if (isDynamicArray) {
    if (featureTableProperty.arrayOffsetBufferView < 0 ||
        featureTableProperty.arrayOffsetBufferView >=
            model.bufferViews.size()) {
      return std::nullopt;
    }

    const BufferView& arrayOffsetBufferView =
        model.bufferViews[featureTableProperty.arrayOffsetBufferView]; 
    if (!createMetaBuffer(
            model,
            arrayOffsetBufferView,
            instanceCount,
            type,
            arrayOffsetMetaBuffer)) {
      return std::nullopt;
    }
  }

  // string will always require offset buffer no matter what
  MetaBuffer stringOffsetMetaBuffer;
  bool isString = type & static_cast<uint32_t>(PropertyType::String); 
  if (isString) {
    if (featureTableProperty.stringOffsetBufferView < 0 ||
        featureTableProperty.stringOffsetBufferView >=
            model.bufferViews.size()) {
      return std::nullopt;
    }

    const BufferView& stringOffsetBufferView =
        model.bufferViews[featureTableProperty.arrayOffsetBufferView]; 
    if (!createMetaBuffer(
            model,
            stringOffsetBufferView,
            instanceCount,
            type,
            stringOffsetMetaBuffer)) {
      return std::nullopt;
    }
  }

  // dynamic array and string will require offset type here. Default to be uint32_t
  PropertyType offsetType = PropertyType::None;
  if (isDynamicArray || isString) 
  {
    offsetType = getOffsetType(featureTableProperty.offsetType);
    if (offsetType == PropertyType::None) {
      offsetType = PropertyType::Uint32; 
    }
  }

  return PropertyAccessorView(
      valueMetaBuffer,
      arrayOffsetMetaBuffer,
      stringOffsetMetaBuffer,
      offsetType,
      &classProperty,
      type,
      instanceCount);
}

/*static*/ size_t
PropertyAccessorView::getNumberPropertyTypeSize(uint32_t type) {
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

/*static*/ uint32_t
PropertyAccessorView::getPropertyType(const ClassProperty& property) {
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

/*static*/ bool PropertyAccessorView::createMetaBuffer(
    const Model &model,
    const BufferView& bufferView,
    size_t instanceCount,
    uint32_t type,
    MetaBuffer& metaBuffer) 
{
  size_t typeSize = getNumberPropertyTypeSize(type);
  size_t stride = typeSize;
  if (bufferView.byteStride && bufferView.byteStride > 0) {
    stride = *bufferView.byteStride;
  }

  const Buffer& buffer = model.buffers[bufferView.buffer];
  if (static_cast<size_t>(bufferView.byteOffset + bufferView.byteLength) >
      buffer.cesium.data.size()) {
    return false;
  }

  if (static_cast<size_t>(bufferView.byteOffset) +
          stride * (instanceCount - 1) + typeSize >
      buffer.cesium.data.size()) {
    return false;
  }

  metaBuffer.buffer = gsl::span<const std::byte>(
      reinterpret_cast<const std::byte*>(
          buffer.cesium.data.data() + bufferView.byteOffset),
      bufferView.byteLength);
  metaBuffer.stride = stride;

  return true;
}
} // namespace CesiumGltf