#include "Cesium3DTilesSelection/GltfModifier.h"
#include "CesiumGltf/Accessor.h"
#include "CesiumGltf/AccessorView.h"

#include <Cesium3DTilesSelection/ManifoldGltfModifier.h>

#include <manifold/common.h>
#include <manifold/manifold.h>

using namespace CesiumGltf;

namespace Cesium3DTilesSelection {
CesiumAsync::Future<std::optional<GltfModifierOutput>>
ManifoldGltfModifier::apply(GltfModifierInput&& input) {
  manifold::Manifold boxManifold = manifold::Manifold::Cube(
      manifold::vec3(this->box.lengthX, this->box.lengthY, this->box.lengthZ));

  GltfModifierOutput output;

  const CesiumGltf::Model& model = input.previousModel;
  output.modifiedModel = CesiumGltf::Model(model);
  CesiumGltf::Model& newModel = output.modifiedModel;
  newModel.meshes.clear();
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
          vertexCount = static_cast<size_t>(
              positionAccessor.computeNumberOfComponents() *
              positionAccessor.count);
        }

        stride += attrAccessor.computeNumberOfComponents();
      }

      if (positionIndex < 0) {
        continue;
      }

      AccessorView indicesAccessor =
          AccessorView<uint64_t>(model, primitive.indices);
      size_t vertOffset = manifoldMesh.vertProperties.size();
      manifoldMesh.vertProperties.resize(
          manifoldMesh.vertProperties.size() + (size_t)stride * vertexCount,
          0.0f);
      manifoldMesh.numProp = static_cast<uint64_t>(stride);

      AccessorView positionAccessor =
          AccessorView<glm::dvec3>(model, positionIndex);
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

    manifoldMesh.Merge();
    manifold::Manifold manifold(manifoldMesh);
    manifold::Manifold resultManifold =
        manifold.Boolean(boxManifold, manifold::OpType::Subtract);

    manifold::MeshGL64 resultMesh = resultManifold.GetMeshGL64();

    CesiumGltf::BufferCesium indicesBufferCesium;
    indicesBufferCesium.data.resize(
        manifoldMesh.triVerts.size() * sizeof(uint32_t));
    for (size_t i = 0; i < manifoldMesh.triVerts.size(); i++) {
      uint32_t idx = (uint32_t)manifoldMesh.triVerts[i];
      std::memcpy(
          indicesBufferCesium.data.data() + (i * sizeof(uint32_t)),
          &idx,
          sizeof(uint32_t));
    }
    int32_t indicesBufferIdx = (int32_t)newModel.buffers.size();
    CesiumGltf::Buffer indicesBuffer;
    indicesBuffer.cesium = std::move(indicesBufferCesium);
    indicesBuffer.byteLength = (int64_t)indicesBuffer.cesium.data.size();
    CesiumGltf::BufferView indicesBufferView;
    indicesBufferView.buffer = indicesBufferIdx;
    indicesBufferView.byteLength = indicesBuffer.byteLength;
    indicesBufferView.byteOffset = 0;
    indicesBufferView.byteStride = sizeof(uint32_t);
    newModel.buffers.emplace_back(std::move(indicesBuffer));
    int32_t indicesBufferViewIdx = (int32_t)newModel.bufferViews.size();
    newModel.bufferViews.emplace_back(std::move(indicesBufferView));

    CesiumGltf::BufferCesium verticesBufferCesium;
    verticesBufferCesium.data.resize(
        manifoldMesh.vertProperties.size() * sizeof(float));
    for (size_t i = 0; i < manifoldMesh.vertProperties.size(); i++) {
      float prop = (float)manifoldMesh.vertProperties[i];
      std::memcpy(
          verticesBufferCesium.data.data() + (i * sizeof(float)),
          &prop,
          sizeof(float));
    }
    int32_t verticesBufferIdx = (int32_t)newModel.buffers.size();
    CesiumGltf::Buffer verticesBuffer;
    verticesBuffer.cesium = std::move(verticesBufferCesium);
    verticesBuffer.byteLength = (int64_t)verticesBuffer.cesium.data.size();
    CesiumGltf::BufferView verticesBufferView;
    verticesBufferView.buffer = verticesBufferIdx;
    verticesBufferView.byteLength = verticesBuffer.byteLength;
    verticesBufferView.byteOffset = 0;
    verticesBufferView.byteStride = resultMesh.numProp * sizeof(float);
    newModel.buffers.emplace_back(std::move(verticesBuffer));
    int32_t verticesBufferViewIdx = (int32_t)newModel.bufferViews.size();
    newModel.bufferViews.emplace_back(std::move(verticesBufferView));

    CesiumGltf::Mesh& newMesh = newModel.meshes.emplace_back();
    for (size_t i = 0; i < resultMesh.runIndex.size(); i++) {
      CesiumGltf::MeshPrimitive& primitive = newMesh.primitives.emplace_back();
      int64_t count =
          i < resultMesh.runIndex.size()
              ? (int64_t)(resultMesh.runIndex[i + 1] - resultMesh.runIndex[i])
              : (int64_t)(resultMesh.runIndex.size() - resultMesh.runIndex[i]);

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
      positionAccessor.byteOffset =
          (int64_t)(resultMesh.runIndex[i] * resultMesh.numProp *
                    sizeof(float));
      positionAccessor.count = count;
      positionAccessor.type = Accessor::Type::VEC3;
      positionAccessor.componentType = Accessor::ComponentType::FLOAT;

      const CesiumGltf::MeshPrimitive& oldPrimitive =
          mesh.primitives[resultMesh.runOriginalID[i]];
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

        primitive.attributes.emplace(attribute.first, (int32_t)newModel.accessors.size());
        CesiumGltf::Accessor& newAccessor = newModel.accessors.emplace_back();
        newAccessor.bufferView = verticesBufferViewIdx;
        newAccessor.byteOffset = (int64_t)((resultMesh.runIndex[i] * resultMesh.numProp + offset) * sizeof(float));
        newAccessor.count = count;
        newAccessor.type = attrAccessor.type;
        newAccessor.componentType = attrAccessor.componentType;

        offset += (uint64_t)attrAccessor.computeNumberOfComponents();
      }
    }
  }


  return input.asyncSystem.createResolvedFuture(std::optional<GltfModifierOutput>(output));
}
} // namespace Cesium3DTilesSelection