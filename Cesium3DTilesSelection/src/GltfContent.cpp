#include "Cesium3DTilesSelection/GltfContent.h"

#include "Cesium3DTilesSelection/spdlog-cesium.h"

#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/AxisTransforms.h>
#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/AccessorWriter.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/Math.h>
#include <CesiumUtility/Tracing.h>
#include <CesiumUtility/joinToString.h>

#include <optional>
#include <stdexcept>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumGltfReader;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

/*static*/ CesiumGltfReader::GltfReader GltfContent::_gltfReader{};

Future<std::unique_ptr<TileContentLoadResult>>
GltfContent::load(const TileContentLoadInput& input) {
  return load(
      input.asyncSystem,
      input.pLogger,
      input.pRequest->url(),
      input.pRequest->headers(),
      input.pAssetAccessor,
      input.pRequest->response()->data());
}

/*static*/
Future<std::unique_ptr<TileContentLoadResult>> GltfContent::load(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& url,
    const HttpHeaders& headers,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const gsl::span<const std::byte>& data) {
  CESIUM_TRACE("Cesium3DTilesSelection::GltfContent::load");

  CesiumGltfReader::GltfReaderResult loadedModel =
      GltfContent::_gltfReader.readGltf(data);
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

  return CesiumGltfReader::GltfReader::resolveExternalData(
             asyncSystem,
             url,
             headers,
             pAssetAccessor,
             std::move(loadedModel))
      .thenInWorkerThread(
          [pLogger, url](CesiumGltfReader::GltfReaderResult&& resolvedModel) {
            std::unique_ptr<TileContentLoadResult> pResult =
                std::make_unique<TileContentLoadResult>();

            if (!resolvedModel.errors.empty()) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Failed resolving external glTF buffers from {}:\n- {}",
                  url,
                  CesiumUtility::joinToString(resolvedModel.errors, "\n- "));
            }

            if (!resolvedModel.warnings.empty()) {
              SPDLOG_LOGGER_WARN(
                  pLogger,
                  "Warning when resolving external gltf buffers from {}:\n- {}",
                  url,
                  CesiumUtility::joinToString(resolvedModel.warnings, "\n- "));
            }

            pResult->model = std::move(resolvedModel.model);
            return pResult;
          });
}

/*static*/ std::optional<TileContentDetailsForOverlays>
GltfContent::createRasterOverlayTextureCoordinates(
    CesiumGltf::Model& gltf,
    const glm::dmat4& modelToEcefTransform,
    int32_t firstTextureCoordinateID,
    const std::optional<GlobeRectangle>& globeRectangle,
    std::vector<Projection>&& projections) {
  if (projections.empty()) {
    return std::nullopt;
  }

  CESIUM_TRACE("Cesium3DTilesSelection::GltfContent::"
               "createRasterOverlayTextureCoordinates");

  // Compute the bounds of the tile if they're not provided.
  GlobeRectangle bounds =
      globeRectangle
          ? *globeRectangle
          : GltfContent::computeBoundingRegion(gltf, modelToEcefTransform)
                .getRectangle();

  // Currently, a Longitude/Latitude Rectangle maps perfectly to all possible
  // projection types, because the only possible projection types are
  // Geographic and Web Mercator. In the future if/when we add projections
  // that don't have this convenient property, we'll need to compute the
  // Rectangle for each projection directly from the vertex positions.
  std::vector<Rectangle> rectangles(projections.size());
  for (size_t i = 0; i < projections.size(); ++i) {
    rectangles[i] = projectRectangleSimple(projections[i], bounds);
  }

  glm::dmat4 rootTransform = modelToEcefTransform;
  rootTransform = applyRtcCenter(gltf, rootTransform);
  rootTransform = applyGltfUpAxisTransform(gltf, rootTransform);

  std::vector<int> positionAccessorsToTextureCoordinateAccessor;
  positionAccessorsToTextureCoordinateAccessor.resize(gltf.accessors.size(), 0);

  // When computing the tile's bounds, ignore tiles that are less than 1/1000th
  // of a tile width from the North or South pole. Longitudes cannot be trusted
  // at such extreme latitudes.
  BoundingRegionBuilder computedBounds;
  computedBounds.setPoleTolerance(0.001 * bounds.computeHeight());

  auto createTextureCoordinatesForPrimitive =
      [&](CesiumGltf::Model& gltf,
          CesiumGltf::Node& /*node*/,
          CesiumGltf::Mesh& /*mesh*/,
          CesiumGltf::MeshPrimitive& primitive,
          const glm::dmat4& nodeTransform) {
        auto positionIt = primitive.attributes.find("POSITION");
        if (positionIt == primitive.attributes.end()) {
          return;
        }

        const int positionAccessorIndex = positionIt->second;
        if (positionAccessorIndex < 0 ||
            positionAccessorIndex >= static_cast<int>(gltf.accessors.size())) {
          return;
        }

        const int32_t firstTextureCoordinateAccessorIndex =
            positionAccessorsToTextureCoordinateAccessor[static_cast<size_t>(
                positionAccessorIndex)];
        if (firstTextureCoordinateAccessorIndex > 0) {
          // Already created texture coordinates for this projection, so use
          // them.
          for (size_t i = 0; i < projections.size(); ++i) {
            std::string attributeName =
                "_CESIUMOVERLAY_" +
                std::to_string(firstTextureCoordinateID + int32_t(i));
            primitive.attributes[attributeName] =
                firstTextureCoordinateAccessorIndex + int32_t(i);
          }
          return;
        }

        const glm::dmat4 fullTransform = rootTransform * nodeTransform;

        std::vector<CesiumGltf::Buffer>& buffers = gltf.buffers;
        std::vector<CesiumGltf::BufferView>& bufferViews = gltf.bufferViews;
        std::vector<CesiumGltf::Accessor>& accessors = gltf.accessors;

        positionAccessorsToTextureCoordinateAccessor[size_t(
            positionAccessorIndex)] = int32_t(gltf.accessors.size());

        // Create a buffer, bufferView, accessor, and writer for each set of
        // coordinates. Reserve space for them to avoid unnecessary
        // reallocations and to prevent earlier buffers from becoming invalid
        // after we've created an AccessorWriter for it and then add _another_
        // buffer.
        std::vector<CesiumGltf::AccessorWriter<glm::vec2>> uvWriters;
        uvWriters.reserve(projections.size());
        buffers.reserve(buffers.size() + projections.size());
        bufferViews.reserve(bufferViews.size() + projections.size());
        accessors.reserve(accessors.size() + projections.size());

        const CesiumGltf::AccessorView<glm::vec3> positionView(
            gltf,
            positionAccessorIndex);
        if (positionView.status() != CesiumGltf::AccessorViewStatus::Valid) {
          return;
        }

        for (size_t i = 0; i < projections.size(); ++i) {
          const int uvBufferId = static_cast<int>(buffers.size());
          CesiumGltf::Buffer& uvBuffer = buffers.emplace_back();

          const int uvBufferViewId = static_cast<int>(bufferViews.size());
          bufferViews.emplace_back();

          const int uvAccessorId = static_cast<int>(accessors.size());
          accessors.emplace_back();

          uvBuffer.cesium.data.resize(
              size_t(positionView.size()) * 2 * sizeof(float));

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

          [[maybe_unused]] AccessorWriter<glm::vec2>& uvWriter =
              uvWriters.emplace_back(gltf, uvAccessorId);
          assert(uvWriter.status() == CesiumGltf::AccessorViewStatus::Valid);

          std::string attributeName =
              "_CESIUMOVERLAY_" +
              std::to_string(firstTextureCoordinateID + int32_t(i));
          primitive.attributes[attributeName] = uvAccessorId;
        }

        // Generate texture coordinates for each position.
        for (int64_t positionIndex = 0; positionIndex < positionView.size();
             ++positionIndex) {
          // Get the ECEF position
          const glm::vec3 position = positionView[positionIndex];
          const glm::dvec3 positionEcef =
              glm::dvec3(fullTransform * glm::dvec4(position, 1.0));

          // Convert it to cartographic
          const std::optional<CesiumGeospatial::Cartographic> cartographic =
              CesiumGeospatial::Ellipsoid::WGS84.cartesianToCartographic(
                  positionEcef);
          if (!cartographic) {
            for (AccessorWriter<glm::vec2>& uvWriter : uvWriters) {
              uvWriter[positionIndex] = glm::dvec2(0.0, 0.0);
            }
            continue;
          }

          computedBounds.expandToIncludePosition(*cartographic);

          // Generate texture coordinates at this position for each projection
          for (size_t projectionIndex = 0; projectionIndex < projections.size();
               ++projectionIndex) {
            const Projection& projection = projections[projectionIndex];
            const Rectangle& rectangle = rectangles[projectionIndex];

            // Project it with the raster overlay's projection
            glm::dvec3 projectedPosition =
                projectPosition(projection, cartographic.value());

            double longitude = cartographic.value().longitude;
            const double latitude = cartographic.value().latitude;
            const double ellipsoidHeight = cartographic.value().height;

            // If the position is near the anti-meridian and the projected
            // position is outside the expected range, try using the equivalent
            // longitude on the other side of the anti-meridian to see if that
            // gets us closer.
            if (glm::abs(
                    glm::abs(cartographic.value().longitude) -
                    CesiumUtility::Math::ONE_PI) <
                    CesiumUtility::Math::EPSILON5 &&
                (projectedPosition.x < rectangle.minimumX ||
                 projectedPosition.x > rectangle.maximumX ||
                 projectedPosition.y < rectangle.minimumY ||
                 projectedPosition.y > rectangle.maximumY)) {
              const double testLongitude = longitude + longitude < 0.0
                                               ? CesiumUtility::Math::TwoPi
                                               : -CesiumUtility::Math::TwoPi;
              const glm::dvec3 projectedPosition2 = projectPosition(
                  projection,
                  Cartographic(testLongitude, latitude, ellipsoidHeight));

              const double distance1 = rectangle.computeSignedDistance(
                  glm::dvec2(projectedPosition));
              const double distance2 = rectangle.computeSignedDistance(
                  glm::dvec2(projectedPosition2));

              if (distance2 < distance1) {
                projectedPosition = projectedPosition2;
                longitude = testLongitude;
              }
            }

            // Scale to (0.0, 0.0) at the (minimumX, minimumY) corner, and
            // (1.0, 1.0) at the (maximumX, maximumY) corner. The coordinates
            // should stay inside these bounds if the input rectangle actually
            // bounds the vertices, but we'll clamp to be safe.
            glm::vec2 uv(
                CesiumUtility::Math::clamp(
                    (projectedPosition.x - rectangle.minimumX) /
                        rectangle.computeWidth(),
                    0.0,
                    1.0),
                CesiumUtility::Math::clamp(
                    (projectedPosition.y - rectangle.minimumY) /
                        rectangle.computeHeight(),
                    0.0,
                    1.0));

            uvWriters[projectionIndex][positionIndex] = uv;
          }
        }
      };

  gltf.forEachPrimitiveInScene(-1, createTextureCoordinatesForPrimitive);

  return TileContentDetailsForOverlays{
      std::move(projections),
      std::move(rectangles),
      computedBounds.toRegion()};
}

/*static*/ CesiumGeospatial::BoundingRegion GltfContent::computeBoundingRegion(
    const CesiumGltf::Model& gltf,
    const glm::dmat4& transform) {
  CESIUM_TRACE("Cesium3DTilesSelection::GltfContent::"
               "computeBoundingRegion");

  glm::dmat4 rootTransform = transform;
  rootTransform = applyRtcCenter(gltf, rootTransform);
  rootTransform = applyGltfUpAxisTransform(gltf, rootTransform);

  // When computing the tile's bounds, ignore tiles that are less than 1/1000th
  // of a tile width from the North or South pole. Longitudes cannot be trusted
  // at such extreme latitudes.
  BoundingRegionBuilder computedBounds;

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

        for (int64_t i = 0; i < positionView.size(); ++i) {
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

/*static*/ glm::dmat4x4 GltfContent::applyRtcCenter(
    const CesiumGltf::Model& gltf,
    const glm::dmat4x4& rootTransform) {
  auto rtcCenterIt = gltf.extras.find("RTC_CENTER");
  if (rtcCenterIt == gltf.extras.end()) {
    return rootTransform;
  }
  const CesiumUtility::JsonValue& rtcCenter = rtcCenterIt->second;
  const std::vector<CesiumUtility::JsonValue>* pArray =
      std::get_if<CesiumUtility::JsonValue::Array>(&rtcCenter.value);
  if (!pArray) {
    return rootTransform;
  }
  if (pArray->size() != 3) {
    return rootTransform;
  }
  const double x = (*pArray)[0].getSafeNumberOrDefault(0.0);
  const double y = (*pArray)[1].getSafeNumberOrDefault(0.0);
  const double z = (*pArray)[2].getSafeNumberOrDefault(0.0);
  const glm::dmat4x4 rtcTransform(
      glm::dvec4(1.0, 0.0, 0.0, 0.0),
      glm::dvec4(0.0, 1.0, 0.0, 0.0),
      glm::dvec4(0.0, 0.0, 1.0, 0.0),
      glm::dvec4(x, y, z, 1.0));
  return rootTransform * rtcTransform;
}

/*static*/ glm::dmat4x4 GltfContent::applyGltfUpAxisTransform(
    const CesiumGltf::Model& model,
    const glm::dmat4x4& rootTransform) {
  auto gltfUpAxisIt = model.extras.find("gltfUpAxis");
  if (gltfUpAxisIt == model.extras.end()) {
    // The default up-axis of glTF is the Y-axis, and no other
    // up-axis was specified. Transform the Y-axis to the Z-axis,
    // to match the 3D Tiles specification
    return rootTransform * CesiumGeometry::AxisTransforms::Y_UP_TO_Z_UP;
  }
  const CesiumUtility::JsonValue& gltfUpAxis = gltfUpAxisIt->second;
  int gltfUpAxisValue = static_cast<int>(gltfUpAxis.getSafeNumberOrDefault(1));
  if (gltfUpAxisValue == static_cast<int>(CesiumGeometry::Axis::X)) {
    return rootTransform * CesiumGeometry::AxisTransforms::X_UP_TO_Z_UP;
  } else if (gltfUpAxisValue == static_cast<int>(CesiumGeometry::Axis::Y)) {
    return rootTransform * CesiumGeometry::AxisTransforms::Y_UP_TO_Z_UP;
  } else if (gltfUpAxisValue == static_cast<int>(CesiumGeometry::Axis::Z)) {
    // No transform required
  }
  return rootTransform;
}

} // namespace Cesium3DTilesSelection
