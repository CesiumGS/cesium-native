#include "CesiumGltf/AccessorUtility.h"

#include "CesiumGltf/Model.h"

namespace CesiumGltf {
TexCoordAccessorType GetTexCoordAccessorView(
    const Model& model,
    const MeshPrimitive& primitive,
    int32_t textureCoordinateSetIndex) {
  const std::string texCoordName =
      "TEXCOORD_" + std::to_string(textureCoordinateSetIndex);
  auto texCoord = primitive.attributes.find(texCoordName);
  if (texCoord == primitive.attributes.end()) {
    return TexCoordAccessorType();
  }

  const Accessor* pAccessor = model.getSafe<Accessor>(
      &model.accessors,
      primitive.attributes.at(texCoordName));
  if (!pAccessor || pAccessor->type != Accessor::Type::VEC2) {
    return TexCoordAccessorType();
  }

  switch (pAccessor->componentType) {
  case Accessor::ComponentType::UNSIGNED_BYTE:
    if (pAccessor->normalized) {
      // Unsigned byte texcoords must be normalized.
      return AccessorView<AccessorTypes::VEC2<uint8_t>>(model, *pAccessor);
    }
    [[fallthrough]];
  case Accessor::ComponentType::UNSIGNED_SHORT:
    if (pAccessor->normalized) {
      // Unsigned short texcoords must be normalized.
      return AccessorView<AccessorTypes::VEC2<uint16_t>>(model, *pAccessor);
    }
    [[fallthrough]];
  case Accessor::ComponentType::FLOAT:
    return AccessorView<AccessorTypes::VEC2<float>>(model, *pAccessor);
  default:
    return TexCoordAccessorType();
  }
}

} // namespace CesiumGltf
