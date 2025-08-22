#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/IntersectionTests.h>
#include <CesiumGeometry/Ray.h>
#include <CesiumGeometry/Transforms.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorSpec.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/Animation.h>
#include <CesiumGltf/AnimationSampler.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/ExtensionBufferExtMeshoptCompression.h>
#include <CesiumGltf/ExtensionBufferViewExtMeshoptCompression.h>
#include <CesiumGltf/ExtensionCesiumPrimitiveOutline.h>
#include <CesiumGltf/ExtensionCesiumRTC.h>
#include <CesiumGltf/ExtensionCesiumTileEdges.h>
#include <CesiumGltf/ExtensionExtMeshFeatures.h>
#include <CesiumGltf/ExtensionExtMeshGpuInstancing.h>
#include <CesiumGltf/ExtensionKhrDracoMeshCompression.h>
#include <CesiumGltf/ExtensionKhrTextureBasisu.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/ExtensionTextureWebp.h>
#include <CesiumGltf/FeatureId.h>
#include <CesiumGltf/Image.h>
#include <CesiumGltf/Material.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/PropertyTable.h>
#include <CesiumGltf/PropertyTexture.h>
#include <CesiumGltf/Skin.h>
#include <CesiumGltf/Texture.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumGltfContent/SkirtMeshMetadata.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/JsonValue.h>

#include <fmt/format.h>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/matrix.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
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
    const glm::dmat4& transform,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  glm::dmat4 rootTransform = transform;
  rootTransform = applyRtcCenter(gltf, rootTransform);
  rootTransform = applyGltfUpAxisTransform(gltf, rootTransform);

  // When computing the tile's bounds, ignore tiles that are less than 1/1000th
  // of a tile width from the North or South pole. Longitudes cannot be trusted
  // at such extreme latitudes.
  CesiumGeospatial::BoundingRegionBuilder computedBounds;

  gltf.forEachPrimitiveInScene(
      -1,
      [&rootTransform, &computedBounds, &ellipsoid](
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
              ellipsoid.cartesianToCartographic(positionEcef);
          if (!cartographic) {
            continue;
          }

          computedBounds.expandToIncludePosition(*cartographic);
        }
      });

  return computedBounds.toRegion(ellipsoid);
}

std::vector<std::string_view>
GltfUtilities::parseGltfCopyright(const CesiumGltf::Model& gltf) {
  if (gltf.asset.copyright) {
    return GltfUtilities::parseGltfCopyright(*gltf.asset.copyright);
  } else {
    return {};
  }
}

namespace {
std::string_view trimWhitespace(const std::string_view& s) {
  size_t end = s.find_last_not_of(" \t");
  if (end == std::string::npos)
    return {};

  std::string_view trimmedRight = s.substr(0, end + 1);
  size_t start = trimmedRight.find_first_not_of(" \t");
  if (start == std::string::npos)
    return {};

  return trimmedRight.substr(start);
}
} // namespace

std::vector<std::string_view>
GltfUtilities::parseGltfCopyright(const std::string_view& s) {
  std::vector<std::string_view> result;
  if (s.empty())
    return result;

  size_t start = 0;

  auto addPart = [&](size_t end) {
    std::string_view trimmed = trimWhitespace(s.substr(start, end - start));
    if (!trimmed.empty())
      result.emplace_back(std::move(trimmed));
  };

  for (size_t i = 0, length = s.size(); i < length; ++i) {
    if (s[i] == ';') {
      addPart(i);
      start = i + 1;
    }
  }

  addPart(s.size());

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
    CESIUM_ASSERT(false);
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

template <class PositionViewType>
void findClosestRayHit(
    const CesiumGeometry::Ray& ray,
    const PositionViewType& positionView,
    const CesiumGltf::MeshPrimitive& primitive,
    bool cullBackFaces,
    double& tMinOut,
    std::vector<std::string>& warnings) {

  // Need at least 3 positions to form a triangle
  if (positionView.size() < 3) {
    warnings.emplace_back("Skipping mesh with less than 3 vertex positions");
    return;
  }

  double tClosest = -1.0;
  std::optional<double> tCurr;

  if (primitive.mode == MeshPrimitive::Mode::TRIANGLES) {
    // Iterate through all complete triangles
    for (int64_t i = 2; i < positionView.size(); i += 3) {
      int64_t vert0Index = i - 2;
      int64_t vert1Index = i - 1;
      int64_t vert2Index = i;

      auto& viewVert0 = positionView[vert0Index];
      auto& viewVert1 = positionView[vert1Index];
      auto& viewVert2 = positionView[vert2Index];

      glm::dvec3 vert0(
          static_cast<double>(viewVert0.value[0]),
          static_cast<double>(viewVert0.value[1]),
          static_cast<double>(viewVert0.value[2]));
      glm::dvec3 vert1(
          static_cast<double>(viewVert1.value[0]),
          static_cast<double>(viewVert1.value[1]),
          static_cast<double>(viewVert1.value[2]));
      glm::dvec3 vert2(
          static_cast<double>(viewVert2.value[0]),
          static_cast<double>(viewVert2.value[1]),
          static_cast<double>(viewVert2.value[2]));

      tCurr = CesiumGeometry::IntersectionTests::rayTriangleParametric(
          ray,
          vert0,
          vert1,
          vert2,
          cullBackFaces);

      // Set result to this hit if closer, or the first one
      // Only consider hits in front of the ray
      bool validHit = tCurr && tCurr.value() >= 0;
      if (validHit && (tClosest == -1.0 || tCurr.value() < tClosest))
        tClosest = tCurr.value();
    }
  } else if (primitive.mode == MeshPrimitive::Mode::TRIANGLE_STRIP) {
    for (int64_t i = 2; i < positionView.size(); ++i) {
      int64_t vert0Index = i - 2;
      int64_t vert1Index;
      int64_t vert2Index;
      if (i % 2) {
        vert1Index = i;
        vert2Index = i - 1;
      } else {
        vert1Index = i - 1;
        vert2Index = i;
      }

      auto& viewVert0 = positionView[vert0Index];
      auto& viewVert1 = positionView[vert1Index];
      auto& viewVert2 = positionView[vert2Index];

      glm::dvec3 vert0(
          static_cast<double>(viewVert0.value[0]),
          static_cast<double>(viewVert0.value[1]),
          static_cast<double>(viewVert0.value[2]));
      glm::dvec3 vert1(
          static_cast<double>(viewVert1.value[0]),
          static_cast<double>(viewVert1.value[1]),
          static_cast<double>(viewVert1.value[2]));
      glm::dvec3 vert2(
          static_cast<double>(viewVert2.value[0]),
          static_cast<double>(viewVert2.value[1]),
          static_cast<double>(viewVert2.value[2]));

      tCurr = CesiumGeometry::IntersectionTests::rayTriangleParametric(
          ray,
          vert0,
          vert1,
          vert2,
          cullBackFaces);

      bool validHit = tCurr && tCurr.value() >= 0;
      if (validHit && (tClosest == -1.0 || tCurr.value() < tClosest))
        tClosest = tCurr.value();
    }
  } else {
    assert(primitive.mode == MeshPrimitive::Mode::TRIANGLE_FAN);

    auto& viewVert0 = positionView[0];
    glm::dvec3 vert0(
        static_cast<double>(viewVert0.value[0]),
        static_cast<double>(viewVert0.value[1]),
        static_cast<double>(viewVert0.value[2]));

    for (int64_t i = 2; i < positionView.size(); ++i) {
      int64_t vert1Index = i - 1;
      int64_t vert2Index = i - 0;

      auto& viewVert1 = positionView[vert1Index];
      auto& viewVert2 = positionView[vert2Index];

      glm::dvec3 vert1(
          static_cast<double>(viewVert1.value[0]),
          static_cast<double>(viewVert1.value[1]),
          static_cast<double>(viewVert1.value[2]));
      glm::dvec3 vert2(
          static_cast<double>(viewVert2.value[0]),
          static_cast<double>(viewVert2.value[1]),
          static_cast<double>(viewVert2.value[2]));

      tCurr = CesiumGeometry::IntersectionTests::rayTriangleParametric(
          ray,
          vert0,
          vert1,
          vert2,
          cullBackFaces);

      bool validHit = tCurr && tCurr.value() >= 0;
      if (validHit && (tClosest == -1.0 || tCurr.value() < tClosest))
        tClosest = tCurr.value();
    }
  }
  tMinOut = tClosest;
}

template <class PositionViewType, class IndexViewType>
void findClosestIndexedRayHit(
    const CesiumGeometry::Ray& ray,
    const PositionViewType& positionView,
    const IndexViewType& indicesView,
    const CesiumGltf::MeshPrimitive& primitive,
    bool cullBackFaces,
    double& tMinOut,
    std::vector<std::string>& warnings) {

  // Need at least 3 vertices to form a triangle
  if (indicesView.size() < 3) {
    warnings.emplace_back("Skipping indexed mesh with less than 3 indices");
    return;
  }

  // Converts from various Accessor::ComponentType::XXX values

  double tClosest = -1.0;
  std::optional<double> tCurr;
  int64_t positionsCount = positionView.size();
  bool foundInvalidIndex = false;

  if (primitive.mode == MeshPrimitive::Mode::TRIANGLES) {
    // Iterate through all complete triangles
    for (int64_t i = 2; i < indicesView.size(); i += 3) {
      int64_t vert0Index = static_cast<int64_t>(indicesView[i - 2].value[0]);
      int64_t vert1Index = static_cast<int64_t>(indicesView[i - 1].value[0]);
      int64_t vert2Index = static_cast<int64_t>(indicesView[i].value[0]);

      // Ignore triangle if any index is bogus
      bool validIndices = vert0Index >= 0 && vert0Index < positionsCount &&
                          vert1Index >= 0 && vert1Index < positionsCount &&
                          vert2Index >= 0 && vert2Index < positionsCount;
      if (!validIndices) {
        foundInvalidIndex = true;
        continue;
      }

      auto& viewVert0 = positionView[vert0Index];
      auto& viewVert1 = positionView[vert1Index];
      auto& viewVert2 = positionView[vert2Index];

      glm::dvec3 vert0(
          static_cast<double>(viewVert0.value[0]),
          static_cast<double>(viewVert0.value[1]),
          static_cast<double>(viewVert0.value[2]));
      glm::dvec3 vert1(
          static_cast<double>(viewVert1.value[0]),
          static_cast<double>(viewVert1.value[1]),
          static_cast<double>(viewVert1.value[2]));
      glm::dvec3 vert2(
          static_cast<double>(viewVert2.value[0]),
          static_cast<double>(viewVert2.value[1]),
          static_cast<double>(viewVert2.value[2]));

      tCurr = CesiumGeometry::IntersectionTests::rayTriangleParametric(
          ray,
          vert0,
          vert1,
          vert2,
          cullBackFaces);

      // Set result to this hit if closer, or the first one
      // Only consider hits in front of the ray
      bool validHit = tCurr && tCurr.value() >= 0;
      if (validHit && (tClosest == -1.0 || tCurr.value() < tClosest))
        tClosest = tCurr.value();
    }
  } else if (primitive.mode == MeshPrimitive::Mode::TRIANGLE_STRIP) {
    for (int64_t i = 2; i < indicesView.size(); ++i) {
      int64_t vert0Index = static_cast<int64_t>(indicesView[i - 2].value[0]);
      int64_t vert1Index;
      int64_t vert2Index;
      if (i % 2) {
        vert1Index = static_cast<int64_t>(indicesView[i].value[0]);
        vert2Index = static_cast<int64_t>(indicesView[i - 1].value[0]);
      } else {
        vert1Index = static_cast<int64_t>(indicesView[i - 1].value[0]);
        vert2Index = static_cast<int64_t>(indicesView[i].value[0]);
      }

      bool validIndices = vert0Index >= 0 && vert0Index < positionsCount &&
                          vert1Index >= 0 && vert1Index < positionsCount &&
                          vert2Index >= 0 && vert2Index < positionsCount;
      if (!validIndices) {
        foundInvalidIndex = true;
        continue;
      }

      auto& viewVert0 = positionView[vert0Index];
      auto& viewVert1 = positionView[vert1Index];
      auto& viewVert2 = positionView[vert2Index];

      glm::dvec3 vert0(
          static_cast<double>(viewVert0.value[0]),
          static_cast<double>(viewVert0.value[1]),
          static_cast<double>(viewVert0.value[2]));
      glm::dvec3 vert1(
          static_cast<double>(viewVert1.value[0]),
          static_cast<double>(viewVert1.value[1]),
          static_cast<double>(viewVert1.value[2]));
      glm::dvec3 vert2(
          static_cast<double>(viewVert2.value[0]),
          static_cast<double>(viewVert2.value[1]),
          static_cast<double>(viewVert2.value[2]));

      tCurr = CesiumGeometry::IntersectionTests::rayTriangleParametric(
          ray,
          vert0,
          vert1,
          vert2,
          cullBackFaces);

      bool validHit = tCurr && tCurr.value() >= 0;
      if (validHit && (tClosest == -1.0 || tCurr.value() < tClosest))
        tClosest = tCurr.value();
    }
  } else {
    assert(primitive.mode == MeshPrimitive::Mode::TRIANGLE_FAN);

    int64_t vert0Index = static_cast<int64_t>(indicesView[0].value[0]);

    if (vert0Index < 0 || vert0Index >= positionsCount) {
      foundInvalidIndex = true;
    } else {
      auto& viewVert0 = positionView[vert0Index];
      glm::dvec3 vert0(
          static_cast<double>(viewVert0.value[0]),
          static_cast<double>(viewVert0.value[1]),
          static_cast<double>(viewVert0.value[2]));

      for (int64_t i = 2; i < indicesView.size(); ++i) {
        int64_t vert1Index = static_cast<int64_t>(indicesView[i - 1].value[0]);
        int64_t vert2Index = static_cast<int64_t>(indicesView[i].value[0]);

        bool validIndices = vert1Index >= 0 && vert1Index < positionsCount &&
                            vert2Index >= 0 && vert2Index < positionsCount;
        if (!validIndices) {
          foundInvalidIndex = true;
          continue;
        }

        auto& viewVert1 = positionView[vert1Index];
        auto& viewVert2 = positionView[vert2Index];

        glm::dvec3 vert1(
            static_cast<double>(viewVert1.value[0]),
            static_cast<double>(viewVert1.value[1]),
            static_cast<double>(viewVert1.value[2]));
        glm::dvec3 vert2(
            static_cast<double>(viewVert2.value[0]),
            static_cast<double>(viewVert2.value[1]),
            static_cast<double>(viewVert2.value[2]));

        tCurr = CesiumGeometry::IntersectionTests::rayTriangleParametric(
            ray,
            vert0,
            vert1,
            vert2,
            cullBackFaces);

        bool validHit = tCurr && tCurr.value() >= 0;
        if (validHit && (tCurr < tClosest || tClosest == -1.0))
          tClosest = tCurr.value();
      }
    }
  }

  if (foundInvalidIndex)
    warnings.emplace_back(
        "Found one or more invalid index values for indexed mesh");

  tMinOut = tClosest;
}
} // namespace

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
  for (bool usedIndex : usedIndices) {
    if (usedIndex) {
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

struct VisitMeshIds {
  template <typename Func> void operator()(Model& gltf, Func&& callback) {
    for (Node& node : gltf.nodes) {
      callback(node.mesh);
    }
  }
};

struct VisitMaterialIds {
  template <typename Func> void operator()(Model& gltf, Func&& callback) {
    for (Mesh& mesh : gltf.meshes) {
      for (MeshPrimitive& primitive : mesh.primitives) {
        callback(primitive.material);
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
      CESIUM_ASSERT(newIndex >= 0);
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
            CESIUM_ASSERT(index >= 0 && size_t(index) < usedElements.size());
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

void GltfUtilities::removeUnusedMeshes(
    CesiumGltf::Model& gltf,
    const std::vector<int32_t>& extraUsedMeshIndices) {
  removeUnusedElements(gltf, extraUsedMeshIndices, gltf.meshes, VisitMeshIds());
}

void GltfUtilities::removeUnusedMaterials(
    CesiumGltf::Model& gltf,
    const std::vector<int32_t>& extraUsedMaterialIndices) {
  removeUnusedElements(
      gltf,
      extraUsedMaterialIndices,
      gltf.materials,
      VisitMaterialIds());
}

void GltfUtilities::compactBuffers(CesiumGltf::Model& gltf) {
  for (size_t i = 0; i < gltf.buffers.size() &&
                     i < size_t(std::numeric_limits<int32_t>::max());
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

  CESIUM_ASSERT(size_t(pBuffer->byteLength) == pBuffer->cesium.data.size());

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

  CESIUM_ASSERT(size_t(pBuffer->byteLength) == pBuffer->cesium.data.size());

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

namespace {

template <typename TCallback>
std::invoke_result_t<TCallback, AccessorView<AccessorTypes::VEC3<float>>>
createPositionView(
    const Model& model,
    const Accessor& accessor,
    TCallback&& callback) {
  assert(accessor.type == Accessor::Type::VEC3);

  switch (accessor.componentType) {
  case Accessor::ComponentType::BYTE:
    return callback(AccessorView<AccessorTypes::VEC3<int8_t>>(model, accessor));
  case Accessor::ComponentType::UNSIGNED_BYTE:
    return callback(
        AccessorView<AccessorTypes::VEC3<uint8_t>>(model, accessor));
  case Accessor::ComponentType::SHORT:
    return callback(
        AccessorView<AccessorTypes::VEC3<int16_t>>(model, accessor));
  case Accessor::ComponentType::UNSIGNED_SHORT:
    return callback(
        AccessorView<AccessorTypes::VEC3<uint16_t>>(model, accessor));
  case Accessor::ComponentType::UNSIGNED_INT:
    return callback(
        AccessorView<AccessorTypes::VEC3<uint32_t>>(model, accessor));
  case Accessor::ComponentType::FLOAT:
    return callback(AccessorView<AccessorTypes::VEC3<float>>(model, accessor));
  default:
    return callback(AccessorView<AccessorTypes::VEC3<float>>(
        AccessorViewStatus::InvalidComponentType));
  }
}

std::optional<glm::dvec3> intersectRayScenePrimitive(
    const CesiumGeometry::Ray& ray,
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    const Accessor& positionAccessor,
    const glm::dmat4x4& primitiveToWorld,
    bool cullBackFaces,
    std::vector<std::string>& warnings) {
  glm::dmat4x4 worldToPrimitive = glm::inverse(primitiveToWorld);
  CesiumGeometry::Ray transformedRay = ray.transform(worldToPrimitive);

  // Ignore primitive if we have an AABB from the accessor min/max and the ray
  // doesn't intersect it.
  const std::vector<double>& min = positionAccessor.min;
  const std::vector<double>& max = positionAccessor.max;

  if (min.size() >= 3 && max.size() >= 3) {
    std::optional<double> boxT =
        CesiumGeometry::IntersectionTests::rayAABBParametric(
            transformedRay,
            CesiumGeometry::AxisAlignedBox(
                min[0],
                min[1],
                min[2],
                max[0],
                max[1],
                max[2]));
    if (!boxT)
      return std::optional<glm::dvec3>();
  }

  double tClosest = -1.0;

  // Support all variations of position component types
  //
  // From the glTF spec...
  // "Floating-point data MUST use IEEE-754 single precision format."
  //
  // Yet, the KHR_mesh_quantization extension can specify more
  //
  createPositionView(
      model,
      positionAccessor,
      [&transformedRay,
       &model,
       &primitive,
       cullBackFaces,
       &tClosest,
       &warnings](const auto& positionView) {
        // Bail on invalid view
        if (positionView.status() != AccessorViewStatus::Valid) {
          warnings.emplace_back(
              "Skipping mesh with an invalid position component type");
          return;
        }

        bool hasIndexedTriangles = primitive.indices != -1;
        if (hasIndexedTriangles) {
          const Accessor* indexAccessor =
              Model::getSafe(&model.accessors, primitive.indices);

          if (!indexAccessor) {
            warnings.emplace_back(
                "Skipping mesh with an invalid index accessor id");
            return;
          }

          // Ignore float index types, these are invalid
          // From the glTF spec...
          // "Indices MUST be non-negative integer numbers."
          if (indexAccessor->componentType == Accessor::ComponentType::FLOAT) {
            warnings.emplace_back(
                "Skipping mesh with an invalid index component type");
            return;
          }

          createAccessorView(
              model,
              *indexAccessor,
              [&transformedRay,
               &positionView,
               &primitive,
               cullBackFaces,
               &tClosest,
               &warnings](const auto& indexView) {
                // Bail on invalid view
                if (indexView.status() != AccessorViewStatus::Valid) {
                  warnings.emplace_back(
                      "Could not create accessor view for mesh indices");
                  return;
                }

                findClosestIndexedRayHit(
                    transformedRay,
                    positionView,
                    indexView,
                    primitive,
                    cullBackFaces,
                    tClosest,
                    warnings);
              });
        } else {
          // Non-indexed triangles
          findClosestRayHit(
              transformedRay,
              positionView,
              primitive,
              cullBackFaces,
              tClosest,
              warnings);
        }
      });
  assert(tClosest >= -1.0);

  if (tClosest == -1.0)
    return std::optional<glm::dvec3>();

  // It's temping to return the t value to the caller, but each primitive might
  // have different matrix transforms with different scaling values. The caller
  // should instead compare world distances.
  return transformedRay.pointFromDistance(tClosest);
}

std::string intersectGltfUnsupportedExtensions[] = {
    ExtensionKhrDracoMeshCompression::ExtensionName,
    ExtensionBufferViewExtMeshoptCompression::ExtensionName,
    ExtensionExtMeshGpuInstancing::ExtensionName,
    "KHR_mesh_quantization"};

} // namespace

GltfUtilities::IntersectResult GltfUtilities::intersectRayGltfModel(
    const CesiumGeometry::Ray& ray,
    const CesiumGltf::Model& gltf,
    bool cullBackFaces,
    const glm::dmat4x4& gltfTransform) {
  // We can't currently intersect a ray with a model if the model has any funny
  // business with its vertex positions or if it uses instancing.
  for (const std::string& unsupportedExtension :
       intersectGltfUnsupportedExtensions) {
    if (gltf.isExtensionRequired(unsupportedExtension)) {
      return IntersectResult{
          std::nullopt,
          {fmt::format(
              "Cannot intersect a ray with a glTF model with the {} extension.",
              unsupportedExtension)}};
    }
  }

  glm::dmat4x4 rootTransform = applyRtcCenter(gltf, gltfTransform);
  rootTransform = applyGltfUpAxisTransform(gltf, rootTransform);

  IntersectResult result;

  gltf.forEachPrimitiveInScene(
      -1,
      [ray, cullBackFaces, rootTransform, &result](
          const CesiumGltf::Model& model,
          const CesiumGltf::Node& /*node*/,
          const CesiumGltf::Mesh& mesh,
          const CesiumGltf::MeshPrimitive& primitive,
          const glm::dmat4& nodeTransform) {
        // Ignore non-triangles. Points and lines have no area to intersect
        bool isTriangleMode =
            primitive.mode == MeshPrimitive::Mode::TRIANGLES ||
            primitive.mode == MeshPrimitive::Mode::TRIANGLE_STRIP ||
            primitive.mode == MeshPrimitive::Mode::TRIANGLE_FAN;
        if (!isTriangleMode)
          return;

        // Skip primitives that can't access positions
        auto positionAccessorIt = primitive.attributes.find("POSITION");
        if (positionAccessorIt == primitive.attributes.end()) {
          result.warnings.emplace_back(
              "Skipping mesh without a position attribute");
          return;
        }
        int positionAccessorID = positionAccessorIt->second;
        const Accessor* pPositionAccessor =
            Model::getSafe(&model.accessors, positionAccessorID);
        if (!pPositionAccessor) {
          result.warnings.emplace_back(
              "Skipping mesh with an invalid position accessor id");
          return;
        }

        // From the glTF spec, the POSITION accessor must use VEC3
        // But we should still protect against malformed gltfs
        if (pPositionAccessor->type != AccessorSpec::Type::VEC3) {
          result.warnings.emplace_back(
              "Skipping mesh with a non-vec3 position accessor");
          return;
        }

        glm::dmat4x4 primitiveToWorld = rootTransform * nodeTransform;

        std::optional<glm::dvec3> primitiveHitPoint;
        primitiveHitPoint = intersectRayScenePrimitive(
            ray,
            model,
            primitive,
            *pPositionAccessor,
            primitiveToWorld,
            cullBackFaces,
            result.warnings);

        if (!primitiveHitPoint.has_value())
          return;

        // We have a hit, determine if it's the closest one

        // Normalize the homogeneous coordinates
        // Ex. transformed by projection matrx
        glm::dvec4 homogeneousWorldPoint =
            primitiveToWorld * glm::dvec4(*primitiveHitPoint, 1.0);
        bool needsWDivide =
            homogeneousWorldPoint.w != 1.0 && homogeneousWorldPoint.w != 0.0;
        if (needsWDivide) {
          homogeneousWorldPoint.x /= homogeneousWorldPoint.w;
          homogeneousWorldPoint.y /= homogeneousWorldPoint.w;
          homogeneousWorldPoint.z /= homogeneousWorldPoint.w;
        }
        glm::dvec3 worldPoint(
            homogeneousWorldPoint.x,
            homogeneousWorldPoint.y,
            homogeneousWorldPoint.z);

        glm::dvec3 rayToWorldPoint = worldPoint - ray.getOrigin();
        double rayToWorldPointDistanceSq =
            glm::dot(rayToWorldPoint, rayToWorldPoint);

        // Use in result if it's first
        int32_t meshId = static_cast<int32_t>(&mesh - &model.meshes[0]);
        int32_t primitiveId =
            static_cast<int32_t>(&primitive - &mesh.primitives[0]);

        if (!result.hit.has_value()) {
          result.hit = RayGltfHit{
              std::move(*primitiveHitPoint),
              std::move(primitiveToWorld),
              std::move(worldPoint),
              rayToWorldPointDistanceSq,
              meshId,
              primitiveId};
          return;
        }

        // Use in result if it's closer
        if (rayToWorldPointDistanceSq < result.hit->rayToWorldPointDistanceSq) {
          result.hit->primitivePoint = std::move(*primitiveHitPoint);
          result.hit->primitiveToWorld = std::move(primitiveToWorld);
          result.hit->worldPoint = std::move(worldPoint);
          result.hit->rayToWorldPointDistanceSq = rayToWorldPointDistanceSq;
          result.hit->meshId = meshId;
          result.hit->primitiveId = primitiveId;
        }
      });

  return result;
}

} // namespace CesiumGltfContent
