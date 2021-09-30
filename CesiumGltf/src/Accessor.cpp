#include "CesiumGltf/Accessor.h"

#include "CesiumGltf/Model.h"

using namespace CesiumGltf;

/*static*/ int8_t
Accessor::computeNumberOfComponents(CesiumGltf::Accessor::Type type) noexcept {
  switch (type) {
  case CesiumGltf::Accessor::Type::SCALAR:
    return 1;
  case CesiumGltf::Accessor::Type::VEC2:
    return 2;
  case CesiumGltf::Accessor::Type::VEC3:
    return 3;
  case CesiumGltf::Accessor::Type::VEC4:
  case CesiumGltf::Accessor::Type::MAT2:
    return 4;
  case CesiumGltf::Accessor::Type::MAT3:
    return 9;
  case CesiumGltf::Accessor::Type::MAT4:
    return 16;
  default:
    return 0;
  }
}

/*static*/ int8_t Accessor::computeByteSizeOfComponent(
    CesiumGltf::Accessor::ComponentType componentType) noexcept {
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
    return 0;
  }
}

int8_t Accessor::computeNumberOfComponents() const noexcept {
  return Accessor::computeNumberOfComponents(this->type);
}

int8_t Accessor::computeByteSizeOfComponent() const noexcept {
  return Accessor::computeByteSizeOfComponent(this->componentType);
}

int64_t Accessor::computeBytesPerVertex() const noexcept {
  return this->computeByteSizeOfComponent() * this->computeNumberOfComponents();
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
