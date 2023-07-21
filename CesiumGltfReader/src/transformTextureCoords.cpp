#include "transformTextureCoords.h"

#include "CesiumGltf/ExtensionKhrTextureTransform.h"
#include "CesiumGltfReader/GltfReader.h"

#include <CesiumGltf/AccessorView.h>
using namespace CesiumGltf;
namespace CesiumGltfReader {
namespace {
void transformBufferView(
    const AccessorView<glm::vec2>& accessorView,
    Buffer& buffer,
    ExtensionKhrTextureTransform& textureTransform) {

  glm::vec2 Offset(textureTransform.offset[0], textureTransform.offset[1]);
  glm::vec2 Scale(textureTransform.scale[0], textureTransform.scale[1]);
  float Rotation = static_cast<float>(textureTransform.rotation);
  glm::mat3 translation = glm::mat3(1, 0, 0, 0, 1, 0, Offset.x, Offset.y, 1);
  glm::mat3 rotation = glm::mat3(
      cos(Rotation),
      sin(Rotation),
      0,
      -sin(Rotation),
      cos(Rotation),
      0,
      0,
      0,
      1);
  glm::mat3 scale = glm::mat3(Scale.x, 0, 0, 0, Scale.y, 0, 0, 0, 1);
  glm::mat3 matrix = translation * rotation * scale;

  glm::vec2* uvs = reinterpret_cast<glm::vec2*>(buffer.cesium.data.data());

  for (int i = 0; i < accessorView.size(); i++) {
    *uvs++ = glm::vec2((matrix * glm::vec3(accessorView[i], 1)));
  }
}
} // namespace

void processTextureInfo(
    Model& model,
    MeshPrimitive& primitive,
    TextureInfo& textureInfo) {
  ExtensionKhrTextureTransform* pTextureTransform =
      textureInfo.getExtension<ExtensionKhrTextureTransform>();
  if (pTextureTransform) {
    int64_t texCoord = 0;
    if (pTextureTransform->texCoord) {
      texCoord = *pTextureTransform->texCoord;
    }
    auto find =
        primitive.attributes.find("TEXCOORD_" + std::to_string(texCoord));
    if (find != primitive.attributes.end()) {
      const Accessor* pAccessor =
          Model::getSafe(&model.accessors, find->second);
      if (pAccessor) {
        const BufferView* pBufferView =
            Model::getSafe(&model.bufferViews, pAccessor->bufferView);
        if (pBufferView) {
          Accessor& accessor = model.accessors.emplace_back(*pAccessor);
          Buffer& buffer = model.buffers.emplace_back();
          buffer.cesium.data.resize(
              static_cast<size_t>(pBufferView->byteLength));
          const AccessorView<glm::vec2> accessorView(model, accessor);
          if (accessorView.status() == AccessorViewStatus::Valid) {
            transformBufferView(accessorView, buffer, *pTextureTransform);
            accessor.bufferView =
                static_cast<int32_t>(model.bufferViews.size());
            BufferView& bufferView =
                model.bufferViews.emplace_back(*pBufferView);
            bufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
            find->second = static_cast<int32_t>(model.accessors.size() - 1);
            textureInfo.extensions.erase(
                ExtensionKhrTextureTransform::ExtensionName);
          }
        }
      }
    }
  }
} // namespace

void transformTexture(Model& model) {
  for (Mesh& mesh : model.meshes) {
    for (MeshPrimitive& primitive : mesh.primitives) {
      Material* pMaterial =
          Model::getSafe(&model.materials, primitive.material);
      if (pMaterial) {
        if (pMaterial->pbrMetallicRoughness) {
          std::optional<TextureInfo>& textureInfo =
              pMaterial->pbrMetallicRoughness->baseColorTexture;
          if (textureInfo) {
            processTextureInfo(model, primitive, *textureInfo);
          }
        }
      }
    }
  }
}
} // namespace CesiumGltfReader
