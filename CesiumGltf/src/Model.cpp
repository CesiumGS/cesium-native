#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/Animation.h>
#include <CesiumGltf/AnimationChannel.h>
#include <CesiumGltf/AnimationSampler.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Class.h>
#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/Enum.h>
#include <CesiumGltf/ExtensionBufferViewExtMeshoptCompression.h>
#include <CesiumGltf/ExtensionCesiumPrimitiveOutline.h>
#include <CesiumGltf/ExtensionCesiumTileEdges.h>
#include <CesiumGltf/ExtensionExtMeshFeatures.h>
#include <CesiumGltf/ExtensionExtMeshGpuInstancing.h>
#include <CesiumGltf/ExtensionKhrDracoMeshCompression.h>
#include <CesiumGltf/ExtensionKhrTextureBasisu.h>
#include <CesiumGltf/ExtensionMeshPrimitiveExtStructuralMetadata.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/ExtensionTextureWebp.h>
#include <CesiumGltf/FeatureId.h>
#include <CesiumGltf/Image.h>
#include <CesiumGltf/Material.h>
#include <CesiumGltf/MaterialPBRMetallicRoughness.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Node.h>
#include <CesiumGltf/PropertyAttribute.h>
#include <CesiumGltf/PropertyTable.h>
#include <CesiumGltf/PropertyTableProperty.h>
#include <CesiumGltf/PropertyTexture.h>
#include <CesiumGltf/PropertyTextureProperty.h>
#include <CesiumGltf/Scene.h>
#include <CesiumGltf/Schema.h>
#include <CesiumGltf/Skin.h>
#include <CesiumGltf/Texture.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/ErrorList.h>

#include <fmt/format.h>
#include <glm/exponential.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/quaternion_double.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/norm.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

using namespace CesiumUtility;

namespace CesiumGltf {

namespace {

template <typename T>
size_t copyElements(std::vector<T>& to, std::vector<T>& from) {
  const size_t out = to.size();
  to.resize(out + from.size());
  for (size_t i = 0; i < from.size(); ++i) {
    to[out + i] = std::move(from[i]);
  }

  return out;
}

void updateIndex(int32_t& index, size_t offset) noexcept {
  if (index == -1) {
    return;
  }
  index += int32_t(offset);
}

void mergeSchemas(
    Schema& lhs,
    Schema& rhs,
    std::map<std::string, std::string>& classNameMap);

} // namespace

ErrorList Model::merge(Model&& rhs) {
  ErrorList result;

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

  const size_t firstAccessor = copyElements(this->accessors, rhs.accessors);
  const size_t firstAnimation = copyElements(this->animations, rhs.animations);
  const size_t firstBuffer = copyElements(this->buffers, rhs.buffers);
  const size_t firstBufferView =
      copyElements(this->bufferViews, rhs.bufferViews);
  const size_t firstCamera = copyElements(this->cameras, rhs.cameras);
  const size_t firstImage = copyElements(this->images, rhs.images);
  const size_t firstMaterial = copyElements(this->materials, rhs.materials);
  const size_t firstMesh = copyElements(this->meshes, rhs.meshes);
  const size_t firstNode = copyElements(this->nodes, rhs.nodes);
  const size_t firstSampler = copyElements(this->samplers, rhs.samplers);
  const size_t firstScene = copyElements(this->scenes, rhs.scenes);
  const size_t firstSkin = copyElements(this->skins, rhs.skins);
  const size_t firstTexture = copyElements(this->textures, rhs.textures);

  size_t firstPropertyTable = 0;
  size_t firstPropertyTexture = 0;
  size_t firstPropertyAttribute = 0;

  ExtensionModelExtStructuralMetadata* pRhsMetadata =
      rhs.getExtension<ExtensionModelExtStructuralMetadata>();
  if (pRhsMetadata) {
    ExtensionModelExtStructuralMetadata& metadata =
        this->addExtension<ExtensionModelExtStructuralMetadata>();

    if (metadata.schemaUri && pRhsMetadata->schemaUri &&
        *metadata.schemaUri != *pRhsMetadata->schemaUri) {
      // We can't merge schema URIs. So the thing to do here is download both
      // schemas and merge them. But for now we're just reporting an error.
      result.emplaceError("Cannot merge EXT_structural_metadata extensions "
                          "with different schemaUris.");
    } else if (pRhsMetadata->schemaUri) {
      metadata.schemaUri = pRhsMetadata->schemaUri;
    }

    std::map<std::string, std::string> classNameMap;

    if (metadata.schema && pRhsMetadata->schema) {
      mergeSchemas(*metadata.schema, *pRhsMetadata->schema, classNameMap);
    } else if (pRhsMetadata->schema) {
      metadata.schema = std::move(pRhsMetadata->schema);
    }

    firstPropertyTable =
        copyElements(metadata.propertyTables, pRhsMetadata->propertyTables);
    firstPropertyTexture =
        copyElements(metadata.propertyTextures, pRhsMetadata->propertyTextures);
    firstPropertyAttribute = copyElements(
        metadata.propertyAttributes,
        pRhsMetadata->propertyAttributes);

    for (size_t i = firstPropertyTable; i < metadata.propertyTables.size();
         ++i) {
      PropertyTable& propertyTable = metadata.propertyTables[i];
      auto it = classNameMap.find(propertyTable.classProperty);
      if (it != classNameMap.end()) {
        propertyTable.classProperty = it->second;
      }

      for (std::pair<const std::string, PropertyTableProperty>& pair :
           propertyTable.properties) {
        updateIndex(pair.second.values, firstBufferView);
        updateIndex(pair.second.arrayOffsets, firstBufferView);
        updateIndex(pair.second.stringOffsets, firstBufferView);
      }
    }

    for (size_t i = firstPropertyTexture; i < metadata.propertyTextures.size();
         ++i) {
      PropertyTexture& propertyTexture = metadata.propertyTextures[i];
      auto it = classNameMap.find(propertyTexture.classProperty);
      if (it != classNameMap.end()) {
        propertyTexture.classProperty = it->second;
      }

      for (std::pair<const std::string, PropertyTextureProperty>& pair :
           propertyTexture.properties) {
        updateIndex(pair.second.index, firstTexture);
      }
    }

    for (size_t i = firstPropertyAttribute;
         i < metadata.propertyAttributes.size();
         ++i) {
      PropertyAttribute& propertyAttribute = metadata.propertyAttributes[i];
      auto it = classNameMap.find(propertyAttribute.classProperty);
      if (it != classNameMap.end()) {
        propertyAttribute.classProperty = it->second;
      }
    }
  }

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

    ExtensionBufferViewExtMeshoptCompression* pMeshOpt =
        bufferView.getExtension<ExtensionBufferViewExtMeshoptCompression>();
    if (pMeshOpt) {
      updateIndex(pMeshOpt->buffer, firstBuffer);
    }
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

      ExtensionKhrDracoMeshCompression* pDraco =
          primitive.getExtension<ExtensionKhrDracoMeshCompression>();
      if (pDraco) {
        updateIndex(pDraco->bufferView, firstBufferView);
      }

      ExtensionMeshPrimitiveExtStructuralMetadata* pMetadata =
          primitive.getExtension<ExtensionMeshPrimitiveExtStructuralMetadata>();
      if (pMetadata) {
        for (int32_t& propertyTextureID : pMetadata->propertyTextures) {
          updateIndex(propertyTextureID, firstPropertyTexture);
        }

        for (int32_t& propertyAttributeID : pMetadata->propertyAttributes) {
          updateIndex(propertyAttributeID, firstPropertyAttribute);
        }
      }

      ExtensionExtMeshFeatures* pMeshFeatures =
          primitive.getExtension<ExtensionExtMeshFeatures>();
      if (pMeshFeatures) {
        for (FeatureId& featureId : pMeshFeatures->featureIds) {
          updateIndex(featureId.propertyTable, firstPropertyTable);

          if (featureId.texture) {
            updateIndex(featureId.texture->index, firstTexture);
          }
        }
      }

      ExtensionCesiumTileEdges* pTileEdges =
          primitive.getExtension<ExtensionCesiumTileEdges>();
      if (pTileEdges) {
        updateIndex(pTileEdges->left, firstAccessor);
        updateIndex(pTileEdges->bottom, firstAccessor);
        updateIndex(pTileEdges->right, firstAccessor);
        updateIndex(pTileEdges->top, firstAccessor);
      }

      ExtensionCesiumPrimitiveOutline* pPrimitiveOutline =
          primitive.getExtension<ExtensionCesiumPrimitiveOutline>();
      if (pPrimitiveOutline) {
        updateIndex(pPrimitiveOutline->indices, firstAccessor);
      }
    }
  }

  for (size_t i = firstNode; i < this->nodes.size(); ++i) {
    Node& node = this->nodes[i];

    updateIndex(node.camera, firstCamera);
    updateIndex(node.skin, firstSkin);
    updateIndex(node.mesh, firstMesh);

    for (auto& nodeIndex : node.children) {
      updateIndex(nodeIndex, firstNode);
    }

    ExtensionExtMeshGpuInstancing* pInstancing =
        node.getExtension<ExtensionExtMeshGpuInstancing>();
    if (pInstancing) {
      for (auto& pair : pInstancing->attributes) {
        updateIndex(pair.second, firstAccessor);
      }
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

    ExtensionKhrTextureBasisu* pKtx =
        texture.getExtension<ExtensionKhrTextureBasisu>();
    if (pKtx)
      updateIndex(pKtx->source, firstImage);

    ExtensionTextureWebp* pWebP = texture.getExtension<ExtensionTextureWebp>();
    if (pWebP)
      updateIndex(pWebP->source, firstImage);
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
    const size_t originalNodeCount = newScene.nodes.size();
    newScene.nodes.resize(originalNodeCount + pRhsDefaultScene->nodes.size());
    std::copy(
        pRhsDefaultScene->nodes.begin(),
        pRhsDefaultScene->nodes.end(),
        newScene.nodes.begin() + int64_t(originalNodeCount));

    // No need to update indices because they've already been updated when
    // we copied them from rhs to this.

    this->scene = int32_t(this->scenes.size() - 1);
  }

  return result;
}

namespace {
template <typename TCallback>
void forEachPrimitiveInMeshObject(
    const glm::dmat4x4& transform,
    const Model& model,
    const Node& node,
    const Mesh& mesh,
    TCallback& callback) {
  for (const MeshPrimitive& primitive : mesh.primitives) {
    callback(model, node, mesh, primitive, transform);
  }
}

std::optional<glm::dmat4x4> getNodeTransform(const CesiumGltf::Node& node) {
  if (!node.matrix.empty() && node.matrix.size() < 16) {
    return std::nullopt;
  }

  // clang-format off
  // This is column-major, just like glm and glTF
  static constexpr std::array<double, 16> identityMatrix = {
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0};
  // clang-format on

  const std::vector<double>& matrix = node.matrix;

  if (node.matrix.size() >= 16 && !std::equal(
                                      identityMatrix.begin(),
                                      identityMatrix.end(),
                                      matrix.begin())) {
    return glm::dmat4x4(
        glm::dvec4(matrix[0], matrix[1], matrix[2], matrix[3]),
        glm::dvec4(matrix[4], matrix[5], matrix[6], matrix[7]),
        glm::dvec4(matrix[8], matrix[9], matrix[10], matrix[11]),
        glm::dvec4(matrix[12], matrix[13], matrix[14], matrix[15]));
  }

  if (!node.translation.empty() || !node.rotation.empty() ||
      !node.scale.empty()) {
    glm::dmat4x4 translation(1.0);
    if (node.translation.size() >= 3) {
      translation[3] = glm::dvec4(
          node.translation[0],
          node.translation[1],
          node.translation[2],
          1.0);
    } else if (!node.translation.empty()) {
      return std::nullopt;
    }

    glm::dquat rotationQuat(1.0, 0.0, 0.0, 0.0);
    if (node.rotation.size() >= 4) {
      rotationQuat[0] = node.rotation[0];
      rotationQuat[1] = node.rotation[1];
      rotationQuat[2] = node.rotation[2];
      rotationQuat[3] = node.rotation[3];
    } else if (!node.rotation.empty()) {
      return std::nullopt;
    }

    glm::dmat4x4 scale(1.0);
    if (node.scale.size() >= 3) {
      scale[0].x = node.scale[0];
      scale[1].y = node.scale[1];
      scale[2].z = node.scale[2];
    } else if (!node.scale.empty()) {
      return std::nullopt;
    }

    return translation * glm::dmat4x4(rotationQuat) * scale;
  }

  return glm::dmat4x4(1.0);
}

template <typename TCallback>
void forEachPrimitiveInNodeObject(
    const glm::dmat4x4& transform,
    const Model& model,
    const Node& node,
    TCallback& callback) {

  // This should just call GltfUtilities::getNodeTransform, but it can't because
  // it's in CesiumGltfContent. We should merge these two libraries, but until
  // then, it's duplicated.
  glm::dmat4x4 nodeTransform =
      transform * getNodeTransform(node).value_or(glm::dmat4x4(1.0));

  const int32_t meshId = node.mesh;
  if (meshId >= 0 && size_t(meshId) < model.meshes.size()) {
    const Mesh& mesh = model.meshes[size_t(meshId)];
    forEachPrimitiveInMeshObject(nodeTransform, model, node, mesh, callback);
  }

  for (const int32_t childNodeId : node.children) {
    if (childNodeId >= 0 && size_t(childNodeId) < model.nodes.size()) {
      forEachPrimitiveInNodeObject(
          nodeTransform,
          model,
          model.nodes[size_t(childNodeId)],
          callback);
    }
  }
}

template <typename TCallback>
void forEachNode(
    const glm::dmat4x4& transform,
    const Model& model,
    const Node& node,
    TCallback& callback) {

  glm::dmat4x4 nodeTransform =
      transform * getNodeTransform(node).value_or(glm::dmat4x4(1.0));

  callback(model, node, nodeTransform);

  for (const int32_t childNodeId : node.children) {
    if (childNodeId >= 0 && size_t(childNodeId) < model.nodes.size()) {
      forEachNode(
          nodeTransform,
          model,
          model.nodes[size_t(childNodeId)],
          callback);
    }
  }
}

template <typename TCallback>
void forEachNodeInSceneObject(
    const Model& model,
    const Scene& scene,
    TCallback& callback) {
  for (int32_t node : scene.nodes) {
    const Node* pNode = model.getSafe(&model.nodes, node);
    if (!pNode)
      continue;

    callback(model, *pNode);
  }
}

} // namespace

void Model::forEachRootNodeInScene(
    int32_t sceneID,
    std::function<ForEachRootNodeInSceneCallback>&& callback) {
  return const_cast<const Model*>(this)->forEachRootNodeInScene(
      sceneID,
      [&callback](const Model& gltf, const Node& node) {
        callback(const_cast<Model&>(gltf), const_cast<Node&>(node));
      });
}

void Model::forEachRootNodeInScene(
    int32_t sceneID,
    std::function<ForEachRootNodeInSceneConstCallback>&& callback) const {
  if (sceneID >= 0) {
    // Use the user-specified scene if it exists.
    if (size_t(sceneID) < this->scenes.size()) {
      forEachNodeInSceneObject(*this, this->scenes[size_t(sceneID)], callback);
    }
  } else if (this->scene >= 0 && size_t(this->scene) < this->scenes.size()) {
    // Use the default scene
    forEachNodeInSceneObject(
        *this,
        this->scenes[size_t(this->scene)],
        callback);
  } else if (!this->scenes.empty()) {
    // There's no default, so use the first scene
    const Scene& defaultScene = this->scenes[0];
    forEachNodeInSceneObject(*this, defaultScene, callback);
  } else if (!this->nodes.empty()) {
    // No scenes at all, use the first node as the root node.
    callback(*this, this->nodes.front());
  }
}

void Model::forEachNodeInScene(
    int32_t sceneID,
    std::function<ForEachNodeInSceneCallback>&& callback) {
  return const_cast<const Model*>(this)->forEachNodeInScene(
      sceneID,
      [&callback](
          const Model& gltf,
          const Node& node,
          const glm::dmat4& transform) {
        callback(const_cast<Model&>(gltf), const_cast<Node&>(node), transform);
      });
}

void Model::forEachNodeInScene(
    int32_t sceneID,
    std::function<ForEachNodeInSceneConstCallback>&& callback) const {
  this->forEachRootNodeInScene(
      sceneID,
      [callback = std::move(callback)](const Model& model, const Node& node) {
        forEachNode(glm::dmat4x4(1.0), model, node, callback);
      });
}

void Model::forEachPrimitiveInScene(
    int32_t sceneID,
    std::function<ForEachPrimitiveInSceneCallback>&& callback) {
  return const_cast<const Model*>(this)->forEachPrimitiveInScene(
      sceneID,
      [&callback](
          const Model& gltf,
          const Node& node,
          const Mesh& mesh,
          const MeshPrimitive& primitive,
          const glm::dmat4& transform) {
        callback(
            const_cast<Model&>(gltf),
            const_cast<Node&>(node),
            const_cast<Mesh&>(mesh),
            const_cast<MeshPrimitive&>(primitive),
            transform);
      });
}

void Model::forEachPrimitiveInScene(
    int32_t sceneID,
    std::function<ForEachPrimitiveInSceneConstCallback>&& callback) const {
  bool anythingVisited = false;
  this->forEachRootNodeInScene(
      sceneID,
      [&anythingVisited,
       callback = std::move(callback)](const Model& model, const Node& node) {
        anythingVisited = true;
        forEachPrimitiveInNodeObject(glm::dmat4x4(1.0), model, node, callback);
      });

  if (!anythingVisited) {
    // No root nodes at all in this model, so enumerate all the meshes.
    for (const Mesh& mesh : this->meshes) {
      forEachPrimitiveInMeshObject(
          glm::dmat4x4(1.0),
          *this,
          Node(),
          mesh,
          callback);
    }
  }
}

namespace {
template <typename TIndex>
void addTriangleNormalToVertexNormals(
    const std::span<glm::vec3>& normals,
    const AccessorView<glm::vec3>& positionView,
    TIndex tIndex0,
    TIndex tIndex1,
    TIndex tIndex2) {

  // Add the triangle's normal to each vertex's accumulated normal.

  const uint32_t index0 = static_cast<uint32_t>(tIndex0);
  const uint32_t index1 = static_cast<uint32_t>(tIndex1);
  const uint32_t index2 = static_cast<uint32_t>(tIndex2);

  const glm::vec3& vertex0 = positionView[index0];
  const glm::vec3& vertex1 = positionView[index1];
  const glm::vec3& vertex2 = positionView[index2];

  const glm::vec3 triangleNormal =
      glm::cross(vertex1 - vertex0, vertex2 - vertex0);

  // Add the triangle normal to each vertex's accumulated normal. At the end
  // we will normalize the accumulated vertex normals to average.
  normals[index0] += triangleNormal;
  normals[index1] += triangleNormal;
  normals[index2] += triangleNormal;
}

template <typename TIndex, typename GetIndex>
bool accumulateNormals(
    int32_t meshPrimitiveMode,
    const std::span<glm::vec3>& normals,
    const AccessorView<glm::vec3>& positionView,
    int64_t numIndices,
    GetIndex getIndex) {

  switch (meshPrimitiveMode) {
  case MeshPrimitive::Mode::TRIANGLES:
    for (int64_t i = 2; i < numIndices; i += 3) {
      TIndex index0 = getIndex(i - 2);
      TIndex index1 = getIndex(i - 1);
      TIndex index2 = getIndex(i);

      addTriangleNormalToVertexNormals<TIndex>(
          normals,
          positionView,
          index0,
          index1,
          index2);
    }
    break;

  case MeshPrimitive::Mode::TRIANGLE_STRIP:
    for (int64_t i = 0; i < numIndices - 2; ++i) {
      TIndex index0;
      TIndex index1;
      TIndex index2;

      if (i % 2) {
        index0 = getIndex(i);
        index1 = getIndex(i + 2);
        index2 = getIndex(i + 1);
      } else {
        index0 = getIndex(i);
        index1 = getIndex(i + 1);
        index2 = getIndex(i + 2);
      }

      addTriangleNormalToVertexNormals<TIndex>(
          normals,
          positionView,
          index0,
          index1,
          index2);
    }
    break;

  case MeshPrimitive::Mode::TRIANGLE_FAN:
    if (numIndices < 3) {
      return false;
    }

    {
      TIndex index0 = getIndex(0);
      for (int64_t i = 2; i < numIndices; ++i) {
        TIndex index1 = getIndex(i - 1);
        TIndex index2 = getIndex(i);

        addTriangleNormalToVertexNormals<TIndex>(
            normals,
            positionView,
            index0,
            index1,
            index2);
      }
    }
    break;

  default:
    return false;
  }

  return true;
}

template <typename TIndex>
void generateSmoothNormals(
    Model& gltf,
    MeshPrimitive& primitive,
    const AccessorView<glm::vec3>& positionView,
    const std::optional<Accessor>& indexAccessor) {

  const size_t count = static_cast<size_t>(positionView.size());
  const size_t normalBufferStride = sizeof(glm::vec3);
  const size_t normalBufferSize = count * normalBufferStride;

  std::vector<std::byte> normalByteBuffer(normalBufferSize);
  std::span<glm::vec3> normals(
      reinterpret_cast<glm::vec3*>(normalByteBuffer.data()),
      count);

  // In the indexed case, the positions are accessed with the
  // indices from the indexView. Otherwise, the elements are
  // accessed directly.
  bool accumulationResult = false;
  if (indexAccessor) {
    CesiumGltf::AccessorView<TIndex> indexView(gltf, *indexAccessor);
    if (indexView.status() != AccessorViewStatus::Valid) {
      return;
    }

    accumulationResult = accumulateNormals<TIndex>(
        primitive.mode,
        normals,
        positionView,
        indexView.size(),
        [&indexView](int64_t index) { return indexView[index]; });

  } else {
    accumulationResult = accumulateNormals<TIndex>(
        primitive.mode,
        normals,
        positionView,
        int64_t(count),
        [](int64_t index) { return static_cast<TIndex>(index); });
  }

  if (!accumulationResult) {
    return;
  }

  // normalizes the accumulated vertex normals
  for (size_t i = 0; i < count; ++i) {
    const float lengthSquared = glm::length2(normals[i]);
    if (lengthSquared < 1e-8f) {
      normals[i] = glm::vec3(0.0f);
    } else {
      normals[i] /= glm::sqrt(lengthSquared);
    }
  }

  const size_t normalBufferId = gltf.buffers.size();
  Buffer& normalBuffer = gltf.buffers.emplace_back();
  normalBuffer.byteLength = static_cast<int64_t>(normalBufferSize);
  normalBuffer.cesium.data = std::move(normalByteBuffer);

  const size_t normalBufferViewId = gltf.bufferViews.size();
  BufferView& normalBufferView = gltf.bufferViews.emplace_back();
  normalBufferView.buffer = static_cast<int32_t>(normalBufferId);
  normalBufferView.byteLength = static_cast<int64_t>(normalBufferSize);
  normalBufferView.byteOffset = 0;
  normalBufferView.byteStride = static_cast<int64_t>(normalBufferStride);
  normalBufferView.target = BufferView::Target::ARRAY_BUFFER;

  const size_t normalAccessorId = gltf.accessors.size();
  Accessor& normalAccessor = gltf.accessors.emplace_back();
  normalAccessor.byteOffset = 0;
  normalAccessor.bufferView = static_cast<int32_t>(normalBufferViewId);
  normalAccessor.componentType = Accessor::ComponentType::FLOAT;
  normalAccessor.count = positionView.size();
  normalAccessor.type = Accessor::Type::VEC3;

  primitive.attributes.emplace("NORMAL", int32_t(normalAccessorId));
}

void generateSmoothNormals(
    Model& gltf,
    MeshPrimitive& primitive,
    const AccessorView<glm::vec3>& positionView,
    const std::optional<Accessor>& indexAccessor) {
  if (indexAccessor) {
    switch (indexAccessor->componentType) {
    case Accessor::ComponentType::UNSIGNED_BYTE:
      generateSmoothNormals<uint8_t>(
          gltf,
          primitive,
          positionView,
          indexAccessor);
      break;
    case Accessor::ComponentType::UNSIGNED_SHORT:
      generateSmoothNormals<uint16_t>(
          gltf,
          primitive,
          positionView,
          indexAccessor);
      break;
    case Accessor::ComponentType::UNSIGNED_INT:
      generateSmoothNormals<uint32_t>(
          gltf,
          primitive,
          positionView,
          indexAccessor);
      break;
    default:
      return;
    };
  } else {
    generateSmoothNormals<uint32_t>(
        gltf,
        primitive,
        positionView,
        std::nullopt);
  }
}
} // namespace

void Model::generateMissingNormalsSmooth() {
  forEachPrimitiveInScene(
      -1,
      [](Model& gltf,
         Node& /*node*/,
         Mesh& /*mesh*/,
         MeshPrimitive& primitive,
         const glm::dmat4& /*transform*/) {
        // if normals already exist, there is nothing to do
        auto normalIt = primitive.attributes.find("NORMAL");
        if (normalIt != primitive.attributes.end()) {
          return;
        }

        // if positions do not exist, we cannot create normals.
        auto positionIt = primitive.attributes.find("POSITION");
        if (positionIt == primitive.attributes.end()) {
          return;
        }

        const int32_t positionAccessorId = positionIt->second;
        const AccessorView<glm::vec3> positionView(gltf, positionAccessorId);
        if (positionView.status() != AccessorViewStatus::Valid) {
          return;
        }

        if (primitive.indices < 0 ||
            size_t(primitive.indices) >= gltf.accessors.size()) {
          generateSmoothNormals(gltf, primitive, positionView, std::nullopt);
        } else {
          Accessor& indexAccessor = gltf.accessors[size_t(primitive.indices)];
          generateSmoothNormals(gltf, primitive, positionView, indexAccessor);
        }
      });
}

void Model::addExtensionUsed(const std::string& extensionName) {
  if (!this->isExtensionUsed(extensionName)) {
    this->extensionsUsed.emplace_back(extensionName);
  }
}

void Model::addExtensionRequired(const std::string& extensionName) {
  this->addExtensionUsed(extensionName);

  if (!this->isExtensionRequired(extensionName)) {
    this->extensionsRequired.emplace_back(extensionName);
  }
}

void Model::removeExtensionUsed(const std::string& extensionName) {
  this->extensionsUsed.erase(
      std::remove(
          this->extensionsUsed.begin(),
          this->extensionsUsed.end(),
          extensionName),
      this->extensionsUsed.end());
}

void Model::removeExtensionRequired(const std::string& extensionName) {
  this->removeExtensionUsed(extensionName);

  this->extensionsRequired.erase(
      std::remove(
          this->extensionsRequired.begin(),
          this->extensionsRequired.end(),
          extensionName),
      this->extensionsRequired.end());
}

bool Model::isExtensionUsed(const std::string& extensionName) const noexcept {
  return std::find(
             this->extensionsUsed.begin(),
             this->extensionsUsed.end(),
             extensionName) != this->extensionsUsed.end();
}

bool Model::isExtensionRequired(
    const std::string& extensionName) const noexcept {
  return std::find(
             this->extensionsRequired.begin(),
             this->extensionsRequired.end(),
             extensionName) != this->extensionsRequired.end();
}

namespace {

template <typename T>
std::string findAvailableName(
    const std::unordered_map<std::string, T>& map,
    const std::string& name) {
  auto it = map.find(name);
  if (it == map.end())
    return name;

  // Name already exists in the map, so find a numbered name that doesn't.

  uint64_t number = 1;
  while (number < std::numeric_limits<uint64_t>::max()) {
    std::string newName = fmt::format("{}_{}", name, number);
    it = map.find(newName);
    if (it == map.end()) {
      return newName;
    }

    ++number;
  }

  // Realistically, this can't happen. It would mean we checked all 2^64
  // possible names and none of them are available.
  CESIUM_ASSERT(false);
  return name;
}

void mergeSchemas(
    Schema& lhs,
    Schema& rhs,
    std::map<std::string, std::string>& classNameMap) {
  if (!lhs.name)
    lhs.name = rhs.name;
  else if (rhs.name && *lhs.name != *rhs.name)
    lhs.name.emplace("Merged");

  if (!lhs.description)
    lhs.description = rhs.description;
  else if (rhs.description && *lhs.description != *rhs.description)
    lhs.description.emplace("This is a merged schema created by combining "
                            "together the schemas from multiple glTFs.");

  if (!lhs.version)
    lhs.version = rhs.version;
  else if (rhs.version && *lhs.version != *rhs.version)
    lhs.version.reset();

  std::map<std::string, std::string> enumNameMap;

  for (std::pair<const std::string, Enum>& pair : rhs.enums) {
    std::string availableName = findAvailableName(lhs.enums, pair.first);
    lhs.enums[availableName] = std::move(pair.second);
    if (availableName != pair.first)
      enumNameMap.emplace(pair.first, availableName);
  }

  for (std::pair<const std::string, Class>& pair : rhs.classes) {
    std::string availableName = findAvailableName(lhs.classes, pair.first);
    Class& klass = lhs.classes[availableName];
    klass = std::move(pair.second);

    if (availableName != pair.first)
      classNameMap.emplace(pair.first, availableName);

    // Remap enum names in class properties.
    for (std::pair<const std::string, ClassProperty>& propertyPair :
         klass.properties) {
      if (propertyPair.second.enumType) {
        auto it = enumNameMap.find(*propertyPair.second.enumType);
        if (it != enumNameMap.end()) {
          propertyPair.second.enumType = it->second;
        }
      }
    }
  }
}

} // namespace

} // namespace CesiumGltf
