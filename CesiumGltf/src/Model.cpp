#include "CesiumGltf/Model.h"
#include "CesiumGltf/EXT_mesh_gpu_instancing.h"
#include "CesiumGltf/KHR_draco_mesh_compression.h"
#include <algorithm>

using namespace CesiumGltf;

namespace {
template <typename T>
size_t copyElements(std::vector<T>& to, std::vector<T>& from) {
  size_t out = to.size();
  to.resize(out + from.size());
  for (size_t i = 0; i < from.size(); ++i) {
    to[out + i] = std::move(from[i]);
  }

  return out;
}

void updateIndex(int32_t& index, size_t offset) {
  if (index == -1) {
    return;
  }
  index += int32_t(offset);
}
} // namespace

void Model::merge(Model&& rhs) {
  // TODO: we could generate this pretty easily if the glTF JSON schema made
  // it clear which index properties refer to which types of objects.

  // Copy all the source data into this instance.
  copyElements(this->extensionsUsed, rhs.extensionsUsed);
  std::sort(this->extensionsUsed.begin(), this->extensionsUsed.end());
  this->extensionsUsed.erase(
      std::unique(this->extensionsUsed.begin(), this->extensionsUsed.end()),
      this->extensionsUsed.end());

  copyElements(this->extensionsRequired, rhs.extensionsRequired);
  std::sort(this->extensionsRequired.begin(), this->extensionsRequired.end());
  this->extensionsRequired.erase(
      std::unique(
          this->extensionsRequired.begin(),
          this->extensionsRequired.end()),
      this->extensionsRequired.end());

  size_t firstAccessor = copyElements(this->accessors, rhs.accessors);
  size_t firstAnimation = copyElements(this->animations, rhs.animations);
  size_t firstBuffer = copyElements(this->buffers, rhs.buffers);
  size_t firstBufferView = copyElements(this->bufferViews, rhs.bufferViews);
  size_t firstCamera = copyElements(this->cameras, rhs.cameras);
  size_t firstImage = copyElements(this->images, rhs.images);
  size_t firstMaterial = copyElements(this->materials, rhs.materials);
  size_t firstMesh = copyElements(this->meshes, rhs.meshes);
  size_t firstNode = copyElements(this->nodes, rhs.nodes);
  size_t firstSampler = copyElements(this->samplers, rhs.samplers);
  size_t firstScene = copyElements(this->scenes, rhs.scenes);
  size_t firstSkin = copyElements(this->skins, rhs.skins);
  size_t firstTexture = copyElements(this->textures, rhs.textures);

  // Update the copied indices
  for (size_t i = firstAccessor; i < this->accessors.size(); ++i) {
    Accessor& accessor = this->accessors[i];
    updateIndex(accessor.bufferView, firstBufferView);

    if (accessor.sparse) {
      updateIndex(accessor.sparse.value().indices.bufferView, firstBufferView);
      updateIndex(accessor.sparse.value().values.bufferView, firstBufferView);
    }
  }

  for (size_t i = firstAnimation; i < this->animations.size(); ++i) {
    Animation& animation = this->animations[i];

    for (AnimationChannel& channel : animation.channels) {
      updateIndex(channel.sampler, firstSampler);
      updateIndex(channel.target.node, firstNode);
    }

    for (AnimationSampler& sampler : animation.samplers) {
      updateIndex(sampler.input, firstAccessor);
      updateIndex(sampler.output, firstAccessor);
    }
  }

  for (size_t i = firstBufferView; i < this->bufferViews.size(); ++i) {
    BufferView& bufferView = this->bufferViews[i];
    updateIndex(bufferView.buffer, firstBuffer);
  }

  for (size_t i = firstImage; i < this->images.size(); ++i) {
    Image& image = this->images[i];
    updateIndex(image.bufferView, firstBufferView);
  }

  for (size_t i = firstMesh; i < this->meshes.size(); ++i) {
    Mesh& mesh = this->meshes[i];

    for (MeshPrimitive& primitive : mesh.primitives) {
      updateIndex(primitive.indices, firstAccessor);
      updateIndex(primitive.material, firstMaterial);

      for (auto& attribute : primitive.attributes) {
        updateIndex(attribute.second, firstAccessor);
      }

      for (auto& target : primitive.targets) {
        for (auto& displacement : target) {
          updateIndex(displacement.second, firstAccessor);
        }
      }

      KHR_draco_mesh_compression* pDraco =
          primitive.getExtension<KHR_draco_mesh_compression>();
      if (pDraco) {
        updateIndex(pDraco->bufferView, firstBufferView);
      }
    }
  }

  for (size_t i = firstNode; i < this->nodes.size(); ++i) {
    Node& node = this->nodes[i];

    updateIndex(node.camera, firstCamera);
    updateIndex(node.skin, firstSkin);
    updateIndex(node.mesh, firstMesh);

    EXT_mesh_gpu_instancing* pInstancing =
        node.getExtension<EXT_mesh_gpu_instancing>();
    if (pInstancing) {
      for (auto& attribute : pInstancing->attributes) {
        updateIndex(attribute.second, firstAccessor);
      }
    }

    for (auto& nodeIndex : node.children) {
      updateIndex(nodeIndex, firstNode);
    }
  }

  for (size_t i = firstScene; i < this->scenes.size(); ++i) {
    Scene& currentScene = this->scenes[i];
    for (int32_t& node : currentScene.nodes) {
      updateIndex(node, firstNode);
    }
  }

  for (size_t i = firstSkin; i < this->skins.size(); ++i) {
    Skin& skin = this->skins[i];

    updateIndex(skin.inverseBindMatrices, firstAccessor);
    updateIndex(skin.skeleton, firstNode);

    for (int32_t& nodeIndex : skin.joints) {
      updateIndex(nodeIndex, firstNode);
    }
  }

  for (size_t i = firstTexture; i < this->textures.size(); ++i) {
    Texture& texture = this->textures[i];

    updateIndex(texture.sampler, firstSampler);
    updateIndex(texture.source, firstImage);
  }

  for (size_t i = firstMaterial; i < this->materials.size(); ++i) {
    Material& material = this->materials[i];

    if (material.normalTexture) {
      updateIndex(material.normalTexture.value().index, firstTexture);
    }
    if (material.occlusionTexture) {
      updateIndex(material.occlusionTexture.value().index, firstTexture);
    }
    if (material.pbrMetallicRoughness) {
      MaterialPBRMetallicRoughness& pbr = material.pbrMetallicRoughness.value();
      if (pbr.baseColorTexture) {
        updateIndex(pbr.baseColorTexture.value().index, firstTexture);
      }
      if (pbr.metallicRoughnessTexture) {
        updateIndex(pbr.metallicRoughnessTexture.value().index, firstTexture);
      }
    }
    if (material.emissiveTexture) {
      updateIndex(material.emissiveTexture.value().index, firstTexture);
    }
  }

  Scene* pThisDefaultScene = Model::getSafe(&this->scenes, this->scene);
  Scene* pRhsDefaultScene =
      Model::getSafe(&this->scenes, rhs.scene + int32_t(firstScene));

  if (!pThisDefaultScene) {
    this->scene = rhs.scene;
    updateIndex(this->scene, firstScene);
  } else if (pRhsDefaultScene) {
    // Create a new default scene that has all the root nodes in
    // the default scene of either model.
    Scene& newScene = this->scenes.emplace_back();

    // Refresh the scene pointers potentially invalidated by the above.
    pThisDefaultScene = Model::getSafe(&this->scenes, this->scene);
    pRhsDefaultScene =
        Model::getSafe(&this->scenes, rhs.scene + int32_t(firstScene));

    newScene.nodes = pThisDefaultScene->nodes;
    size_t originalNodeCount = newScene.nodes.size();
    newScene.nodes.resize(originalNodeCount + pRhsDefaultScene->nodes.size());
    std::copy(
        pRhsDefaultScene->nodes.begin(),
        pRhsDefaultScene->nodes.end(),
        newScene.nodes.begin() + int64_t(originalNodeCount));

    // No need to update indices because they've already been updated when
    // we copied them from rhs to this.

    this->scene = int32_t(this->scenes.size() - 1);
  }
}
