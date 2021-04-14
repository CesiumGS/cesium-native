#include "Cesium3DTiles/GltfContent.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "CesiumGltf/AccessorView.h"
#include "CesiumGltf/AccessorWriter.h"
#include "CesiumGltf/GltfReader.h"
#include "CesiumUtility/Math.h"
#include "CesiumUtility/joinToString.h"
#include <stdexcept>

namespace Cesium3DTiles {

std::unique_ptr<TileContentLoadResult>
GltfContent::load(const TileContentLoadInput& input) {
  return load(input.pLogger, input.url, input.data);
}

/*static*/ std::unique_ptr<TileContentLoadResult> GltfContent::load(
    std::shared_ptr<spdlog::logger> pLogger,
    const std::string& url,
    const gsl::span<const std::byte>& data) {
  std::unique_ptr<TileContentLoadResult> pResult =
      std::make_unique<TileContentLoadResult>();

  // TODO: don't create a new Reader every time.
  CesiumGltf::GltfReader reader;
  CesiumGltf::ModelReaderResult loadedModel = reader.readModel(data);
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
    loadedModel.model.value().extras["Cesium3DTiles_TileUrl"] = url;
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

} // namespace Cesium3DTiles
