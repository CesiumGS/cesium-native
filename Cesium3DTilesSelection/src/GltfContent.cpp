#include "Cesium3DTilesSelection/GltfContent.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"
#include "CesiumGltf/AccessorView.h"
#include "CesiumGltf/AccessorWriter.h"
#include "CesiumUtility/Math.h"
#include "CesiumUtility/Tracing.h"
#include "CesiumUtility/joinToString.h"
#include <optional>
#include <stdexcept>

namespace Cesium3DTilesSelection {

/*static*/ CesiumGltf::GltfReader GltfContent::_gltfReader{};

std::unique_ptr<TileContentLoadResult>
GltfContent::load(const TileContentLoadInput& input) {
  return load(input.pLogger, input.url, input.data);
}

/*static*/ std::unique_ptr<TileContentLoadResult> GltfContent::load(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& url,
    const gsl::span<const std::byte>& data) {
  CESIUM_TRACE("Cesium3DTilesSelection::GltfContent::load");
  std::unique_ptr<TileContentLoadResult> pResult =
      std::make_unique<TileContentLoadResult>();

  CesiumGltf::ModelReaderResult loadedModel =
      GltfContent::_gltfReader.readModel(data);
  if (!loadedModel.errors.empty()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Failed to load binary glTF from {}:\n- {}",
        url,
        CesiumUtility::joinToString(loadedModel.errors, "\n- "));
  }
  if (!loadedModel.warnings.empty()) {
    SPDLOG_LOGGER_WARN(
        pLogger,
        "Warning when loading binary glTF from {}:\n- {}",
        url,
        CesiumUtility::joinToString(loadedModel.warnings, "\n- "));
  }

  if (loadedModel.model) {
    loadedModel.model.value().extras["Cesium3DTilesSelection_TileUrl"] = url;
  }

  pResult->model = std::move(loadedModel.model);
  return pResult;
}

static int generateOverlayTextureCoordinates(
    CesiumGltf::Model& gltf,
    int positionAccessorIndex,
    const glm::dmat4x4& transform,
    const CesiumGeospatial::Projection& projection,
    const CesiumGeometry::Rectangle& rectangle,
    double& west,
    double& south,
    double& east,
    double& north,
    double& minimumHeight,
    double& maximumHeight) {
  CESIUM_TRACE(
      "Cesium3DTilesSelection::GltfContent::generateOverlayTextureCoordinates");
  std::vector<CesiumGltf::Buffer>& buffers = gltf.buffers;
  std::vector<CesiumGltf::BufferView>& bufferViews = gltf.bufferViews;
  std::vector<CesiumGltf::Accessor>& accessors = gltf.accessors;

  int uvBufferId = static_cast<int>(buffers.size());
  CesiumGltf::Buffer& uvBuffer = buffers.emplace_back();

  int uvBufferViewId = static_cast<int>(bufferViews.size());
  bufferViews.emplace_back();

  int uvAccessorId = static_cast<int>(accessors.size());
  accessors.emplace_back();

  CesiumGltf::AccessorView<glm::vec3> positionView(gltf, positionAccessorIndex);
  if (positionView.status() != CesiumGltf::AccessorViewStatus::Valid) {
    return -1;
  }

  uvBuffer.cesium.data.resize(size_t(positionView.size()) * 2 * sizeof(float));

  CesiumGltf::BufferView& uvBufferView =
      gltf.bufferViews[static_cast<size_t>(uvBufferViewId)];
  uvBufferView.buffer = uvBufferId;
  uvBufferView.byteOffset = 0;
  uvBufferView.byteStride = 2 * sizeof(float);
  uvBufferView.byteLength = int64_t(uvBuffer.cesium.data.size());
  uvBufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;

  CesiumGltf::Accessor& uvAccessor =
      gltf.accessors[static_cast<size_t>(uvAccessorId)];
  uvAccessor.bufferView = uvBufferViewId;
  uvAccessor.byteOffset = 0;
  uvAccessor.componentType = CesiumGltf::Accessor::ComponentType::FLOAT;
  uvAccessor.count = int64_t(positionView.size());
  uvAccessor.type = CesiumGltf::Accessor::Type::VEC2;

  CesiumGltf::AccessorWriter<glm::vec2> uvWriter(gltf, uvAccessorId);
  assert(uvWriter.status() == CesiumGltf::AccessorViewStatus::Valid);

  double width = rectangle.computeWidth();
  double height = rectangle.computeHeight();

  for (int64_t i = 0; i < positionView.size(); ++i) {
    // Get the ECEF position
    glm::vec3 position = positionView[i];
    glm::dvec3 positionEcef = transform * glm::dvec4(position, 1.0);

    // Convert it to cartographic
    std::optional<CesiumGeospatial::Cartographic> cartographic =
        CesiumGeospatial::Ellipsoid::WGS84.cartesianToCartographic(
            positionEcef);
    if (!cartographic) {
      uvWriter[i] = glm::dvec2(0.0, 0.0);
      continue;
    }

    // Project it with the raster overlay's projection
    glm::dvec3 projectedPosition =
        projectPosition(projection, cartographic.value());

    double longitude = cartographic.value().longitude;
    double latitude = cartographic.value().latitude;
    double ellipsoidHeight = cartographic.value().height;

    // If the position is near the anti-meridian and the projected position is
    // outside the expected range, try using the equivalent longitude on the
    // other side of the anti-meridian to see if that gets us closer.
    if (glm::abs(
            glm::abs(cartographic.value().longitude) -
            CesiumUtility::Math::ONE_PI) < CesiumUtility::Math::EPSILON5 &&
        (projectedPosition.x < rectangle.minimumX ||
         projectedPosition.x > rectangle.maximumX ||
         projectedPosition.y < rectangle.minimumY ||
         projectedPosition.y > rectangle.maximumY)) {
      cartographic.value().longitude += cartographic.value().longitude < 0.0
                                            ? CesiumUtility::Math::TWO_PI
                                            : -CesiumUtility::Math::TWO_PI;
      glm::dvec3 projectedPosition2 =
          projectPosition(projection, cartographic.value());

      double distance1 = rectangle.computeSignedDistance(projectedPosition);
      double distance2 = rectangle.computeSignedDistance(projectedPosition2);

      if (distance2 < distance1) {
        projectedPosition = projectedPosition2;
        longitude = cartographic.value().longitude;
      }
    }

    // The computation of longitude is very unstable at the poles,
    // so don't let extreme latitudes affect the longitude bounding box.
    if (glm::abs(glm::abs(latitude) - CesiumUtility::Math::PI_OVER_TWO) >
        CesiumUtility::Math::EPSILON6) {
      west = glm::min(west, longitude);
      east = glm::max(east, longitude);
    }
    south = glm::min(south, latitude);
    north = glm::max(north, latitude);
    minimumHeight = glm::min(minimumHeight, ellipsoidHeight);
    maximumHeight = glm::max(maximumHeight, ellipsoidHeight);

    // Scale to (0.0, 0.0) at the (minimumX, minimumY) corner, and (1.0, 1.0) at
    // the (maximumX, maximumY) corner. The coordinates should stay inside these
    // bounds if the input rectangle actually bounds the vertices, but we'll
    // clamp to be safe.
    glm::vec2 uv(
        CesiumUtility::Math::clamp(
            (projectedPosition.x - rectangle.minimumX) / width,
            0.0,
            1.0),
        CesiumUtility::Math::clamp(
            (projectedPosition.y - rectangle.minimumY) / height,
            0.0,
            1.0));

    uvWriter[i] = uv;
  }

  return uvAccessorId;
}

/*static*/ CesiumGeospatial::BoundingRegion
GltfContent::createRasterOverlayTextureCoordinates(
    CesiumGltf::Model& gltf,
    uint32_t textureCoordinateID,
    const CesiumGeospatial::Projection& projection,
    const CesiumGeometry::Rectangle& rectangle) {
  CESIUM_TRACE("Cesium3DTilesSelection::GltfContent::"
               "createRasterOverlayTextureCoordinates");
  std::vector<int> positionAccessorsToTextureCoordinateAccessor;
  positionAccessorsToTextureCoordinateAccessor.resize(gltf.accessors.size(), 0);

  std::string attributeName =
      "_CESIUMOVERLAY_" + std::to_string(textureCoordinateID);

  double west = CesiumUtility::Math::ONE_PI;
  double south = CesiumUtility::Math::PI_OVER_TWO;
  double east = -CesiumUtility::Math::ONE_PI;
  double north = -CesiumUtility::Math::PI_OVER_TWO;
  double minimumHeight = std::numeric_limits<double>::max();
  double maximumHeight = std::numeric_limits<double>::lowest();

  Gltf::forEachPrimitiveInScene(
      gltf,
      -1,
      [&positionAccessorsToTextureCoordinateAccessor,
       &attributeName,
       &projection,
       &rectangle,
       &west,
       &south,
       &east,
       &north,
       &minimumHeight,
       &maximumHeight](
          CesiumGltf::Model& gltf_,
          CesiumGltf::Node& /*node*/,
          CesiumGltf::Mesh& /*mesh*/,
          CesiumGltf::MeshPrimitive& primitive,
          const glm::dmat4& transform) {
        auto positionIt = primitive.attributes.find("POSITION");
        if (positionIt == primitive.attributes.end()) {
          return;
        }

        int positionAccessorIndex = positionIt->second;
        if (positionAccessorIndex < 0 ||
            positionAccessorIndex >= static_cast<int>(gltf_.accessors.size())) {
          return;
        }

        int textureCoordinateAccessorIndex =
            positionAccessorsToTextureCoordinateAccessor[static_cast<size_t>(
                positionAccessorIndex)];
        if (textureCoordinateAccessorIndex > 0) {
          primitive.attributes[attributeName] = textureCoordinateAccessorIndex;
          return;
        }

        // TODO remove this check
        if (primitive.attributes.find(attributeName) !=
            primitive.attributes.end()) {
          return;
        }

        // Generate new texture coordinates
        int nextTextureCoordinateAccessorIndex =
            generateOverlayTextureCoordinates(
                gltf_,
                positionAccessorIndex,
                transform,
                projection,
                rectangle,
                west,
                south,
                east,
                north,
                minimumHeight,
                maximumHeight);
        if (nextTextureCoordinateAccessorIndex < 0) {
          return;
        }

        primitive.attributes[attributeName] =
            nextTextureCoordinateAccessorIndex;
        positionAccessorsToTextureCoordinateAccessor[static_cast<size_t>(
            positionAccessorIndex)] = nextTextureCoordinateAccessorIndex;
      });

  return CesiumGeospatial::BoundingRegion(
      CesiumGeospatial::GlobeRectangle(west, south, east, north),
      minimumHeight,
      maximumHeight);
}

template <typename TIndex>
static void generateNormals(
    CesiumGltf::Model& gltf,
    CesiumGltf::MeshPrimitive& primitive,
    const CesiumGltf::AccessorView<glm::vec3>& positionView,
    const std::optional<CesiumGltf::Accessor>& indexAccessor) {

  size_t count = static_cast<size_t>(positionView.size());
  size_t normalBufferStride = sizeof(glm::vec3);
  size_t normalBufferSize = count * normalBufferStride;

  std::vector<std::byte> normalByteBuffer(normalBufferSize);
  gsl::span<glm::vec3> normals(
      reinterpret_cast<glm::vec3*>(normalByteBuffer.data()),
      count);
  std::vector<uint8_t> trisPerVertex(count);

  // Accumulates a smooth normal for each vertex over its connected triangles.
  auto smoothNormalsOverTriangle =
      [&positionView,
       &normals,
       &trisPerVertex](TIndex tIndex0, TIndex tIndex1, TIndex tIndex2) -> void {
    size_t index0 = static_cast<size_t>(tIndex0);
    size_t index1 = static_cast<size_t>(tIndex1);
    size_t index2 = static_cast<size_t>(tIndex2);

    const glm::vec3& vertex0 = positionView[static_cast<int64_t>(index0)];
    const glm::vec3& vertex1 = positionView[static_cast<int64_t>(index1)];
    const glm::vec3& vertex2 = positionView[static_cast<int64_t>(index2)];

    glm::vec3 triangleNormal =
        glm::normalize(glm::cross(vertex1 - vertex0, vertex2 - vertex0));

    // Average the vertex's currently accumulated normal with the normal of the
    // new triangle. This is a weighted average of n:1 where n is the number of
    // triangles found to be connected to this vertex so far.
    normals[index0] = glm::normalize(
        (float)trisPerVertex[index0] * normals[index0] + triangleNormal);
    ++trisPerVertex[index0];

    normals[index1] = glm::normalize(
        (float)trisPerVertex[index1] * normals[index1] + triangleNormal);
    ++trisPerVertex[index1];

    normals[index2] = glm::normalize(
        (float)trisPerVertex[index2] * normals[index2] + triangleNormal);
    ++trisPerVertex[index2];
  };

  if (indexAccessor) {
    CesiumGltf::AccessorView<TIndex> indexView(gltf, *indexAccessor);
    if (indexView.status() != CesiumGltf::AccessorViewStatus::Valid) {
      return;
    }

    switch (primitive.mode) {
    case CesiumGltf::MeshPrimitive::Mode::TRIANGLES:
      for (int64_t i = 2; i < indexView.size(); i += 3) {
        TIndex index0 = indexView[i - 2];
        TIndex index1 = indexView[i - 1];
        TIndex index2 = indexView[i];

        smoothNormalsOverTriangle(index0, index1, index2);
      }
      break;

    case CesiumGltf::MeshPrimitive::Mode::TRIANGLE_STRIP:
      for (int64_t i = 0; i < indexView.size() - 2; ++i) {
        TIndex index0;
        TIndex index1;
        TIndex index2;

        if (i % 2) {
          index0 = indexView[i];
          index1 = indexView[i + 2];
          index2 = indexView[i + 1];
        } else {
          index0 = indexView[i];
          index1 = indexView[i + 1];
          index2 = indexView[i + 2];
        }

        smoothNormalsOverTriangle(index0, index1, index2);
      }
      break;

    case CesiumGltf::MeshPrimitive::Mode::TRIANGLE_FAN:
      if (indexView.size() < 3) {
        return;
      }

      {
        TIndex index0 = indexView[0];
        for (int64_t i = 2; i < indexView.size(); ++i) {
          TIndex index1 = indexView[i - 1];
          TIndex index2 = indexView[i];

          smoothNormalsOverTriangle(index0, index1, index2);
        }
      }
      break;

    default:
      return;
    }

  } else {

    // No index buffer available, just use the vertex buffer sequentially.
    switch (primitive.mode) {
    case CesiumGltf::MeshPrimitive::Mode::TRIANGLES:
      for (TIndex i = 2; i < static_cast<TIndex>(count); i += TIndex(3)) {
        smoothNormalsOverTriangle(i - TIndex(2), i - TIndex(1), i);
      }
      break;

    case CesiumGltf::MeshPrimitive::Mode::TRIANGLE_STRIP:
      for (TIndex i = 0; i < static_cast<TIndex>(count) - 2; ++i) {
        TIndex index0;
        TIndex index1;
        TIndex index2;

        if (i % 2) {
          index0 = i;
          index1 = i + TIndex(2);
          index2 = i + TIndex(1);
        } else {
          index0 = i;
          index1 = i + TIndex(1);
          index2 = i + TIndex(2);
        }

        smoothNormalsOverTriangle(index0, index1, index2);
      }
      break;

    case CesiumGltf::MeshPrimitive::Mode::TRIANGLE_FAN:
      for (TIndex i = 2; i < static_cast<TIndex>(count); ++i) {
        smoothNormalsOverTriangle(0, i - TIndex(1), i);
      }
      break;

    default:
      return;
    }
  }

  size_t normalBufferId = gltf.buffers.size();
  CesiumGltf::Buffer& normalBuffer = gltf.buffers.emplace_back();
  normalBuffer.byteLength = static_cast<int64_t>(normalBufferSize);
  normalBuffer.cesium.data = std::move(normalByteBuffer);

  size_t normalBufferViewId = gltf.bufferViews.size();
  CesiumGltf::BufferView& normalBufferView = gltf.bufferViews.emplace_back();
  normalBufferView.buffer = static_cast<int32_t>(normalBufferId);
  normalBufferView.byteLength = static_cast<int64_t>(normalBufferSize);
  normalBufferView.byteOffset = 0;
  normalBufferView.byteStride = static_cast<int64_t>(normalBufferStride);
  normalBufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;

  size_t normalAccessorId = gltf.accessors.size();
  CesiumGltf::Accessor& normalAccessor = gltf.accessors.emplace_back();
  normalAccessor.byteOffset = 0;
  normalAccessor.bufferView = static_cast<int32_t>(normalBufferViewId);
  normalAccessor.componentType = CesiumGltf::Accessor::ComponentType::FLOAT;
  normalAccessor.count = positionView.size();
  normalAccessor.type = CesiumGltf::Accessor::Type::VEC3;

  primitive.attributes.emplace(
      "NORMAL",
      static_cast<int32_t>(normalAccessorId));
}

static void generateNormals(
    CesiumGltf::Model& gltf,
    CesiumGltf::MeshPrimitive& primitive,
    const CesiumGltf::AccessorView<glm::vec3>& positionView,
    const std::optional<CesiumGltf::Accessor>& indexAccessor) {
  if (indexAccessor) {
    switch (indexAccessor->componentType) {
    case CesiumGltf::Accessor::ComponentType::BYTE:
      generateNormals<int8_t>(gltf, primitive, positionView, indexAccessor);
      break;
    case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE:
      generateNormals<uint8_t>(gltf, primitive, positionView, indexAccessor);
      break;
    case CesiumGltf::Accessor::ComponentType::SHORT:
      generateNormals<int16_t>(gltf, primitive, positionView, indexAccessor);
      break;
    case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT:
      generateNormals<uint16_t>(gltf, primitive, positionView, indexAccessor);
      break;
    case CesiumGltf::Accessor::ComponentType::UNSIGNED_INT:
      generateNormals<uint32_t>(gltf, primitive, positionView, indexAccessor);
      break;
    case CesiumGltf::Accessor::ComponentType::FLOAT:
      return;
    };
  } else {
    generateNormals<size_t>(gltf, primitive, positionView, std::nullopt);
  }
}

/*static*/
void GltfContent::generateMissingNormalsSmooth(CesiumGltf::Model& gltf) {
  Gltf::forEachPrimitiveInScene(
      gltf,
      -1,
      [](CesiumGltf::Model& gltf_,
         CesiumGltf::Node& /*node*/,
         CesiumGltf::Mesh& /*mesh*/,
         CesiumGltf::MeshPrimitive& primitive,
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

        int positionAccessorId = positionIt->second;
        CesiumGltf::AccessorView<glm::vec3> positionView(
            gltf_,
            positionAccessorId);
        if (positionView.status() != CesiumGltf::AccessorViewStatus::Valid) {
          return;
        }

        if (primitive.indices < 0 ||
            static_cast<size_t>(primitive.indices) >= gltf_.accessors.size()) {
          generateNormals(gltf_, primitive, positionView, std::nullopt);
        } else {
          CesiumGltf::Accessor& indexAccessor =
              gltf_.accessors[static_cast<size_t>(primitive.indices)];
          generateNormals(gltf_, primitive, positionView, indexAccessor);
        }
      });
}
} // namespace Cesium3DTilesSelection
