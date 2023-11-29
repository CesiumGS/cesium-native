#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGltf/AccessorWriter.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumGltfContent/SkirtMeshMetadata.h>
#include <CesiumRasterOverlays/RasterOverlayUtilities.h>

using namespace CesiumGltfContent;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;

namespace CesiumRasterOverlays {

/*static*/ std::optional<RasterOverlayDetails>
RasterOverlayUtilities::createRasterOverlayTextureCoordinates(
    CesiumGltf::Model& model,
    const glm::dmat4& modelToEcefTransform,
    const std::optional<CesiumGeospatial::GlobeRectangle>& globeRectangle,
    std::vector<CesiumGeospatial::Projection>&& projections,
    bool invertVCoordinate,
    const std::string& textureCoordinateAttributeBaseName,
    int32_t firstTextureCoordinateID) {
  if (projections.empty()) {
    return std::nullopt;
  }

  // Compute the bounds of the tile if they're not provided.
  CesiumGeospatial::GlobeRectangle bounds =
      globeRectangle
          ? *globeRectangle
          : GltfUtilities::computeBoundingRegion(model, modelToEcefTransform)
                .getRectangle();

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
  rootTransform = GltfUtilities::applyRtcCenter(model, rootTransform);
  rootTransform = GltfUtilities::applyGltfUpAxisTransform(model, rootTransform);

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
                textureCoordinateAttributeBaseName +
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

          uvBuffer.byteLength = int64_t(uvBuffer.cesium.data.size());

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
          uvAccessor.min = {0.0, 0.0};
          uvAccessor.max = {1.0, 1.0};

          [[maybe_unused]] CesiumGltf::AccessorWriter<glm::vec2>& uvWriter =
              uvWriters.emplace_back(gltf, uvAccessorId);
          assert(uvWriter.status() == CesiumGltf::AccessorViewStatus::Valid);

          std::string attributeName =
              textureCoordinateAttributeBaseName +
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

            if (invertVCoordinate) {
              uv.y = 1.0f - uv.y;
            }

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

/*static*/ glm::dvec2 RasterOverlayUtilities::computeDesiredScreenPixels(
    double geometricError,
    double maximumScreenSpaceError,
    const CesiumGeospatial::Projection& projection,
    const CesiumGeometry::Rectangle& rectangle,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  // We're aiming to estimate the maximum number of pixels (in each projected
  // direction) the tile will occupy on the screen. They will be determined by
  // the tile's geometric error, because when less error is needed (i.e. the
  // viewer moved closer), the LOD will switch to show the tile's children
  // instead of this tile.
  //
  // It works like this:
  // * Estimate the size of the projected rectangle in world coordinates.
  // * Compute the distance at which tile will switch to its children, based on
  // its geometric error and the tileset SSE.
  // * Compute the on-screen size of the projected rectangle at that distance.
  //
  // For the two compute steps, we use the usual perspective projection SSE
  // equation:
  // screenSize = (realSize * viewportHeight) / (distance * 2 * tan(0.5 * fovY))
  //
  // Conveniently a bunch of terms cancel out, so the screen pixel size at the
  // switch distance is not actually dependent on the screen dimensions or
  // field-of-view angle.

  // We can get a more accurate estimate of the real-world size of the projected
  // rectangle if we consider the rectangle at the true height of the geometry
  // rather than assuming it's on the ellipsoid. This will make basically no
  // difference for small tiles (because surface normals on opposite ends of
  // tiles are effectively identical), and only a small difference for large
  // ones (because heights will be small compared to the total size of a large
  // tile). So we're skipping this complexity for now and estimating geometry
  // width/height as if it's on the ellipsoid surface.
  const double heightForSizeEstimation = 0.0;

  glm::dvec2 diameters = computeProjectedRectangleSize(
      projection,
      rectangle,
      heightForSizeEstimation,
      ellipsoid);
  return diameters * maximumScreenSpaceError / geometricError;
}

/*static*/ glm::dvec4 RasterOverlayUtilities::computeTranslationAndScale(
    const Rectangle& geometryRectangle,
    const Rectangle& overlayRectangle) {
  const double geometryWidth = geometryRectangle.computeWidth();
  const double geometryHeight = geometryRectangle.computeHeight();

  const double scaleX = geometryWidth / overlayRectangle.computeWidth();
  const double scaleY = geometryHeight / overlayRectangle.computeHeight();
  glm::dvec2 translation = glm::dvec2(
      (scaleX * (geometryRectangle.minimumX - overlayRectangle.minimumX)) /
          geometryWidth,
      (scaleY * (geometryRectangle.minimumY - overlayRectangle.minimumY)) /
          geometryHeight);
  glm::dvec2 scale = glm::dvec2(scaleX, scaleY);

  return glm::dvec4(translation, scale);
}

} // namespace CesiumRasterOverlays
