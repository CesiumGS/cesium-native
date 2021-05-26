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
} // namespace

namespace CesiumGltf {
bool PropertyAccessorView::getBoolean(size_t /*instance*/) const {
  return false;
}

std::string_view PropertyAccessorView::getString(size_t instance) const {
  size_t currentOffset =
      getOffsetFromOffsetBuffer(instance, _stringOffsetBuffer, _offsetType);
  size_t nextOffset =
      getOffsetFromOffsetBuffer(instance + 1, _stringOffsetBuffer, _offsetType);
  return std::string_view(
      reinterpret_cast<const char*>(_valueBuffer.buffer.data() + currentOffset),
      (nextOffset - currentOffset));
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
  if (valueBufferView.buffer >= model.buffers.size() ||
      valueBufferView.byteOffset < 0) {
    return std::nullopt;
  }

  bool isArray = type & static_cast<uint32_t>(PropertyType::Array);
  bool isFixedArray = isArray && classProperty.componentCount &&
                      classProperty.componentCount > 1;
  bool isString = type & static_cast<uint32_t>(PropertyType::String);
  bool isDynamicArray = isArray && !isFixedArray;
  size_t componentCount = 1;
  if (isFixedArray) {
    componentCount = classProperty.componentCount.value();
  }

  MetaBuffer valueMetaBuffer;
  if (isArray) {
    // make sure component type is defined
    PropertyType componentType = PropertyType::None;
    if (classProperty.componentType.isString()) {
      componentType =
          convertStringToPropertyType(classProperty.componentType.getString());
    } else {
      // TODO: check enum here
      componentType = PropertyType::None;
    }

    if (componentType == PropertyType::None) {
      return std::nullopt;
    }

    if (!createMetaBuffer(
            model,
            valueBufferView,
            instanceCount,
            componentCount,
            static_cast<uint32_t>(componentType),
            valueMetaBuffer)) {
      return std::nullopt;
    }
  } else {
    if (!createMetaBuffer(
            model,
            valueBufferView,
            instanceCount,
            1,
            type,
            valueMetaBuffer)) {
      return std::nullopt;
    }
  }

  // dynamic array and string will require offset type here. Default to be
  // uint32_t
  PropertyType offsetType = PropertyType::None;
  if (isDynamicArray || isString) {
    offsetType = getOffsetType(featureTableProperty.offsetType);
    if (offsetType == PropertyType::None) {
      offsetType = PropertyType::Uint32;
    }
  }

  // make sure dynamic array will have offset buffer
  MetaBuffer arrayOffsetMetaBuffer;
  if (isDynamicArray) {
    if (featureTableProperty.arrayOffsetBufferView < 0 ||
        featureTableProperty.arrayOffsetBufferView >=
            model.bufferViews.size()) {
      return std::nullopt;
    }

    // specs requires that the number of offset instances is instanceCount + 1
    const BufferView& arrayOffsetBufferView =
        model.bufferViews[featureTableProperty.arrayOffsetBufferView];
    if (!createMetaBuffer(
            model,
            arrayOffsetBufferView,
            instanceCount + 1,
            1,
            static_cast<uint32_t>(offsetType),
            arrayOffsetMetaBuffer)) {
      return std::nullopt;
    }
  }

  // string will always require offset buffer no matter what
  MetaBuffer stringOffsetMetaBuffer;
  if (isString) {
    if (featureTableProperty.stringOffsetBufferView < 0 ||
        featureTableProperty.stringOffsetBufferView >=
            model.bufferViews.size()) {
      return std::nullopt;
    }

    // specs requires that the number of offset instances is instanceCount + 1
    const BufferView& stringOffsetBufferView =
        model.bufferViews[featureTableProperty.stringOffsetBufferView];
    if (!createMetaBuffer(
            model,
            stringOffsetBufferView,
            instanceCount + 1,
            1,
            static_cast<uint32_t>(offsetType),
            stringOffsetMetaBuffer)) {
      return std::nullopt;
    }
  }

  return PropertyAccessorView(
      valueMetaBuffer,
      arrayOffsetMetaBuffer.buffer,
      stringOffsetMetaBuffer.buffer,
      offsetType,
      &classProperty,
      type,
      instanceCount);
}

/*static*/ size_t PropertyAccessorView::getOffsetFromOffsetBuffer(
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
    const Model& model,
    const BufferView& bufferView,
    size_t instanceCount,
    size_t componentCount,
    uint32_t type,
    MetaBuffer& metaBuffer) {
  size_t typeSize = getNumberPropertyTypeSize(type) * componentCount;
  size_t stride = typeSize;

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