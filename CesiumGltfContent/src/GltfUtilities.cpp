#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/IntersectionTests.h>
#include <CesiumGeometry/Ray.h>
#include <CesiumGeometry/Transforms.h>
#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/ExtensionCesiumRTC.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumGltfContent/SkirtMeshMetadata.h>

#include <limits>
#include <vector>

using namespace CesiumGltf;

namespace CesiumGltfContent {
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

/*static*/ void GltfUtilities::collapseToSingleBuffer(CesiumGltf::Model& gltf) {
  if (gltf.buffers.empty())
    return;

  Buffer& destinationBuffer = gltf.buffers[0];

  for (size_t i = 1; i < gltf.buffers.size(); ++i) {
    Buffer& sourceBuffer = gltf.buffers[i];
    GltfUtilities::moveBufferContent(gltf, destinationBuffer, sourceBuffer);
  }

  // Remove all the old buffers
  gltf.buffers.resize(1);
}

/*static*/ void GltfUtilities::moveBufferContent(
    CesiumGltf::Model& gltf,
    CesiumGltf::Buffer& destination,
    CesiumGltf::Buffer& source) {
  // Assert that the byteLength and the size of the cesium data vector are in
  // sync.
  assert(source.byteLength == int64_t(source.cesium.data.size()));
  assert(destination.byteLength == int64_t(destination.cesium.data.size()));

  int64_t sourceIndex = &source - &gltf.buffers[0];
  int64_t destinationIndex = &destination - &gltf.buffers[0];

  // Both buffers must exist in the glTF.
  if (sourceIndex < 0 || sourceIndex >= int64_t(gltf.buffers.size()) ||
      destinationIndex < 0 ||
      destinationIndex >= int64_t(gltf.buffers.size())) {
    assert(false);
    return;
  }

  // Copy the data to the destination and keep track of where we put it.
  size_t start = destination.cesium.data.size();

  destination.cesium.data.insert(
      destination.cesium.data.end(),
      source.cesium.data.begin(),
      source.cesium.data.end());

  source.byteLength = 0;
  source.cesium.data.clear();
  source.cesium.data.shrink_to_fit();

  destination.byteLength = int64_t(destination.cesium.data.size());

  // Update all the bufferViews that previously referred to the source Buffer to
  // refer to the destination Buffer instead.
  for (BufferView& bufferView : gltf.bufferViews) {
    if (bufferView.buffer != sourceIndex)
      continue;

    bufferView.buffer = int32_t(destinationIndex);
    bufferView.byteOffset += int64_t(start);
  }
}

namespace {

double signAwareMin(double n1, double n2) {
  if (n2 < n1)
    std::swap(n1, n2);
  return n1 < 0 ? n2 : n1;
}

template <class T>
bool intersectRayPrimitiveParametric(
    const CesiumGeometry::Ray& ray,
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    bool cullBackFaces,
    double& tMin) {

  auto positionAccessorIt = primitive.attributes.find("POSITION");
  if (positionAccessorIt == primitive.attributes.end()) {
    return false;
  }

  int positionAccessorID = positionAccessorIt->second;
  const Accessor* pPositionAccessor =
      Model::getSafe(&model.accessors, positionAccessorID);
  if (!pPositionAccessor) {
    return false;
  }

  const std::vector<double>& min = pPositionAccessor->min;
  const std::vector<double>& max = pPositionAccessor->max;

  double t;
  if (!CesiumGeometry::IntersectionTests::rayAABBParametric(
          ray,
          CesiumGeometry::AxisAlignedBox(
              min[0],
              min[1],
              min[2],
              max[0],
              max[1],
              max[2]),
          t)) {
    return false;
  }

  AccessorView<T> indicesView(model, primitive.indices);
  AccessorView<glm::vec3> positionView(model, *pPositionAccessor);

  tMin = -std::numeric_limits<double>::max();
  double tCurr;
  bool intersected = false;

  if (primitive.mode == CesiumGltf::MeshPrimitive::Mode::TRIANGLES) {
    for (int32_t i = 0; i < indicesView.size(); i += 3) {
      if (CesiumGeometry::IntersectionTests::rayTriangleParametric(
              ray,
              glm::dvec3(positionView[static_cast<int32_t>(indicesView[i])]),
              glm::dvec3(
                  positionView[static_cast<int32_t>(indicesView[i + 1])]),
              glm::dvec3(
                  positionView[static_cast<int32_t>(indicesView[i + 2])]),
              tCurr,
              cullBackFaces)) {
        intersected = true;
        tMin = signAwareMin(tMin, tCurr);
      }
    }
  } else {
    for (int32_t i = 0; i < indicesView.size() - 2; ++i) {
      if (i % 2) {
        if (CesiumGeometry::IntersectionTests::rayTriangleParametric(
                ray,
                glm::dvec3(positionView[static_cast<int32_t>(indicesView[i])]),
                glm::dvec3(
                    positionView[static_cast<int32_t>(indicesView[i + 2])]),
                glm::dvec3(
                    positionView[static_cast<int32_t>(indicesView[i + 1])]),
                tCurr,
                true)) {
          intersected = true;
          tMin = signAwareMin(tMin, tCurr);
        }
      } else {
        if (CesiumGeometry::IntersectionTests::rayTriangleParametric(
                ray,
                glm::dvec3(positionView[static_cast<int32_t>(indicesView[i])]),
                glm::dvec3(
                    positionView[static_cast<int32_t>(indicesView[i + 1])]),
                glm::dvec3(
                    positionView[static_cast<int32_t>(indicesView[i + 2])]),
                tCurr,
                true)) {
          intersected = true;
          tMin = signAwareMin(tMin, tCurr);
        }
      }
    }
  }
  return intersected;
}
} // namespace

bool GltfUtilities::intersectRayGltfModelParametric(
    const CesiumGeometry::Ray& ray,
    const CesiumGltf::Model& gltf,
    double& tMin,
    bool cullBackFaces,
    const glm::dmat4x4& modelToWorld) {
  glm::dmat4x4 rootTransform = applyRtcCenter(gltf, modelToWorld);
  rootTransform = applyGltfUpAxisTransform(gltf, rootTransform);

  tMin = -std::numeric_limits<double>::max();
  bool intersected = false;

  gltf.forEachPrimitiveInScene(
      -1,
      [ray, cullBackFaces, rootTransform, &intersected, &tMin](
          const CesiumGltf::Model& model,
          const CesiumGltf::Node& /*node*/,
          const CesiumGltf::Mesh& /*mesh*/,
          const CesiumGltf::MeshPrimitive& primitive,
          const glm::dmat4& nodeTransform) {
        if (primitive.mode != MeshPrimitive::Mode::TRIANGLES &&
            primitive.mode != MeshPrimitive::Mode::TRIANGLE_STRIP) {
          return;
        }

        glm::dmat4x4 worldToPrimitive =
            glm::inverse(rootTransform * nodeTransform);

        double tCurr = -std::numeric_limits<double>::max();
        bool intersectedPrimitive = false;

        switch (model.accessors[static_cast<uint32_t>(primitive.indices)]
                    .componentType) {
        case Accessor::ComponentType::UNSIGNED_BYTE:
          intersectedPrimitive = intersectRayPrimitiveParametric<uint8_t>(
              ray.transform(worldToPrimitive),
              model,
              primitive,
              cullBackFaces,
              tCurr);
          break;
        case Accessor::ComponentType::UNSIGNED_SHORT:
          intersectedPrimitive = intersectRayPrimitiveParametric<uint16_t>(
              ray.transform(worldToPrimitive),
              model,
              primitive,
              cullBackFaces,
              tCurr);
          break;
        case Accessor::ComponentType::UNSIGNED_INT:
          intersectedPrimitive = intersectRayPrimitiveParametric<uint32_t>(
              ray.transform(worldToPrimitive),
              model,
              primitive,
              cullBackFaces,
              tCurr);
          break;
        }
        if (intersectedPrimitive) {
          intersected = true;
          tMin = signAwareMin(tMin, tCurr);
        }
      });

  return intersected;
}

std::optional<glm::dvec3> GltfUtilities::intersectRayGltfModel(
    const CesiumGeometry::Ray& ray,
    const CesiumGltf::Model& gltf,
    bool cullBackFaces,
    const glm::dmat4x4& modelToWorld) {
  double t;
  return intersectRayGltfModelParametric(
             ray,
             gltf,
             t,
             cullBackFaces,
             modelToWorld) &&
                 t >= 0
             ? std::make_optional<glm::dvec3>(ray.getPointAlongRay(t))
             : std::nullopt;
}

} // namespace CesiumGltfContent
