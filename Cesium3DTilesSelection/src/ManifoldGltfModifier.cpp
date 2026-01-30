#include "Cesium3DTilesSelection/GltfModifier.h"
#include "CesiumGltf/Accessor.h"
#include "CesiumGltf/AccessorView.h"
#include "CesiumGltf/BufferView.h"

#include <Cesium3DTilesSelection/ManifoldGltfModifier.h>

#include <manifold/common.h>
#include <manifold/manifold.h>

using namespace CesiumGltf;

namespace Cesium3DTilesSelection {
CesiumAsync::Future<std::optional<GltfModifierOutput>>
ManifoldGltfModifier::apply(GltfModifierInput&& input) {
  manifold::MeshGL64 boxMeshGl;
  boxMeshGl.triVerts = std::vector<uint64_t>{
      1, 5, 0, 0, 5, 4, 6, 2, 7, 7, 2, 3, 3, 0, 7, 7, 0, 4,
      7, 4, 6, 6, 4, 5, 6, 5, 2, 2, 5, 1, 2, 1, 3, 3, 1, 0};
  boxMeshGl.vertProperties = {
      this->box.minimumX, this->box.minimumY, this->box.minimumZ,
      this->box.maximumX, this->box.minimumY, this->box.minimumZ,
      this->box.maximumX, this->box.maximumY, this->box.minimumZ,
      this->box.minimumX, this->box.maximumY, this->box.minimumZ,
      this->box.minimumX, this->box.minimumY, this->box.maximumZ,
      this->box.maximumX, this->box.minimumY, this->box.maximumZ,
      this->box.maximumX, this->box.maximumY, this->box.maximumZ,
      this->box.minimumX, this->box.maximumY, this->box.maximumZ};
  boxMeshGl.numProp = 3;
  manifold::Manifold boxManifold(boxMeshGl);
  // manifold::Manifold boxManifold = manifold::Manifold::Cube(
  //     manifold::vec3(this->box.lengthX, this->box.lengthY,
  //     this->box.lengthZ));

  GltfModifierOutput output;

  const CesiumGltf::Model& model = input.previousModel;
  output.modifiedModel = CesiumGltf::Model(model);
  CesiumGltf::Model& newModel = output.modifiedModel;
  newModel.meshes.clear();
  newModel.buffers.clear();
  newModel.buffers.emplace_back();
  newModel.accessors.clear();
  newModel.bufferViews.clear();
  newModel.meshes.reserve(model.meshes.size());

  for (size_t meshIdx = 0; meshIdx < model.meshes.size(); meshIdx++) {
    const Mesh& mesh = model.meshes[meshIdx];
    manifold::MeshGL64 manifoldMesh;
    for (size_t primitiveIdx = 0; primitiveIdx < mesh.primitives.size();
         primitiveIdx++) {
      const MeshPrimitive& primitive = mesh.primitives[primitiveIdx];
      std::vector<int32_t> attributeIndices;
      int32_t positionIndex = -1;
      size_t vertexCount = 0;
      int64_t stride = 0;

      for (const std::pair<const std::string, int32_t>& attribute :
           primitive.attributes) {
        const Accessor& attrAccessor =
            Model::getSafe(model.accessors, attribute.second);
        if (attrAccessor.componentType != Accessor::ComponentType::FLOAT) {
          continue;
        }

        attributeIndices.push_back(attribute.second);
        if (attribute.first == "POSITION") {
          positionIndex = attribute.second;
          const Accessor& positionAccessor =
              Model::getSafe(model.accessors, attribute.second);
          vertexCount = static_cast<size_t>(positionAccessor.count);
        }

        stride += attrAccessor.computeNumberOfComponents();
      }

      if (positionIndex < 0) {
        continue;
      }

      AccessorView indicesAccessor =
          AccessorView<uint32_t>(model, primitive.indices);
      size_t vertOffset = manifoldMesh.vertProperties.size();
      manifoldMesh.vertProperties.resize(
          manifoldMesh.vertProperties.size() + (size_t)stride * vertexCount,
          0.0f);
      manifoldMesh.numProp = static_cast<uint64_t>(stride);

      AccessorView positionAccessor =
          AccessorView<glm::vec3>(model, positionIndex);
      assert(positionAccessor.status() == AccessorViewStatus::Valid);
      for (int64_t i = 0; i < positionAccessor.size(); i++) {
        manifoldMesh.vertProperties[vertOffset + (size_t)(i * stride)] =
            positionAccessor[i].x;
        manifoldMesh.vertProperties[vertOffset + (size_t)(i * stride) + 1] =
            positionAccessor[i].y;
        manifoldMesh.vertProperties[vertOffset + (size_t)(i * stride) + 2] =
            positionAccessor[i].z;
      }

      size_t offset = 0;
      for (size_t i = 0; i < attributeIndices.size(); i++) {
        if (attributeIndices[i] == positionIndex) {
          continue;
        }
        Accessor accessor =
            Model::getSafe(model.accessors, attributeIndices[i]);
        AccessorView<float> accessorView(model, attributeIndices[i]);
        int64_t numComponents = accessor.computeNumberOfComponents();
        for (int64_t j = 0; j < accessor.count; j++) {
          for (int64_t k = 0; k < numComponents; k++) {
            manifoldMesh.vertProperties
                [vertOffset + (size_t)(j * stride) + offset + (size_t)k] =
                accessorView[j * numComponents + k];
          }
        }
        offset += static_cast<size_t>(numComponents);
      }

      size_t triOffset = manifoldMesh.triVerts.size();
      for (int64_t i = 0; i < indicesAccessor.size(); i++) {
        manifoldMesh.triVerts.push_back(indicesAccessor[i]);
      }

      manifoldMesh.runIndex.push_back(triOffset);
      manifoldMesh.runOriginalID.push_back(static_cast<uint32_t>(primitiveIdx));
    }

    // manifoldMesh.Merge();
    manifold::Manifold manifold(manifoldMesh);
    manifold::Manifold resultManifold =
        manifold.Boolean(boxManifold, manifold::OpType::Subtract);

    manifold::MeshGL64 resultMesh = resultManifold.GetMeshGL64();

    // Entire mesh clipped
    if (resultMesh.triVerts.empty()) {
      if (!newModel.nodes.empty()) {
        for (Node& node : newModel.nodes) {
          node.mesh = -1;
        }
      }

      return input.asyncSystem.createResolvedFuture(
          std::optional<GltfModifierOutput>(output));
    }

    CesiumGltf::Buffer& outputBuffer = newModel.buffers[0];

    size_t outIndicesSize = resultMesh.triVerts.size() * sizeof(uint32_t);
    size_t outIndicesStart = outputBuffer.cesium.data.size();
    outputBuffer.cesium.data.resize(
        outputBuffer.cesium.data.size() + outIndicesSize);
    for (size_t i = 0; i < resultMesh.triVerts.size(); i++) {
      uint32_t idx = (uint32_t)resultMesh.triVerts[i];
      std::memcpy(
          outputBuffer.cesium.data.data() + outIndicesStart +
              (i * sizeof(uint32_t)),
          &idx,
          sizeof(uint32_t));
    }
    CesiumGltf::BufferView indicesBufferView;
    indicesBufferView.buffer = 0;
    indicesBufferView.byteLength = (int64_t)outIndicesSize;
    indicesBufferView.byteOffset = (int64_t)outIndicesStart;
    indicesBufferView.target = BufferView::Target::ELEMENT_ARRAY_BUFFER;
    int32_t indicesBufferViewIdx = (int32_t)newModel.bufferViews.size();
    newModel.bufferViews.emplace_back(std::move(indicesBufferView));

    size_t outVerticesSize = resultMesh.vertProperties.size() * sizeof(float);
    size_t outVerticesStart = outputBuffer.cesium.data.size();
    outputBuffer.cesium.data.resize(
        outputBuffer.cesium.data.size() + outVerticesSize);
    for (size_t i = 0; i < resultMesh.vertProperties.size(); i++) {
      float prop = (float)resultMesh.vertProperties[i];
      std::memcpy(
          outputBuffer.cesium.data.data() + outVerticesStart +
              (i * sizeof(float)),
          &prop,
          sizeof(float));
    }
    CesiumGltf::BufferView verticesBufferView;
    verticesBufferView.buffer = 0;
    verticesBufferView.byteLength = (int64_t)outVerticesSize;
    verticesBufferView.byteOffset = (int64_t)outVerticesStart;
    verticesBufferView.byteStride = resultMesh.numProp * sizeof(float);
    verticesBufferView.target = BufferView::Target::ARRAY_BUFFER;
    int32_t verticesBufferViewIdx = (int32_t)newModel.bufferViews.size();
    newModel.bufferViews.emplace_back(std::move(verticesBufferView));

    outputBuffer.byteLength = (int64_t)outputBuffer.cesium.data.size();

    glm::dvec3 minPos = resultMesh.vertProperties.empty()
                            ? glm::dvec3()
                            : glm::dvec3(
                                  resultMesh.vertProperties[0],
                                  resultMesh.vertProperties[1],
                                  resultMesh.vertProperties[2]);
    glm::dvec3 maxPos = resultMesh.vertProperties.empty()
                            ? glm::dvec3()
                            : glm::dvec3(
                                  resultMesh.vertProperties[0],
                                  resultMesh.vertProperties[1],
                                  resultMesh.vertProperties[2]);

    for (size_t i = 0; i < resultMesh.vertProperties.size();
         i += resultMesh.numProp) {
      glm::dvec3 pos(
          resultMesh.vertProperties[i],
          resultMesh.vertProperties[i + 1],
          resultMesh.vertProperties[i + 2]);
      minPos.x = std::min(minPos.x, pos.x);
      minPos.y = std::min(minPos.y, pos.y);
      minPos.z = std::min(minPos.z, pos.z);
      maxPos.x = std::max(maxPos.x, pos.x);
      maxPos.y = std::max(maxPos.y, pos.y);
      maxPos.z = std::max(maxPos.z, pos.z);
    }

    CesiumGltf::Mesh& newMesh = newModel.meshes.emplace_back();
    for (size_t i = 0; i < resultMesh.runIndex.size() - 1; i++) {
      CesiumGltf::MeshPrimitive& primitive = newMesh.primitives.emplace_back();
      int64_t count =
          i < (resultMesh.runIndex.size() - 1)
              ? (int64_t)(resultMesh.runIndex[i + 1] - resultMesh.runIndex[i])
              : (int64_t)(resultMesh.triVerts.size() - resultMesh.runIndex[i]);

      int32_t indicesAccessorIdx = (int32_t)newModel.accessors.size();
      CesiumGltf::Accessor& indicesAccessor = newModel.accessors.emplace_back();
      indicesAccessor.bufferView = indicesBufferViewIdx;
      indicesAccessor.byteOffset =
          (int64_t)(resultMesh.runIndex[i] * sizeof(uint32_t));
      indicesAccessor.count = count;
      indicesAccessor.type = Accessor::Type::SCALAR;
      indicesAccessor.componentType = Accessor::ComponentType::UNSIGNED_INT;
      primitive.indices = indicesAccessorIdx;

      primitive.attributes.emplace(
          "POSITION",
          (int32_t)newModel.accessors.size());
      CesiumGltf::Accessor& positionAccessor =
          newModel.accessors.emplace_back();
      positionAccessor.bufferView = verticesBufferViewIdx;
      positionAccessor.byteOffset = 0;
      positionAccessor.count = (int64_t)(resultMesh.vertProperties.size() / 3);
      positionAccessor.type = Accessor::Type::VEC3;
      positionAccessor.componentType = Accessor::ComponentType::FLOAT;
      positionAccessor.min = std::vector<double>{minPos.x, minPos.y, minPos.z};
      positionAccessor.max = std::vector<double>{maxPos.x, maxPos.y, maxPos.z};

      const CesiumGltf::MeshPrimitive& oldPrimitive =
          mesh.primitives[0]; // mesh.primitives[resultMesh.runOriginalID[i]];
      primitive.material = oldPrimitive.material;
      primitive.mode = oldPrimitive.mode;

      // Start with offset for position.
      uint64_t offset = 3;
      for (const std::pair<const std::string, int32_t>& attribute :
           oldPrimitive.attributes) {
        const Accessor& attrAccessor =
            Model::getSafe(model.accessors, attribute.second);
        if (attrAccessor.componentType != Accessor::ComponentType::FLOAT) {
          continue;
        }

        if (attribute.first == "POSITION") {
          continue;
        }

        primitive.attributes.emplace(
            attribute.first,
            (int32_t)newModel.accessors.size());
        CesiumGltf::Accessor& newAccessor = newModel.accessors.emplace_back();
        newAccessor.bufferView = verticesBufferViewIdx;
        newAccessor.byteOffset =
            (int64_t)((resultMesh.runIndex[i] * resultMesh.numProp + offset) *
                      sizeof(float));
        newAccessor.count = count;
        newAccessor.type = attrAccessor.type;
        newAccessor.componentType = attrAccessor.componentType;

        offset += (uint64_t)attrAccessor.computeNumberOfComponents();
      }
    }
  }

  return input.asyncSystem.createResolvedFuture(
      std::optional<GltfModifierOutput>(output));
}
} // namespace Cesium3DTilesSelection