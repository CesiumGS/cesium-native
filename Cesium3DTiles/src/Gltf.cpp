#include "Cesium3DTiles/Gltf.h"
#include <glm/gtc/quaternion.hpp>

namespace Cesium3DTiles {
/*static*/ void Gltf::forEachPrimitiveInScene(
    CesiumGltf::Model& gltf,
    int sceneID,
    std::function<ForEachPrimitiveInSceneCallback>&& callback) {
  return Gltf::forEachPrimitiveInScene(
      const_cast<const CesiumGltf::Model&>(gltf),
      sceneID,
      [&callback](
          const CesiumGltf::Model& gltf_,
          const CesiumGltf::Node& node,
          const CesiumGltf::Mesh& mesh,
          const CesiumGltf::MeshPrimitive& primitive,
          const glm::dmat4& transform) {
        callback(
            const_cast<CesiumGltf::Model&>(gltf_),
            const_cast<CesiumGltf::Node&>(node),
            const_cast<CesiumGltf::Mesh&>(mesh),
            const_cast<CesiumGltf::MeshPrimitive&>(primitive),
            transform);
      });
}

static void forEachPrimitiveInMeshObject(
    const glm::dmat4x4& transform,
    const CesiumGltf::Model& model,
    const CesiumGltf::Node& node,
    const CesiumGltf::Mesh& mesh,
    std::function<Gltf::ForEachPrimitiveInSceneConstCallback>& callback) {
  for (const CesiumGltf::MeshPrimitive& primitive : mesh.primitives) {
    callback(model, node, mesh, primitive, transform);
  }
}

static void forEachPrimitiveInNodeObject(
    const glm::dmat4x4& transform,
    const CesiumGltf::Model& model,
    const CesiumGltf::Node& node,
    std::function<Gltf::ForEachPrimitiveInSceneConstCallback>& callback) {

  static constexpr std::array<double, 16> identityMatrix = {
      1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      1.0};

  glm::dmat4x4 nodeTransform = transform;
  const std::vector<double>& matrix = node.matrix;

  if (node.matrix.size() == 16 &&
      !std::equal(matrix.begin(), matrix.end(), identityMatrix.begin())) {

    const glm::dmat4& nodeTransformGltf =
        *reinterpret_cast<const glm::dmat4*>(matrix.data());

    nodeTransform = nodeTransform * nodeTransformGltf;
  } else if (
      !node.translation.empty() || !node.rotation.empty() ||
      !node.scale.empty()) {

    glm::dmat4 translation(1.0);
    if (node.translation.size() == 3) {
      translation[3] = glm::dvec4(
          node.translation[0],
          node.translation[1],
          node.translation[2],
          1.0);
    }

    glm::dquat rotationQuat(1.0, 0.0, 0.0, 0.0);
    if (node.rotation.size() == 4) {
      rotationQuat[0] = node.rotation[0];
      rotationQuat[1] = node.rotation[1];
      rotationQuat[2] = node.rotation[2];
      rotationQuat[3] = node.rotation[3];
    }

    glm::dmat4 scale(1.0);
    if (node.scale.size() == 3) {
      scale[0].x = node.scale[0];
      scale[1].y = node.scale[1];
      scale[2].z = node.scale[2];
    }

    nodeTransform =
        nodeTransform * translation * glm::dmat4(rotationQuat) * scale;
  }

  int meshId = node.mesh;
  if (meshId >= 0 && meshId < static_cast<int>(model.meshes.size())) {
    const CesiumGltf::Mesh& mesh = model.meshes[static_cast<size_t>(meshId)];
    forEachPrimitiveInMeshObject(nodeTransform, model, node, mesh, callback);
  }

  for (int childNodeId : node.children) {
    if (childNodeId >= 0 &&
        childNodeId < static_cast<int>(model.nodes.size())) {
      forEachPrimitiveInNodeObject(
          nodeTransform,
          model,
          model.nodes[static_cast<size_t>(childNodeId)],
          callback);
    }
  }
}

static void forEachPrimitiveInSceneObject(
    const glm::dmat4x4& transform,
    const CesiumGltf::Model& model,
    const CesiumGltf::Scene& scene,
    std::function<Gltf::ForEachPrimitiveInSceneConstCallback>& callback) {
  for (int nodeID : scene.nodes) {
    if (nodeID >= 0 && nodeID < static_cast<int>(model.nodes.size())) {
      forEachPrimitiveInNodeObject(
          transform,
          model,
          model.nodes[static_cast<size_t>(nodeID)],
          callback);
    }
  }
}

/*static*/ void Gltf::forEachPrimitiveInScene(
    const CesiumGltf::Model& gltf,
    int sceneID,
    std::function<Gltf::ForEachPrimitiveInSceneConstCallback>&& callback) {
  glm::dmat4x4 rootTransform(1.0);

  if (sceneID >= 0) {
    // Use the user-specified scene if it exists.
    if (sceneID < static_cast<int>(gltf.scenes.size())) {
      forEachPrimitiveInSceneObject(
          rootTransform,
          gltf,
          gltf.scenes[static_cast<size_t>(sceneID)],
          callback);
    }
  } else if (
      gltf.scene >= 0 &&
      gltf.scene < static_cast<int32_t>(gltf.scenes.size())) {
    // Use the default scene
    forEachPrimitiveInSceneObject(
        rootTransform,
        gltf,
        gltf.scenes[static_cast<size_t>(gltf.scene)],
        callback);
  } else if (!gltf.scenes.empty()) {
    // There's no default, so use the first scene
    const CesiumGltf::Scene& defaultScene = gltf.scenes[0];
    forEachPrimitiveInSceneObject(rootTransform, gltf, defaultScene, callback);
  } else if (!gltf.nodes.empty()) {
    // No scenes at all, use the first node as the root node.
    forEachPrimitiveInNodeObject(rootTransform, gltf, gltf.nodes[0], callback);
  } else if (!gltf.meshes.empty()) {
    // No nodes either, show all the meshes.
    for (const CesiumGltf::Mesh& mesh : gltf.meshes) {
      forEachPrimitiveInMeshObject(
          rootTransform,
          gltf,
          CesiumGltf::Node(),
          mesh,
          callback);
    }
  }
}

} // namespace Cesium3DTiles
