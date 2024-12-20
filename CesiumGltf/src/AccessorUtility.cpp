#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorUtility.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/ExtensionExtMeshGpuInstancing.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Node.h>

#include <cstdint>
#include <string>

namespace CesiumGltf {
PositionAccessorType
getPositionAccessorView(const Model& model, const MeshPrimitive& primitive) {
  auto positionAttribute = primitive.attributes.find("POSITION");
  if (positionAttribute == primitive.attributes.end()) {
    return PositionAccessorType();
  }

  const Accessor* pAccessor =
      model.getSafe<Accessor>(&model.accessors, positionAttribute->second);
  if (!pAccessor || pAccessor->type != Accessor::Type::VEC3 ||
      pAccessor->componentType != Accessor::ComponentType::FLOAT) {
    return PositionAccessorType();
  }

  return PositionAccessorType(model, *pAccessor);
}

NormalAccessorType
getNormalAccessorView(const Model& model, const MeshPrimitive& primitive) {
  auto normalAttribute = primitive.attributes.find("NORMAL");
  if (normalAttribute == primitive.attributes.end()) {
    return NormalAccessorType();
  }

  const Accessor* pAccessor =
      model.getSafe<Accessor>(&model.accessors, normalAttribute->second);
  if (!pAccessor || pAccessor->type != Accessor::Type::VEC3 ||
      pAccessor->componentType != Accessor::ComponentType::FLOAT) {
    return NormalAccessorType();
  }

  return NormalAccessorType(model, *pAccessor);
}

namespace {
FeatureIdAccessorType getFeatureIdViewFromAccessorIndex(
    const Model& model,
    int32_t featureIdAccessor,
    bool instanceAttribute) {
  const Accessor* pAccessor =
      model.getSafe<Accessor>(&model.accessors, featureIdAccessor);
  if (!pAccessor || pAccessor->type != Accessor::Type::SCALAR ||
      pAccessor->normalized) {
    return FeatureIdAccessorType();
  }

  switch (pAccessor->componentType) {
  case Accessor::ComponentType::BYTE:
    return AccessorView<int8_t>(model, *pAccessor);
  case Accessor::ComponentType::UNSIGNED_BYTE:
    return AccessorView<uint8_t>(model, *pAccessor);
  case Accessor::ComponentType::SHORT:
    return AccessorView<int16_t>(model, *pAccessor);
  case Accessor::ComponentType::UNSIGNED_SHORT:
    return AccessorView<uint16_t>(model, *pAccessor);
  case Accessor::ComponentType::UNSIGNED_INT:
    // UNSIGNED_INT isn't supported for vertex attribute accessors, as described
    // in the implementation notes for EXT_mesh_features. But neither
    // EXT_instance_features nor EXT_mesh_gpu_instancing mention such a
    // restriction.
    if (instanceAttribute) {
      return AccessorView<uint32_t>(model, *pAccessor);
    } else {
      return FeatureIdAccessorType();
    }
  case Accessor::ComponentType::FLOAT:
    return AccessorView<float>(model, *pAccessor);
  default:
    return FeatureIdAccessorType();
  }
}
} // namespace

FeatureIdAccessorType getFeatureIdAccessorView(
    const Model& model,
    const MeshPrimitive& primitive,
    int32_t featureIdAttributeIndex) {
  const std::string attributeName =
      "_FEATURE_ID_" + std::to_string(featureIdAttributeIndex);
  auto featureId = primitive.attributes.find(attributeName);
  if (featureId == primitive.attributes.end()) {
    return FeatureIdAccessorType();
  }

  return getFeatureIdViewFromAccessorIndex(model, featureId->second, false);
}

FeatureIdAccessorType getFeatureIdAccessorView(
    const Model& model,
    const Node& node,
    int32_t featureIdAttributeIndex) {
  const auto* extInstancing =
      node.getExtension<ExtensionExtMeshGpuInstancing>();
  if (!extInstancing) {
    return FeatureIdAccessorType();
  }
  const std::string attributeName =
      "_FEATURE_ID_" + std::to_string(featureIdAttributeIndex);
  auto featureId = extInstancing->attributes.find(attributeName);
  if (featureId == extInstancing->attributes.end()) {
    return FeatureIdAccessorType();
  }
  return getFeatureIdViewFromAccessorIndex(model, featureId->second, true);
}

IndexAccessorType
getIndexAccessorView(const Model& model, const MeshPrimitive& primitive) {
  if (primitive.indices < 0) {
    return IndexAccessorType();
  }

  const Accessor* pAccessor =
      model.getSafe<Accessor>(&model.accessors, primitive.indices);
  if (!pAccessor || pAccessor->type != Accessor::Type::SCALAR ||
      pAccessor->normalized) {
    return AccessorView<uint8_t>();
  }

  switch (pAccessor->componentType) {
  case Accessor::ComponentType::UNSIGNED_BYTE:
    return AccessorView<uint8_t>(model, *pAccessor);
  case Accessor::ComponentType::UNSIGNED_SHORT:
    return AccessorView<uint16_t>(model, *pAccessor);
  case Accessor::ComponentType::UNSIGNED_INT:
    return AccessorView<uint32_t>(model, *pAccessor);
  default:
    return AccessorView<uint8_t>();
  }
}

TexCoordAccessorType getTexCoordAccessorView(
    const Model& model,
    const MeshPrimitive& primitive,
    int32_t textureCoordinateSetIndex) {
  const std::string texCoordName =
      "TEXCOORD_" + std::to_string(textureCoordinateSetIndex);
  auto texCoord = primitive.attributes.find(texCoordName);
  if (texCoord == primitive.attributes.end()) {
    return TexCoordAccessorType();
  }

  const Accessor* pAccessor =
      model.getSafe<Accessor>(&model.accessors, texCoord->second);
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

QuaternionAccessorType
getQuaternionAccessorView(const Model& model, const Accessor* pAccessor) {
  if (!pAccessor) {
    return QuaternionAccessorType();
  }
  switch (pAccessor->componentType) {
  case Accessor::ComponentType::BYTE:
    return AccessorView<AccessorTypes::VEC4<int8_t>>(model, *pAccessor);
    [[fallthrough]];
  case Accessor::ComponentType::UNSIGNED_BYTE:
    return AccessorView<AccessorTypes::VEC4<uint8_t>>(model, *pAccessor);
    [[fallthrough]];
  case Accessor::ComponentType::SHORT:
    return AccessorView<AccessorTypes::VEC4<int16_t>>(model, *pAccessor);
    [[fallthrough]];
  case Accessor::ComponentType::UNSIGNED_SHORT:
    return AccessorView<AccessorTypes::VEC4<uint16_t>>(model, *pAccessor);
    [[fallthrough]];
  case Accessor::ComponentType::FLOAT:
    return AccessorView<AccessorTypes::VEC4<float>>(model, *pAccessor);
  default:
    return QuaternionAccessorType();
  }
}

QuaternionAccessorType
getQuaternionAccessorView(const Model& model, int32_t accessorIndex) {
  const Accessor* pAccessor =
      model.getSafe<Accessor>(&model.accessors, accessorIndex);
  if (!pAccessor || pAccessor->type != Accessor::Type::VEC4) {
    return QuaternionAccessorType();
  }
  return getQuaternionAccessorView(model, pAccessor);
}
} // namespace CesiumGltf
