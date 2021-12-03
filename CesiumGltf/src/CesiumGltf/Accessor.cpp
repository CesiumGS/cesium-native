#include "CesiumGltf/Accessor.h"

#include "CesiumGltf/Model.h"

namespace CesiumGltf {
/*static*/ int8_t
Accessor::computeNumberOfComponents(const std::string& type) noexcept {
  if (type == CesiumGltf::Accessor::Type::SCALAR) {
    return 1;
  }
  if (type == CesiumGltf::Accessor::Type::VEC2) {
    return 2;
  }
  if (type == CesiumGltf::Accessor::Type::VEC3) {
    return 3;
  }
  if (type == CesiumGltf::Accessor::Type::VEC4) {
    return 4;
  }
  if (type == CesiumGltf::Accessor::Type::MAT2) {
    return 4;
  }
  if (type == CesiumGltf::Accessor::Type::MAT3) {
    return 9;
  }
  if (type == CesiumGltf::Accessor::Type::MAT4) {
    return 16;
  }
  // TODO Print a warning here!
  return 0;
}

/*static*/ int8_t
Accessor::computeByteSizeOfComponent(const int32_t componentType) noexcept {
  switch (componentType) {
  case CesiumGltf::Accessor::ComponentType::BYTE:
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE:
    return 1;
  case CesiumGltf::Accessor::ComponentType::SHORT:
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT:
    return 2;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_INT:
  case CesiumGltf::Accessor::ComponentType::FLOAT:
    return 4;
  default:
    // TODO Print a warning here!
    return 0;
  }
}

/*static*/ int64_t Accessor::computeByteSizeOfElement(
    const std::string& type,
    const int32_t componentType) noexcept {
  return int64_t{computeByteSizeOfComponent(componentType)} *
         int64_t{computeNumberOfComponents(type)};
}

/*static*/ int64_t Accessor::computeByteSize(
    const std::string& type,
    const int32_t componentType,
    const int64_t count) noexcept {
  return Accessor::computeByteSizeOfElement(type, componentType) * count;
}

/*static*/ int32_t
Accessor::computeIndexComponentType(const int64_t vertexCount) {
  // glTF 2.0 Spec
  // (https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html)
  //
  // Indices accessor MUST NOT contain the maximum possible value for the
  // component type used (i.e., 255 for unsigned bytes, 65535 for unsigned
  // shorts, 4294967295 for unsigned ints).
  //
  // Implementation Note:
  // The maximum values trigger primitive restart in some graphics APIs and
  // would require client implementations to rebuild the index buffer.

  if (vertexCount < 255) {
    return CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE;
  } else if (vertexCount < 65535) {
    return CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT;
  } else if (vertexCount < 4294967295) {
    return CesiumGltf::Accessor::ComponentType::UNSIGNED_INT;
  } else
    // TODO Print a warning here!
    return 0;
}

int8_t Accessor::computeNumberOfComponents() const noexcept {
  return Accessor::computeNumberOfComponents(this->type);
}

int8_t Accessor::computeByteSizeOfComponent() const noexcept {
  return Accessor::computeByteSizeOfComponent(this->componentType);
}

int64_t Accessor::computeByteSizeOfElement() const noexcept {
  return Accessor::computeByteSizeOfElement(this->type, this->componentType);
}

int64_t Accessor::computeByteSize() const noexcept {
  return Accessor::computeByteSize(
      this->type,
      this->componentType,
      this->count);
}

int64_t
Accessor::computeByteStride(const CesiumGltf::Model& model) const noexcept {
  const BufferView* pBufferView =
      Model::getSafe(&model.bufferViews, this->bufferView);
  if (!pBufferView) {
    return 0;
  }

  if (pBufferView->byteStride) {
    return pBufferView->byteStride.value();
  }
  return computeNumberOfComponents(this->type) *
         computeByteSizeOfComponent(this->componentType);
}
} // namespace CesiumGltf
