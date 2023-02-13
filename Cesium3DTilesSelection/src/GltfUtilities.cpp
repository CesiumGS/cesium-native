#include "SkirtMeshMetadata.h"

#include <Cesium3DTilesSelection/GltfUtilities.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/Transforms.h>
#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/AccessorWriter.h>
#include <CesiumGltf/ExtensionCesiumRTC.h>

#include <vector>

namespace Cesium3DTilesSelection {
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

/*static*/ std::optional<RasterOverlayDetails>
GltfUtilities::createRasterOverlayTextureCoordinates(
    CesiumGltf::Model& model,
    const glm::dmat4& modelToEcefTransform,
    int32_t firstTextureCoordinateID,
    const std::optional<CesiumGeospatial::GlobeRectangle>& globeRectangle,
    std::vector<CesiumGeospatial::Projection>&& projections) {
  if (projections.empty()) {
    return std::nullopt;
  }

  // Compute the bounds of the tile if they're not provided.
  CesiumGeospatial::GlobeRectangle bounds =
      globeRectangle
          ? *globeRectangle
          : computeBoundingRegion(model, modelToEcefTransform).getRectangle();

  // Currently, a Longitude/Latitude Rectangle maps perfectly to all possible
  // projection types, because the only possible projection types are
  // Geographic and Web Mercator. In the future if/when we add projections
  // that don't have this convenient property, we'll need to compute the
  // Rectangle for each projection directly from the vertex positions.
  std::vector<CesiumGeometry::Rectangle> rectangles(projections.size());
  for (size_t i = 0; i < projections.size(); ++i) {
    rectangles[i] = projectRectangleSimple(projections[i], bounds);
  }

  glm::dmat4 rootTransform = modelToEcefTransform;
  rootTransform = applyRtcCenter(model, rootTransform);
  rootTransform = applyGltfUpAxisTransform(model, rootTransform);

  std::vector<int> positionAccessorsToTextureCoordinateAccessor;
  positionAccessorsToTextureCoordinateAccessor.resize(
      model.accessors.size(),
      0);

  // When computing the tile's bounds, ignore tiles that are less than 1/1000th
  // of a tile width from the North or South pole. Longitudes cannot be trusted
  // at such extreme latitudes.
  CesiumGeospatial::BoundingRegionBuilder computedBounds;
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

          [[maybe_unused]] CesiumGltf::AccessorWriter<glm::vec2>& uvWriter =
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
            for (CesiumGltf::AccessorWriter<glm::vec2>& uvWriter : uvWriters) {
              uvWriter[positionIndex] = glm::dvec2(0.0, 0.0);
            }
            continue;
          }

          // exclude skirt vertices from bounds
          if (positionIndex >= vertexBegin && positionIndex < vertexEnd) {
            computedBounds.expandToIncludePosition(*cartographic);
          }

          // Generate texture coordinates at this position for each projection
          for (size_t projectionIndex = 0; projectionIndex < projections.size();
               ++projectionIndex) {
            const CesiumGeospatial::Projection& projection =
                projections[projectionIndex];
            const CesiumGeometry::Rectangle& rectangle =
                rectangles[projectionIndex];

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
                    CesiumUtility::Math::OnePi) <
                    CesiumUtility::Math::Epsilon5 &&
                (projectedPosition.x < rectangle.minimumX ||
                 projectedPosition.x > rectangle.maximumX ||
                 projectedPosition.y < rectangle.minimumY ||
                 projectedPosition.y > rectangle.maximumY)) {
              const double testLongitude = longitude + longitude < 0.0
                                               ? CesiumUtility::Math::TwoPi
                                               : -CesiumUtility::Math::TwoPi;
              const glm::dvec3 projectedPosition2 = projectPosition(
                  projection,
                  CesiumGeospatial::Cartographic(
                      testLongitude,
                      latitude,
                      ellipsoidHeight));

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

  model.forEachPrimitiveInScene(-1, createTextureCoordinatesForPrimitive);

  return RasterOverlayDetails{
      std::move(projections),
      std::move(rectangles),
      computedBounds.toRegion()};
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

std::vector<Credit> GltfUtilities::parseGltfCopyright(
    CreditSystem& creditSystem,
    const CesiumGltf::Model& gltf,
    bool showOnScreen) {
  std::vector<Credit> result;
  if (gltf.asset.copyright) {
    const std::string& copyright = *gltf.asset.copyright;
    if (copyright.size() > 0) {
      size_t start = 0;
      size_t end;
      size_t ltrim;
      size_t rtrim;
      do {
        ltrim = copyright.find_first_not_of(" \t", start);
        end = copyright.find(';', ltrim);
        rtrim = copyright.find_last_not_of(" \t", end - 1);
        result.push_back(creditSystem.createCredit(
            copyright.substr(ltrim, rtrim - ltrim + 1),
            showOnScreen));
        start = end + 1;
      } while (end != std::string::npos);
    }
  }

  return result;
}
} // namespace Cesium3DTilesSelection
