#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeometry/clipTriangleAtAxisAlignedThreshold.h>
#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/AccessorWriter.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/Image.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/PropertyTableProperty.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumGltfContent/SkirtMeshMetadata.h>
#include <CesiumRasterOverlays/RasterOverlayDetails.h>
#include <CesiumRasterOverlays/RasterOverlayUtilities.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/Math.h>
#include <CesiumUtility/Tracing.h>

#include <glm/common.hpp>
#include <glm/detail/setup.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

using namespace CesiumGltf;
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
    const std::string_view& textureCoordinateAttributeBaseName,
    int32_t firstTextureCoordinateID) {
  if (projections.empty()) {
    return std::nullopt;
  }

  const Ellipsoid& ellipsoid = getProjectionEllipsoid(projections.front());

  // Compute the bounds of the tile if they're not provided.
  CesiumGeospatial::GlobeRectangle bounds =
      globeRectangle ? *globeRectangle
                     : GltfUtilities::computeBoundingRegion(
                           model,
                           modelToEcefTransform,
                           ellipsoid)
                           .getRectangle();

  // Don't let the bounding rectangle cross the anti-meridian. If it does, split
  // it into two rectangles. Ideally we'd map both of them (separately) to the
  // model, but in our current "only Geographic and Web Mercator are supported"
  // world, crossing the anti-meridian is almost certain to simply be numerical
  // noise. So we just use the larger of the two rectangles.
  std::pair<GlobeRectangle, std::optional<GlobeRectangle>> splits =
      bounds.splitAtAntiMeridian();
  bounds = splits.first;

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

  // When computing the tile's bounds, ignore vertices that are less than
  // 1/1000th of a tile height from the North or South pole. Longitudes cannot
  // be trusted at such extreme latitudes.
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
                std::string(textureCoordinateAttributeBaseName) +
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
        std::vector<std::vector<double>*> mins;
        std::vector<std::vector<double>*> maxs;
        uvWriters.reserve(projections.size());
        mins.reserve(projections.size());
        maxs.reserve(projections.size());
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
          uvAccessor.min = {1.0, 1.0};
          uvAccessor.max = {0.0, 0.0};

          [[maybe_unused]] CesiumGltf::AccessorWriter<glm::vec2>& uvWriter =
              uvWriters.emplace_back(gltf, uvAccessorId);
          CESIUM_ASSERT(
              uvWriter.status() == CesiumGltf::AccessorViewStatus::Valid);

          std::string attributeName =
              std::string(textureCoordinateAttributeBaseName) +
              std::to_string(firstTextureCoordinateID + int32_t(i));
          primitive.attributes[attributeName] = uvAccessorId;

          mins.emplace_back(&uvAccessor.min);
          maxs.emplace_back(&uvAccessor.max);
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
              ellipsoid.cartesianToCartographic(positionEcef);
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

            mins[projectionIndex]->at(0) =
                glm::min(mins[projectionIndex]->at(0), double(uv.x));
            mins[projectionIndex]->at(1) =
                glm::min(mins[projectionIndex]->at(1), double(uv.y));
            maxs[projectionIndex]->at(0) =
                glm::max(maxs[projectionIndex]->at(0), double(uv.x));
            maxs[projectionIndex]->at(1) =
                glm::max(maxs[projectionIndex]->at(1), double(uv.y));
            uvWriters[projectionIndex][positionIndex] = uv;
          }
        }
      };

  model.forEachPrimitiveInScene(-1, createTextureCoordinatesForPrimitive);

  return RasterOverlayDetails{
      std::move(projections),
      std::move(rectangles),
      computedBounds.toRegion(ellipsoid)};
}

namespace {
struct EdgeVertex {
  uint32_t index;
  glm::vec2 uv;
};

struct EdgeIndices {
  std::vector<EdgeVertex> west;
  std::vector<EdgeVertex> south;
  std::vector<EdgeVertex> east;
  std::vector<EdgeVertex> north;
};

bool upsamplePrimitiveForRasterOverlays(
    const Model& parentModel,
    Model& model,
    Mesh& mesh,
    MeshPrimitive& primitive,
    CesiumGeometry::UpsampledQuadtreeNode childID,
    bool hasInvertedVCoordinate,
    const std::string_view& textureCoordinateAttributeBaseName,
    int32_t textureCoordinateIndex,
    const CesiumGeospatial::Ellipsoid& ellipsoid);

struct FloatVertexAttribute {
  const std::vector<std::byte>& buffer;
  int64_t offset;
  int64_t stride;
  int64_t numberOfFloatsPerVertex;
  int32_t accessorIndex;
  std::vector<double> minimums;
  std::vector<double> maximums;
};

void addClippedPolygon(
    std::vector<float>& output,
    std::vector<uint32_t>& indices,
    std::vector<FloatVertexAttribute>& attributes,
    std::vector<uint32_t>& vertexMap,
    std::vector<uint32_t>& clipVertexToIndices,
    const std::vector<CesiumGeometry::TriangleClipVertex>& complements,
    const std::vector<CesiumGeometry::TriangleClipVertex>& clipResult);

void addEdge(
    EdgeIndices& edgeIndices,
    double thresholdU,
    double thresholdV,
    bool keepAboveU,
    bool keepAboveV,
    bool invertV,
    const AccessorView<glm::vec2>& uvs,
    const std::vector<uint32_t>& clipVertexToIndices,
    const std::vector<CesiumGeometry::TriangleClipVertex>& complements,
    const std::vector<CesiumGeometry::TriangleClipVertex>& clipResult);

void addSkirt(
    std::vector<float>& output,
    std::vector<uint32_t>& indices,
    std::vector<FloatVertexAttribute>& attributes,
    const std::vector<uint32_t>& edgeIndices,
    const glm::dvec3& center,
    double skirtHeight,
    int64_t vertexSizeFloats,
    int32_t positionAttributeIndex,
    const CesiumGeospatial::Ellipsoid& ellipsoid);

void addSkirts(
    std::vector<float>& output,
    std::vector<uint32_t>& indices,
    std::vector<FloatVertexAttribute>& attributes,
    CesiumGeometry::UpsampledQuadtreeNode childID,
    SkirtMeshMetadata& currentSkirt,
    const SkirtMeshMetadata& parentSkirt,
    EdgeIndices& edgeIndices,
    int64_t vertexSizeFloats,
    int32_t positionAttributeIndex,
    bool hasInvertedVCoordinate,
    const CesiumGeospatial::Ellipsoid& ellipsoid);

bool isWestChild(CesiumGeometry::UpsampledQuadtreeNode childID) noexcept {
  return (childID.tileID.x % 2) == 0;
}

bool isSouthChild(CesiumGeometry::UpsampledQuadtreeNode childID) noexcept {
  return (childID.tileID.y % 2) == 0;
}

void copyImages(const Model& parentModel, Model& result);
void copyMetadataTables(const Model& parentModel, Model& result);

/**
 * @brief Helper struct for working with non-indexed triangles. Returns either
 * the indices from an index accessor view, or generates new indices.
 */
template <typename TIndex> struct IndicesViewRemapper {
  IndicesViewRemapper(
      const Model& model,
      const MeshPrimitive& primitive,
      int32_t primitiveIndices,
      int64_t numVertices)
      : accessorView(std::nullopt),
        indicesCount(0),
        primitiveMode(primitive.mode) {
    AccessorView<TIndex> view(model, primitiveIndices);
    viewStatus = view.status();
    if (viewStatus == AccessorViewStatus::Valid) {
      accessorView = std::move(view);
      if (primitiveMode == MeshPrimitive::Mode::TRIANGLES) {
        indicesCount = accessorView->size();
      } else if (
          primitiveMode == MeshPrimitive::Mode::TRIANGLE_STRIP ||
          primitiveMode == MeshPrimitive::Mode::TRIANGLE_FAN) {
        // With a triangle strip or fan, each additional vertex past the first
        // three adds an additional triangle
        indicesCount = (accessorView->size() - 2) * 3;
      }
    } else if (primitiveIndices < 0) {
      // Non-indexed triangles
      indicesCount = numVertices;
      viewStatus = AccessorViewStatus::Valid;
    }
  }

  int64_t size() const { return indicesCount; }

  AccessorViewStatus status() const { return viewStatus; }

  const TIndex operator[](int64_t i) const {
    if (i < 0 || i >= indicesCount) {
      throw std::range_error("index out of range");
    }

    if (accessorView && primitiveMode == MeshPrimitive::Mode::TRIANGLES) {
      return (*accessorView)[i];
    } else if (
        accessorView && primitiveMode == MeshPrimitive::Mode::TRIANGLE_STRIP) {
      // Indices 0, 1, 2 map normally, indices 3, 4, 5 map to 2, 1, 3,
      // indices 6, 7, 8 map to 2, 3, 4, etc.
      const int64_t startIndex = i / 3;
      const int64_t triIndex = i % 3;
      // For every other triangle we need to reverse the order of the first two
      // indices to maintain proper winding.
      if (startIndex % 2 == 1 && triIndex < 2) {
        return triIndex == 0 ? (*accessorView)[startIndex + 1]
                             : (*accessorView)[startIndex];
      }
      return (*accessorView)[startIndex + triIndex];
    } else if (
        accessorView && primitiveMode == MeshPrimitive::Mode::TRIANGLE_FAN) {
      // Indices 0, 1, 2 map normally, indices 3, 4, 5 map to 0, 2, 3,
      // indices 6, 7, 8 map to 0, 3, 4, etc.
      const int64_t startIndex = i / 3;
      const int64_t triIndex = i % 3;
      if (triIndex == 0) {
        return (*accessorView)[0];
      }
      return (*accessorView)[startIndex + triIndex];
    }

    // The indices of a non-indexed primitive are simply 0, 1, 2, 3, 4...
    return static_cast<TIndex>(i);
  }

private:
  std::optional<AccessorView<TIndex>> accessorView;
  int64_t indicesCount;
  int32_t primitiveMode;
  AccessorViewStatus viewStatus;
};

} // namespace

/*static*/ std::optional<Model>
RasterOverlayUtilities::upsampleGltfForRasterOverlays(
    const Model& parentModel,
    UpsampledQuadtreeNode childID,
    bool hasInvertedVCoordinate,
    const std::string_view& textureCoordinateAttributeBaseName,
    int32_t textureCoordinateIndex,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  CESIUM_TRACE("upsampleGltfForRasterOverlays");
  Model result;

  // Copy the entire parent model except for the buffers, bufferViews, and
  // accessors, which we'll be rewriting.
  result.animations = parentModel.animations;
  result.materials = parentModel.materials;
  result.meshes = parentModel.meshes;
  result.nodes = parentModel.nodes;
  result.textures = parentModel.textures;
  result.images = parentModel.images;
  result.skins = parentModel.skins;
  result.samplers = parentModel.samplers;
  result.cameras = parentModel.cameras;
  result.scenes = parentModel.scenes;
  result.scene = parentModel.scene;
  result.extensionsUsed = parentModel.extensionsUsed;
  result.extensionsRequired = parentModel.extensionsRequired;
  result.asset = parentModel.asset;
  result.extras = parentModel.extras;

  // TODO: check if this is enough, not enough, or overkill
  result.extensions = parentModel.extensions;
  // result.extras_json_string = parentModel.extras_json_string;
  // result.extensions_json_string = parentModel.extensions_json_string;

  copyImages(parentModel, result);

  // Copy EXT_structural_metadata property table buffer views and unique
  // buffers.
  copyMetadataTables(parentModel, result);

  // If the glTF has a name, update it with upsample info.
  auto nameIt = result.extras.find("Cesium3DTiles_TileUrl");
  if (nameIt != result.extras.end()) {
    std::string name = nameIt->second.getStringOrDefault("");
    const std::string::size_type upsampledIndex = name.find(" upsampled");
    if (upsampledIndex != std::string::npos) {
      name = name.substr(0, upsampledIndex);
    }
    name += " upsampled L" + std::to_string(childID.tileID.level);
    name += "-X" + std::to_string(childID.tileID.x);
    name += "-Y" + std::to_string(childID.tileID.y);
    nameIt->second = name;
  }

  bool containsPrimitives = false;

  for (Mesh& mesh : result.meshes) {
    for (size_t i = 0; i < mesh.primitives.size(); ++i) {
      MeshPrimitive& primitive = mesh.primitives[i];

      bool keep = upsamplePrimitiveForRasterOverlays(
          parentModel,
          result,
          mesh,
          primitive,
          childID,
          hasInvertedVCoordinate,
          textureCoordinateAttributeBaseName,
          textureCoordinateIndex,
          ellipsoid);

      // We're assuming here that nothing references primitives by index, so we
      // can remove them without any drama.
      if (!keep) {
        mesh.primitives.erase(mesh.primitives.begin() + int64_t(i));
        --i;
      }
      containsPrimitives |= !mesh.primitives.empty();
    }
  }

  return containsPrimitives ? std::make_optional<Model>(std::move(result))
                            : std::nullopt;
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

namespace {

void copyVertexAttributes(
    std::vector<FloatVertexAttribute>& vertexAttributes,
    const CesiumGeometry::TriangleClipVertex& vertex,
    std::vector<float>& output,
    bool skipMinMaxUpdate = false) {
  struct Operation {
    std::vector<FloatVertexAttribute>& vertexAttributes;
    std::vector<float>& output;
    bool skipMinMaxUpdate;

    void operator()(int vertexIndex) {
      for (FloatVertexAttribute& attribute : vertexAttributes) {
        const float* pInput = reinterpret_cast<const float*>(
            attribute.buffer.data() + attribute.offset +
            attribute.stride * vertexIndex);
        for (int32_t i = 0; i < attribute.numberOfFloatsPerVertex; ++i) {
          const float value = *pInput;
          output.push_back(value);
          if (!skipMinMaxUpdate) {
            attribute.minimums[static_cast<size_t>(i)] = glm::min(
                attribute.minimums[static_cast<size_t>(i)],
                static_cast<double>(value));
            attribute.maximums[static_cast<size_t>(i)] = glm::max(
                attribute.maximums[static_cast<size_t>(i)],
                static_cast<double>(value));
          }
          ++pInput;
        }
      }
    }

    void operator()(const CesiumGeometry::InterpolatedVertex& vertex) {
      for (FloatVertexAttribute& attribute : vertexAttributes) {
        const float* pInput0 = reinterpret_cast<const float*>(
            attribute.buffer.data() + attribute.offset +
            attribute.stride * vertex.first);
        const float* pInput1 = reinterpret_cast<const float*>(
            attribute.buffer.data() + attribute.offset +
            attribute.stride * vertex.second);
        for (int32_t i = 0; i < attribute.numberOfFloatsPerVertex; ++i) {
          const float value = glm::mix(*pInput0, *pInput1, vertex.t);
          output.push_back(value);
          if (!skipMinMaxUpdate) {
            attribute.minimums[static_cast<size_t>(i)] = glm::min(
                attribute.minimums[static_cast<size_t>(i)],
                static_cast<double>(value));
            attribute.maximums[static_cast<size_t>(i)] = glm::max(
                attribute.maximums[static_cast<size_t>(i)],
                static_cast<double>(value));
          }
          ++pInput0;
          ++pInput1;
        }
      }
    }
  };

  std::visit(Operation{vertexAttributes, output, skipMinMaxUpdate}, vertex);
}

void copyVertexAttributes(
    std::vector<FloatVertexAttribute>& vertexAttributes,
    const std::vector<CesiumGeometry::TriangleClipVertex>& complements,
    const CesiumGeometry::TriangleClipVertex& vertex,
    std::vector<float>& output) {
  struct Operation {
    std::vector<FloatVertexAttribute>& vertexAttributes;
    const std::vector<CesiumGeometry::TriangleClipVertex>& complements;
    std::vector<float>& output;

    void operator()(int vertexIndex) {
      if (vertexIndex < 0) {
        copyVertexAttributes(
            vertexAttributes,
            complements[static_cast<size_t>(~vertexIndex)],
            output);
      } else {
        copyVertexAttributes(vertexAttributes, vertexIndex, output);
      }
    }

    void operator()(const CesiumGeometry::InterpolatedVertex& vertex) {
      size_t outputIndex0 = output.size();

      // Copy the two vertices into the output array
      if (vertex.first < 0) {
        copyVertexAttributes(
            vertexAttributes,
            complements[static_cast<size_t>(~vertex.first)],
            output,
            true);
      } else {
        copyVertexAttributes(vertexAttributes, vertex.first, output, true);
      }

      size_t outputIndex1 = output.size();

      if (vertex.second < 0) {
        copyVertexAttributes(
            vertexAttributes,
            complements[static_cast<size_t>(~vertex.second)],
            output,
            true);
      } else {
        copyVertexAttributes(vertexAttributes, vertex.second, output, true);
      }

      // Interpolate between them and overwrite the first with the result.
      for (FloatVertexAttribute& attribute : vertexAttributes) {
        for (int32_t i = 0; i < attribute.numberOfFloatsPerVertex; ++i) {
          float value =
              glm::mix(output[outputIndex0], output[outputIndex1], vertex.t);
          output[outputIndex0] = value;
          attribute.minimums[static_cast<size_t>(i)] = glm::min(
              attribute.minimums[static_cast<size_t>(i)],
              static_cast<double>(value));
          attribute.maximums[static_cast<size_t>(i)] = glm::max(
              attribute.maximums[static_cast<size_t>(i)],
              static_cast<double>(value));
          ++outputIndex0;
          ++outputIndex1;
        }
      }

      // Remove the temporary second, which is now pointed to be outputIndex0.
      output.erase(
          output.begin() +
              static_cast<std::vector<float>::iterator::difference_type>(
                  outputIndex0),
          output.end());
    }
  };

  std::visit(
      Operation{
          vertexAttributes,
          complements,
          output,
      },
      vertex);
}

template <class T>
T getVertexValue(
    const AccessorView<T>& accessor,
    const CesiumGeometry::TriangleClipVertex& vertex) {
  struct Operation {
    const AccessorView<T>& accessor;

    T operator()(int vertexIndex) { return accessor[vertexIndex]; }

    T operator()(const CesiumGeometry::InterpolatedVertex& vertex) {
      const T& v0 = accessor[vertex.first];
      const T& v1 = accessor[vertex.second];
      return glm::mix(v0, v1, vertex.t);
    }
  };

  return std::visit(Operation{accessor}, vertex);
}

template <class T>
T getVertexValue(
    const AccessorView<T>& accessor,
    const std::vector<CesiumGeometry::TriangleClipVertex>& complements,
    const CesiumGeometry::TriangleClipVertex& vertex) {
  struct Operation {
    const AccessorView<T>& accessor;
    const std::vector<CesiumGeometry::TriangleClipVertex>& complements;

    T operator()(int vertexIndex) {
      if (vertexIndex < 0) {
        return getVertexValue(
            accessor,
            complements,
            complements[static_cast<size_t>(~vertexIndex)]);
      }

      return accessor[vertexIndex];
    }

    T operator()(const CesiumGeometry::InterpolatedVertex& vertex) {
      T v0{};
      if (vertex.first < 0) {
        v0 = getVertexValue(
            accessor,
            complements,
            complements[static_cast<size_t>(~vertex.first)]);
      } else {
        v0 = accessor[vertex.first];
      }

      T v1{};
      if (vertex.second < 0) {
        v1 = getVertexValue(
            accessor,
            complements,
            complements[static_cast<size_t>(~vertex.second)]);
      } else {
        v1 = accessor[vertex.second];
      }

      return glm::mix(v0, v1, vertex.t);
    }
  };

  return std::visit(Operation{accessor, complements}, vertex);
}

template <class TIndex>
bool upsamplePrimitiveForRasterOverlays(
    const Model& parentModel,
    Model& model,
    Mesh& /*mesh*/,
    MeshPrimitive& primitive,
    CesiumGeometry::UpsampledQuadtreeNode childID,
    bool hasInvertedVCoordinate,
    const std::string_view& textureCoordinateAttributeBaseName,
    int32_t textureCoordinateIndex,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  CESIUM_TRACE("upsamplePrimitiveForRasterOverlays");

  // Add up the per-vertex size of all attributes and create buffers,
  // bufferViews, and accessors
  std::vector<FloatVertexAttribute> attributes;
  attributes.reserve(primitive.attributes.size());

  const size_t vertexBufferIndex = model.buffers.size();
  model.buffers.emplace_back();

  const size_t indexBufferIndex = model.buffers.size();
  model.buffers.emplace_back();

  const size_t vertexBufferViewIndex = model.bufferViews.size();
  model.bufferViews.emplace_back();

  const size_t indexBufferViewIndex = model.bufferViews.size();
  model.bufferViews.emplace_back();

  BufferView& vertexBufferView = model.bufferViews[vertexBufferViewIndex];
  vertexBufferView.buffer = static_cast<int>(vertexBufferIndex);
  vertexBufferView.target = BufferView::Target::ARRAY_BUFFER;

  BufferView& indexBufferView = model.bufferViews[indexBufferViewIndex];
  indexBufferView.buffer = static_cast<int>(indexBufferIndex);
  indexBufferView.target = BufferView::Target::ELEMENT_ARRAY_BUFFER;

  int64_t vertexSizeFloats = 0;
  int64_t positionAttributeCount = 0;
  int32_t uvAccessorIndex = -1;
  int32_t positionAttributeIndex = -1;

  std::vector<std::string> toRemove;

  std::string textureCoordinateName =
      std::string(textureCoordinateAttributeBaseName) +
      std::to_string(textureCoordinateIndex);

  for (std::pair<const std::string, int>& attribute : primitive.attributes) {
    if (attribute.first.starts_with(textureCoordinateAttributeBaseName)) {
      if (uvAccessorIndex == -1) {
        if (attribute.first == textureCoordinateName) {
          uvAccessorIndex = attribute.second;
        }
      }
      // Do not include textureCoordinateName (e.g., TEXCOORD_* or
      // _CESIUMOVERLAY_*), it will be generated later.
      toRemove.push_back(attribute.first);
      continue;
    }

    if (attribute.second < 0 ||
        attribute.second >= static_cast<int>(parentModel.accessors.size())) {
      toRemove.push_back(attribute.first);
      continue;
    }

    const Accessor& accessor =
        parentModel.accessors[static_cast<size_t>(attribute.second)];
    if (accessor.bufferView < 0 ||
        accessor.bufferView >=
            static_cast<int>(parentModel.bufferViews.size())) {
      toRemove.push_back(attribute.first);
      continue;
    }

    const BufferView& bufferView =
        parentModel.bufferViews[static_cast<size_t>(accessor.bufferView)];
    if (bufferView.buffer < 0 ||
        bufferView.buffer >= static_cast<int>(parentModel.buffers.size())) {
      toRemove.push_back(attribute.first);
      continue;
    }

    const Buffer& buffer =
        parentModel.buffers[static_cast<size_t>(bufferView.buffer)];

    const int64_t accessorByteStride = accessor.computeByteStride(parentModel);
    const int64_t accessorComponentElements =
        accessor.computeNumberOfComponents();
    if (accessor.componentType != Accessor::ComponentType::FLOAT) {
      // Can only interpolate floating point vertex attributes
      toRemove.push_back(attribute.first);
      continue;
    }

    attribute.second = static_cast<int>(model.accessors.size());
    model.accessors.emplace_back();
    Accessor& newAccessor = model.accessors.back();
    newAccessor.bufferView = static_cast<int>(vertexBufferViewIndex);
    newAccessor.byteOffset = vertexSizeFloats * int64_t(sizeof(float));
    newAccessor.componentType = Accessor::ComponentType::FLOAT;
    newAccessor.type = accessor.type;

    vertexSizeFloats += accessorComponentElements;

    attributes.push_back(FloatVertexAttribute{
        buffer.cesium.data,
        bufferView.byteOffset + accessor.byteOffset,
        accessorByteStride,
        accessorComponentElements,
        attribute.second,
        std::vector<double>(
            static_cast<size_t>(accessorComponentElements),
            std::numeric_limits<double>::max()),
        std::vector<double>(
            static_cast<size_t>(accessorComponentElements),
            std::numeric_limits<double>::lowest()),
    });

    // get position to be used to create skirts later
    if (attribute.first == "POSITION") {
      positionAttributeIndex = int32_t(attributes.size() - 1);
      positionAttributeCount = accessor.count;
    }
  }

  if (uvAccessorIndex == -1) {
    // We don't know how to divide this primitive, so just remove it.
    return false;
  }

  for (const std::string& attribute : toRemove) {
    primitive.attributes.erase(attribute);
  }

  const bool keepAboveU = !isWestChild(childID);
  const bool keepAboveV = !isSouthChild(childID);

  const AccessorView<glm::vec2> uvView(parentModel, uvAccessorIndex);
  const IndicesViewRemapper<TIndex> indicesView(
      parentModel,
      primitive,
      primitive.indices,
      positionAttributeCount);

  if (uvView.status() != AccessorViewStatus::Valid ||
      indicesView.status() != AccessorViewStatus::Valid) {
    return false;
  }

  // check if the primitive has skirts
  int64_t indicesBegin = 0;
  int64_t indicesCount = indicesView.size();
  std::optional<SkirtMeshMetadata> parentSkirtMeshMetadata =
      SkirtMeshMetadata::parseFromGltfExtras(primitive.extras);
  const bool hasSkirt = (parentSkirtMeshMetadata != std::nullopt) &&
                        (positionAttributeIndex != -1);
  if (hasSkirt) {
    indicesBegin = parentSkirtMeshMetadata->noSkirtIndicesBegin;
    indicesCount = parentSkirtMeshMetadata->noSkirtIndicesCount;
  }

  std::vector<uint32_t> clipVertexToIndices;
  std::vector<CesiumGeometry::TriangleClipVertex> clippedA;
  std::vector<CesiumGeometry::TriangleClipVertex> clippedB;

  // Maps old (parentModel) vertex indices to new (model) vertex indices.
  std::vector<uint32_t> vertexMap(
      size_t(uvView.size()),
      std::numeric_limits<uint32_t>::max());

  // std::vector<unsigned char> newVertexBuffer(vertexSizeFloats *
  // sizeof(float)); std::span<float>
  // newVertexFloats(reinterpret_cast<float*>(newVertexBuffer.data()),
  // newVertexBuffer.size() / sizeof(float));
  std::vector<float> newVertexFloats;
  std::vector<uint32_t> indices;
  EdgeIndices edgeIndices;

  for (int64_t i = indicesBegin; i < indicesBegin + indicesCount; i += 3) {
    TIndex i0 = indicesView[i];
    TIndex i1 = indicesView[i + 1];
    TIndex i2 = indicesView[i + 2];

    const glm::vec2 uv0 = uvView[i0];
    const glm::vec2 uv1 = uvView[i1];
    const glm::vec2 uv2 = uvView[i2];

    // Clip this triangle against the East-West boundary
    clippedA.clear();
    clipTriangleAtAxisAlignedThreshold(
        0.5,
        keepAboveU,
        static_cast<int>(i0),
        static_cast<int>(i1),
        static_cast<int>(i2),
        uv0.x,
        uv1.x,
        uv2.x,
        clippedA);

    if (clippedA.size() < 3) {
      // No part of this triangle is inside the target tile.
      continue;
    }

    // Clip the first clipped triange against the North-South boundary
    clipVertexToIndices.clear();
    clippedB.clear();
    clipTriangleAtAxisAlignedThreshold(
        0.5,
        hasInvertedVCoordinate ? !keepAboveV : keepAboveV,
        ~0,
        ~1,
        ~2,
        getVertexValue(uvView, clippedA[0]).y,
        getVertexValue(uvView, clippedA[1]).y,
        getVertexValue(uvView, clippedA[2]).y,
        clippedB);

    // Add the clipped triangle or quad, if any
    addClippedPolygon(
        newVertexFloats,
        indices,
        attributes,
        vertexMap,
        clipVertexToIndices,
        clippedA,
        clippedB);
    if (hasSkirt) {
      addEdge(
          edgeIndices,
          0.5,
          0.5,
          keepAboveU,
          keepAboveV,
          hasInvertedVCoordinate,
          uvView,
          clipVertexToIndices,
          clippedA,
          clippedB);
    }

    // If the East-West clip yielded a quad (rather than a triangle), clip the
    // second triangle of the quad, too.
    if (clippedA.size() > 3) {
      clipVertexToIndices.clear();
      clippedB.clear();
      clipTriangleAtAxisAlignedThreshold(
          0.5,
          hasInvertedVCoordinate ? !keepAboveV : keepAboveV,
          ~0,
          ~2,
          ~3,
          getVertexValue(uvView, clippedA[0]).y,
          getVertexValue(uvView, clippedA[2]).y,
          getVertexValue(uvView, clippedA[3]).y,
          clippedB);

      // Add the clipped triangle or quad, if any
      addClippedPolygon(
          newVertexFloats,
          indices,
          attributes,
          vertexMap,
          clipVertexToIndices,
          clippedA,
          clippedB);
      if (hasSkirt) {
        addEdge(
            edgeIndices,
            0.5,
            0.5,
            keepAboveU,
            keepAboveV,
            hasInvertedVCoordinate,
            uvView,
            clipVertexToIndices,
            clippedA,
            clippedB);
      }
    }
  }

  // create mesh with skirt
  std::optional<SkirtMeshMetadata> skirtMeshMetadata;
  if (hasSkirt) {
    skirtMeshMetadata = std::make_optional<SkirtMeshMetadata>();
    skirtMeshMetadata->noSkirtIndicesBegin = 0;
    skirtMeshMetadata->noSkirtIndicesCount =
        static_cast<uint32_t>(indices.size());
    skirtMeshMetadata->noSkirtVerticesBegin = 0;
    skirtMeshMetadata->noSkirtVerticesCount =
        uint32_t(newVertexFloats.size() / size_t(vertexSizeFloats));
    skirtMeshMetadata->meshCenter = parentSkirtMeshMetadata->meshCenter;
    addSkirts(
        newVertexFloats,
        indices,
        attributes,
        childID,
        *skirtMeshMetadata,
        *parentSkirtMeshMetadata,
        edgeIndices,
        vertexSizeFloats,
        positionAttributeIndex,
        hasInvertedVCoordinate,
        ellipsoid);
  }

  if (newVertexFloats.empty() || indices.empty()) {
    return false;
  }

  // Update the accessor vertex counts and min/max values
  const int64_t numberOfVertices =
      int64_t(newVertexFloats.size()) / vertexSizeFloats;
  for (const FloatVertexAttribute& attribute : attributes) {
    Accessor& accessor =
        model.accessors[static_cast<size_t>(attribute.accessorIndex)];
    accessor.count = numberOfVertices;
    accessor.min = attribute.minimums;
    accessor.max = attribute.maximums;
  }

  // Add an accessor for the indices
  const size_t indexAccessorIndex = model.accessors.size();
  model.accessors.emplace_back();
  Accessor& newIndicesAccessor = model.accessors.back();
  newIndicesAccessor.bufferView = static_cast<int>(indexBufferViewIndex);
  newIndicesAccessor.byteOffset = 0;
  newIndicesAccessor.count = int64_t(indices.size());
  newIndicesAccessor.componentType = Accessor::ComponentType::UNSIGNED_INT;
  newIndicesAccessor.type = Accessor::Type::SCALAR;

  // Populate the buffers
  Buffer& vertexBuffer = model.buffers[vertexBufferIndex];
  vertexBuffer.cesium.data.resize(newVertexFloats.size() * sizeof(float));
  float* pAsFloats = reinterpret_cast<float*>(vertexBuffer.cesium.data.data());
  std::copy(newVertexFloats.begin(), newVertexFloats.end(), pAsFloats);
  vertexBuffer.byteLength = vertexBufferView.byteLength =
      int64_t(vertexBuffer.cesium.data.size());
  vertexBufferView.byteStride = vertexSizeFloats * int64_t(sizeof(float));

  Buffer& indexBuffer = model.buffers[indexBufferIndex];
  indexBuffer.cesium.data.resize(indices.size() * sizeof(uint32_t));
  uint32_t* pAsUint32s =
      reinterpret_cast<uint32_t*>(indexBuffer.cesium.data.data());
  std::copy(indices.begin(), indices.end(), pAsUint32s);
  indexBuffer.byteLength = indexBufferView.byteLength =
      int64_t(indexBuffer.cesium.data.size());

  bool onlyWater = false;
  bool onlyLand = true;
  int64_t waterMaskTextureId = -1;

  auto onlyWaterIt = primitive.extras.find("OnlyWater");
  auto onlyLandIt = primitive.extras.find("OnlyLand");

  if (onlyWaterIt != primitive.extras.end() && onlyWaterIt->second.isBool() &&
      onlyLandIt != primitive.extras.end() && onlyLandIt->second.isBool()) {

    onlyWater = onlyWaterIt->second.getBoolOrDefault(false);
    onlyLand = onlyLandIt->second.getBoolOrDefault(true);

    if (!onlyWater && !onlyLand) {
      // We have to use the parent's water mask
      auto waterMaskTextureIdIt = primitive.extras.find("WaterMaskTex");
      if (waterMaskTextureIdIt != primitive.extras.end() &&
          waterMaskTextureIdIt->second.isInt64()) {
        waterMaskTextureId = waterMaskTextureIdIt->second.getInt64OrDefault(-1);
      }
    }
  }

  double waterMaskTranslationX = 0.0;
  double waterMaskTranslationY = 0.0;
  double waterMaskScale = 0.0;

  auto waterMaskTranslationXIt = primitive.extras.find("WaterMaskTranslationX");
  auto waterMaskTranslationYIt = primitive.extras.find("WaterMaskTranslationY");
  auto waterMaskScaleIt = primitive.extras.find("WaterMaskScale");

  if (waterMaskTranslationXIt != primitive.extras.end() &&
      waterMaskTranslationXIt->second.isDouble() &&
      waterMaskTranslationYIt != primitive.extras.end() &&
      waterMaskTranslationYIt->second.isDouble() &&
      waterMaskScaleIt != primitive.extras.end() &&
      waterMaskScaleIt->second.isDouble()) {
    waterMaskScale = 0.5 * waterMaskScaleIt->second.getDoubleOrDefault(0.0);
    waterMaskTranslationX =
        waterMaskTranslationXIt->second.getDoubleOrDefault(0.0) +
        waterMaskScale * (childID.tileID.x % 2);
    waterMaskTranslationY =
        waterMaskTranslationYIt->second.getDoubleOrDefault(0.0) +
        waterMaskScale * (childID.tileID.y % 2);
  }

  // add skirts to extras to be upsampled later if needed
  if (hasSkirt) {
    primitive.extras = SkirtMeshMetadata::createGltfExtras(*skirtMeshMetadata);
  }

  primitive.extras.emplace("OnlyWater", onlyWater);
  primitive.extras.emplace("OnlyLand", onlyLand);

  primitive.extras.emplace("WaterMaskTex", waterMaskTextureId);

  primitive.extras.emplace("WaterMaskTranslationX", waterMaskTranslationX);
  primitive.extras.emplace("WaterMaskTranslationY", waterMaskTranslationY);
  primitive.extras.emplace("WaterMaskScale", waterMaskScale);

  primitive.indices = static_cast<int>(indexAccessorIndex);

  return true;
}

uint32_t getOrCreateVertex(
    std::vector<float>& output,
    std::vector<FloatVertexAttribute>& attributes,
    std::vector<uint32_t>& vertexMap,
    const std::vector<CesiumGeometry::TriangleClipVertex>& complements,
    const CesiumGeometry::TriangleClipVertex& clipVertex) {
  const int* pIndex = std::get_if<int>(&clipVertex);
  if (pIndex) {
    if (*pIndex < 0) {
      return getOrCreateVertex(
          output,
          attributes,
          vertexMap,
          complements,
          complements[static_cast<size_t>(~(*pIndex))]);
    }

    const uint32_t existingIndex = vertexMap[static_cast<size_t>(*pIndex)];
    if (existingIndex != std::numeric_limits<uint32_t>::max()) {
      return existingIndex;
    }
  }

  const uint32_t beforeOutput = static_cast<uint32_t>(output.size());
  copyVertexAttributes(attributes, complements, clipVertex, output);
  uint32_t newIndex =
      beforeOutput / (static_cast<uint32_t>(output.size()) - beforeOutput);

  if (pIndex && *pIndex >= 0) {
    vertexMap[static_cast<size_t>(*pIndex)] = newIndex;
  }

  return newIndex;
}

void addClippedPolygon(
    std::vector<float>& output,
    std::vector<uint32_t>& indices,
    std::vector<FloatVertexAttribute>& attributes,
    std::vector<uint32_t>& vertexMap,
    std::vector<uint32_t>& clipVertexToIndices,
    const std::vector<CesiumGeometry::TriangleClipVertex>& complements,
    const std::vector<CesiumGeometry::TriangleClipVertex>& clipResult) {
  if (clipResult.size() < 3) {
    return;
  }

  const uint32_t i0 = getOrCreateVertex(
      output,
      attributes,
      vertexMap,
      complements,
      clipResult[0]);
  const uint32_t i1 = getOrCreateVertex(
      output,
      attributes,
      vertexMap,
      complements,
      clipResult[1]);
  const uint32_t i2 = getOrCreateVertex(
      output,
      attributes,
      vertexMap,
      complements,
      clipResult[2]);

  indices.push_back(i0);
  indices.push_back(i1);
  indices.push_back(i2);

  clipVertexToIndices.push_back(i0);
  clipVertexToIndices.push_back(i1);
  clipVertexToIndices.push_back(i2);

  if (clipResult.size() > 3) {
    const uint32_t i3 = getOrCreateVertex(
        output,
        attributes,
        vertexMap,
        complements,
        clipResult[3]);

    indices.push_back(i0);
    indices.push_back(i2);
    indices.push_back(i3);

    clipVertexToIndices.push_back(i3);
  }
}

void addEdge(
    EdgeIndices& edgeIndices,
    double thresholdU,
    double thresholdV,
    bool keepAboveU,
    bool keepAboveV,
    bool invertV,
    const AccessorView<glm::vec2>& uvs,
    const std::vector<uint32_t>& clipVertexToIndices,
    const std::vector<CesiumGeometry::TriangleClipVertex>& complements,
    const std::vector<CesiumGeometry::TriangleClipVertex>& clipResult) {
  for (uint32_t i = 0; i < clipVertexToIndices.size(); ++i) {
    const glm::vec2 uv = getVertexValue(uvs, complements, clipResult[i]);

    if (CesiumUtility::Math::equalsEpsilon(
            uv.x,
            0.0,
            CesiumUtility::Math::Epsilon4)) {
      edgeIndices.west.emplace_back(EdgeVertex{clipVertexToIndices[i], uv});
    }

    if (CesiumUtility::Math::equalsEpsilon(
            uv.x,
            1.0,
            CesiumUtility::Math::Epsilon4)) {
      edgeIndices.east.emplace_back(EdgeVertex{clipVertexToIndices[i], uv});
    }

    if (CesiumUtility::Math::equalsEpsilon(
            uv.x,
            thresholdU,
            CesiumUtility::Math::Epsilon4)) {
      if (keepAboveU) {
        edgeIndices.west.emplace_back(EdgeVertex{clipVertexToIndices[i], uv});
      } else {
        edgeIndices.east.emplace_back(EdgeVertex{clipVertexToIndices[i], uv});
      }
    }

    if (CesiumUtility::Math::equalsEpsilon(
            uv.y,
            invertV ? 1.0f : 0.0f,
            CesiumUtility::Math::Epsilon4)) {
      edgeIndices.south.emplace_back(EdgeVertex{clipVertexToIndices[i], uv});
    }

    if (CesiumUtility::Math::equalsEpsilon(
            uv.y,
            invertV ? 0.0f : 1.0f,
            CesiumUtility::Math::Epsilon4)) {
      edgeIndices.north.emplace_back(EdgeVertex{clipVertexToIndices[i], uv});
    }

    if (CesiumUtility::Math::equalsEpsilon(
            uv.y,
            thresholdV,
            CesiumUtility::Math::Epsilon4)) {
      if (keepAboveV) {
        edgeIndices.south.emplace_back(EdgeVertex{clipVertexToIndices[i], uv});
      } else {
        edgeIndices.north.emplace_back(EdgeVertex{clipVertexToIndices[i], uv});
      }
    }
  }
}

void addSkirt(
    std::vector<float>& output,
    std::vector<uint32_t>& indices,
    std::vector<FloatVertexAttribute>& attributes,
    const std::vector<uint32_t>& edgeIndices,
    const glm::dvec3& center,
    double skirtHeight,
    int64_t vertexSizeFloats,
    int32_t positionAttributeIndex,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  uint32_t newEdgeIndex = uint32_t(output.size() / size_t(vertexSizeFloats));
  for (size_t i = 0; i < edgeIndices.size(); ++i) {
    const uint32_t edgeIdx = edgeIndices[i];
    uint32_t offset = 0;
    for (size_t j = 0; j < attributes.size(); ++j) {
      FloatVertexAttribute& attribute = attributes[j];
      const uint32_t valueIndex = offset + uint32_t(vertexSizeFloats) * edgeIdx;

      if (int32_t(j) == positionAttributeIndex) {
        glm::dvec3 position{
            output[valueIndex],
            output[valueIndex + 1],
            output[valueIndex + 2]};
        position += center;

        position -= skirtHeight * ellipsoid.geodeticSurfaceNormal(position);
        position -= center;

        for (uint32_t c = 0; c < 3; ++c) {
          output.push_back(static_cast<float>(position[glm::length_t(c)]));
          attribute.minimums[c] =
              glm::min(attribute.minimums[c], position[glm::length_t(c)]);
          attribute.maximums[c] =
              glm::max(attribute.maximums[c], position[glm::length_t(c)]);
        }
      } else {
        for (uint32_t c = 0;
             c < static_cast<uint32_t>(attribute.numberOfFloatsPerVertex);
             ++c) {
          output.push_back(output[valueIndex + c]);
          attribute.minimums[c] = glm::min(
              attribute.minimums[c],
              static_cast<double>(output.back()));
          attribute.maximums[c] = glm::max(
              attribute.maximums[c],
              static_cast<double>(output.back()));
        }
      }

      offset += static_cast<uint32_t>(attribute.numberOfFloatsPerVertex);
    }

    if (i < edgeIndices.size() - 1) {
      const uint32_t nextEdgeIdx = edgeIndices[i + 1];
      indices.push_back(edgeIdx);
      indices.push_back(nextEdgeIdx);
      indices.push_back(newEdgeIndex);

      indices.push_back(newEdgeIndex);
      indices.push_back(nextEdgeIdx);
      indices.push_back(newEdgeIndex + 1);
    }

    ++newEdgeIndex;
  }
}

void addSkirts(
    std::vector<float>& output,
    std::vector<uint32_t>& indices,
    std::vector<FloatVertexAttribute>& attributes,
    CesiumGeometry::UpsampledQuadtreeNode childID,
    SkirtMeshMetadata& currentSkirt,
    const SkirtMeshMetadata& parentSkirt,
    EdgeIndices& edgeIndices,
    int64_t vertexSizeFloats,
    int32_t positionAttributeIndex,
    bool hasInvertedVCoordinate,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  CESIUM_TRACE("addSkirts");

  const glm::dvec3 center = currentSkirt.meshCenter;
  double shortestSkirtHeight =
      glm::min(parentSkirt.skirtWestHeight, parentSkirt.skirtEastHeight);
  shortestSkirtHeight =
      glm::min(shortestSkirtHeight, parentSkirt.skirtSouthHeight);
  shortestSkirtHeight =
      glm::min(shortestSkirtHeight, parentSkirt.skirtNorthHeight);

  // west
  if (isWestChild(childID)) {
    currentSkirt.skirtWestHeight = parentSkirt.skirtWestHeight;
  } else {
    currentSkirt.skirtWestHeight = shortestSkirtHeight * 0.5;
  }

  std::vector<uint32_t> sortEdgeIndices(edgeIndices.west.size());
  std::sort(
      edgeIndices.west.begin(),
      edgeIndices.west.end(),
      [](const EdgeVertex& lhs, const EdgeVertex& rhs) {
        return lhs.uv.y < rhs.uv.y;
      });
  std::transform(
      edgeIndices.west.begin(),
      edgeIndices.west.end(),
      sortEdgeIndices.begin(),
      [](const EdgeVertex& v) { return v.index; });
  addSkirt(
      output,
      indices,
      attributes,
      sortEdgeIndices,
      center,
      currentSkirt.skirtWestHeight,
      vertexSizeFloats,
      positionAttributeIndex,
      ellipsoid);

  // south
  if (isSouthChild(childID)) {
    currentSkirt.skirtSouthHeight = parentSkirt.skirtSouthHeight;
  } else {
    currentSkirt.skirtSouthHeight = shortestSkirtHeight * 0.5;
  }

  sortEdgeIndices.resize(edgeIndices.south.size());
  std::sort(
      edgeIndices.south.begin(),
      edgeIndices.south.end(),
      [](const EdgeVertex& lhs, const EdgeVertex& rhs) {
        return lhs.uv.x > rhs.uv.x;
      });
  if (hasInvertedVCoordinate) {
    std::transform(
        edgeIndices.south.rbegin(),
        edgeIndices.south.rend(),
        sortEdgeIndices.begin(),
        [](const EdgeVertex& v) { return v.index; });
  } else {
    std::transform(
        edgeIndices.south.begin(),
        edgeIndices.south.end(),
        sortEdgeIndices.begin(),
        [](const EdgeVertex& v) { return v.index; });
  }

  addSkirt(
      output,
      indices,
      attributes,
      sortEdgeIndices,
      center,
      currentSkirt.skirtSouthHeight,
      vertexSizeFloats,
      positionAttributeIndex,
      ellipsoid);

  // east
  if (!isWestChild(childID)) {
    currentSkirt.skirtEastHeight = parentSkirt.skirtEastHeight;
  } else {
    currentSkirt.skirtEastHeight = shortestSkirtHeight * 0.5;
  }

  sortEdgeIndices.resize(edgeIndices.east.size());
  std::sort(
      edgeIndices.east.begin(),
      edgeIndices.east.end(),
      [](const EdgeVertex& lhs, const EdgeVertex& rhs) {
        return lhs.uv.y > rhs.uv.y;
      });
  std::transform(
      edgeIndices.east.begin(),
      edgeIndices.east.end(),
      sortEdgeIndices.begin(),
      [](const EdgeVertex& v) { return v.index; });
  addSkirt(
      output,
      indices,
      attributes,
      sortEdgeIndices,
      center,
      currentSkirt.skirtEastHeight,
      vertexSizeFloats,
      positionAttributeIndex,
      ellipsoid);

  // north
  if (!isSouthChild(childID)) {
    currentSkirt.skirtNorthHeight = parentSkirt.skirtNorthHeight;
  } else {
    currentSkirt.skirtNorthHeight = shortestSkirtHeight * 0.5;
  }

  sortEdgeIndices.resize(edgeIndices.north.size());
  std::sort(
      edgeIndices.north.begin(),
      edgeIndices.north.end(),
      [](const EdgeVertex& lhs, const EdgeVertex& rhs) {
        return lhs.uv.x < rhs.uv.x;
      });
  if (hasInvertedVCoordinate) {
    std::transform(
        edgeIndices.north.rbegin(),
        edgeIndices.north.rend(),
        sortEdgeIndices.begin(),
        [](const EdgeVertex& v) { return v.index; });
  } else {
    std::transform(
        edgeIndices.north.begin(),
        edgeIndices.north.end(),
        sortEdgeIndices.begin(),
        [](const EdgeVertex& v) { return v.index; });
  }

  addSkirt(
      output,
      indices,
      attributes,
      sortEdgeIndices,
      center,
      currentSkirt.skirtNorthHeight,
      vertexSizeFloats,
      positionAttributeIndex,
      ellipsoid);
}

bool upsamplePrimitiveForRasterOverlays(
    const Model& parentModel,
    Model& model,
    Mesh& mesh,
    MeshPrimitive& primitive,
    CesiumGeometry::UpsampledQuadtreeNode childID,
    bool hasInvertedVCoordinate,
    const std::string_view& textureCoordinateAttributeBaseName,
    int32_t textureCoordinateIndex,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  if (primitive.mode != MeshPrimitive::Mode::TRIANGLES &&
      primitive.mode != MeshPrimitive::Mode::TRIANGLE_FAN &&
      primitive.mode != MeshPrimitive::Mode::TRIANGLE_STRIP) {
    // Not triangles, so we don't know how to divide this primitive
    // (yet). So remove it.
    return false;
  }

  if (primitive.indices < 0 ||
      primitive.indices > static_cast<int64_t>(parentModel.accessors.size())) {
    const auto& positionIt = primitive.attributes.find("POSITION");
    if (positionIt == primitive.attributes.end()) {
      // No position buffer - nothing we can do here
      return false;
    }

    // No indices buffer - pick the smallest indices type that will fit all
    // vertices
    const Accessor& accessor =
        parentModel.getSafe(parentModel.accessors, positionIt->second);
    if (accessor.count < 1) {
      // Invalid accessor
      return false;
    } else if (accessor.count < 0xff) {
      return upsamplePrimitiveForRasterOverlays<uint8_t>(
          parentModel,
          model,
          mesh,
          primitive,
          childID,
          hasInvertedVCoordinate,
          textureCoordinateAttributeBaseName,
          textureCoordinateIndex,
          ellipsoid);
    } else if (accessor.count < 0xffff) {
      return upsamplePrimitiveForRasterOverlays<uint16_t>(
          parentModel,
          model,
          mesh,
          primitive,
          childID,
          hasInvertedVCoordinate,
          textureCoordinateAttributeBaseName,
          textureCoordinateIndex,
          ellipsoid);
    } else {
      return upsamplePrimitiveForRasterOverlays<uint32_t>(
          parentModel,
          model,
          mesh,
          primitive,
          childID,
          hasInvertedVCoordinate,
          textureCoordinateAttributeBaseName,
          textureCoordinateIndex,
          ellipsoid);
    }
  }

  const Accessor& indicesAccessorGltf =
      parentModel.accessors[static_cast<size_t>(primitive.indices)];
  if (indicesAccessorGltf.componentType ==
      Accessor::ComponentType::UNSIGNED_BYTE) {
    return upsamplePrimitiveForRasterOverlays<uint8_t>(
        parentModel,
        model,
        mesh,
        primitive,
        childID,
        hasInvertedVCoordinate,
        textureCoordinateAttributeBaseName,
        textureCoordinateIndex,
        ellipsoid);
  } else if (
      indicesAccessorGltf.componentType ==
      Accessor::ComponentType::UNSIGNED_SHORT) {
    return upsamplePrimitiveForRasterOverlays<uint16_t>(
        parentModel,
        model,
        mesh,
        primitive,
        childID,
        hasInvertedVCoordinate,
        textureCoordinateAttributeBaseName,
        textureCoordinateIndex,
        ellipsoid);
  } else if (
      indicesAccessorGltf.componentType ==
      Accessor::ComponentType::UNSIGNED_INT) {
    return upsamplePrimitiveForRasterOverlays<uint32_t>(
        parentModel,
        model,
        mesh,
        primitive,
        childID,
        hasInvertedVCoordinate,
        textureCoordinateAttributeBaseName,
        textureCoordinateIndex,
        ellipsoid);
  }

  return false;
}

// Copy a buffer view from a parent to a child. Create a new buffer on the
// child corresponding to the section of the parent buffer specified by the
// view.
int32_t copyBufferView(
    const Model& parentModel,
    int32_t parentBufferViewId,
    Model& result) {

  // Check invalid buffer view.
  if (parentBufferViewId < 0 || static_cast<size_t>(parentBufferViewId) >=
                                    parentModel.bufferViews.size()) {
    return -1;
  }

  const BufferView& parentBufferView =
      parentModel.bufferViews[static_cast<size_t>(parentBufferViewId)];

  // Check invalid buffer.
  if (parentBufferView.buffer < 0 ||
      static_cast<size_t>(parentBufferView.buffer) >=
          parentModel.buffers.size()) {
    // Should we return a valid buffer view with an invalid buffer instead?
    return -1;
  }

  const Buffer& parentBuffer =
      parentModel.buffers[static_cast<size_t>(parentBufferView.buffer)];

  size_t bufferId = result.buffers.size();
  Buffer& buffer = result.buffers.emplace_back();
  buffer.byteLength = parentBufferView.byteLength;
  buffer.cesium.data.resize(static_cast<size_t>(parentBufferView.byteLength));
  std::memcpy(
      buffer.cesium.data.data(),
      &parentBuffer.cesium
           .data[static_cast<size_t>(parentBufferView.byteOffset)],
      static_cast<size_t>(parentBufferView.byteLength));

  size_t bufferViewId = result.bufferViews.size();
  BufferView& bufferView = result.bufferViews.emplace_back();
  bufferView.buffer = static_cast<int32_t>(bufferId);
  bufferView.byteOffset = 0;
  bufferView.byteLength = parentBufferView.byteLength;
  bufferView.byteStride = parentBufferView.byteStride;

  return static_cast<int32_t>(bufferViewId);
}

void copyImages(const Model& parentModel, Model& result) {
  for (Image& image : result.images) {
    image.bufferView = copyBufferView(parentModel, image.bufferView, result);
  }
}

// Copy and reconstruct buffer views and buffers from EXT_structural_metadata
// property tables.
void copyMetadataTables(const Model& parentModel, Model& result) {
  ExtensionModelExtStructuralMetadata* pMetadata =
      result.getExtension<ExtensionModelExtStructuralMetadata>();
  if (pMetadata) {
    for (auto& propertyTable : pMetadata->propertyTables) {
      for (auto& propertyPair : propertyTable.properties) {
        PropertyTableProperty& property = propertyPair.second;
        property.values = copyBufferView(parentModel, property.values, result);
        property.arrayOffsets =
            copyBufferView(parentModel, property.arrayOffsets, result);
        property.stringOffsets =
            copyBufferView(parentModel, property.stringOffsets, result);
      }
    }
  }
}

} // namespace

} // namespace CesiumRasterOverlays
