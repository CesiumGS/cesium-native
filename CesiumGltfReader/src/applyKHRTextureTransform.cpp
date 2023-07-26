#include "applyKHRTextureTransform.h"

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

  if (textureTransform.offset.size() < 2 || textureTransform.scale.size() < 2) {
    return;
  }

  float Rotation = static_cast<float>(textureTransform.rotation);

  if (Rotation == 0.0f) {
    float OffsetX = static_cast<float>(textureTransform.offset[0]);
    float OffsetY = static_cast<float>(textureTransform.offset[1]);
    float ScaleX = static_cast<float>(textureTransform.scale[0]);
    float ScaleY = static_cast<float>(textureTransform.scale[1]);

    glm::vec2* uvs = reinterpret_cast<glm::vec2*>(buffer.cesium.data.data());
    for (int i = 0; i < accessorView.size(); i++) {
      glm::vec2 uv = accessorView[i];
      uv.x = uv.x * ScaleX + OffsetX;
      uv.y = uv.y * ScaleY + OffsetY;
      *uvs++ = uv;
    }
  } else {
    glm::vec2 Offset(textureTransform.offset[0], textureTransform.offset[1]);
    glm::vec2 Scale(textureTransform.scale[0], textureTransform.scale[1]);
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
}
} // namespace

template <typename T>
void processTextureInfo(
    Model& model,
    MeshPrimitive& primitive,
    T& textureInfo) {
  if (!textureInfo) {
    return;
  }
  ExtensionKhrTextureTransform* pTextureTransform =
      textureInfo->getExtension<ExtensionKhrTextureTransform>();
  if (!pTextureTransform) {
    return;
  }
  int64_t texCoord = 0;
  if (pTextureTransform->texCoord) {
    texCoord = *pTextureTransform->texCoord;
  }
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
  Accessor& accessor = model.accessors.emplace_back(*pAccessor);
  const AccessorView<glm::vec2> accessorView(model, accessor);
  if (accessorView.status() == AccessorViewStatus::Valid) {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(static_cast<size_t>(pBufferView->byteLength));
    transformBufferView(accessorView, buffer, *pTextureTransform);
    accessor.bufferView = static_cast<int32_t>(model.bufferViews.size());
    BufferView& bufferView = model.bufferViews.emplace_back(*pBufferView);
    bufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    find->second = static_cast<int32_t>(model.accessors.size() - 1);
    textureInfo->extensions.erase(ExtensionKhrTextureTransform::ExtensionName);
  }
} // namespace

void applyKHRTextureTransform(Model& model) {
  for (Mesh& mesh : model.meshes) {
    for (MeshPrimitive& primitive : mesh.primitives) {
      Material* material = Model::getSafe(&model.materials, primitive.material);
      if (material) {
        processTextureInfo(
            model,
            primitive,
            material->pbrMetallicRoughness->baseColorTexture);
        processTextureInfo(
            model,
            primitive,
            material->pbrMetallicRoughness->metallicRoughnessTexture);
        processTextureInfo(model, primitive, material->normalTexture);
        processTextureInfo(model, primitive, material->occlusionTexture);
        processTextureInfo(model, primitive, material->emissiveTexture);
      }
    }
  }
}
} // namespace CesiumGltfReader
