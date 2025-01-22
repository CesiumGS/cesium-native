#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/AccessorWriter.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Node.h>
#include <CesiumGltfContent/SkirtMeshMetadata.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumRasterOverlays/RasterOverlayUtilities.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/ext/vector_bool3.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/trigonometric.hpp>

#include <cstdint>
#include <cstring>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumUtility;
using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumGltfContent;
using namespace CesiumRasterOverlays;
using namespace CesiumGltfReader;

static void checkSkirt(
    const Ellipsoid& ellipsoid,
    const glm::vec3& edgeUpsampledPosition,
    const glm::vec3& skirtUpsampledPosition,
    glm::dvec3 center,
    double skirtHeight) {
  glm::dvec3 edgePosition =
      static_cast<glm::dvec3>(edgeUpsampledPosition) + center;
  glm::dvec3 geodeticNormal = ellipsoid.geodeticSurfaceNormal(edgePosition);
  glm::dvec3 expectedPosition = edgePosition - skirtHeight * geodeticNormal;

  glm::dvec3 skirtPosition =
      static_cast<glm::dvec3>(skirtUpsampledPosition) + center;
  REQUIRE(
      Math::equalsEpsilon(expectedPosition.x, skirtPosition.x, Math::Epsilon7));
  REQUIRE(
      Math::equalsEpsilon(expectedPosition.y, skirtPosition.y, Math::Epsilon7));
  REQUIRE(
      Math::equalsEpsilon(expectedPosition.z, skirtPosition.z, Math::Epsilon7));
}

TEST_CASE("upsampleGltfForRasterOverlay with UNSIGNED_SHORT indices") {
  const Ellipsoid& ellipsoid = CesiumGeospatial::Ellipsoid::WGS84;
  Cartographic bottomLeftCart{glm::radians(110.0), glm::radians(32.0), 0.0};
  Cartographic topLeftCart{
      bottomLeftCart.longitude,
      bottomLeftCart.latitude + glm::radians(1.0),
      0.0};
  Cartographic topRightCart{
      bottomLeftCart.longitude + glm::radians(1.0),
      bottomLeftCart.latitude + glm::radians(1.0),
      0.0};
  Cartographic bottomRightCart{
      bottomLeftCart.longitude + glm::radians(1.0),
      bottomLeftCart.latitude,
      0.0};
  Cartographic centerCart{
      (bottomLeftCart.longitude + topRightCart.longitude) / 2.0,
      (bottomLeftCart.latitude + topRightCart.latitude) / 2.0,
      0.0};
  glm::dvec3 center = ellipsoid.cartographicToCartesian(centerCart);
  std::vector<glm::vec3> positions{
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(bottomLeftCart) - center),
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(topLeftCart) - center),
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(topRightCart) - center),
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(bottomRightCart) - center),
  };
  std::vector<glm::vec2> uvs{
      glm::vec2{0.0, 0.0},
      glm::vec2{0.0, 1.0},
      glm::vec2{1.0, 0.0},
      glm::vec2{1.0, 1.0}};
  std::vector<uint16_t> indices{0, 2, 1, 1, 2, 3};
  uint32_t positionsBufferSize =
      static_cast<uint32_t>(positions.size() * sizeof(glm::vec3));
  uint32_t uvsBufferSize =
      static_cast<uint32_t>(uvs.size() * sizeof(glm::vec2));
  uint32_t indicesBufferSize =
      static_cast<uint32_t>(indices.size() * sizeof(uint16_t));

  Model model;

  // create buffer
  model.buffers.emplace_back();
  Buffer& buffer = model.buffers.back();
  buffer.cesium.data.resize(
      positionsBufferSize + uvsBufferSize + indicesBufferSize);
  std::memcpy(buffer.cesium.data.data(), positions.data(), positionsBufferSize);
  std::memcpy(
      buffer.cesium.data.data() + positionsBufferSize,
      uvs.data(),
      uvsBufferSize);
  std::memcpy(
      buffer.cesium.data.data() + positionsBufferSize + uvsBufferSize,
      indices.data(),
      indicesBufferSize);

  // create position
  model.bufferViews.emplace_back();
  BufferView& positionBufferView = model.bufferViews.emplace_back();
  positionBufferView.buffer = static_cast<int>(model.buffers.size() - 1);
  positionBufferView.byteOffset = 0;
  positionBufferView.byteLength = positionsBufferSize;

  model.accessors.emplace_back();
  Accessor& positionAccessor = model.accessors.back();
  positionAccessor.bufferView =
      static_cast<int32_t>(model.bufferViews.size() - 1);
  positionAccessor.byteOffset = 0;
  positionAccessor.count = static_cast<int64_t>(positions.size());
  positionAccessor.componentType = Accessor::ComponentType::FLOAT;
  positionAccessor.type = Accessor::Type::VEC3;

  int32_t positionAccessorIdx =
      static_cast<int32_t>(model.accessors.size() - 1);

  // create uv
  model.bufferViews.emplace_back();
  BufferView& uvBufferView = model.bufferViews.emplace_back();
  uvBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  uvBufferView.byteOffset = positionsBufferSize;
  uvBufferView.byteLength = uvsBufferSize;

  model.accessors.emplace_back();
  Accessor& uvAccessor = model.accessors.back();
  uvAccessor.bufferView = static_cast<int32_t>(model.bufferViews.size() - 1);
  uvAccessor.byteOffset = 0;
  uvAccessor.count = static_cast<int64_t>(uvs.size());
  uvAccessor.componentType = Accessor::ComponentType::FLOAT;
  uvAccessor.type = Accessor::Type::VEC2;

  int32_t uvAccessorIdx = static_cast<int32_t>(model.accessors.size() - 1);

  // create indices
  model.bufferViews.emplace_back();
  BufferView& indicesBufferView = model.bufferViews.emplace_back();
  indicesBufferView.buffer = static_cast<int>(model.buffers.size() - 1);
  indicesBufferView.byteOffset = positionsBufferSize + uvsBufferSize;
  indicesBufferView.byteLength = indicesBufferSize;

  model.accessors.emplace_back();
  Accessor& indicesAccessor = model.accessors.back();
  indicesAccessor.bufferView = static_cast<int>(model.bufferViews.size() - 1);
  indicesAccessor.byteOffset = 0;
  indicesAccessor.count = static_cast<int64_t>(indices.size());
  indicesAccessor.componentType = Accessor::ComponentType::UNSIGNED_SHORT;
  indicesAccessor.type = Accessor::Type::SCALAR;

  int indicesAccessorIdx = static_cast<int>(model.accessors.size() - 1);

  model.meshes.emplace_back();
  Mesh& mesh = model.meshes.back();
  mesh.primitives.emplace_back();

  MeshPrimitive& primitive = mesh.primitives.back();
  primitive.mode = MeshPrimitive::Mode::TRIANGLES;
  primitive.attributes["_CESIUMOVERLAY_0"] = uvAccessorIdx;
  primitive.attributes["POSITION"] = positionAccessorIdx;
  primitive.indices = indicesAccessorIdx;

  // create node and update bounding volume
  model.nodes.emplace_back();
  Node& node = model.nodes[0];
  node.mesh = static_cast<int>(model.meshes.size() - 1);
  node.matrix = {
      1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      0.0,
      -1.0,
      0.0,
      0.0,
      1.0,
      0.0,
      0.0,
      center.x,
      center.z,
      -center.y,
      1.0};

  CesiumGeometry::UpsampledQuadtreeNode lowerLeft{
      CesiumGeometry::QuadtreeTileID(1, 0, 0)};
  CesiumGeometry::UpsampledQuadtreeNode upperLeft{
      CesiumGeometry::QuadtreeTileID(1, 0, 1)};
  CesiumGeometry::UpsampledQuadtreeNode lowerRight{
      CesiumGeometry::QuadtreeTileID(1, 1, 0)};
  CesiumGeometry::UpsampledQuadtreeNode upperRight{
      CesiumGeometry::QuadtreeTileID(1, 1, 1)};

  SUBCASE("Upsample bottom left child") {
    Model upsampledModel =
        *RasterOverlayUtilities::upsampleGltfForRasterOverlays(
            model,
            lowerLeft,
            false);

    REQUIRE(upsampledModel.meshes.size() == 1);
    const Mesh& upsampledMesh = upsampledModel.meshes.back();

    REQUIRE(upsampledMesh.primitives.size() == 1);
    const MeshPrimitive& upsampledPrimitive = upsampledMesh.primitives.back();

    REQUIRE(upsampledPrimitive.indices >= 0);
    REQUIRE(
        upsampledPrimitive.attributes.find("POSITION") !=
        upsampledPrimitive.attributes.end());
    AccessorView<glm::vec3> upsampledPosition(
        upsampledModel,
        upsampledPrimitive.attributes.at("POSITION"));
    AccessorView<uint32_t> upsampledIndices(
        upsampledModel,
        upsampledPrimitive.indices);

    glm::vec3 p0 = upsampledPosition[0];
    REQUIRE(
        glm::epsilonEqual(
            p0,
            positions[0],
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p1 = upsampledPosition[1];
    REQUIRE(
        glm::epsilonEqual(
            p1,
            (positions[0] + positions[2]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p2 = upsampledPosition[2];
    REQUIRE(
        glm::epsilonEqual(
            p2,
            (upsampledPosition[1] + positions[1]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p3 = upsampledPosition[3];
    REQUIRE(
        glm::epsilonEqual(
            p3,
            (positions[0] + positions[1]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p4 = upsampledPosition[4];
    REQUIRE(
        glm::epsilonEqual(
            p4,
            (positions[0] + positions[2]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p5 = upsampledPosition[5];
    REQUIRE(
        glm::epsilonEqual(
            p5,
            (positions[1] + positions[2]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p6 = upsampledPosition[6];
    REQUIRE(
        glm::epsilonEqual(
            p6,
            (upsampledPosition[4] + positions[1]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));
  }

  SUBCASE("Upsample bottom left child with inverted texture coordinates") {
    // Invert the V coordinate
    AccessorWriter<glm::vec2> uvWriter(model, 1);
    for (int64_t i = 0; i < uvWriter.size(); ++i) {
      uvWriter[i].y = 1.0f - uvWriter[i].y;
    }

    Model upsampledModel =
        *RasterOverlayUtilities::upsampleGltfForRasterOverlays(
            model,
            lowerLeft,
            true);

    REQUIRE(upsampledModel.meshes.size() == 1);
    const Mesh& upsampledMesh = upsampledModel.meshes.back();

    REQUIRE(upsampledMesh.primitives.size() == 1);
    const MeshPrimitive& upsampledPrimitive = upsampledMesh.primitives.back();

    REQUIRE(upsampledPrimitive.indices >= 0);
    REQUIRE(
        upsampledPrimitive.attributes.find("POSITION") !=
        upsampledPrimitive.attributes.end());
    AccessorView<glm::vec3> upsampledPosition(
        upsampledModel,
        upsampledPrimitive.attributes.at("POSITION"));
    AccessorView<uint32_t> upsampledIndices(
        upsampledModel,
        upsampledPrimitive.indices);

    glm::vec3 p0 = upsampledPosition[0];
    REQUIRE(
        glm::epsilonEqual(
            p0,
            positions[0],
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p1 = upsampledPosition[1];
    REQUIRE(
        glm::epsilonEqual(
            p1,
            (positions[0] + positions[2]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p2 = upsampledPosition[2];
    REQUIRE(
        glm::epsilonEqual(
            p2,
            (upsampledPosition[1] + positions[1]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p3 = upsampledPosition[3];
    REQUIRE(
        glm::epsilonEqual(
            p3,
            (positions[0] + positions[1]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p4 = upsampledPosition[4];
    REQUIRE(
        glm::epsilonEqual(
            p4,
            (positions[0] + positions[2]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p5 = upsampledPosition[5];
    REQUIRE(
        glm::epsilonEqual(
            p5,
            (positions[1] + positions[2]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p6 = upsampledPosition[6];
    REQUIRE(
        glm::epsilonEqual(
            p6,
            (upsampledPosition[4] + positions[1]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));
  }

  SUBCASE("Upsample upper left child") {
    Model upsampledModel =
        *RasterOverlayUtilities::upsampleGltfForRasterOverlays(
            model,
            upperLeft,
            false);

    REQUIRE(upsampledModel.meshes.size() == 1);
    const Mesh& upsampledMesh = upsampledModel.meshes.back();

    REQUIRE(upsampledMesh.primitives.size() == 1);
    const MeshPrimitive& upsampledPrimitive = upsampledMesh.primitives.back();

    REQUIRE(upsampledPrimitive.indices >= 0);
    REQUIRE(
        upsampledPrimitive.attributes.find("POSITION") !=
        upsampledPrimitive.attributes.end());
    AccessorView<glm::vec3> upsampledPosition(
        upsampledModel,
        upsampledPrimitive.attributes.at("POSITION"));
    AccessorView<uint32_t> upsampledIndices(
        upsampledModel,
        upsampledPrimitive.indices);

    glm::vec3 p0 = upsampledPosition[0];
    REQUIRE(
        glm::epsilonEqual(
            p0,
            positions[1],
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p1 = upsampledPosition[1];
    REQUIRE(
        glm::epsilonEqual(
            p1,
            (positions[0] + positions[1]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p2 = upsampledPosition[2];
    REQUIRE(
        glm::epsilonEqual(
            p2,
            (positions[1] + 0.5f * (positions[0] + positions[2])) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p3 = upsampledPosition[3];
    REQUIRE(
        glm::epsilonEqual(
            p3,
            (positions[1] + positions[2]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p4 = upsampledPosition[4];
    REQUIRE(
        glm::epsilonEqual(
            p4,
            p2,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p5 = upsampledPosition[5];
    REQUIRE(
        glm::epsilonEqual(
            p5,
            (positions[1] + positions[2]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p6 = upsampledPosition[6];
    REQUIRE(
        glm::epsilonEqual(
            p6,
            (positions[1] + positions[3]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));
  }

  SUBCASE("Upsample upper right child") {
    Model upsampledModel =
        *RasterOverlayUtilities::upsampleGltfForRasterOverlays(
            model,
            upperRight,
            false);

    REQUIRE(upsampledModel.meshes.size() == 1);
    const Mesh& upsampledMesh = upsampledModel.meshes.back();

    REQUIRE(upsampledMesh.primitives.size() == 1);
    const MeshPrimitive& upsampledPrimitive = upsampledMesh.primitives.back();

    REQUIRE(upsampledPrimitive.indices >= 0);
    REQUIRE(
        upsampledPrimitive.attributes.find("POSITION") !=
        upsampledPrimitive.attributes.end());
    AccessorView<glm::vec3> upsampledPosition(
        upsampledModel,
        upsampledPrimitive.attributes.at("POSITION"));
    AccessorView<uint32_t> upsampledIndices(
        upsampledModel,
        upsampledPrimitive.indices);

    glm::vec3 p0 = upsampledPosition[0];
    REQUIRE(
        glm::epsilonEqual(
            p0,
            positions[3],
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p1 = upsampledPosition[1];
    REQUIRE(
        glm::epsilonEqual(
            p1,
            (positions[1] + positions[3]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p2 = upsampledPosition[2];
    REQUIRE(
        glm::epsilonEqual(
            p2,
            (positions[2] + 0.5f * (positions[1] + positions[3])) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p3 = upsampledPosition[3];
    REQUIRE(
        glm::epsilonEqual(
            p3,
            (positions[3] + positions[2]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p4 = upsampledPosition[4];
    REQUIRE(
        glm::epsilonEqual(
            p4,
            (positions[1] + positions[3]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p5 = upsampledPosition[5];
    REQUIRE(
        glm::epsilonEqual(
            p5,
            (positions[1] + positions[2]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p6 = upsampledPosition[6];
    REQUIRE(
        glm::epsilonEqual(
            p6,
            p2,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));
  }

  SUBCASE("Upsample bottom right child") {
    Model upsampledModel =
        *RasterOverlayUtilities::upsampleGltfForRasterOverlays(
            model,
            lowerRight,
            false);

    REQUIRE(upsampledModel.meshes.size() == 1);
    const Mesh& upsampledMesh = upsampledModel.meshes.back();

    REQUIRE(upsampledMesh.primitives.size() == 1);
    const MeshPrimitive& upsampledPrimitive = upsampledMesh.primitives.back();

    REQUIRE(upsampledPrimitive.indices >= 0);
    REQUIRE(
        upsampledPrimitive.attributes.find("POSITION") !=
        upsampledPrimitive.attributes.end());
    AccessorView<glm::vec3> upsampledPosition(
        upsampledModel,
        upsampledPrimitive.attributes.at("POSITION"));
    AccessorView<uint32_t> upsampledIndices(
        upsampledModel,
        upsampledPrimitive.indices);

    glm::vec3 p0 = upsampledPosition[0];
    REQUIRE(
        glm::epsilonEqual(
            p0,
            positions[2],
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p1 = upsampledPosition[1];
    REQUIRE(
        glm::epsilonEqual(
            p1,
            (positions[1] + positions[2]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p2 = upsampledPosition[2];
    REQUIRE(
        glm::epsilonEqual(
            p2,
            (positions[0] + positions[2]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p3 = upsampledPosition[3];
    REQUIRE(
        glm::epsilonEqual(
            p3,
            (positions[2] + positions[3]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p4 = upsampledPosition[4];
    REQUIRE(
        glm::epsilonEqual(
            p4,
            (positions[2] + (positions[1] + positions[3]) * 0.5f) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p5 = upsampledPosition[5];
    REQUIRE(
        glm::epsilonEqual(
            p5,
            (positions[1] + positions[2]) * 0.5f,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

    glm::vec3 p6 = upsampledPosition[6];
    REQUIRE(
        glm::epsilonEqual(
            p6,
            p4,
            glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));
  }

  SUBCASE("Check skirt") {
    // add skirts info to primitive extra in case we need to upsample from it
    double skirtHeight = 12.0;
    SkirtMeshMetadata skirtMeshMetadata;
    skirtMeshMetadata.noSkirtIndicesBegin = 0;
    skirtMeshMetadata.noSkirtIndicesCount =
        static_cast<uint32_t>(indices.size());
    skirtMeshMetadata.meshCenter = center;
    skirtMeshMetadata.skirtWestHeight = skirtHeight;
    skirtMeshMetadata.skirtSouthHeight = skirtHeight;
    skirtMeshMetadata.skirtEastHeight = skirtHeight;
    skirtMeshMetadata.skirtNorthHeight = skirtHeight;

    primitive.extras = SkirtMeshMetadata::createGltfExtras(skirtMeshMetadata);

    SUBCASE("Check bottom left skirt") {
      Model upsampledModel =
          *RasterOverlayUtilities::upsampleGltfForRasterOverlays(
              model,
              lowerLeft,
              false);

      REQUIRE(upsampledModel.meshes.size() == 1);
      const Mesh& upsampledMesh = upsampledModel.meshes.back();

      REQUIRE(upsampledMesh.primitives.size() == 1);
      const MeshPrimitive& upsampledPrimitive = upsampledMesh.primitives.back();

      REQUIRE(upsampledPrimitive.indices >= 0);
      REQUIRE(
          upsampledPrimitive.attributes.find("POSITION") !=
          upsampledPrimitive.attributes.end());
      AccessorView<glm::vec3> upsampledPosition(
          upsampledModel,
          upsampledPrimitive.attributes.at("POSITION"));
      AccessorView<uint32_t> upsampledIndices(
          upsampledModel,
          upsampledPrimitive.indices);

      // check west edge
      checkSkirt(
          ellipsoid,
          upsampledPosition[0],
          upsampledPosition[7],
          center,
          skirtHeight);
      checkSkirt(
          ellipsoid,
          upsampledPosition[3],
          upsampledPosition[8],
          center,
          skirtHeight);

      // check south edge
      checkSkirt(
          ellipsoid,
          upsampledPosition[1],
          upsampledPosition[9],
          center,
          skirtHeight);
      checkSkirt(
          ellipsoid,
          upsampledPosition[4],
          upsampledPosition[10],
          center,
          skirtHeight);
      checkSkirt(
          ellipsoid,
          upsampledPosition[0],
          upsampledPosition[11],
          center,
          skirtHeight);

      // check east edge
      checkSkirt(
          ellipsoid,
          upsampledPosition[5],
          upsampledPosition[12],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[1],
          upsampledPosition[13],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[4],
          upsampledPosition[14],
          center,
          skirtHeight * 0.5);

      // check north edge
      checkSkirt(
          ellipsoid,
          upsampledPosition[3],
          upsampledPosition[15],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[2],
          upsampledPosition[16],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[6],
          upsampledPosition[17],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[5],
          upsampledPosition[18],
          center,
          skirtHeight * 0.5);
    }

    SUBCASE("Check upper left skirt") {
      Model upsampledModel =
          *RasterOverlayUtilities::upsampleGltfForRasterOverlays(
              model,
              upperLeft,
              false);

      REQUIRE(upsampledModel.meshes.size() == 1);
      const Mesh& upsampledMesh = upsampledModel.meshes.back();

      REQUIRE(upsampledMesh.primitives.size() == 1);
      const MeshPrimitive& upsampledPrimitive = upsampledMesh.primitives.back();

      REQUIRE(upsampledPrimitive.indices >= 0);
      REQUIRE(
          upsampledPrimitive.attributes.find("POSITION") !=
          upsampledPrimitive.attributes.end());
      AccessorView<glm::vec3> upsampledPosition(
          upsampledModel,
          upsampledPrimitive.attributes.at("POSITION"));
      AccessorView<uint32_t> upsampledIndices(
          upsampledModel,
          upsampledPrimitive.indices);

      // check west edge
      checkSkirt(
          ellipsoid,
          upsampledPosition[1],
          upsampledPosition[7],
          center,
          skirtHeight);
      checkSkirt(
          ellipsoid,
          upsampledPosition[0],
          upsampledPosition[8],
          center,
          skirtHeight);
      checkSkirt(
          ellipsoid,
          upsampledPosition[0],
          upsampledPosition[9],
          center,
          skirtHeight);
      checkSkirt(
          ellipsoid,
          upsampledPosition[0],
          upsampledPosition[10],
          center,
          skirtHeight);

      // check south edge
      checkSkirt(
          ellipsoid,
          upsampledPosition[3],
          upsampledPosition[11],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[5],
          upsampledPosition[12],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[2],
          upsampledPosition[13],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[4],
          upsampledPosition[14],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[1],
          upsampledPosition[15],
          center,
          skirtHeight * 0.5);

      // check east edge
      checkSkirt(
          ellipsoid,
          upsampledPosition[6],
          upsampledPosition[16],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[3],
          upsampledPosition[17],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[5],
          upsampledPosition[18],
          center,
          skirtHeight * 0.5);

      // check north edge
      checkSkirt(
          ellipsoid,
          upsampledPosition[0],
          upsampledPosition[19],
          center,
          skirtHeight);
      checkSkirt(
          ellipsoid,
          upsampledPosition[0],
          upsampledPosition[20],
          center,
          skirtHeight);
      checkSkirt(
          ellipsoid,
          upsampledPosition[0],
          upsampledPosition[21],
          center,
          skirtHeight);
      checkSkirt(
          ellipsoid,
          upsampledPosition[6],
          upsampledPosition[22],
          center,
          skirtHeight);
    }

    SUBCASE("Check upper right skirt") {
      Model upsampledModel =
          *RasterOverlayUtilities::upsampleGltfForRasterOverlays(
              model,
              upperRight,
              false);

      REQUIRE(upsampledModel.meshes.size() == 1);
      const Mesh& upsampledMesh = upsampledModel.meshes.back();

      REQUIRE(upsampledMesh.primitives.size() == 1);
      const MeshPrimitive& upsampledPrimitive = upsampledMesh.primitives.back();

      REQUIRE(upsampledPrimitive.indices >= 0);
      REQUIRE(
          upsampledPrimitive.attributes.find("POSITION") !=
          upsampledPrimitive.attributes.end());
      AccessorView<glm::vec3> upsampledPosition(
          upsampledModel,
          upsampledPrimitive.attributes.at("POSITION"));
      AccessorView<uint32_t> upsampledIndices(
          upsampledModel,
          upsampledPrimitive.indices);

      // check west edge
      checkSkirt(
          ellipsoid,
          upsampledPosition[5],
          upsampledPosition[7],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[1],
          upsampledPosition[8],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[4],
          upsampledPosition[9],
          center,
          skirtHeight * 0.5);

      // check south edge
      checkSkirt(
          ellipsoid,
          upsampledPosition[3],
          upsampledPosition[10],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[2],
          upsampledPosition[11],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[6],
          upsampledPosition[12],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[5],
          upsampledPosition[13],
          center,
          skirtHeight * 0.5);

      // check east edge
      checkSkirt(
          ellipsoid,
          upsampledPosition[0],
          upsampledPosition[14],
          center,
          skirtHeight);
      checkSkirt(
          ellipsoid,
          upsampledPosition[3],
          upsampledPosition[15],
          center,
          skirtHeight);

      // check north edge
      checkSkirt(
          ellipsoid,
          upsampledPosition[1],
          upsampledPosition[16],
          center,
          skirtHeight);
      checkSkirt(
          ellipsoid,
          upsampledPosition[4],
          upsampledPosition[17],
          center,
          skirtHeight);
      checkSkirt(
          ellipsoid,
          upsampledPosition[0],
          upsampledPosition[18],
          center,
          skirtHeight);
    }

    SUBCASE("Check bottom right skirt") {
      Model upsampledModel =
          *RasterOverlayUtilities::upsampleGltfForRasterOverlays(
              model,
              lowerRight,
              false);

      REQUIRE(upsampledModel.meshes.size() == 1);
      const Mesh& upsampledMesh = upsampledModel.meshes.back();

      REQUIRE(upsampledMesh.primitives.size() == 1);
      const MeshPrimitive& upsampledPrimitive = upsampledMesh.primitives.back();

      REQUIRE(upsampledPrimitive.indices >= 0);
      REQUIRE(
          upsampledPrimitive.attributes.find("POSITION") !=
          upsampledPrimitive.attributes.end());
      AccessorView<glm::vec3> upsampledPosition(
          upsampledModel,
          upsampledPrimitive.attributes.at("POSITION"));
      AccessorView<uint32_t> upsampledIndices(
          upsampledModel,
          upsampledPrimitive.indices);

      // check west edge
      checkSkirt(
          ellipsoid,
          upsampledPosition[2],
          upsampledPosition[7],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[1],
          upsampledPosition[8],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[5],
          upsampledPosition[9],
          center,
          skirtHeight * 0.5);

      // check south edge
      checkSkirt(
          ellipsoid,
          upsampledPosition[0],
          upsampledPosition[10],
          center,
          skirtHeight);
      checkSkirt(
          ellipsoid,
          upsampledPosition[0],
          upsampledPosition[11],
          center,
          skirtHeight);
      checkSkirt(
          ellipsoid,
          upsampledPosition[0],
          upsampledPosition[12],
          center,
          skirtHeight);
      checkSkirt(
          ellipsoid,
          upsampledPosition[2],
          upsampledPosition[13],
          center,
          skirtHeight);

      // check east edge
      checkSkirt(
          ellipsoid,
          upsampledPosition[3],
          upsampledPosition[14],
          center,
          skirtHeight);
      checkSkirt(
          ellipsoid,
          upsampledPosition[0],
          upsampledPosition[15],
          center,
          skirtHeight);
      checkSkirt(
          ellipsoid,
          upsampledPosition[0],
          upsampledPosition[16],
          center,
          skirtHeight);
      checkSkirt(
          ellipsoid,
          upsampledPosition[0],
          upsampledPosition[17],
          center,
          skirtHeight);

      // check north edge
      checkSkirt(
          ellipsoid,
          upsampledPosition[1],
          upsampledPosition[18],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[5],
          upsampledPosition[19],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[4],
          upsampledPosition[20],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[6],
          upsampledPosition[21],
          center,
          skirtHeight * 0.5);
      checkSkirt(
          ellipsoid,
          upsampledPosition[3],
          upsampledPosition[22],
          center,
          skirtHeight * 0.5);
    }
  }
}

TEST_CASE("upsampleGltfForRasterOverlay with UNSIGNED_BYTE indices") {
  const Ellipsoid& ellipsoid = CesiumGeospatial::Ellipsoid::WGS84;
  Cartographic bottomLeftCart{glm::radians(110.0), glm::radians(32.0), 0.0};
  Cartographic topLeftCart{
      bottomLeftCart.longitude,
      bottomLeftCart.latitude + glm::radians(1.0),
      0.0};
  Cartographic topRightCart{
      bottomLeftCart.longitude + glm::radians(1.0),
      bottomLeftCart.latitude + glm::radians(1.0),
      0.0};
  Cartographic bottomRightCart{
      bottomLeftCart.longitude + glm::radians(1.0),
      bottomLeftCart.latitude,
      0.0};
  Cartographic centerCart{
      (bottomLeftCart.longitude + topRightCart.longitude) / 2.0,
      (bottomLeftCart.latitude + topRightCart.latitude) / 2.0,
      0.0};
  glm::dvec3 center = ellipsoid.cartographicToCartesian(centerCart);
  std::vector<glm::vec3> positions{
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(bottomLeftCart) - center),
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(topLeftCart) - center),
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(topRightCart) - center),
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(bottomRightCart) - center),
  };
  std::vector<glm::vec2> uvs{
      glm::vec2{0.0, 0.0},
      glm::vec2{0.0, 1.0},
      glm::vec2{1.0, 0.0},
      glm::vec2{1.0, 1.0}};
  std::vector<uint8_t> indices{0, 2, 1, 1, 2, 3};
  uint32_t positionsBufferSize =
      static_cast<uint32_t>(positions.size() * sizeof(glm::vec3));
  uint32_t uvsBufferSize =
      static_cast<uint32_t>(uvs.size() * sizeof(glm::vec2));
  uint32_t indicesBufferSize =
      static_cast<uint32_t>(indices.size() * sizeof(uint8_t));

  Model model;

  // create buffer
  model.buffers.emplace_back();
  Buffer& buffer = model.buffers.back();
  buffer.cesium.data.resize(
      positionsBufferSize + uvsBufferSize + indicesBufferSize);
  std::memcpy(buffer.cesium.data.data(), positions.data(), positionsBufferSize);
  std::memcpy(
      buffer.cesium.data.data() + positionsBufferSize,
      uvs.data(),
      uvsBufferSize);
  std::memcpy(
      buffer.cesium.data.data() + positionsBufferSize + uvsBufferSize,
      indices.data(),
      indicesBufferSize);

  // create position
  model.bufferViews.emplace_back();
  BufferView& positionBufferView = model.bufferViews.emplace_back();
  positionBufferView.buffer = static_cast<int>(model.buffers.size() - 1);
  positionBufferView.byteOffset = 0;
  positionBufferView.byteLength = positionsBufferSize;

  model.accessors.emplace_back();
  Accessor& positionAccessor = model.accessors.back();
  positionAccessor.bufferView =
      static_cast<int32_t>(model.bufferViews.size() - 1);
  positionAccessor.byteOffset = 0;
  positionAccessor.count = static_cast<int64_t>(positions.size());
  positionAccessor.componentType = Accessor::ComponentType::FLOAT;
  positionAccessor.type = Accessor::Type::VEC3;

  int32_t positionAccessorIdx =
      static_cast<int32_t>(model.accessors.size() - 1);

  // create uv
  model.bufferViews.emplace_back();
  BufferView& uvBufferView = model.bufferViews.emplace_back();
  uvBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  uvBufferView.byteOffset = positionsBufferSize;
  uvBufferView.byteLength = uvsBufferSize;

  model.accessors.emplace_back();
  Accessor& uvAccessor = model.accessors.back();
  uvAccessor.bufferView = static_cast<int32_t>(model.bufferViews.size() - 1);
  uvAccessor.byteOffset = 0;
  uvAccessor.count = static_cast<int64_t>(uvs.size());
  uvAccessor.componentType = Accessor::ComponentType::FLOAT;
  uvAccessor.type = Accessor::Type::VEC2;

  int32_t uvAccessorIdx = static_cast<int32_t>(model.accessors.size() - 1);

  // create indices
  model.bufferViews.emplace_back();
  BufferView& indicesBufferView = model.bufferViews.emplace_back();
  indicesBufferView.buffer = static_cast<int>(model.buffers.size() - 1);
  indicesBufferView.byteOffset = positionsBufferSize + uvsBufferSize;
  indicesBufferView.byteLength = indicesBufferSize;

  model.accessors.emplace_back();
  Accessor& indicesAccessor = model.accessors.back();
  indicesAccessor.bufferView = static_cast<int>(model.bufferViews.size() - 1);
  indicesAccessor.byteOffset = 0;
  indicesAccessor.count = static_cast<int64_t>(indices.size());
  indicesAccessor.componentType = Accessor::ComponentType::UNSIGNED_BYTE;
  indicesAccessor.type = Accessor::Type::SCALAR;

  int indicesAccessorIdx = static_cast<int>(model.accessors.size() - 1);

  model.meshes.emplace_back();
  Mesh& mesh = model.meshes.back();
  mesh.primitives.emplace_back();

  MeshPrimitive& primitive = mesh.primitives.back();
  primitive.mode = MeshPrimitive::Mode::TRIANGLES;
  primitive.attributes["_CESIUMOVERLAY_0"] = uvAccessorIdx;
  primitive.attributes["POSITION"] = positionAccessorIdx;
  primitive.indices = indicesAccessorIdx;

  // create node and update bounding volume
  model.nodes.emplace_back();
  Node& node = model.nodes[0];
  node.mesh = static_cast<int>(model.meshes.size() - 1);
  node.matrix = {
      1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      0.0,
      -1.0,
      0.0,
      0.0,
      1.0,
      0.0,
      0.0,
      center.x,
      center.z,
      -center.y,
      1.0};

  CesiumGeometry::UpsampledQuadtreeNode lowerLeft{
      CesiumGeometry::QuadtreeTileID(1, 0, 0)};

  Model upsampledModel = *RasterOverlayUtilities::upsampleGltfForRasterOverlays(
      model,
      lowerLeft,
      false);

  REQUIRE(upsampledModel.meshes.size() == 1);
  const Mesh& upsampledMesh = upsampledModel.meshes.back();

  REQUIRE(upsampledMesh.primitives.size() == 1);
  const MeshPrimitive& upsampledPrimitive = upsampledMesh.primitives.back();

  REQUIRE(upsampledPrimitive.indices >= 0);
  REQUIRE(
      upsampledPrimitive.attributes.find("POSITION") !=
      upsampledPrimitive.attributes.end());
  AccessorView<glm::vec3> upsampledPosition(
      upsampledModel,
      upsampledPrimitive.attributes.at("POSITION"));
  AccessorView<uint32_t> upsampledIndices(
      upsampledModel,
      upsampledPrimitive.indices);

  glm::vec3 p0 = upsampledPosition[0];
  REQUIRE(
      glm::epsilonEqual(
          p0,
          positions[0],
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p1 = upsampledPosition[1];
  REQUIRE(
      glm::epsilonEqual(
          p1,
          (positions[0] + positions[2]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p2 = upsampledPosition[2];
  REQUIRE(
      glm::epsilonEqual(
          p2,
          (upsampledPosition[1] + positions[1]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p3 = upsampledPosition[3];
  REQUIRE(
      glm::epsilonEqual(
          p3,
          (positions[0] + positions[1]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p4 = upsampledPosition[4];
  REQUIRE(
      glm::epsilonEqual(
          p4,
          (positions[0] + positions[2]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p5 = upsampledPosition[5];
  REQUIRE(
      glm::epsilonEqual(
          p5,
          (positions[1] + positions[2]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p6 = upsampledPosition[6];
  REQUIRE(
      glm::epsilonEqual(
          p6,
          (upsampledPosition[4] + positions[1]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));
}

TEST_CASE("upsampleGltfForRasterOverlay with non-indexed triangles") {
  const Ellipsoid& ellipsoid = CesiumGeospatial::Ellipsoid::WGS84;
  Cartographic bottomLeftCart{glm::radians(110.0), glm::radians(32.0), 0.0};
  Cartographic topLeftCart{
      bottomLeftCart.longitude,
      bottomLeftCart.latitude + glm::radians(1.0),
      0.0};
  Cartographic topRightCart{
      bottomLeftCart.longitude + glm::radians(1.0),
      bottomLeftCart.latitude + glm::radians(1.0),
      0.0};
  Cartographic bottomRightCart{
      bottomLeftCart.longitude + glm::radians(1.0),
      bottomLeftCart.latitude,
      0.0};
  Cartographic centerCart{
      (bottomLeftCart.longitude + topRightCart.longitude) / 2.0,
      (bottomLeftCart.latitude + topRightCart.latitude) / 2.0,
      0.0};
  glm::dvec3 center = ellipsoid.cartographicToCartesian(centerCart);
  std::vector<glm::vec3> positions{
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(bottomLeftCart) - center),
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(topRightCart) - center),
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(topLeftCart) - center),
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(topLeftCart) - center),
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(topRightCart) - center),
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(bottomRightCart) - center),
  };
  std::vector<glm::vec2> uvs{
      glm::vec2{0.0, 0.0},
      glm::vec2{1.0, 0.0},
      glm::vec2{0.0, 1.0},
      glm::vec2{0.0, 1.0},
      glm::vec2{1.0, 0.0},
      glm::vec2{1.0, 1.0}};
  uint32_t positionsBufferSize =
      static_cast<uint32_t>(positions.size() * sizeof(glm::vec3));
  uint32_t uvsBufferSize =
      static_cast<uint32_t>(uvs.size() * sizeof(glm::vec2));

  Model model;

  // create buffer
  model.buffers.emplace_back();
  Buffer& buffer = model.buffers.back();
  buffer.cesium.data.resize(positionsBufferSize + uvsBufferSize);
  std::memcpy(buffer.cesium.data.data(), positions.data(), positionsBufferSize);
  std::memcpy(
      buffer.cesium.data.data() + positionsBufferSize,
      uvs.data(),
      uvsBufferSize);

  // create position
  model.bufferViews.emplace_back();
  BufferView& positionBufferView = model.bufferViews.emplace_back();
  positionBufferView.buffer = static_cast<int>(model.buffers.size() - 1);
  positionBufferView.byteOffset = 0;
  positionBufferView.byteLength = positionsBufferSize;

  model.accessors.emplace_back();
  Accessor& positionAccessor = model.accessors.back();
  positionAccessor.bufferView =
      static_cast<int32_t>(model.bufferViews.size() - 1);
  positionAccessor.byteOffset = 0;
  positionAccessor.count = static_cast<int64_t>(positions.size());
  positionAccessor.componentType = Accessor::ComponentType::FLOAT;
  positionAccessor.type = Accessor::Type::VEC3;

  int32_t positionAccessorIdx =
      static_cast<int32_t>(model.accessors.size() - 1);

  // create uv
  model.bufferViews.emplace_back();
  BufferView& uvBufferView = model.bufferViews.emplace_back();
  uvBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  uvBufferView.byteOffset = positionsBufferSize;
  uvBufferView.byteLength = uvsBufferSize;

  model.accessors.emplace_back();
  Accessor& uvAccessor = model.accessors.back();
  uvAccessor.bufferView = static_cast<int32_t>(model.bufferViews.size() - 1);
  uvAccessor.byteOffset = 0;
  uvAccessor.count = static_cast<int64_t>(uvs.size());
  uvAccessor.componentType = Accessor::ComponentType::FLOAT;
  uvAccessor.type = Accessor::Type::VEC2;

  int32_t uvAccessorIdx = static_cast<int32_t>(model.accessors.size() - 1);

  model.meshes.emplace_back();
  Mesh& mesh = model.meshes.back();
  mesh.primitives.emplace_back();

  MeshPrimitive& primitive = mesh.primitives.back();
  primitive.mode = MeshPrimitive::Mode::TRIANGLES;
  primitive.attributes["_CESIUMOVERLAY_0"] = uvAccessorIdx;
  primitive.attributes["POSITION"] = positionAccessorIdx;

  // create node and update bounding volume
  model.nodes.emplace_back();
  Node& node = model.nodes[0];
  node.mesh = static_cast<int>(model.meshes.size() - 1);
  node.matrix = {
      1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      0.0,
      -1.0,
      0.0,
      0.0,
      1.0,
      0.0,
      0.0,
      center.x,
      center.z,
      -center.y,
      1.0};

  CesiumGeometry::UpsampledQuadtreeNode lowerLeft{
      CesiumGeometry::QuadtreeTileID(1, 0, 0)};

  std::optional<Model> upsampledModelOptional =
      RasterOverlayUtilities::upsampleGltfForRasterOverlays(
          model,
          lowerLeft,
          false);

  REQUIRE(upsampledModelOptional);

  Model& upsampledModel = *upsampledModelOptional;

  REQUIRE(upsampledModel.meshes.size() == 1);
  const Mesh& upsampledMesh = upsampledModel.meshes.back();

  REQUIRE(upsampledMesh.primitives.size() == 1);
  const MeshPrimitive& upsampledPrimitive = upsampledMesh.primitives.back();

  REQUIRE(upsampledPrimitive.indices >= 0);
  REQUIRE(
      upsampledPrimitive.attributes.find("POSITION") !=
      upsampledPrimitive.attributes.end());
  AccessorView<glm::vec3> upsampledPosition(
      upsampledModel,
      upsampledPrimitive.attributes.at("POSITION"));
  AccessorView<uint32_t> upsampledIndices(
      upsampledModel,
      upsampledPrimitive.indices);

  glm::vec3 p0 = upsampledPosition[0];
  CHECK(
      glm::epsilonEqual(
          p0,
          positions[0],
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p1 = upsampledPosition[1];
  CHECK(
      glm::epsilonEqual(
          p1,
          (positions[0] + positions[1]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p2 = upsampledPosition[2];
  CHECK(
      glm::epsilonEqual(
          p2,
          (upsampledPosition[1] + positions[2]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p3 = upsampledPosition[3];
  CHECK(
      glm::epsilonEqual(
          p3,
          (positions[0] + positions[2]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p4 = upsampledPosition[4];
  CHECK(
      glm::epsilonEqual(
          p4,
          (positions[0] + positions[1]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p5 = upsampledPosition[5];
  CHECK(
      glm::epsilonEqual(
          p5,
          (positions[2] + positions[1]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p6 = upsampledPosition[6];
  CHECK(
      glm::epsilonEqual(
          p6,
          (upsampledPosition[4] + positions[2]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));
}

TEST_CASE("upsampleGltfForRasterOverlay with a triangle strip primitive") {
  const Ellipsoid& ellipsoid = CesiumGeospatial::Ellipsoid::WGS84;
  Cartographic bottomLeftCart{glm::radians(110.0), glm::radians(32.0), 0.0};
  Cartographic topLeftCart{
      bottomLeftCart.longitude,
      bottomLeftCart.latitude + glm::radians(1.0),
      0.0};
  Cartographic topRightCart{
      bottomLeftCart.longitude + glm::radians(1.0),
      bottomLeftCart.latitude + glm::radians(1.0),
      0.0};
  Cartographic bottomRightCart{
      bottomLeftCart.longitude + glm::radians(1.0),
      bottomLeftCart.latitude,
      0.0};
  Cartographic centerCart{
      (bottomLeftCart.longitude + topRightCart.longitude) / 2.0,
      (bottomLeftCart.latitude + topRightCart.latitude) / 2.0,
      0.0};
  glm::dvec3 center = ellipsoid.cartographicToCartesian(centerCart);
  std::vector<glm::vec3> positions{
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(bottomLeftCart) - center),
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(topLeftCart) - center),
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(topRightCart) - center),
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(bottomRightCart) - center),
  };
  std::vector<glm::vec2> uvs{
      glm::vec2{0.0, 0.0},
      glm::vec2{0.0, 1.0},
      glm::vec2{1.0, 0.0},
      glm::vec2{1.0, 1.0}};
  std::vector<uint8_t> indices{0, 2, 1, 3};
  uint32_t positionsBufferSize =
      static_cast<uint32_t>(positions.size() * sizeof(glm::vec3));
  uint32_t uvsBufferSize =
      static_cast<uint32_t>(uvs.size() * sizeof(glm::vec2));
  uint32_t indicesBufferSize =
      static_cast<uint32_t>(indices.size() * sizeof(uint8_t));

  Model model;

  // create buffer
  model.buffers.emplace_back();
  Buffer& buffer = model.buffers.back();
  buffer.cesium.data.resize(
      positionsBufferSize + uvsBufferSize + indicesBufferSize);
  std::memcpy(buffer.cesium.data.data(), positions.data(), positionsBufferSize);
  std::memcpy(
      buffer.cesium.data.data() + positionsBufferSize,
      uvs.data(),
      uvsBufferSize);
  std::memcpy(
      buffer.cesium.data.data() + positionsBufferSize + uvsBufferSize,
      indices.data(),
      indicesBufferSize);

  // create position
  model.bufferViews.emplace_back();
  BufferView& positionBufferView = model.bufferViews.emplace_back();
  positionBufferView.buffer = static_cast<int>(model.buffers.size() - 1);
  positionBufferView.byteOffset = 0;
  positionBufferView.byteLength = positionsBufferSize;

  model.accessors.emplace_back();
  Accessor& positionAccessor = model.accessors.back();
  positionAccessor.bufferView =
      static_cast<int32_t>(model.bufferViews.size() - 1);
  positionAccessor.byteOffset = 0;
  positionAccessor.count = static_cast<int64_t>(positions.size());
  positionAccessor.componentType = Accessor::ComponentType::FLOAT;
  positionAccessor.type = Accessor::Type::VEC3;

  int32_t positionAccessorIdx =
      static_cast<int32_t>(model.accessors.size() - 1);

  // create uv
  model.bufferViews.emplace_back();
  BufferView& uvBufferView = model.bufferViews.emplace_back();
  uvBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  uvBufferView.byteOffset = positionsBufferSize;
  uvBufferView.byteLength = uvsBufferSize;

  model.accessors.emplace_back();
  Accessor& uvAccessor = model.accessors.back();
  uvAccessor.bufferView = static_cast<int32_t>(model.bufferViews.size() - 1);
  uvAccessor.byteOffset = 0;
  uvAccessor.count = static_cast<int64_t>(uvs.size());
  uvAccessor.componentType = Accessor::ComponentType::FLOAT;
  uvAccessor.type = Accessor::Type::VEC2;

  int32_t uvAccessorIdx = static_cast<int32_t>(model.accessors.size() - 1);

  // create indices
  model.bufferViews.emplace_back();
  BufferView& indicesBufferView = model.bufferViews.emplace_back();
  indicesBufferView.buffer = static_cast<int>(model.buffers.size() - 1);
  indicesBufferView.byteOffset = positionsBufferSize + uvsBufferSize;
  indicesBufferView.byteLength = indicesBufferSize;

  model.accessors.emplace_back();
  Accessor& indicesAccessor = model.accessors.back();
  indicesAccessor.bufferView = static_cast<int>(model.bufferViews.size() - 1);
  indicesAccessor.byteOffset = 0;
  indicesAccessor.count = static_cast<int64_t>(indices.size());
  indicesAccessor.componentType = Accessor::ComponentType::UNSIGNED_BYTE;
  indicesAccessor.type = Accessor::Type::SCALAR;

  int indicesAccessorIdx = static_cast<int>(model.accessors.size() - 1);

  model.meshes.emplace_back();
  Mesh& mesh = model.meshes.back();
  mesh.primitives.emplace_back();

  MeshPrimitive& primitive = mesh.primitives.back();
  primitive.mode = MeshPrimitive::Mode::TRIANGLE_STRIP;
  primitive.attributes["_CESIUMOVERLAY_0"] = uvAccessorIdx;
  primitive.attributes["POSITION"] = positionAccessorIdx;
  primitive.indices = indicesAccessorIdx;

  // create node and update bounding volume
  model.nodes.emplace_back();
  Node& node = model.nodes[0];
  node.mesh = static_cast<int>(model.meshes.size() - 1);
  node.matrix = {
      1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      0.0,
      -1.0,
      0.0,
      0.0,
      1.0,
      0.0,
      0.0,
      center.x,
      center.z,
      -center.y,
      1.0};

  CesiumGeometry::UpsampledQuadtreeNode lowerLeft{
      CesiumGeometry::QuadtreeTileID(1, 0, 0)};

  Model upsampledModel = *RasterOverlayUtilities::upsampleGltfForRasterOverlays(
      model,
      lowerLeft,
      false);

  REQUIRE(upsampledModel.meshes.size() == 1);
  const Mesh& upsampledMesh = upsampledModel.meshes.back();

  REQUIRE(upsampledMesh.primitives.size() == 1);
  const MeshPrimitive& upsampledPrimitive = upsampledMesh.primitives.back();

  REQUIRE(upsampledPrimitive.indices >= 0);
  REQUIRE(
      upsampledPrimitive.attributes.find("POSITION") !=
      upsampledPrimitive.attributes.end());
  AccessorView<glm::vec3> upsampledPosition(
      upsampledModel,
      upsampledPrimitive.attributes.at("POSITION"));
  AccessorView<uint32_t> upsampledIndices(
      upsampledModel,
      upsampledPrimitive.indices);

  glm::vec3 p0 = upsampledPosition[0];
  REQUIRE(
      glm::epsilonEqual(
          p0,
          positions[0],
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p1 = upsampledPosition[1];
  REQUIRE(
      glm::epsilonEqual(
          p1,
          (positions[0] + positions[2]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p2 = upsampledPosition[2];
  REQUIRE(
      glm::epsilonEqual(
          p2,
          (upsampledPosition[1] + positions[1]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p3 = upsampledPosition[3];
  REQUIRE(
      glm::epsilonEqual(
          p3,
          (positions[0] + positions[1]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p4 = upsampledPosition[4];
  REQUIRE(
      glm::epsilonEqual(
          p4,
          (positions[0] + positions[2]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p5 = upsampledPosition[5];
  REQUIRE(
      glm::epsilonEqual(
          p5,
          (positions[1] + positions[2]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p6 = upsampledPosition[6];
  REQUIRE(
      glm::epsilonEqual(
          p6,
          (upsampledPosition[4] + positions[1]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));
}

TEST_CASE("upsampleGltfForRasterOverlay with a triangle fan primitive") {
  const Ellipsoid& ellipsoid = CesiumGeospatial::Ellipsoid::WGS84;
  Cartographic bottomLeftCart{glm::radians(110.0), glm::radians(32.0), 0.0};
  Cartographic topLeftCart{
      bottomLeftCart.longitude,
      bottomLeftCart.latitude + glm::radians(1.0),
      0.0};
  Cartographic topRightCart{
      bottomLeftCart.longitude + glm::radians(1.0),
      bottomLeftCart.latitude + glm::radians(1.0),
      0.0};
  Cartographic bottomRightCart{
      bottomLeftCart.longitude + glm::radians(1.0),
      bottomLeftCart.latitude,
      0.0};
  Cartographic centerCart{
      (bottomLeftCart.longitude + topRightCart.longitude) / 2.0,
      (bottomLeftCart.latitude + topRightCart.latitude) / 2.0,
      0.0};
  glm::dvec3 center = ellipsoid.cartographicToCartesian(centerCart);
  std::vector<glm::vec3> positions{
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(bottomLeftCart) - center),
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(topLeftCart) - center),
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(topRightCart) - center),
      static_cast<glm::vec3>(
          ellipsoid.cartographicToCartesian(bottomRightCart) - center),
  };
  std::vector<glm::vec2> uvs{
      glm::vec2{0.0, 0.0},
      glm::vec2{0.0, 1.0},
      glm::vec2{1.0, 0.0},
      glm::vec2{1.0, 1.0}};
  std::vector<uint8_t> indices{0, 3, 2, 1};
  uint32_t positionsBufferSize =
      static_cast<uint32_t>(positions.size() * sizeof(glm::vec3));
  uint32_t uvsBufferSize =
      static_cast<uint32_t>(uvs.size() * sizeof(glm::vec2));
  uint32_t indicesBufferSize =
      static_cast<uint32_t>(indices.size() * sizeof(uint8_t));

  Model model;

  // create buffer
  model.buffers.emplace_back();
  Buffer& buffer = model.buffers.back();
  buffer.cesium.data.resize(
      positionsBufferSize + uvsBufferSize + indicesBufferSize);
  std::memcpy(buffer.cesium.data.data(), positions.data(), positionsBufferSize);
  std::memcpy(
      buffer.cesium.data.data() + positionsBufferSize,
      uvs.data(),
      uvsBufferSize);
  std::memcpy(
      buffer.cesium.data.data() + positionsBufferSize + uvsBufferSize,
      indices.data(),
      indicesBufferSize);

  // create position
  model.bufferViews.emplace_back();
  BufferView& positionBufferView = model.bufferViews.emplace_back();
  positionBufferView.buffer = static_cast<int>(model.buffers.size() - 1);
  positionBufferView.byteOffset = 0;
  positionBufferView.byteLength = positionsBufferSize;

  model.accessors.emplace_back();
  Accessor& positionAccessor = model.accessors.back();
  positionAccessor.bufferView =
      static_cast<int32_t>(model.bufferViews.size() - 1);
  positionAccessor.byteOffset = 0;
  positionAccessor.count = static_cast<int64_t>(positions.size());
  positionAccessor.componentType = Accessor::ComponentType::FLOAT;
  positionAccessor.type = Accessor::Type::VEC3;

  int32_t positionAccessorIdx =
      static_cast<int32_t>(model.accessors.size() - 1);

  // create uv
  model.bufferViews.emplace_back();
  BufferView& uvBufferView = model.bufferViews.emplace_back();
  uvBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  uvBufferView.byteOffset = positionsBufferSize;
  uvBufferView.byteLength = uvsBufferSize;

  model.accessors.emplace_back();
  Accessor& uvAccessor = model.accessors.back();
  uvAccessor.bufferView = static_cast<int32_t>(model.bufferViews.size() - 1);
  uvAccessor.byteOffset = 0;
  uvAccessor.count = static_cast<int64_t>(uvs.size());
  uvAccessor.componentType = Accessor::ComponentType::FLOAT;
  uvAccessor.type = Accessor::Type::VEC2;

  int32_t uvAccessorIdx = static_cast<int32_t>(model.accessors.size() - 1);

  // create indices
  model.bufferViews.emplace_back();
  BufferView& indicesBufferView = model.bufferViews.emplace_back();
  indicesBufferView.buffer = static_cast<int>(model.buffers.size() - 1);
  indicesBufferView.byteOffset = positionsBufferSize + uvsBufferSize;
  indicesBufferView.byteLength = indicesBufferSize;

  model.accessors.emplace_back();
  Accessor& indicesAccessor = model.accessors.back();
  indicesAccessor.bufferView = static_cast<int>(model.bufferViews.size() - 1);
  indicesAccessor.byteOffset = 0;
  indicesAccessor.count = static_cast<int64_t>(indices.size());
  indicesAccessor.componentType = Accessor::ComponentType::UNSIGNED_BYTE;
  indicesAccessor.type = Accessor::Type::SCALAR;

  int indicesAccessorIdx = static_cast<int>(model.accessors.size() - 1);

  model.meshes.emplace_back();
  Mesh& mesh = model.meshes.back();
  mesh.primitives.emplace_back();

  MeshPrimitive& primitive = mesh.primitives.back();
  primitive.mode = MeshPrimitive::Mode::TRIANGLE_FAN;
  primitive.attributes["_CESIUMOVERLAY_0"] = uvAccessorIdx;
  primitive.attributes["POSITION"] = positionAccessorIdx;
  primitive.indices = indicesAccessorIdx;

  // create node and update bounding volume
  model.nodes.emplace_back();
  Node& node = model.nodes[0];
  node.mesh = static_cast<int>(model.meshes.size() - 1);
  node.matrix = {
      1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      0.0,
      -1.0,
      0.0,
      0.0,
      1.0,
      0.0,
      0.0,
      center.x,
      center.z,
      -center.y,
      1.0};

  CesiumGeometry::UpsampledQuadtreeNode lowerLeft{
      CesiumGeometry::QuadtreeTileID(1, 0, 0)};

  Model upsampledModel = *RasterOverlayUtilities::upsampleGltfForRasterOverlays(
      model,
      lowerLeft,
      false);

  REQUIRE(upsampledModel.meshes.size() == 1);
  const Mesh& upsampledMesh = upsampledModel.meshes.back();

  REQUIRE(upsampledMesh.primitives.size() == 1);
  const MeshPrimitive& upsampledPrimitive = upsampledMesh.primitives.back();

  REQUIRE(upsampledPrimitive.indices >= 0);
  REQUIRE(
      upsampledPrimitive.attributes.find("POSITION") !=
      upsampledPrimitive.attributes.end());
  AccessorView<glm::vec3> upsampledPosition(
      upsampledModel,
      upsampledPrimitive.attributes.at("POSITION"));
  AccessorView<uint32_t> upsampledIndices(
      upsampledModel,
      upsampledPrimitive.indices);

  glm::vec3 p0 = upsampledPosition[0];
  CHECK(
      glm::epsilonEqual(
          p0,
          positions[0],
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p1 = upsampledPosition[1];
  CHECK(
      glm::epsilonEqual(
          p1,
          (positions[0] + positions[3]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p2 = upsampledPosition[2];
  CHECK(
      glm::epsilonEqual(
          p2,
          (positions[0] + positions[2]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p3 = upsampledPosition[3];
  CHECK(
      glm::epsilonEqual(
          p3,
          (positions[0] + positions[2]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p4 = upsampledPosition[4];
  CHECK(
      glm::epsilonEqual(
          p4,
          (upsampledPosition[3] + positions[1]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p5 = upsampledPosition[5];
  CHECK(
      glm::epsilonEqual(
          p5,
          (positions[0] + positions[1]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));

  glm::vec3 p6 = upsampledPosition[6];
  CHECK(
      glm::epsilonEqual(
          p6,
          (positions[0] + positions[2]) * 0.5f,
          glm::vec3(static_cast<float>(Math::Epsilon7))) == glm::bvec3(true));
}
