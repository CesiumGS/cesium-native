#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorUtility.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/ExtensionExtMeshGpuInstancing.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Node.h>

#include <doctest/doctest.h>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_uint2_sized.hpp>

#include <cstdint>
#include <cstring>
#include <variant>
#include <vector>

using namespace CesiumGltf;

TEST_CASE("Test CountFromAccessor") {
  Model model;
  std::vector<uint8_t> featureIds{1, 2, 3, 4};

  Buffer& buffer = model.buffers.emplace_back();
  buffer.cesium.data.resize(featureIds.size() * sizeof(uint8_t));
  std::memcpy(
      buffer.cesium.data.data(),
      featureIds.data(),
      buffer.cesium.data.size());
  buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

  BufferView& bufferView = model.bufferViews.emplace_back();
  bufferView.buffer = 0;
  bufferView.byteLength = buffer.byteLength;

  Accessor& accessor = model.accessors.emplace_back();
  accessor.bufferView = 0;
  accessor.componentType = Accessor::ComponentType::UNSIGNED_BYTE;
  accessor.type = Accessor::Type::SCALAR;
  accessor.count = bufferView.byteLength;

  SUBCASE("Handles invalid accessor") {
    // Wrong type
    TexCoordAccessorType texcoordAccessor =
        AccessorView<AccessorTypes::VEC2<uint8_t>>(model, accessor);
    REQUIRE(
        std::visit(StatusFromAccessor{}, texcoordAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, texcoordAccessor) == 0);

    // Wrong component type
    FeatureIdAccessorType featureIdAccessor =
        AccessorView<int16_t>(model, accessor);
    REQUIRE(
        std::visit(StatusFromAccessor{}, featureIdAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, featureIdAccessor) == 0);
  }

  SUBCASE("Retrieves from valid accessor") {
    FeatureIdAccessorType featureIdAccessor =
        AccessorView<uint8_t>(model, accessor);
    REQUIRE(
        std::visit(StatusFromAccessor{}, featureIdAccessor) ==
        AccessorViewStatus::Valid);
    int64_t count = std::visit(CountFromAccessor{}, featureIdAccessor);
    REQUIRE(count == static_cast<int64_t>(featureIds.size()));
  }
}

TEST_CASE("Test getPositionAccessorView") {
  Model model;
  std::vector<glm::vec3> positions{
      glm::vec3(0, 1, 2),
      glm::vec3(3, 4, 5),
      glm::vec3(6, 7, 8)};

  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(positions.size() * sizeof(glm::vec3));
    std::memcpy(
        buffer.cesium.data.data(),
        positions.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 0;
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 0;
    accessor.componentType = Accessor::ComponentType::FLOAT;
    accessor.type = Accessor::Type::VEC3;
    accessor.count = static_cast<int64_t>(positions.size());
  }

  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive primitive = mesh.primitives.emplace_back();
  primitive.attributes.insert({"POSITION", 0});

  SUBCASE("Handles invalid accessor type") {
    model.accessors[0].type = Accessor::Type::SCALAR;

    PositionAccessorType positionAccessor =
        getPositionAccessorView(model, primitive);
    REQUIRE(positionAccessor.status() != AccessorViewStatus::Valid);
    REQUIRE(positionAccessor.size() == 0);

    model.accessors[0].type = Accessor::Type::VEC3;
  }

  SUBCASE("Handles unsupported accessor component type") {
    model.accessors[0].componentType = Accessor::ComponentType::BYTE;

    PositionAccessorType positionAccessor =
        getPositionAccessorView(model, primitive);
    REQUIRE(positionAccessor.status() != AccessorViewStatus::Valid);
    REQUIRE(positionAccessor.size() == 0);

    model.accessors[0].componentType = Accessor::ComponentType::FLOAT;
  }

  SUBCASE("Creates from valid accessor") {
    PositionAccessorType positionAccessor =
        getPositionAccessorView(model, primitive);
    REQUIRE(positionAccessor.status() == AccessorViewStatus::Valid);
    REQUIRE(static_cast<size_t>(positionAccessor.size()) == positions.size());
  }
}

TEST_CASE("Test getNormalAccessorView") {
  Model model;
  std::vector<glm::vec3> normals{
      glm::vec3(0, 1, 0),
      glm::vec3(1, 0, 0),
      glm::vec3(0, 0, 1)};

  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(normals.size() * sizeof(glm::vec3));
    std::memcpy(
        buffer.cesium.data.data(),
        normals.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 0;
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 0;
    accessor.componentType = Accessor::ComponentType::FLOAT;
    accessor.type = Accessor::Type::VEC3;
    accessor.count = static_cast<int64_t>(normals.size());
  }

  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive primitive = mesh.primitives.emplace_back();
  primitive.attributes.insert({"NORMAL", 0});

  SUBCASE("Handles invalid accessor type") {
    model.accessors[0].type = Accessor::Type::SCALAR;

    NormalAccessorType normalAccessor = getNormalAccessorView(model, primitive);
    REQUIRE(normalAccessor.status() != AccessorViewStatus::Valid);
    REQUIRE(normalAccessor.size() == 0);

    model.accessors[0].type = Accessor::Type::VEC3;
  }

  SUBCASE("Handles unsupported accessor component type") {
    model.accessors[0].componentType = Accessor::ComponentType::BYTE;

    NormalAccessorType normalAccessor = getNormalAccessorView(model, primitive);
    REQUIRE(normalAccessor.status() != AccessorViewStatus::Valid);
    REQUIRE(normalAccessor.size() == 0);

    model.accessors[0].componentType = Accessor::ComponentType::FLOAT;
  }

  SUBCASE("Creates from valid accessor") {
    NormalAccessorType normalAccessor = getNormalAccessorView(model, primitive);
    REQUIRE(normalAccessor.status() == AccessorViewStatus::Valid);
    REQUIRE(static_cast<size_t>(normalAccessor.size()) == normals.size());
  }
}

TEST_CASE("Test getFeatureIdAccessorView") {
  Model model;
  std::vector<uint8_t> featureIds0{1, 2, 3, 4};

  // First _FEATURE_ID set
  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(featureIds0.size() * sizeof(uint8_t));
    std::memcpy(
        buffer.cesium.data.data(),
        featureIds0.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 0;
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 0;
    accessor.componentType = Accessor::ComponentType::UNSIGNED_BYTE;
    accessor.type = Accessor::Type::SCALAR;
    accessor.count = bufferView.byteLength;
  }

  std::vector<uint16_t> featureIds1{5, 6, 7, 8};

  // Second _FEATURE_ID set
  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(featureIds1.size() * sizeof(uint16_t));
    std::memcpy(
        buffer.cesium.data.data(),
        featureIds1.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 1;
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 1;
    accessor.componentType = Accessor::ComponentType::UNSIGNED_SHORT;
    accessor.type = Accessor::Type::SCALAR;
    accessor.count =
        bufferView.byteLength / static_cast<int64_t>(sizeof(uint16_t));
  }

  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive primitive = mesh.primitives.emplace_back();

  primitive.attributes.insert({"_FEATURE_ID_0", 0});
  primitive.attributes.insert({"_FEATURE_ID_1", 1});

  SUBCASE("Handles invalid feature ID set index") {
    FeatureIdAccessorType featureIDAccessor =
        getFeatureIdAccessorView(model, primitive, 2);
    REQUIRE(
        std::visit(StatusFromAccessor{}, featureIDAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, featureIDAccessor) == 0);
  }

  SUBCASE("Handles invalid accessor type") {
    model.accessors[0].type = Accessor::Type::VEC2;

    FeatureIdAccessorType featureIDAccessor =
        getFeatureIdAccessorView(model, primitive, 0);
    REQUIRE(
        std::visit(StatusFromAccessor{}, featureIDAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, featureIDAccessor) == 0);

    model.accessors[0].type = Accessor::Type::SCALAR;
  }

  SUBCASE("Handles invalid normalized accessor") {
    model.accessors[1].normalized = true;

    FeatureIdAccessorType featureIDAccessor =
        getFeatureIdAccessorView(model, primitive, 1);
    REQUIRE(
        std::visit(StatusFromAccessor{}, featureIDAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, featureIDAccessor) == 0);

    model.accessors[1].normalized = false;
  }

  SUBCASE("Creates from valid feature ID sets") {
    FeatureIdAccessorType featureIDAccessor =
        getFeatureIdAccessorView(model, primitive, 0);
    REQUIRE(
        std::visit(StatusFromAccessor{}, featureIDAccessor) ==
        AccessorViewStatus::Valid);
    REQUIRE(
        std::visit(CountFromAccessor{}, featureIDAccessor) ==
        static_cast<int64_t>(featureIds0.size()));

    featureIDAccessor = getFeatureIdAccessorView(model, primitive, 1);
    REQUIRE(
        std::visit(StatusFromAccessor{}, featureIDAccessor) ==
        AccessorViewStatus::Valid);
    REQUIRE(
        std::visit(CountFromAccessor{}, featureIDAccessor) ==
        static_cast<int64_t>(featureIds1.size()));
  }
}

TEST_CASE("Test getFeatureIdAccessorView for instances") {
  Model model;
  std::vector<int8_t> featureIds{1, 2, 3, 4};

  Buffer& buffer = model.buffers.emplace_back();
  buffer.cesium.data.resize(featureIds.size() * sizeof(int8_t));
  std::memcpy(
      buffer.cesium.data.data(),
      featureIds.data(),
      buffer.cesium.data.size());
  buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

  BufferView& bufferView = model.bufferViews.emplace_back();
  bufferView.buffer = 0;
  bufferView.byteLength = buffer.byteLength;

  Accessor& accessor = model.accessors.emplace_back();
  accessor.bufferView = 0;
  accessor.componentType = Accessor::ComponentType::BYTE;
  accessor.type = Accessor::Type::SCALAR;
  accessor.count = bufferView.byteLength;

  Node& node = model.nodes.emplace_back();
  ExtensionExtMeshGpuInstancing& instancingExtension =
      node.addExtension<ExtensionExtMeshGpuInstancing>();
  instancingExtension.attributes["_FEATURE_ID_0"] = 0;

  SUBCASE("Handles missing extension") {
    Node temporaryNode;

    FeatureIdAccessorType featureIDAccessor =
        getFeatureIdAccessorView(model, temporaryNode, 0);
    REQUIRE(
        std::visit(StatusFromAccessor{}, featureIDAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, featureIDAccessor) == 0);

    model.accessors[0].type = Accessor::Type::SCALAR;
  }

  SUBCASE("Handles invalid feature ID set index") {
    FeatureIdAccessorType featureIDAccessor =
        getFeatureIdAccessorView(model, node, 2);
    REQUIRE(
        std::visit(StatusFromAccessor{}, featureIDAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, featureIDAccessor) == 0);
  }

  SUBCASE("Handles invalid accessor type") {
    model.accessors[0].type = Accessor::Type::VEC2;

    FeatureIdAccessorType featureIDAccessor =
        getFeatureIdAccessorView(model, node, 0);
    REQUIRE(
        std::visit(StatusFromAccessor{}, featureIDAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, featureIDAccessor) == 0);

    model.accessors[0].type = Accessor::Type::SCALAR;
  }

  SUBCASE("Handles invalid normalized accessor") {
    model.accessors[0].normalized = true;

    FeatureIdAccessorType featureIDAccessor =
        getFeatureIdAccessorView(model, node, 1);
    REQUIRE(
        std::visit(StatusFromAccessor{}, featureIDAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, featureIDAccessor) == 0);

    model.accessors[0].normalized = false;
  }

  SUBCASE("Retrieves from valid accessor") {
    FeatureIdAccessorType featureIdAccessor =
        getFeatureIdAccessorView(model, node, 0);
    for (size_t i = 0; i < featureIds.size(); i++) {
      int64_t featureID = std::visit(
          FeatureIdFromAccessor{static_cast<int64_t>(i)},
          featureIdAccessor);
      REQUIRE(featureID == featureIds[i]);
    }
  }
}

TEST_CASE("FeatureIdFromAccessor") {
  Model model;
  std::vector<int8_t> featureIds{1, 2, 3, 4};

  Buffer& buffer = model.buffers.emplace_back();
  buffer.cesium.data.resize(featureIds.size() * sizeof(int8_t));
  std::memcpy(
      buffer.cesium.data.data(),
      featureIds.data(),
      buffer.cesium.data.size());
  buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

  BufferView& bufferView = model.bufferViews.emplace_back();
  bufferView.buffer = 0;
  bufferView.byteLength = buffer.byteLength;

  Accessor& accessor = model.accessors.emplace_back();
  accessor.bufferView = 0;
  accessor.componentType = Accessor::ComponentType::BYTE;
  accessor.type = Accessor::Type::SCALAR;
  accessor.count = bufferView.byteLength;

  SUBCASE("Handles invalid accessor") {
    // Wrong component type
    FeatureIdAccessorType featureIdAccessor =
        AccessorView<int16_t>(model, accessor);
    REQUIRE(std::visit(FeatureIdFromAccessor{0}, featureIdAccessor) == -1);
  }

  SUBCASE("Retrieves from valid accessor") {
    FeatureIdAccessorType featureIdAccessor =
        AccessorView<int8_t>(model, accessor);
    for (size_t i = 0; i < featureIds.size(); i++) {
      int64_t featureID = std::visit(
          FeatureIdFromAccessor{static_cast<int64_t>(i)},
          featureIdAccessor);
      REQUIRE(featureID == featureIds[i]);
    }
  }
}

TEST_CASE("Test getIndexAccessorView") {
  Model model;
  std::vector<uint8_t> indices{0, 1, 2, 0, 2, 3};

  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(indices.size() * sizeof(uint8_t));
    std::memcpy(
        buffer.cesium.data.data(),
        indices.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 0;
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 0;
    accessor.componentType = Accessor::ComponentType::UNSIGNED_BYTE;
    accessor.type = Accessor::Type::SCALAR;
    accessor.count = bufferView.byteLength;
  }

  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive primitive = mesh.primitives.emplace_back();
  primitive.indices = 0;

  SUBCASE("Handles invalid accessor type") {
    model.accessors[0].type = Accessor::Type::VEC2;

    IndexAccessorType indexAccessor = getIndexAccessorView(model, primitive);
    REQUIRE(
        std::visit(StatusFromAccessor{}, indexAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, indexAccessor) == 0);

    model.accessors[0].type = Accessor::Type::SCALAR;
  }

  SUBCASE("Handles unsupported accessor component type") {
    model.accessors[0].componentType = Accessor::ComponentType::BYTE;

    IndexAccessorType indexAccessor = getIndexAccessorView(model, primitive);
    REQUIRE(
        std::visit(StatusFromAccessor{}, indexAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, indexAccessor) == 0);

    model.accessors[0].componentType = Accessor::ComponentType::UNSIGNED_BYTE;
  }

  SUBCASE("Handles invalid normalized accessor") {
    model.accessors[0].normalized = true;

    IndexAccessorType indexAccessor = getIndexAccessorView(model, primitive);
    REQUIRE(
        std::visit(StatusFromAccessor{}, indexAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, indexAccessor) == 0);

    model.accessors[0].normalized = false;
  }

  SUBCASE("Creates from valid accessor") {
    IndexAccessorType indexAccessor = getIndexAccessorView(model, primitive);
    REQUIRE(
        std::visit(StatusFromAccessor{}, indexAccessor) ==
        AccessorViewStatus::Valid);
    REQUIRE(
        std::visit(CountFromAccessor{}, indexAccessor) ==
        static_cast<int64_t>(indices.size()));
  }

  SUBCASE("Creates from nonexistent accessor") {
    primitive.indices = -1;

    IndexAccessorType indexAccessor = getIndexAccessorView(model, primitive);
    REQUIRE(std::get_if<std::monostate>(&indexAccessor));
  }
}

TEST_CASE("Test IndicesForFaceFromAccessor") {
  Model model;
  int64_t vertexCount = 9;

  // Triangle mode indices
  std::vector<uint32_t>
      triangleIndices{0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7, 6, 7, 8};
  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(triangleIndices.size() * sizeof(uint32_t));
    std::memcpy(
        buffer.cesium.data.data(),
        triangleIndices.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = static_cast<int32_t>(model.bufferViews.size() - 1);
    accessor.componentType = Accessor::ComponentType::UNSIGNED_INT;
    accessor.type = Accessor::Type::SCALAR;
    accessor.count =
        bufferView.byteLength / static_cast<int64_t>(sizeof(uint32_t));
  }

  // Triangle strip and fan indices
  std::vector<uint32_t> specialIndices{1, 2, 3, 4, 5, 6, 7, 8, 0};
  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(specialIndices.size() * sizeof(uint32_t));
    std::memcpy(
        buffer.cesium.data.data(),
        specialIndices.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = static_cast<int32_t>(model.bufferViews.size() - 1);
    accessor.componentType = Accessor::ComponentType::UNSIGNED_INT;
    accessor.type = Accessor::Type::SCALAR;
    accessor.count =
        bufferView.byteLength / static_cast<int64_t>(sizeof(uint32_t));
  }

  SUBCASE("Handles invalid accessor") {
    REQUIRE(model.accessors.size() > 0);
    // Wrong component type
    IndexAccessorType indexAccessor =
        AccessorView<uint8_t>(model, model.accessors[0]);
    auto indicesForFace = std::visit(
        IndicesForFaceFromAccessor{
            0,
            vertexCount,
            MeshPrimitive::Mode::TRIANGLES},
        indexAccessor);
    for (int64_t index : indicesForFace) {
      REQUIRE(index == -1);
    }
  }

  SUBCASE("Handles invalid face index") {
    REQUIRE(model.accessors.size() > 0);
    IndexAccessorType indexAccessor =
        AccessorView<uint32_t>(model, model.accessors[0]);
    auto indicesForFace = std::visit(
        IndicesForFaceFromAccessor{
            -1,
            vertexCount,
            MeshPrimitive::Mode::TRIANGLES},
        indexAccessor);
    for (int64_t index : indicesForFace) {
      REQUIRE(index == -1);
    }

    indicesForFace = std::visit(
        IndicesForFaceFromAccessor{
            10,
            vertexCount,
            MeshPrimitive::Mode::TRIANGLES},
        indexAccessor);
    for (int64_t index : indicesForFace) {
      REQUIRE(index == -1);
    }
  }

  SUBCASE("Handles invalid primitive modes") {
    REQUIRE(model.accessors.size() > 0);
    IndexAccessorType indexAccessor =
        AccessorView<uint32_t>(model, model.accessors[0]);
    auto indicesForFace = std::visit(
        IndicesForFaceFromAccessor{
            -1,
            vertexCount,
            MeshPrimitive::Mode::POINTS},
        indexAccessor);
    for (int64_t index : indicesForFace) {
      REQUIRE(index == -1);
    }

    indicesForFace = std::visit(
        IndicesForFaceFromAccessor{10, vertexCount, MeshPrimitive::Mode::LINES},
        indexAccessor);
    for (int64_t index : indicesForFace) {
      REQUIRE(index == -1);
    }

    indicesForFace = std::visit(
        IndicesForFaceFromAccessor{
            10,
            vertexCount,
            MeshPrimitive::Mode::LINE_LOOP},
        indexAccessor);
    for (int64_t index : indicesForFace) {
      REQUIRE(index == -1);
    }
  }

  SUBCASE("Retrieves from valid accessor and face index; triangles mode") {
    REQUIRE(model.accessors.size() > 0);
    IndexAccessorType indexAccessor =
        AccessorView<uint32_t>(model, model.accessors[0]);
    const size_t numFaces =
        static_cast<size_t>(std::visit(CountFromAccessor{}, indexAccessor) / 3);

    for (size_t i = 0; i < numFaces; i++) {
      auto indicesForFace = std::visit(
          IndicesForFaceFromAccessor{
              static_cast<int64_t>(i),
              vertexCount,
              MeshPrimitive::Mode::TRIANGLES},
          indexAccessor);

      for (size_t j = 0; j < indicesForFace.size(); j++) {
        int64_t expected = static_cast<int64_t>(triangleIndices[i * 3 + j]);
        REQUIRE(indicesForFace[j] == expected);
      }
    }
  }

  SUBCASE("Retrieves from valid accessor and face index; triangle strip mode") {
    REQUIRE(model.accessors.size() > 1);
    IndexAccessorType indexAccessor =
        AccessorView<uint32_t>(model, model.accessors[1]);
    const size_t numFaces =
        static_cast<size_t>(std::visit(CountFromAccessor{}, indexAccessor) - 2);
    for (size_t i = 0; i < numFaces; i++) {
      auto indicesForFace = std::visit(
          IndicesForFaceFromAccessor{
              static_cast<int64_t>(i),
              vertexCount,
              MeshPrimitive::Mode::TRIANGLE_STRIP},
          indexAccessor);

      for (size_t j = 0; j < indicesForFace.size(); j++) {
        int64_t expected = static_cast<int64_t>(specialIndices[i + j]);
        REQUIRE(indicesForFace[j] == expected);
      }
    }
  }

  SUBCASE("Retrieves from valid accessor and face index; triangle fan mode") {
    REQUIRE(model.accessors.size() > 1);
    IndexAccessorType indexAccessor =
        AccessorView<uint32_t>(model, model.accessors[1]);
    const size_t numFaces =
        static_cast<size_t>(std::visit(CountFromAccessor{}, indexAccessor) - 2);
    for (size_t i = 0; i < numFaces; i++) {
      auto indicesForFace = std::visit(
          IndicesForFaceFromAccessor{
              static_cast<int64_t>(i),
              vertexCount,
              MeshPrimitive::Mode::TRIANGLE_FAN},
          indexAccessor);

      REQUIRE(indicesForFace[0] == specialIndices[0]);
      REQUIRE(indicesForFace[1] == specialIndices[i + 1]);
      REQUIRE(indicesForFace[2] == specialIndices[i + 2]);
    }
  }

  SUBCASE("Handles invalid face index for nonexistent accessor") {
    IndexAccessorType indexAccessor;
    auto indicesForFace = std::visit(
        IndicesForFaceFromAccessor{
            -1,
            vertexCount,
            MeshPrimitive::Mode::TRIANGLES},
        indexAccessor);
    for (int64_t index : indicesForFace) {
      REQUIRE(index == -1);
    }

    indicesForFace = std::visit(
        IndicesForFaceFromAccessor{
            10,
            vertexCount,
            MeshPrimitive::Mode::TRIANGLES},
        indexAccessor);
    for (int64_t index : indicesForFace) {
      REQUIRE(index == -1);
    }
  }

  SUBCASE("Retrieves from valid face index for nonexistent accessor; triangles "
          "mode") {
    IndexAccessorType indexAccessor;
    const int64_t numFaces = vertexCount / 3;

    for (int64_t i = 0; i < numFaces; i++) {
      auto indicesForFace = std::visit(
          IndicesForFaceFromAccessor{
              i,
              vertexCount,
              MeshPrimitive::Mode::TRIANGLES},
          indexAccessor);

      for (size_t j = 0; j < indicesForFace.size(); j++) {
        int64_t expected = i * 3 + static_cast<int64_t>(j);
        REQUIRE(indicesForFace[j] == expected);
      }
    }
  }

  SUBCASE("Retrieves from valid face index for nonexistent accessor; triangle "
          "strip mode") {
    IndexAccessorType indexAccessor;
    const int64_t numFaces = vertexCount - 2;

    for (int64_t i = 0; i < numFaces; i++) {
      auto indicesForFace = std::visit(
          IndicesForFaceFromAccessor{
              i,
              vertexCount,
              MeshPrimitive::Mode::TRIANGLE_STRIP},
          indexAccessor);

      for (size_t j = 0; j < indicesForFace.size(); j++) {
        int64_t expected = i + static_cast<int64_t>(j);
        REQUIRE(indicesForFace[j] == expected);
      }
    }
  }

  SUBCASE("Retrieves from valid face index for nonexistent accessor; triangle "
          "fan mode") {
    IndexAccessorType indexAccessor;
    const int64_t numFaces = vertexCount - 2;

    for (int64_t i = 0; i < numFaces; i++) {
      auto indicesForFace = std::visit(
          IndicesForFaceFromAccessor{
              i,
              vertexCount,
              MeshPrimitive::Mode::TRIANGLE_FAN},
          indexAccessor);

      REQUIRE(indicesForFace[0] == 0);
      REQUIRE(indicesForFace[1] == i + 1);
      REQUIRE(indicesForFace[2] == i + 2);
    }
  }
}

TEST_CASE("Test IndexFromAccessor") {
  Model model;

  std::vector<uint32_t> indices{0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7, 6, 7, 8};
  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(indices.size() * sizeof(uint32_t));
    std::memcpy(
        buffer.cesium.data.data(),
        indices.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = static_cast<int32_t>(model.bufferViews.size() - 1);
    accessor.componentType = Accessor::ComponentType::UNSIGNED_INT;
    accessor.type = Accessor::Type::SCALAR;
    accessor.count =
        bufferView.byteLength / static_cast<int64_t>(sizeof(uint32_t));
  }

  SUBCASE("Handles invalid accessor") {
    REQUIRE(model.accessors.size() > 0);
    // Wrong component type
    IndexAccessorType indexAccessor =
        AccessorView<uint8_t>(model, model.accessors[0]);
    auto index = std::visit(IndexFromAccessor{0}, indexAccessor);
    REQUIRE(index == -1);
  }

  SUBCASE("Handles invalid index") {
    REQUIRE(model.accessors.size() > 0);
    IndexAccessorType indexAccessor =
        AccessorView<uint32_t>(model, model.accessors[0]);
    auto index = std::visit(IndexFromAccessor{-1}, indexAccessor);
    REQUIRE(index == -1);

    index = std::visit(
        IndexFromAccessor{static_cast<int64_t>(indices.size())},
        indexAccessor);
    REQUIRE(index == -1);
  }

  SUBCASE("Retrieves from valid accessor and index") {
    REQUIRE(model.accessors.size() > 0);
    IndexAccessorType indexAccessor =
        AccessorView<uint32_t>(model, model.accessors[0]);

    for (size_t i = 0; i < indices.size(); i++) {
      auto index =
          std::visit(IndexFromAccessor{static_cast<int64_t>(i)}, indexAccessor);
      REQUIRE(index == indices[i]);
    }
  }
}

TEST_CASE("Test getTexCoordAccessorView") {
  Model model;
  std::vector<glm::vec2> texCoords0{
      glm::vec2(0, 0),
      glm::vec2(1, 0),
      glm::vec2(0, 1),
      glm::vec2(1, 1)};

  // First TEXCOORD set
  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(texCoords0.size() * sizeof(glm::vec2));
    std::memcpy(
        buffer.cesium.data.data(),
        texCoords0.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 0;
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 0;
    accessor.componentType = Accessor::ComponentType::FLOAT;
    accessor.type = Accessor::Type::VEC2;
    accessor.count =
        bufferView.byteLength / static_cast<int64_t>(sizeof(glm::vec2));
  }

  std::vector<glm::u8vec2> texCoords1{
      glm::u8vec2(0, 0),
      glm::u8vec2(0, 255),
      glm::u8vec2(255, 255),
      glm::u8vec2(255, 0)};

  // Second TEXCOORD set
  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(texCoords1.size() * sizeof(glm::u8vec2));
    std::memcpy(
        buffer.cesium.data.data(),
        texCoords1.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 1;
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 1;
    accessor.componentType = Accessor::ComponentType::UNSIGNED_BYTE;
    accessor.type = Accessor::Type::VEC2;
    accessor.normalized = true;
    accessor.count =
        bufferView.byteLength / static_cast<int64_t>(sizeof(glm::u8vec2));
  }

  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive primitive = mesh.primitives.emplace_back();

  primitive.attributes.insert({"TEXCOORD_0", 0});
  primitive.attributes.insert({"TEXCOORD_1", 1});

  SUBCASE("Handles invalid texture coordinate set index") {
    TexCoordAccessorType texCoordAccessor =
        getTexCoordAccessorView(model, primitive, 2);
    REQUIRE(
        std::visit(StatusFromAccessor{}, texCoordAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, texCoordAccessor) == 0);
  }

  SUBCASE("Handles invalid accessor type") {
    model.accessors[0].type = Accessor::Type::SCALAR;

    TexCoordAccessorType texCoordAccessor =
        getTexCoordAccessorView(model, primitive, 0);
    REQUIRE(
        std::visit(StatusFromAccessor{}, texCoordAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, texCoordAccessor) == 0);

    model.accessors[0].type = Accessor::Type::VEC2;
  }

  SUBCASE("Handles unsupported accessor component type") {
    model.accessors[0].componentType = Accessor::ComponentType::BYTE;

    TexCoordAccessorType texCoordAccessor =
        getTexCoordAccessorView(model, primitive, 0);
    REQUIRE(
        std::visit(StatusFromAccessor{}, texCoordAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, texCoordAccessor) == 0);

    model.accessors[0].componentType = Accessor::ComponentType::FLOAT;
  }

  SUBCASE("Handles invalid un-normalized texcoord") {
    model.accessors[1].normalized = false;

    TexCoordAccessorType texCoordAccessor =
        getTexCoordAccessorView(model, primitive, 2);
    REQUIRE(
        std::visit(StatusFromAccessor{}, texCoordAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, texCoordAccessor) == 0);

    model.accessors[1].normalized = true;
  }

  SUBCASE("Creates from valid texture coordinate sets") {
    TexCoordAccessorType texCoordAccessor =
        getTexCoordAccessorView(model, primitive, 0);
    REQUIRE(
        std::visit(StatusFromAccessor{}, texCoordAccessor) ==
        AccessorViewStatus::Valid);
    REQUIRE(
        std::visit(CountFromAccessor{}, texCoordAccessor) ==
        static_cast<int64_t>(texCoords0.size()));

    texCoordAccessor = getTexCoordAccessorView(model, primitive, 1);
    REQUIRE(
        std::visit(StatusFromAccessor{}, texCoordAccessor) ==
        AccessorViewStatus::Valid);
    REQUIRE(
        std::visit(CountFromAccessor{}, texCoordAccessor) ==
        static_cast<int64_t>(texCoords1.size()));
  }
}

TEST_CASE("Test TexCoordFromAccessor") {
  Model model;
  std::vector<glm::vec2> texCoords0{
      glm::vec2(0, 0),
      glm::vec2(1, 0),
      glm::vec2(0, 1),
      glm::vec2(1, 1)};

  // First TEXCOORD set
  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(texCoords0.size() * sizeof(glm::vec2));
    std::memcpy(
        buffer.cesium.data.data(),
        texCoords0.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 0;
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 0;
    accessor.componentType = Accessor::ComponentType::FLOAT;
    accessor.type = Accessor::Type::VEC2;
    accessor.count =
        bufferView.byteLength / static_cast<int64_t>(sizeof(glm::vec2));
  }

  std::vector<glm::u8vec2> texCoords1{
      glm::u8vec2(0, 0),
      glm::u8vec2(0, 255),
      glm::u8vec2(255, 255),
      glm::u8vec2(255, 0)};

  // Second TEXCOORD set
  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(texCoords1.size() * sizeof(glm::u8vec2));
    std::memcpy(
        buffer.cesium.data.data(),
        texCoords1.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 1;
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 1;
    accessor.componentType = Accessor::ComponentType::UNSIGNED_BYTE;
    accessor.type = Accessor::Type::VEC2;
    accessor.normalized = true;
    accessor.count =
        bufferView.byteLength / static_cast<int64_t>(sizeof(glm::u8vec2));
  }

  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive primitive = mesh.primitives.emplace_back();

  primitive.attributes.insert({"TEXCOORD_0", 0});
  primitive.attributes.insert({"TEXCOORD_1", 1});

  SUBCASE("Handles invalid accessor") {
    TexCoordAccessorType texCoordAccessor =
        getTexCoordAccessorView(model, primitive, 2);
    REQUIRE(!std::visit(TexCoordFromAccessor{0}, texCoordAccessor));
  }

  SUBCASE("Handles invalid index") {
    TexCoordAccessorType texCoordAccessor =
        getTexCoordAccessorView(model, primitive, 0);
    REQUIRE(!std::visit(TexCoordFromAccessor{-1}, texCoordAccessor));
    REQUIRE(!std::visit(TexCoordFromAccessor{10}, texCoordAccessor));
  }

  SUBCASE("Retrieves from valid accessor and index") {
    TexCoordAccessorType texCoordAccessor =
        getTexCoordAccessorView(model, primitive, 0);
    for (size_t i = 0; i < texCoords0.size(); i++) {
      auto maybeTexCoord = std::visit(
          TexCoordFromAccessor{static_cast<int64_t>(i)},
          texCoordAccessor);
      REQUIRE(maybeTexCoord);

      auto expected = glm::dvec2(texCoords0[i][0], texCoords0[i][1]);
      REQUIRE(*maybeTexCoord == expected);
    }
  }
  SUBCASE("Retrieves from valid normalized accessor and index") {
    TexCoordAccessorType texCoordAccessor =
        getTexCoordAccessorView(model, primitive, 1);
    for (size_t i = 0; i < texCoords1.size(); i++) {
      auto maybeTexCoord = std::visit(
          TexCoordFromAccessor{static_cast<int64_t>(i)},
          texCoordAccessor);
      REQUIRE(maybeTexCoord);
      auto expected = glm::dvec2(texCoords1[i][0], texCoords1[i][1]);
      expected /= 255;

      REQUIRE(*maybeTexCoord == expected);
    }
  }
}
