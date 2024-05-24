#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/Transforms.h>
#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/ExtensionBufferExtMeshoptCompression.h>
#include <CesiumGltf/ExtensionBufferViewExtMeshoptCompression.h>
#include <CesiumGltf/ExtensionCesiumPrimitiveOutline.h>
#include <CesiumGltf/ExtensionCesiumRTC.h>
#include <CesiumGltf/ExtensionCesiumTileEdges.h>
#include <CesiumGltf/ExtensionExtInstanceFeatures.h>
#include <CesiumGltf/ExtensionExtMeshFeatures.h>
#include <CesiumGltf/ExtensionExtMeshGpuInstancing.h>
#include <CesiumGltf/ExtensionKhrDracoMeshCompression.h>
#include <CesiumGltf/ExtensionKhrTextureBasisu.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/ExtensionTextureWebp.h>
#include <CesiumGltf/FeatureId.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumGltfContent/SkirtMeshMetadata.h>

#include <glm/gtc/quaternion.hpp>

#include <cstring>
#include <unordered_set>
#include <vector>

using namespace CesiumGltf;

namespace CesiumGltfContent {

namespace {
std::vector<int32_t> getIndexMap(const std::vector<bool>& usedIndices);
}

/*static*/ std::optional<glm::dmat4x4>
GltfUtilities::getNodeTransform(const CesiumGltf::Node& node) {
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

/*static*/ void GltfUtilities::setNodeTransform(
    CesiumGltf::Node& node,
    const glm::dmat4x4& newTransform) {
  // Reset these fields to their default, indicating they're not to be used.
  node.translation = {0.0, 0.0, 0.0};
  node.scale = {1.0, 1.0, 1.0};
  node.rotation = {0, 0.0, 0.0, 1.0};

  const glm::dmat4x4& m = newTransform;

  node.matrix.resize(16);
  node.matrix[0] = m[0].x;
  node.matrix[1] = m[0].y;
  node.matrix[2] = m[0].z;
  node.matrix[3] = m[0].w;
  node.matrix[4] = m[1].x;
  node.matrix[5] = m[1].y;
  node.matrix[6] = m[1].z;
  node.matrix[7] = m[1].w;
  node.matrix[8] = m[2].x;
  node.matrix[9] = m[2].y;
  node.matrix[10] = m[2].z;
  node.matrix[11] = m[2].w;
  node.matrix[12] = m[3].x;
  node.matrix[13] = m[3].y;
  node.matrix[14] = m[3].z;
  node.matrix[15] = m[3].w;
}

/*static*/ glm::dmat4x4 GltfUtilities::applyRtcCenter(
    const CesiumGltf::Model& gltf,
    const glm::dmat4x4& rootTransform) {
  const CesiumGltf::ExtensionCesiumRTC* cesiumRTC =
      gltf.getExtension<CesiumGltf::ExtensionCesiumRTC>();
  if (cesiumRTC == nullptr) {
    return rootTransform;
  }
  const std::vector<double>& rtcCenter = cesiumRTC->center;
  if (rtcCenter.size() != 3) {
    return rootTransform;
  }
  const double x = rtcCenter[0];
  const double y = rtcCenter[1];
  const double z = rtcCenter[2];
  const glm::dmat4x4 rtcTransform(
      glm::dvec4(1.0, 0.0, 0.0, 0.0),
      glm::dvec4(0.0, 1.0, 0.0, 0.0),
      glm::dvec4(0.0, 0.0, 1.0, 0.0),
      glm::dvec4(x, y, z, 1.0));
  return rootTransform * rtcTransform;
}

/*static*/ glm::dmat4x4 GltfUtilities::applyGltfUpAxisTransform(
    const CesiumGltf::Model& model,
    const glm::dmat4x4& rootTransform) {
  auto gltfUpAxisIt = model.extras.find("gltfUpAxis");
  if (gltfUpAxisIt == model.extras.end()) {
    // The default up-axis of glTF is the Y-axis, and no other
    // up-axis was specified. Transform the Y-axis to the Z-axis,
    // to match the 3D Tiles specification
    return rootTransform * CesiumGeometry::Transforms::Y_UP_TO_Z_UP;
  }
  const CesiumUtility::JsonValue& gltfUpAxis = gltfUpAxisIt->second;
  int gltfUpAxisValue = static_cast<int>(gltfUpAxis.getSafeNumberOrDefault(1));
  if (gltfUpAxisValue == static_cast<int>(CesiumGeometry::Axis::X)) {
    return rootTransform * CesiumGeometry::Transforms::X_UP_TO_Z_UP;
  } else if (gltfUpAxisValue == static_cast<int>(CesiumGeometry::Axis::Y)) {
    return rootTransform * CesiumGeometry::Transforms::Y_UP_TO_Z_UP;
  } else if (gltfUpAxisValue == static_cast<int>(CesiumGeometry::Axis::Z)) {
    // No transform required
  }
  return rootTransform;
}

/*static*/ CesiumGeospatial::BoundingRegion
GltfUtilities::computeBoundingRegion(
    const CesiumGltf::Model& gltf,
    const glm::dmat4& transform) {
  glm::dmat4 rootTransform = transform;
  rootTransform = applyRtcCenter(gltf, rootTransform);
  rootTransform = applyGltfUpAxisTransform(gltf, rootTransform);

  // When computing the tile's bounds, ignore tiles that are less than 1/1000th
  // of a tile width from the North or South pole. Longitudes cannot be trusted
  // at such extreme latitudes.
  CesiumGeospatial::BoundingRegionBuilder computedBounds;

  gltf.forEachPrimitiveInScene(
      -1,
      [&rootTransform, &computedBounds](
          const CesiumGltf::Model& gltf_,
          const CesiumGltf::Node& /*node*/,
          const CesiumGltf::Mesh& /*mesh*/,
          const CesiumGltf::MeshPrimitive& primitive,
          const glm::dmat4& nodeTransform) {
        auto positionIt = primitive.attributes.find("POSITION");
        if (positionIt == primitive.attributes.end()) {
          return;
        }

        const int positionAccessorIndex = positionIt->second;
        if (positionAccessorIndex < 0 ||
            positionAccessorIndex >= static_cast<int>(gltf_.accessors.size())) {
          return;
        }

        const glm::dmat4 fullTransform = rootTransform * nodeTransform;

        const CesiumGltf::AccessorView<glm::vec3> positionView(
            gltf_,
            positionAccessorIndex);
        if (positionView.status() != CesiumGltf::AccessorViewStatus::Valid) {
          return;
        }

        std::optional<SkirtMeshMetadata> skirtMeshMetadata =
            SkirtMeshMetadata::parseFromGltfExtras(primitive.extras);
        int64_t vertexBegin, vertexEnd;
        if (skirtMeshMetadata.has_value()) {
          vertexBegin = skirtMeshMetadata->noSkirtVerticesBegin;
          vertexEnd = skirtMeshMetadata->noSkirtVerticesBegin +
                      skirtMeshMetadata->noSkirtVerticesCount;
        } else {
          vertexBegin = 0;
          vertexEnd = positionView.size();
        }

        for (int64_t i = vertexBegin; i < vertexEnd; ++i) {
          // Get the ECEF position
          const glm::vec3 position = positionView[i];
          const glm::dvec3 positionEcef =
              glm::dvec3(fullTransform * glm::dvec4(position, 1.0));

          // Convert it to cartographic
          std::optional<CesiumGeospatial::Cartographic> cartographic =
              CesiumGeospatial::Ellipsoid::WGS84.cartesianToCartographic(
                  positionEcef);
          if (!cartographic) {
            continue;
          }

          computedBounds.expandToIncludePosition(*cartographic);
        }
      });

  return computedBounds.toRegion();
}

std::vector<std::string_view>
GltfUtilities::parseGltfCopyright(const CesiumGltf::Model& gltf) {
  std::vector<std::string_view> result;
  if (gltf.asset.copyright) {
    const std::string_view copyright = *gltf.asset.copyright;
    if (copyright.size() > 0) {
      size_t start = 0;
      size_t end;
      size_t ltrim;
      size_t rtrim;
      do {
        ltrim = copyright.find_first_not_of(" \t", start);
        end = copyright.find(';', ltrim);
        rtrim = copyright.find_last_not_of(" \t", end - 1);
        result.emplace_back(copyright.substr(ltrim, rtrim - ltrim + 1));
        start = end + 1;
      } while (end != std::string::npos);
    }
  }

  return result;
}

namespace {

size_t moveBufferContentWithoutRenumbering(
    CesiumGltf::Buffer& destinationBuffer,
    CesiumGltf::Buffer& sourceBuffer) {
  // Assert that the byteLength and the size of the cesium data vector are in
  // sync.
  assert(sourceBuffer.byteLength == int64_t(sourceBuffer.cesium.data.size()));
  assert(
      destinationBuffer.byteLength ==
      int64_t(destinationBuffer.cesium.data.size()));

  // Copy the data to the destination and keep track of where we put it.
  // Align each bufferView to an 8-byte boundary.
  size_t start = destinationBuffer.cesium.data.size();

  size_t alignmentRemainder = start % 8;
  if (alignmentRemainder != 0) {
    start += 8 - alignmentRemainder;
  }

  destinationBuffer.cesium.data.resize(start + sourceBuffer.cesium.data.size());
  std::memcpy(
      destinationBuffer.cesium.data.data() + start,
      sourceBuffer.cesium.data.data(),
      sourceBuffer.cesium.data.size());

  sourceBuffer.byteLength = 0;
  sourceBuffer.cesium.data.clear();
  sourceBuffer.cesium.data.shrink_to_fit();

  destinationBuffer.byteLength = int64_t(destinationBuffer.cesium.data.size());

  return start;
}

} // namespace

/*static*/ void GltfUtilities::collapseToSingleBuffer(CesiumGltf::Model& gltf) {
  if (gltf.buffers.empty())
    return;

  Buffer& destinationBuffer = gltf.buffers[0];

  std::vector<bool> keepBuffer(gltf.buffers.size(), false);
  keepBuffer[0] = true;

  std::vector<int64_t> bufferStarts(gltf.buffers.size(), 0);

  for (size_t i = 1; i < gltf.buffers.size(); ++i) {
    Buffer& sourceBuffer = gltf.buffers[i];

    // Leave intact any buffers that have a URI and no data.
    // Also leave intact meshopt fallback buffers without any data.
    ExtensionBufferExtMeshoptCompression* pMeshOpt =
        sourceBuffer.getExtension<ExtensionBufferExtMeshoptCompression>();
    bool isMeshOptFallback = pMeshOpt && pMeshOpt->fallback;
    if (sourceBuffer.cesium.data.empty() &&
        (sourceBuffer.uri || isMeshOptFallback)) {
      keepBuffer[i] = true;
      continue;
    }

    size_t start =
        moveBufferContentWithoutRenumbering(destinationBuffer, sourceBuffer);
    bufferStarts[i] = int64_t(start);
  }

  // Update the buffer indices based on the buffers being removed.
  std::vector<int32_t> indexMap = getIndexMap(keepBuffer);
  for (BufferView& bufferView : gltf.bufferViews) {
    int32_t& bufferIndex = bufferView.buffer;
    if (bufferIndex >= 0 && size_t(bufferIndex) < indexMap.size()) {
      bufferView.byteOffset += bufferStarts[size_t(bufferIndex)];
      int32_t newIndex = indexMap[size_t(bufferIndex)];
      bufferIndex = newIndex == -1 ? 0 : newIndex;
    }

    ExtensionBufferViewExtMeshoptCompression* pMeshOpt =
        bufferView.getExtension<ExtensionBufferViewExtMeshoptCompression>();
    if (pMeshOpt) {
      int32_t& meshOptBufferIndex = pMeshOpt->buffer;
      if (meshOptBufferIndex >= 0 &&
          size_t(meshOptBufferIndex) < indexMap.size()) {
        pMeshOpt->byteOffset += bufferStarts[size_t(meshOptBufferIndex)];
        int32_t newIndex = indexMap[size_t(meshOptBufferIndex)];
        meshOptBufferIndex = newIndex == -1 ? 0 : newIndex;
      }
    }
  }

  // Remove the unused elements.
  gltf.buffers.erase(
      std::remove_if(
          gltf.buffers.begin(),
          gltf.buffers.end(),
          [&keepBuffer, &gltf](const Buffer& buffer) {
            int64_t index = &buffer - &gltf.buffers[0];
            assert(index >= 0 && size_t(index) < keepBuffer.size());
            return !keepBuffer[size_t(index)];
          }),
      gltf.buffers.end());
}

/*static*/ void GltfUtilities::moveBufferContent(
    CesiumGltf::Model& gltf,
    CesiumGltf::Buffer& destination,
    CesiumGltf::Buffer& source) {
  int64_t sourceIndex = &source - &gltf.buffers[0];
  int64_t destinationIndex = &destination - &gltf.buffers[0];

  // Both buffers must exist in the glTF.
  if (sourceIndex < 0 || sourceIndex >= int64_t(gltf.buffers.size()) ||
      destinationIndex < 0 ||
      destinationIndex >= int64_t(gltf.buffers.size())) {
    assert(false);
    return;
  }

  size_t start = moveBufferContentWithoutRenumbering(destination, source);

  // Update all the bufferViews that previously referred to the source Buffer to
  // refer to the destination Buffer instead.
  for (BufferView& bufferView : gltf.bufferViews) {
    if (bufferView.buffer == sourceIndex) {
      bufferView.buffer = int32_t(destinationIndex);
      bufferView.byteOffset += int64_t(start);
    }

    ExtensionBufferViewExtMeshoptCompression* pMeshOpt =
        bufferView.getExtension<ExtensionBufferViewExtMeshoptCompression>();
    if (pMeshOpt && pMeshOpt->buffer == sourceIndex) {
      pMeshOpt->buffer = int32_t(destinationIndex);
      pMeshOpt->byteOffset += int64_t(start);
    }
  }
}

namespace {

struct VisitTextureIds {
  template <typename Func> void operator()(Model& gltf, Func&& callback) {
    // Find textures in materials
    for (Material& material : gltf.materials) {
      if (material.emissiveTexture)
        callback(material.emissiveTexture->index);
      if (material.normalTexture)
        callback(material.normalTexture->index);
      if (material.pbrMetallicRoughness) {
        if (material.pbrMetallicRoughness->baseColorTexture)
          callback(material.pbrMetallicRoughness->baseColorTexture->index);
        if (material.pbrMetallicRoughness->metallicRoughnessTexture)
          callback(
              material.pbrMetallicRoughness->metallicRoughnessTexture->index);
      }
    }

    // Find textures in metadata
    for (Mesh& mesh : gltf.meshes) {
      for (MeshPrimitive& primitive : mesh.primitives) {
        ExtensionExtMeshFeatures* pMeshFeatures =
            primitive.getExtension<ExtensionExtMeshFeatures>();
        if (pMeshFeatures) {
          for (FeatureId& featureId : pMeshFeatures->featureIds) {
            if (featureId.texture)
              callback(featureId.texture->index);
          }
        }
      }
    }

    ExtensionModelExtStructuralMetadata* pMetadata =
        gltf.getExtension<ExtensionModelExtStructuralMetadata>();
    if (pMetadata) {
      for (PropertyTexture& propertyTexture : pMetadata->propertyTextures) {
        for (auto& pair : propertyTexture.properties) {
          callback(pair.second.index);
        }
      }
    }
  }
};

struct VisitSamplerIds {
  template <typename Func> void operator()(Model& gltf, Func&& callback) {
    // Find samplers in textures
    for (Texture& texture : gltf.textures) {
      callback(texture.sampler);
    }
  }
};

// Get a map of old IDs to new ones after all the unused IDs have been
// removed. A removed ID will map to -1.
std::vector<int32_t> getIndexMap(const std::vector<bool>& usedIndices) {
  std::vector<int32_t> indexMap;
  indexMap.reserve(usedIndices.size());

  int32_t nextIndex = 0;
  for (size_t i = 0; i < usedIndices.size(); ++i) {
    if (usedIndices[i]) {
      indexMap.push_back(nextIndex);
      ++nextIndex;
    } else {
      indexMap.push_back(-1);
    }
  }

  return indexMap;
}

struct VisitImageIds {
  template <typename Func> void operator()(Model& gltf, Func&& callback) {
    // Find images in textures
    for (Texture& texture : gltf.textures) {
      callback(texture.source);

      ExtensionKhrTextureBasisu* pBasis =
          texture.getExtension<ExtensionKhrTextureBasisu>();
      if (pBasis)
        callback(pBasis->source);

      ExtensionTextureWebp* pWebP =
          texture.getExtension<ExtensionTextureWebp>();
      if (pWebP)
        callback(pWebP->source);
    }
  }
};

struct VisitAccessorIds {
  template <typename Func> void operator()(Model& gltf, Func&& callback) {
    for (Mesh& mesh : gltf.meshes) {
      for (MeshPrimitive& primitive : mesh.primitives) {
        callback(primitive.indices);

        for (auto& pair : primitive.attributes) {
          callback(pair.second);
        }

        ExtensionCesiumTileEdges* pTileEdges =
            primitive.getExtension<ExtensionCesiumTileEdges>();
        if (pTileEdges) {
          callback(pTileEdges->left);
          callback(pTileEdges->bottom);
          callback(pTileEdges->right);
          callback(pTileEdges->top);
        }

        ExtensionCesiumPrimitiveOutline* pPrimitiveOutline =
            primitive.getExtension<ExtensionCesiumPrimitiveOutline>();
        if (pPrimitiveOutline) {
          callback(pPrimitiveOutline->indices);
        }
      }
    }

    for (Animation& animation : gltf.animations) {
      for (AnimationSampler& sampler : animation.samplers) {
        callback(sampler.input);
        callback(sampler.output);
      }
    }

    for (Skin& skin : gltf.skins) {
      callback(skin.inverseBindMatrices);
    }

    for (Node& node : gltf.nodes) {
      ExtensionExtMeshGpuInstancing* pInstancing =
          node.getExtension<ExtensionExtMeshGpuInstancing>();
      if (pInstancing) {
        for (auto& pair : pInstancing->attributes) {
          callback(pair.second);
        }
      }
    }
  }
};

struct VisitBufferViewIds {
  template <typename Func> void operator()(Model& gltf, Func&& callback) {
    for (Accessor& accessor : gltf.accessors) {
      callback(accessor.bufferView);

      if (accessor.sparse) {
        callback(accessor.sparse->indices.bufferView);
        callback(accessor.sparse->values.bufferView);
      }
    }

    for (Image& image : gltf.images) {
      callback(image.bufferView);
    }

    for (Mesh& mesh : gltf.meshes) {
      for (MeshPrimitive& primitive : mesh.primitives) {
        ExtensionKhrDracoMeshCompression* pDraco =
            primitive.getExtension<ExtensionKhrDracoMeshCompression>();
        if (pDraco) {
          callback(pDraco->bufferView);
        }
      }
    }

    ExtensionModelExtStructuralMetadata* pMetadata =
        gltf.getExtension<ExtensionModelExtStructuralMetadata>();
    if (pMetadata) {
      for (PropertyTable& propertyTable : pMetadata->propertyTables) {
        for (auto& pair : propertyTable.properties) {
          callback(pair.second.values);
          callback(pair.second.arrayOffsets);
          callback(pair.second.stringOffsets);
        }
      }
    }
  }
};

struct VisitBufferIds {
  template <typename Func> void operator()(Model& gltf, Func&& callback) {
    for (BufferView& bufferView : gltf.bufferViews) {
      callback(bufferView.buffer);

      ExtensionBufferViewExtMeshoptCompression* pMeshOpt =
          bufferView.getExtension<ExtensionBufferViewExtMeshoptCompression>();
      if (pMeshOpt) {
        callback(pMeshOpt->buffer);
      }
    }
  }
};

template <typename T, typename TVisitFunction>
void removeUnusedElements(
    Model& gltf,
    const std::vector<int32_t>& extraUsedIndices,
    std::vector<T>& elements,
    TVisitFunction&& visitFunction) {
  std::vector<bool> usedElements(elements.size(), false);

  for (int32_t index : extraUsedIndices) {
    if (index >= 0 && size_t(index) < usedElements.size())
      usedElements[size_t(index)] = true;
  }

  // Determine which elements are used.
  visitFunction(gltf, [&usedElements](int32_t elementIndex) {
    if (elementIndex >= 0 && size_t(elementIndex) < usedElements.size())
      usedElements[size_t(elementIndex)] = true;
  });

  // Update the element indices based on the unused indices being removed.
  std::vector<int32_t> indexMap = getIndexMap(usedElements);
  visitFunction(gltf, [&indexMap](int32_t& elementIndex) {
    if (elementIndex >= 0 && size_t(elementIndex) < indexMap.size()) {
      int32_t newIndex = indexMap[size_t(elementIndex)];
      assert(newIndex >= 0);
      elementIndex = newIndex;
    }
  });

  // Remove the unused elements.
  elements.erase(
      std::remove_if(
          elements.begin(),
          elements.end(),
          [&usedElements, &elements](T& element) {
            int64_t index = &element - &elements[0];
            assert(index >= 0 && size_t(index) < usedElements.size());
            return !usedElements[size_t(index)];
          }),
      elements.end());
}

} // namespace

void GltfUtilities::removeUnusedTextures(
    Model& gltf,
    const std::vector<int32_t>& extraUsedTextureIndices) {
  removeUnusedElements(
      gltf,
      extraUsedTextureIndices,
      gltf.textures,
      VisitTextureIds());
}

void GltfUtilities::removeUnusedSamplers(
    Model& gltf,
    const std::vector<int32_t>& extraUsedSamplerIndices) {
  removeUnusedElements(
      gltf,
      extraUsedSamplerIndices,
      gltf.samplers,
      VisitSamplerIds());
}

void GltfUtilities::removeUnusedImages(
    Model& gltf,
    const std::vector<int32_t>& extraUsedImageIndices) {
  removeUnusedElements(
      gltf,
      extraUsedImageIndices,
      gltf.images,
      VisitImageIds());
}

void GltfUtilities::removeUnusedAccessors(
    CesiumGltf::Model& gltf,
    const std::vector<int32_t>& extraUsedAccessorIndices) {
  removeUnusedElements(
      gltf,
      extraUsedAccessorIndices,
      gltf.accessors,
      VisitAccessorIds());
}

void GltfUtilities::removeUnusedBufferViews(
    CesiumGltf::Model& gltf,
    const std::vector<int32_t>& extraUsedBufferViewIndices) {
  removeUnusedElements(
      gltf,
      extraUsedBufferViewIndices,
      gltf.bufferViews,
      VisitBufferViewIds());
}

void GltfUtilities::removeUnusedBuffers(
    CesiumGltf::Model& gltf,
    const std::vector<int32_t>& extraUsedBufferIndices) {
  removeUnusedElements(
      gltf,
      extraUsedBufferIndices,
      gltf.buffers,
      VisitBufferIds());
}

void GltfUtilities::compactBuffers(CesiumGltf::Model& gltf) {
  for (size_t i = 0;
       i < gltf.buffers.size() && i < std::numeric_limits<int32_t>::max();
       ++i) {
    GltfUtilities::compactBuffer(gltf, int32_t(i));
  }
}

namespace {

void deleteBufferRange(
    CesiumGltf::Model& gltf,
    int32_t bufferIndex,
    int64_t start,
    int64_t end) {
  Buffer* pBuffer = gltf.getSafe(&gltf.buffers, bufferIndex);
  if (pBuffer == nullptr)
    return;

  assert(size_t(pBuffer->byteLength) == pBuffer->cesium.data.size());

  int64_t bytesToRemove = end - start;

  // In order to ensure that we can't disrupt glTF's alignment requirements,
  // only remove multiples of 8 bytes from within the buffer (removing any
  // number of bytes from the end is fine).
  if (end < pBuffer->byteLength) {
    // Round down to the nearest multiple of 8 by clearing the low three bits.
    bytesToRemove = bytesToRemove & ~0b111;
    if (bytesToRemove == 0)
      return;

    end = start + bytesToRemove;
  }

  // Adjust bufferView offsets for the removed bytes.
  for (BufferView& bufferView : gltf.bufferViews) {
    if (bufferView.buffer == bufferIndex) {
      // Sanity check that we're not removing a part of the buffer used by this
      // bufferView.
      assert(
          bufferView.byteOffset >= end ||
          (bufferView.byteOffset + bufferView.byteLength) <= start);

      // If this bufferView starts after the bytes we're removing, adjust the
      // start position accordingly.
      if (bufferView.byteOffset >= start) {
        bufferView.byteOffset -= bytesToRemove;
      }
    }

    ExtensionBufferViewExtMeshoptCompression* pMeshOpt =
        bufferView.getExtension<ExtensionBufferViewExtMeshoptCompression>();
    if (pMeshOpt && pMeshOpt->buffer == bufferIndex) {
      // Sanity check that we're not removing a part of the buffer used by this
      // meshopt extension.
      assert(
          pMeshOpt->byteOffset >= end ||
          (pMeshOpt->byteOffset + pMeshOpt->byteLength) <= start);

      // If this meshopt extension starts after the bytes we're removing, adjust
      // the start position accordingly.
      if (pMeshOpt->byteOffset >= start) {
        pMeshOpt->byteOffset -= bytesToRemove;
      }
    }
  }

  // Actually remove the bytes from the buffer.
  pBuffer->byteLength -= bytesToRemove;
  pBuffer->cesium.data.erase(
      pBuffer->cesium.data.begin() + start,
      pBuffer->cesium.data.begin() + end);
}

} // namespace

void GltfUtilities::compactBuffer(
    CesiumGltf::Model& gltf,
    int32_t bufferIndex) {
  Buffer* pBuffer = gltf.getSafe(&gltf.buffers, bufferIndex);
  if (!pBuffer)
    return;

  assert(size_t(pBuffer->byteLength) == pBuffer->cesium.data.size());

  struct BufferRange {
    int64_t start; // first byte
    int64_t end;   // one past last byte

    // Order ranges by their start
    bool operator<(const BufferRange& rhs) const {
      return this->start < rhs.start;
    }
  };

  std::vector<BufferRange> usedRanges;

  auto addUsedRange = [&usedRanges](int64_t start, int64_t end) {
    auto it = std::lower_bound(
        usedRanges.begin(),
        usedRanges.end(),
        BufferRange{start, end});
    it = usedRanges.insert(it, BufferRange{start, end});

    // Check if we can merge with the previous range.
    if (it != usedRanges.begin()) {
      auto previousIt = it - 1;
      if (previousIt->end >= it->start) {
        // New range overlaps the previous, so combine them.
        previousIt->end = std::max(previousIt->end, it->end);
        it = usedRanges.erase(it) - 1;
      }
    }

    // Check if we can merge with the next range.
    auto nextIt = it + 1;
    if (nextIt != usedRanges.end()) {
      if (it->end >= nextIt->start) {
        // New range overlaps the next, so combine them.
        it->end = std::max(it->end, nextIt->end);
        it = usedRanges.erase(nextIt) - 1;
      }
    }
  };

  for (BufferView& bufferView : gltf.bufferViews) {
    if (bufferView.buffer == bufferIndex) {
      addUsedRange(
          bufferView.byteOffset,
          bufferView.byteOffset + bufferView.byteLength);
    }

    ExtensionBufferViewExtMeshoptCompression* pMeshOpt =
        bufferView.getExtension<ExtensionBufferViewExtMeshoptCompression>();
    if (pMeshOpt && pMeshOpt->buffer == bufferIndex) {
      addUsedRange(
          pMeshOpt->byteOffset,
          pMeshOpt->byteOffset + pMeshOpt->byteLength);
    }
  }

  // At this point, any gaps in the usedRanges represent buffer bytes that are
  // not referenced by any bufferView. Work through it backwards so that we
  // don't need to update the ranges as we delete unused data from the buffer.
  BufferRange nextRange{pBuffer->byteLength, pBuffer->byteLength};
  for (int64_t i = int64_t(usedRanges.size()) - 1; i >= 0; --i) {
    BufferRange& usedRange = usedRanges[size_t(i)];
    if (usedRange.end < nextRange.start) {
      // This is a gap.
      deleteBufferRange(gltf, bufferIndex, usedRange.end, nextRange.start);
    }
    nextRange = usedRange;
  }

  if (nextRange.start > 0) {
    // There is a gap at the start of the buffer.
    deleteBufferRange(gltf, bufferIndex, 0, nextRange.start);
  }
}

} // namespace CesiumGltfContent
