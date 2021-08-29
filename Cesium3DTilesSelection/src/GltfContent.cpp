#include "Cesium3DTilesSelection/GltfContent.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"
#include "CesiumGltf/AccessorView.h"
#include "CesiumGltf/AccessorWriter.h"
#include "CesiumUtility/Math.h"
#include "CesiumUtility/Tracing.h"
#include "CesiumUtility/joinToString.h"
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
    loadedModel.model.value().extras["Cesium3DTiles_TileUrl"] = url;
  }

  pResult->model = std::move(loadedModel.model);
  return pResult;
}

namespace {

/**
 * @brief Creates a writer for UV texture coordinates in the given gltf.
 *
 * This will populate the given glTF with a new buffer, bufferView and
 * accessor that is capable of storing the specified `count` of 2D
 * floating point texture coordinates, and return a writer to that
 * accessor data.
 *
 * @param gltf The glTF model.
 * @param count The number of elements for the acccessor.
 * @param resultAccessorId Will store the ID of the accessor that
 * the writer will write to.
 * @return The accessor writer.
 */
CesiumGltf::AccessorWriter<glm::vec2>
createUvWriter(CesiumGltf::Model& gltf, size_t count, int& resultUvAccessorId) {
  std::vector<CesiumGltf::Buffer>& buffers = gltf.buffers;
  std::vector<CesiumGltf::BufferView>& bufferViews = gltf.bufferViews;
  std::vector<CesiumGltf::Accessor>& accessors = gltf.accessors;

  int uvBufferId = static_cast<int>(buffers.size());
  CesiumGltf::Buffer& uvBuffer = buffers.emplace_back();

  int uvBufferViewId = static_cast<int>(bufferViews.size());
  bufferViews.emplace_back();

  int uvAccessorId = static_cast<int>(accessors.size());
  accessors.emplace_back();

  uvBuffer.cesium.data.resize(count * 2 * sizeof(float));

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
  uvAccessor.count = int64_t(count);
  uvAccessor.type = CesiumGltf::Accessor::Type::VEC2;

  CesiumGltf::AccessorWriter<glm::vec2> uvWriter(gltf, uvAccessorId);
  assert(uvWriter.status() == CesiumGltf::AccessorViewStatus::Valid);
  resultUvAccessorId = uvAccessorId;
  return uvWriter;
}

bool isNearAntiMeridian(double longitude) {
  return glm::epsilonEqual(
      glm::abs(longitude),
      CesiumUtility::Math::ONE_PI,
      CesiumUtility::Math::EPSILON5);
}
bool isNearPole(double latitude) {
  return glm::epsilonEqual(
      glm::abs(latitude),
      CesiumUtility::Math::PI_OVER_TWO,
      CesiumUtility::Math::EPSILON6);
}
} // namespace

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

  CesiumGltf::AccessorView<glm::vec3> positionView(gltf, positionAccessorIndex);
  if (positionView.status() != CesiumGltf::AccessorViewStatus::Valid) {
    SPDLOG_LOGGER_ERROR(
        spdlog::default_logger(),
        "Invalid position accessor for generateOverlayTextureCoordinates");
    return -1;
  }

  int uvAccessorId = -1;
  CesiumGltf::AccessorWriter<glm::vec2> uvWriter =
      createUvWriter(gltf, size_t(positionView.size()), uvAccessorId);

  double width = rectangle.computeWidth();
  double height = rectangle.computeHeight();

  for (int64_t i = 0; i < positionView.size(); ++i) {
    // Get the ECEF position
    glm::vec3 position = positionView[i];
    glm::dvec3 positionEcef = transform * glm::dvec4(position, 1.0);

    // Convert it to cartographic
    std::optional<CesiumGeospatial::Cartographic> optionalCartographic =
        CesiumGeospatial::Ellipsoid::WGS84.cartesianToCartographic(
            positionEcef);
    if (!optionalCartographic) {
      uvWriter[i] = glm::dvec2(0.0, 0.0);
      continue;
    }

    CesiumGeospatial::Cartographic& cartographic = optionalCartographic.value();

    // Project it with the raster overlay's projection
    glm::dvec3 projectedPosition = projectPosition(projection, cartographic);

    double longitude = cartographic.longitude;
    double latitude = cartographic.latitude;
    double ellipsoidHeight = cartographic.height;

    // If the position is near the anti-meridian and the projected position is
    // outside the expected range, try using the equivalent longitude on the
    // other side of the anti-meridian to see if that gets us closer.

    if (isNearAntiMeridian(cartographic.longitude) &&
        !rectangle.contains(
            glm::dvec2(projectedPosition.x, projectedPosition.y))) {
      double longitude2 = cartographic.longitude < 0.0
                              ? CesiumUtility::Math::TWO_PI
                              : -CesiumUtility::Math::TWO_PI;
      CesiumGeospatial::Cartographic cartographic2(
          longitude2,
          cartographic.latitude,
          cartographic.height);
      glm::dvec3 projectedPosition2 = projectPosition(projection, cartographic);

      double distance1 = rectangle.computeSignedDistance(projectedPosition);
      double distance2 = rectangle.computeSignedDistance(projectedPosition2);

      if (distance2 < distance1) {
        projectedPosition = projectedPosition2;
        longitude = cartographic2.longitude;
      }
    }

    // The computation of longitude is very unstable at the poles,
    // so don't let extreme latitudes affect the longitude bounding box.
    if (!isNearPole(latitude)) {
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
    const glm::dmat4& transform,
    int32_t textureCoordinateID,
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
      [&transform,
       &positionAccessorsToTextureCoordinateAccessor,
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
          const glm::dmat4& nodeTransform) {
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
                transform * nodeTransform,
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

} // namespace Cesium3DTilesSelection
