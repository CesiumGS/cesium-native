#include "applyKhrTextureTransform.h"

#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/ExtensionKhrTextureTransform.h>
#include <CesiumGltf/KhrTextureTransform.h>
#include <CesiumGltf/Material.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/TextureInfo.h>

#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_float2.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace CesiumGltf;

namespace CesiumGltfReader {
namespace {
bool transformBufferView(
    const AccessorView<glm::vec2>& accessorView,
    std::vector<std::byte>& data,
    const ExtensionKhrTextureTransform& textureTransformExtension) {
  KhrTextureTransform textureTransform(textureTransformExtension);
  if (textureTransform.status() != KhrTextureTransformStatus::Valid) {
    return false;
  }

  glm::vec2* transformedUvs = reinterpret_cast<glm::vec2*>(data.data());

  for (int i = 0; i < accessorView.size(); i++) {
    const glm::vec2& uv = accessorView[i];
    glm::dvec2 transformedUv = textureTransform.applyTransform(uv.x, uv.y);
    *transformedUvs = glm::vec2(transformedUv.x, transformedUv.y);
    transformedUvs++;
  }

  return true;
}

template <typename T>
void processTextureInfo(
    Model& model,
    MeshPrimitive& primitive,
    std::optional<T>& maybeTextureInfo) {
  static_assert(std::is_base_of<TextureInfo, T>::value);
  if (!maybeTextureInfo) {
    return;
  }

  const TextureInfo& textureInfo = *maybeTextureInfo;
  const ExtensionKhrTextureTransform* pTextureTransform =
      textureInfo.getExtension<ExtensionKhrTextureTransform>();
  if (!pTextureTransform) {
    return;
  }

  int64_t texCoord = pTextureTransform->texCoord.value_or(textureInfo.texCoord);
  auto find = primitive.attributes.find("TEXCOORD_" + std::to_string(texCoord));
  if (find == primitive.attributes.end()) {
    return;
  }

  const Accessor* pAccessor = Model::getSafe(&model.accessors, find->second);
  if (!pAccessor) {
    return;
  }

  const BufferView* pBufferView =
      Model::getSafe(&model.bufferViews, pAccessor->bufferView);
  if (!pBufferView) {
    return;
  }

  const AccessorView<glm::vec2> accessorView(model, *pAccessor);
  if (accessorView.status() != AccessorViewStatus::Valid) {
    return;
  }

  std::vector<std::byte> data(static_cast<size_t>(pBufferView->byteLength));
  bool success = transformBufferView(accessorView, data, *pTextureTransform);
  if (!success) {
    return;
  }

  Buffer& buffer = model.buffers.emplace_back();
  buffer.byteLength = static_cast<int64_t>(data.size());
  buffer.cesium.data = std::move(data);

  BufferView& bufferView = model.bufferViews.emplace_back(*pBufferView);
  bufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  bufferView.byteLength = buffer.byteLength;
  bufferView.byteOffset = 0;

  Accessor& accessor = model.accessors.emplace_back(*pAccessor);
  accessor.bufferView = static_cast<int32_t>(model.bufferViews.size() - 1);
  accessor.byteOffset = 0;
  accessor.count = accessorView.size();
  accessor.type = Accessor::Type::VEC2;
  accessor.componentType = Accessor::ComponentType::FLOAT;

  find->second = static_cast<int32_t>(model.accessors.size() - 1);

  // Erase the extension so it is not re-applied by client implementations.
  maybeTextureInfo->extensions.erase(
      ExtensionKhrTextureTransform::ExtensionName);
}
} // namespace

void applyKhrTextureTransform(Model& model) {
  for (Mesh& mesh : model.meshes) {
    for (MeshPrimitive& primitive : mesh.primitives) {
      Material* material = Model::getSafe(&model.materials, primitive.material);
      if (material) {
        if (material->pbrMetallicRoughness) {
          processTextureInfo(
              model,
              primitive,
              material->pbrMetallicRoughness->baseColorTexture);
          processTextureInfo(
              model,
              primitive,
              material->pbrMetallicRoughness->metallicRoughnessTexture);
        }
        processTextureInfo(model, primitive, material->normalTexture);
        processTextureInfo(model, primitive, material->occlusionTexture);
        processTextureInfo(model, primitive, material->emissiveTexture);
      }
    }
  }

  model.removeExtensionRequired(ExtensionKhrTextureTransform::ExtensionName);
}
} // namespace CesiumGltfReader
