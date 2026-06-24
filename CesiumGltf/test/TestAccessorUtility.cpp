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
#include <glm/ext/quaternion_double.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_uint2_sized.hpp>
#include <glm/fwd.hpp>

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
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  primitive.attributes.insert({"POSITION", 0});

  SUBCASE("Handles invalid accessor type") {
    model.accessors[0].type = Accessor::Type::SCALAR;

    PositionAccessorType positionAccessor =
        getPositionAccessorView(model, primitive);
    REQUIRE(
        std::visit(StatusFromAccessor{}, positionAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, positionAccessor) == 0);

    model.accessors[0].type = Accessor::Type::VEC3;
  }

  SUBCASE("Handles unsupported accessor component type") {
    model.accessors[0].componentType = Accessor::ComponentType::DOUBLE;

    PositionAccessorType positionAccessor =
        getPositionAccessorView(model, primitive);
    REQUIRE(
        std::visit(StatusFromAccessor{}, positionAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, positionAccessor) == 0);

    model.accessors[0].componentType = Accessor::ComponentType::FLOAT;
  }

  SUBCASE("Creates from valid accessor") {
    PositionAccessorType positionAccessor =
        getPositionAccessorView(model, primitive);
    REQUIRE(
        std::visit(StatusFromAccessor{}, positionAccessor) ==
        AccessorViewStatus::Valid);
    REQUIRE(
        static_cast<size_t>(
            std::visit(CountFromAccessor{}, positionAccessor)) ==
        positions.size());
  }
}

TEST_CASE("Test PositionFromAccessor") {
  Model model;
  std::vector<glm::vec3> positions0{glm::vec3(0, 1, 2)};

  // Float POSITION
  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(positions0.size() * sizeof(glm::vec3));
    std::memcpy(
        buffer.cesium.data.data(),
        positions0.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 0;
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 0;
    accessor.componentType = Accessor::ComponentType::FLOAT;
    accessor.type = Accessor::Type::VEC3;
    accessor.count = static_cast<int64_t>(positions0.size());

    Mesh& mesh = model.meshes.emplace_back();
    MeshPrimitive& primitive = mesh.primitives.emplace_back();
    primitive.attributes.insert({"POSITION", 0});
  }

  std::vector<glm::u8vec3> positions1{glm::u8vec3(0, 127, 255)};

  // Unsigned normalized POSITION
  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(positions1.size() * sizeof(glm::u8vec3));
    std::memcpy(
        buffer.cesium.data.data(),
        positions1.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 1;
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 1;
    accessor.componentType = Accessor::ComponentType::UNSIGNED_BYTE;
    accessor.type = Accessor::Type::VEC3;
    accessor.count = static_cast<int64_t>(positions1.size());
    accessor.normalized = true;

    Mesh& mesh = model.meshes.emplace_back();
    MeshPrimitive& primitive = mesh.primitives.emplace_back();
    primitive.attributes.insert({"POSITION", 1});
  }

  std::vector<glm::i8vec3> positions2{
      glm::i8vec3(-128, 0, 127),
      glm::i8vec3(-127, 0, 127),
  };

  // Signed normalized POSITION
  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(positions2.size() * sizeof(glm::i8vec3));
    std::memcpy(
        buffer.cesium.data.data(),
        positions2.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 2;
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 2;
    accessor.componentType = Accessor::ComponentType::BYTE;
    accessor.type = Accessor::Type::VEC3;
    accessor.count = static_cast<int64_t>(positions2.size());
    accessor.normalized = true;

    Mesh& mesh = model.meshes.emplace_back();
    MeshPrimitive& primitive = mesh.primitives.emplace_back();
    primitive.attributes.insert({"POSITION", 2});
  }

  SUBCASE("Handles invalid accessor") {
    PositionAccessorType positionAccessor;
    REQUIRE(!std::visit(PositionFromAccessor{0}, positionAccessor));
  }

  SUBCASE("Handles invalid index") {
    PositionAccessorType positionAccessor =
        getPositionAccessorView(model, model.meshes[0].primitives[0]);
    REQUIRE(!std::visit(PositionFromAccessor{-1}, positionAccessor));
    REQUIRE(!std::visit(PositionFromAccessor{10}, positionAccessor));
  }

  SUBCASE("Retrieves from valid accessor and index") {
    PositionAccessorType positionAccessor =
        getPositionAccessorView(model, model.meshes[0].primitives[0]);

    for (size_t i = 0; i < positions0.size(); i++) {
      auto maybePosition = std::visit(
          PositionFromAccessor{static_cast<int64_t>(i)},
          positionAccessor);
      REQUIRE(maybePosition);
      auto expected =
          glm::dvec3(positions0[i][0], positions0[i][1], positions0[i][2]);
      REQUIRE(*maybePosition == expected);
    }
  }

  SUBCASE("Retrieves from valid unsigned normalized accessor and index") {
    PositionAccessorType positionAccessor =
        getPositionAccessorView(model, model.meshes[1].primitives[0]);
    for (size_t i = 0; i < positions1.size(); i++) {
      auto maybePosition = std::visit(
          PositionFromAccessor{static_cast<int64_t>(i)},
          positionAccessor);
      REQUIRE(maybePosition);
      auto expected =
          glm::dvec3(positions1[i][0], positions1[i][1], positions1[i][2]);
      expected /= 255.0;
      REQUIRE(*maybePosition == expected);
    }
  }

  SUBCASE("Retrieves from valid signed normalized accessor and index") {
    PositionAccessorType positionAccessor =
        getPositionAccessorView(model, model.meshes[2].primitives[0]);
    for (size_t i = 0; i < positions2.size(); i++) {
      auto maybePosition = std::visit(
          PositionFromAccessor{static_cast<int64_t>(i)},
          positionAccessor);
      REQUIRE(maybePosition);
      auto expected =
          glm::dvec3(positions2[i][0], positions2[i][1], positions2[i][2]);
      expected = glm::max(expected / 127.0, -1.0);
      REQUIRE(*maybePosition == expected);
    }
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
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  primitive.attributes.insert({"NORMAL", 0});

  SUBCASE("Handles invalid accessor type") {
    model.accessors[0].type = Accessor::Type::SCALAR;

    NormalAccessorType normalAccessor = getNormalAccessorView(model, primitive);
    REQUIRE(
        std::visit(StatusFromAccessor{}, normalAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, normalAccessor) == 0);

    model.accessors[0].type = Accessor::Type::VEC3;
  }

  SUBCASE("Handles unsupported accessor component type") {
    model.accessors[0].componentType = Accessor::ComponentType::UNSIGNED_BYTE;

    NormalAccessorType normalAccessor = getNormalAccessorView(model, primitive);
    REQUIRE(
        std::visit(StatusFromAccessor{}, normalAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, normalAccessor) == 0);

    model.accessors[0].componentType = Accessor::ComponentType::FLOAT;
  }

  SUBCASE("Handles invalid un-normalized normal") {
    model.accessors[0].componentType = Accessor::ComponentType::BYTE;

    NormalAccessorType normalAccessor = getNormalAccessorView(model, primitive);
    REQUIRE(
        std::visit(StatusFromAccessor{}, normalAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, normalAccessor) == 0);

    model.accessors[0].componentType = Accessor::ComponentType::FLOAT;
  }

  SUBCASE("Creates from valid accessor") {
    NormalAccessorType normalAccessor = getNormalAccessorView(model, primitive);
    REQUIRE(
        std::visit(StatusFromAccessor{}, normalAccessor) ==
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, normalAccessor) == normals.size());
  }
}

TEST_CASE("Test NormalFromAccessor") {
  Model model;
  std::vector<glm::vec3> normals0{
      glm::vec3(0, 1, 0),
      glm::vec3(-1, 0, 0),
      glm::vec3(0, 0, 1)};

  // Float NORMAL
  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(normals0.size() * sizeof(glm::vec3));
    std::memcpy(
        buffer.cesium.data.data(),
        normals0.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 0;
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 0;
    accessor.componentType = Accessor::ComponentType::FLOAT;
    accessor.type = Accessor::Type::VEC3;
    accessor.count = static_cast<int64_t>(normals0.size());

    Mesh& mesh = model.meshes.emplace_back();
    MeshPrimitive& primitive = mesh.primitives.emplace_back();
    primitive.attributes.insert({"NORMAL", 0});
  }

  std::vector<glm::i8vec3> normals1{
      glm::i8vec3(0, 127, 0),
      glm::i8vec3(-128, 0, 0),
      glm::i8vec3(0, 0, 127)};

  // Signed normalized NORMAL
  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(normals1.size() * sizeof(glm::i8vec3));
    std::memcpy(
        buffer.cesium.data.data(),
        normals1.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 1;
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 1;
    accessor.componentType = Accessor::ComponentType::BYTE;
    accessor.type = Accessor::Type::VEC3;
    accessor.count = static_cast<int64_t>(normals1.size());
    accessor.normalized = true;

    Mesh& mesh = model.meshes.emplace_back();
    MeshPrimitive& primitive = mesh.primitives.emplace_back();
    primitive.attributes.insert({"NORMAL", 1});
  }

  SUBCASE("Handles invalid accessor") {
    NormalAccessorType normalAccessor;
    REQUIRE(!std::visit(NormalFromAccessor{0}, normalAccessor));
  }

  SUBCASE("Handles invalid index") {
    NormalAccessorType normalAccessor =
        getNormalAccessorView(model, model.meshes[0].primitives[0]);
    REQUIRE(!std::visit(NormalFromAccessor{-1}, normalAccessor));
    REQUIRE(!std::visit(NormalFromAccessor{10}, normalAccessor));
  }

  SUBCASE("Retrieves from valid accessor and index") {
    NormalAccessorType normalAccessor =
        getNormalAccessorView(model, model.meshes[0].primitives[0]);
    for (size_t i = 0; i < normals0.size(); i++) {
      auto maybeNormal = std::visit(
          NormalFromAccessor{static_cast<int64_t>(i)},
          normalAccessor);
      REQUIRE(maybeNormal);
      auto expected =
          glm::dvec3(normals0[i][0], normals0[i][1], normals0[i][2]);
      REQUIRE(*maybeNormal == expected);
    }
  }

  SUBCASE("Retrieves from valid signed normalized accessor and index") {
    NormalAccessorType normalAccessor =
        getNormalAccessorView(model, model.meshes[1].primitives[0]);
    for (size_t i = 0; i < normals1.size(); i++) {
      auto maybeNormal = std::visit(
          NormalFromAccessor{static_cast<int64_t>(i)},
          normalAccessor);
      REQUIRE(maybeNormal);
      auto expected =
          glm::dvec3(normals1[i][0], normals1[i][1], normals1[i][2]);
      expected = glm::max(expected / 127.0, -1.0);
      REQUIRE(*maybeNormal == expected);
    }
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
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

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
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
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
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  primitive.attributes.insert({"TEXCOORD_0", 0});
  primitive.attributes.insert({"TEXCOORD_1", 1});

  SUBCASE("Handles invalid texture coordinate set index") {
    TexCoordAccessorType texCoordAccessor =
        getTexCoordAccessorView(model, primitive, 3);
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
    model.accessors[0].componentType = Accessor::ComponentType::DOUBLE;

    TexCoordAccessorType texCoordAccessor =
        getTexCoordAccessorView(model, primitive, 0);
    REQUIRE(
        std::visit(StatusFromAccessor{}, texCoordAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, texCoordAccessor) == 0);

    model.accessors[0].componentType = Accessor::ComponentType::FLOAT;
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
    accessor.count = int64_t(texCoords0.size());
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
    accessor.count = int64_t(texCoords1.size());
  }

  std::vector<glm::i8vec2> texCoords2{
      glm::i8vec2(0, 0),
      glm::i8vec2(0, -128),
      glm::i8vec2(-127, 127),
      glm::i8vec2(127, 0)};

  // Third TEXCOORD set
  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(texCoords2.size() * sizeof(glm::i8vec2));
    std::memcpy(
        buffer.cesium.data.data(),
        texCoords2.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 2;
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 2;
    accessor.componentType = Accessor::ComponentType::BYTE;
    accessor.type = Accessor::Type::VEC2;
    accessor.normalized = true;
    accessor.count = int64_t(texCoords2.size());
  }

  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  primitive.attributes.insert({"TEXCOORD_0", 0});
  primitive.attributes.insert({"TEXCOORD_1", 1});
  primitive.attributes.insert({"TEXCOORD_2", 2});

  SUBCASE("Handles invalid accessor") {
    TexCoordAccessorType texCoordAccessor =
        getTexCoordAccessorView(model, primitive, 3);
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

  SUBCASE("Retrieves from valid unsigned normalized accessor and index") {
    TexCoordAccessorType texCoordAccessor =
        getTexCoordAccessorView(model, primitive, 1);
    for (size_t i = 0; i < texCoords1.size(); i++) {
      auto maybeTexCoord = std::visit(
          TexCoordFromAccessor{static_cast<int64_t>(i)},
          texCoordAccessor);
      REQUIRE(maybeTexCoord);
      auto expected = glm::dvec2(texCoords1[i][0], texCoords1[i][1]);
      expected /= 255.0;

      REQUIRE(*maybeTexCoord == expected);
    }
  }

  SUBCASE("Retrieves from valid signed normalized accessor and index") {
    TexCoordAccessorType texCoordAccessor =
        getTexCoordAccessorView(model, primitive, 2);
    for (size_t i = 0; i < texCoords2.size(); i++) {
      auto maybeTexCoord = std::visit(
          TexCoordFromAccessor{static_cast<int64_t>(i)},
          texCoordAccessor);
      REQUIRE(maybeTexCoord);
      auto expected = glm::dvec2(texCoords2[i][0], texCoords2[i][1]);
      expected = glm::max(expected / 127.0, -1.0);

      REQUIRE(*maybeTexCoord == expected);
    }
  }
}

TEST_CASE("Test getQuaternionAccessorView") {
  Model model;
  std::vector<glm::quat> quaternions{
      glm::quat(0, 1, 0, 0),
      glm::quat(1, 0, 0, 0),
      glm::quat(0, 0, 1, 1)};

  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(quaternions.size() * sizeof(glm::quat));
    std::memcpy(
        buffer.cesium.data.data(),
        quaternions.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 0;
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 0;
    accessor.componentType = Accessor::ComponentType::FLOAT;
    accessor.type = Accessor::Type::VEC4;
    accessor.count = static_cast<int64_t>(quaternions.size());
  }

  SUBCASE("Handles invalid accessor type") {
    model.accessors[0].type = Accessor::Type::SCALAR;

    QuaternionAccessorType quaternionAccessor =
        getQuaternionAccessorView(model, model.accessors[0]);
    REQUIRE(
        std::visit(StatusFromAccessor{}, quaternionAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, quaternionAccessor) == 0);

    model.accessors[0].type = Accessor::Type::VEC4;
  }

  SUBCASE("Handles unsupported accessor component type") {
    model.accessors[0].componentType = Accessor::ComponentType::DOUBLE;

    QuaternionAccessorType quaternionAccessor =
        getQuaternionAccessorView(model, model.accessors[0]);
    REQUIRE(
        std::visit(StatusFromAccessor{}, quaternionAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, quaternionAccessor) == 0);

    model.accessors[0].componentType = Accessor::ComponentType::FLOAT;
  }

  SUBCASE("Handles invalid un-normalized quaternion") {
    model.accessors[0].componentType = Accessor::ComponentType::BYTE;

    QuaternionAccessorType quaternionAccessor =
        getQuaternionAccessorView(model, model.accessors[0]);
    REQUIRE(
        std::visit(StatusFromAccessor{}, quaternionAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, quaternionAccessor) == 0);

    model.accessors[0].componentType = Accessor::ComponentType::FLOAT;
  }

  SUBCASE("Creates from valid accessor") {
    QuaternionAccessorType quaternionAccessor =
        getQuaternionAccessorView(model, model.accessors[0]);
    REQUIRE(
        std::visit(StatusFromAccessor{}, quaternionAccessor) ==
        AccessorViewStatus::Valid);
    REQUIRE(
        std::visit(CountFromAccessor{}, quaternionAccessor) ==
        quaternions.size());
  }

  SUBCASE("Handles invalid accessor index") {
    QuaternionAccessorType quaternionAccessor =
        getQuaternionAccessorView(model, 1);
    REQUIRE(
        std::visit(StatusFromAccessor{}, quaternionAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, quaternionAccessor) == 0);
  }

  SUBCASE("Creates from valid accessor index") {
    QuaternionAccessorType quaternionAccessor =
        getQuaternionAccessorView(model, 0);
    REQUIRE(
        std::visit(StatusFromAccessor{}, quaternionAccessor) ==
        AccessorViewStatus::Valid);
    REQUIRE(
        std::visit(CountFromAccessor{}, quaternionAccessor) ==
        quaternions.size());
  }
}

TEST_CASE("Test QuaternionFromAccessor") {
  Model model;
  std::vector<glm::quat> quaternions0{
      glm::quat(0, 1, 0, 0),
      glm::quat(1, 0, 0, 0),
      glm::quat(0, 0, 1, 1)};

  // Float quaternions
  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(quaternions0.size() * sizeof(glm::quat));
    std::memcpy(
        buffer.cesium.data.data(),
        quaternions0.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 0;
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 0;
    accessor.componentType = Accessor::ComponentType::FLOAT;
    accessor.type = Accessor::Type::VEC4;
    accessor.count = static_cast<int64_t>(quaternions0.size());
  }

  std::vector<glm::u8vec4> quaternions1{glm::u8vec4(0, 127, 255, 255)};

  // Unsigned normalized quaternions
  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(quaternions1.size() * sizeof(glm::u8vec4));
    std::memcpy(
        buffer.cesium.data.data(),
        quaternions1.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 1;
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 1;
    accessor.componentType = Accessor::ComponentType::UNSIGNED_BYTE;
    accessor.type = Accessor::Type::VEC4;
    accessor.count = static_cast<int64_t>(quaternions1.size());
    accessor.normalized = true;
  }

  std::vector<glm::i8vec4> quaternions2{
      glm::i8vec4(-128, 0, 127, -127),
  };

  // Signed normalized quaternions
  {
    Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(quaternions2.size() * sizeof(glm::i8vec4));
    std::memcpy(
        buffer.cesium.data.data(),
        quaternions2.data(),
        buffer.cesium.data.size());
    buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());

    BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = 2;
    bufferView.byteLength = buffer.byteLength;

    Accessor& accessor = model.accessors.emplace_back();
    accessor.bufferView = 2;
    accessor.componentType = Accessor::ComponentType::BYTE;
    accessor.type = Accessor::Type::VEC4;
    accessor.count = static_cast<int64_t>(quaternions2.size());
    accessor.normalized = true;
  }

  SUBCASE("Handles invalid accessor") {
    QuaternionAccessorType quaternionAccessor;
    REQUIRE(!std::visit(QuaternionFromAccessor{0}, quaternionAccessor));
  }

  SUBCASE("Handles invalid index") {
    QuaternionAccessorType quaternionAccessor =
        getQuaternionAccessorView(model, model.accessors[0]);
    REQUIRE(!std::visit(QuaternionFromAccessor{-1}, quaternionAccessor));
    REQUIRE(!std::visit(QuaternionFromAccessor{10}, quaternionAccessor));
  }

  SUBCASE("Retrieves from valid accessor and index") {
    QuaternionAccessorType quaternionAccessor =
        getQuaternionAccessorView(model, model.accessors[0]);

    for (size_t i = 0; i < quaternions0.size(); i++) {
      auto maybeQuaternion = std::visit(
          QuaternionFromAccessor{static_cast<int64_t>(i)},
          quaternionAccessor);
      REQUIRE(maybeQuaternion);
      // glm quat constructor is w,x,y,z
      auto expected = glm::dquat(
          quaternions0[i][3],
          quaternions0[i][0],
          quaternions0[i][1],
          quaternions0[i][2]);
      REQUIRE(*maybeQuaternion == expected);
    }
  }

  SUBCASE("Retrieves from valid unsigned normalized accessor and index") {
    QuaternionAccessorType quaternionAccessor =
        getQuaternionAccessorView(model, model.accessors[1]);
    for (size_t i = 0; i < quaternions1.size(); i++) {
      auto maybeQuaternion = std::visit(
          QuaternionFromAccessor{static_cast<int64_t>(i)},
          quaternionAccessor);
      REQUIRE(maybeQuaternion);
      // glm quat constructor is w,x,y,z
      auto expected = glm::dquat(
          quaternions1[i][3],
          quaternions1[i][0],
          quaternions1[i][1],
          quaternions1[i][2]);
      expected /= 255.0;
      REQUIRE(*maybeQuaternion == expected);
    }
  }

  SUBCASE("Retrieves from valid signed normalized accessor and index") {
    QuaternionAccessorType quaternionAccessor =
        getQuaternionAccessorView(model, model.accessors[2]);
    for (size_t i = 0; i < quaternions2.size(); i++) {
      auto maybeQuaternion = std::visit(
          QuaternionFromAccessor{static_cast<int64_t>(i)},
          quaternionAccessor);
      REQUIRE(maybeQuaternion);
      // glm quat constructor is w,x,y,z
      auto expected = glm::dquat(
          quaternions2[i][3],
          quaternions2[i][0],
          quaternions2[i][1],
          quaternions2[i][2]);
      expected.x = glm::max(expected.x / 127.0, -1.0);
      expected.y = glm::max(expected.y / 127.0, -1.0);
      expected.z = glm::max(expected.z / 127.0, -1.0);
      expected.w = glm::max(expected.w / 127.0, -1.0);
      REQUIRE(*maybeQuaternion == expected);
    }
  }
}

namespace {
template <typename T>
void addColorAttribute(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive,
    const std::vector<T>& colors,
    const std::string& type,
    int32_t componentType,
    bool normalized,
    int32_t setIndex) {
  Buffer& buffer = model.buffers.emplace_back();
  buffer.cesium.data.resize(colors.size() * sizeof(T));
  std::memcpy(
      buffer.cesium.data.data(),
      colors.data(),
      buffer.cesium.data.size());
  buffer.byteLength = int64_t(buffer.cesium.data.size());

  BufferView& bufferView = model.bufferViews.emplace_back();
  bufferView.buffer = int32_t(model.buffers.size() - 1);
  bufferView.byteLength = buffer.byteLength;

  Accessor& accessor = model.accessors.emplace_back();
  accessor.bufferView = int32_t(model.bufferViews.size() - 1);
  accessor.componentType = componentType;
  accessor.type = type;
  accessor.count = int64_t(colors.size());
  accessor.normalized = normalized;

  std::string name = fmt::format("COLOR_{}", setIndex);
  primitive.attributes.emplace(name, int32_t(model.accessors.size() - 1));
}
} // namespace

TEST_CASE("Test getColorAccessorView") {
  Model model;
  MeshPrimitive& primitive =
      model.meshes.emplace_back().primitives.emplace_back();

  std::vector<glm::u8vec3> colors0{glm::u8vec3(0, 128, 255)};
  std::vector<glm::u8vec4> colors1{glm::u8vec4(0, 128, 255, 128)};
  std::vector<glm::u16vec3> colors2{glm::u16vec3(0, 32767, 65535)};
  std::vector<glm::u16vec4> colors3{glm::u16vec4(0, 32767, 65535, 32767)};
  std::vector<glm::vec3> colors4{glm::fvec3(0, 0.5, 1.0)};
  std::vector<glm::vec4> colors5{glm::fvec4(0, 0.5, 1.0, 0.5)};

  addColorAttribute(
      model,
      primitive,
      colors0,
      CesiumGltf::Accessor::Type::VEC3,
      CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE,
      true,
      0);

  addColorAttribute(
      model,
      primitive,
      colors1,
      CesiumGltf::Accessor::Type::VEC4,
      CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE,
      true,
      1);

  addColorAttribute(
      model,
      primitive,
      colors2,
      CesiumGltf::Accessor::Type::VEC3,
      CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT,
      true,
      2);

  addColorAttribute(
      model,
      primitive,
      colors3,
      CesiumGltf::Accessor::Type::VEC4,
      CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT,
      true,
      3);

  addColorAttribute(
      model,
      primitive,
      colors4,
      CesiumGltf::Accessor::Type::VEC3,
      CesiumGltf::Accessor::ComponentType::FLOAT,
      false,
      4);

  addColorAttribute(
      model,
      primitive,
      colors5,
      CesiumGltf::Accessor::Type::VEC4,
      CesiumGltf::Accessor::ComponentType::FLOAT,
      false,
      5);

  SUBCASE("Handles invalid color set index") {
    ColorAccessorType colorAccessor = getColorAccessorView(model, primitive, 6);
    REQUIRE(
        std::visit(StatusFromAccessor{}, colorAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, colorAccessor) == 0);
  }

  SUBCASE("Handles invalid accessor type") {
    model.accessors[0].type = Accessor::Type::SCALAR;

    ColorAccessorType colorAccessor = getColorAccessorView(model, primitive, 0);
    REQUIRE(
        std::visit(StatusFromAccessor{}, colorAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, colorAccessor) == 0);

    model.accessors[0].type = Accessor::Type::VEC3;
  }

  SUBCASE("Handles unsupported accessor component type") {
    model.accessors[0].componentType = Accessor::ComponentType::BYTE;

    ColorAccessorType colorAccessor = getColorAccessorView(model, primitive, 0);
    REQUIRE(
        std::visit(StatusFromAccessor{}, colorAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, colorAccessor) == 0);

    model.accessors[0].componentType = Accessor::ComponentType::UNSIGNED_BYTE;
  }

  SUBCASE("Handles invalid un-normalized color") {
    model.accessors[0].normalized = false;

    ColorAccessorType colorAccessor = getColorAccessorView(model, primitive, 0);
    REQUIRE(
        std::visit(StatusFromAccessor{}, colorAccessor) !=
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, colorAccessor) == 0);

    model.accessors[0].normalized = true;
  }

  SUBCASE("Creates from valid color sets") {
    ColorAccessorType colorAccessor0 =
        getColorAccessorView(model, primitive, 0);
    REQUIRE(
        std::visit(StatusFromAccessor{}, colorAccessor0) ==
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, colorAccessor0) == colors0.size());

    ColorAccessorType colorAccessor1 =
        getColorAccessorView(model, primitive, 1);
    REQUIRE(
        std::visit(StatusFromAccessor{}, colorAccessor1) ==
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, colorAccessor1) == colors1.size());

    ColorAccessorType colorAccessor2 =
        getColorAccessorView(model, primitive, 2);
    REQUIRE(
        std::visit(StatusFromAccessor{}, colorAccessor2) ==
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, colorAccessor2) == colors2.size());

    ColorAccessorType colorAccessor3 =
        getColorAccessorView(model, primitive, 3);
    REQUIRE(
        std::visit(StatusFromAccessor{}, colorAccessor3) ==
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, colorAccessor3) == colors3.size());

    ColorAccessorType colorAccessor4 =
        getColorAccessorView(model, primitive, 4);
    REQUIRE(
        std::visit(StatusFromAccessor{}, colorAccessor4) ==
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, colorAccessor4) == colors4.size());

    ColorAccessorType colorAccessor5 =
        getColorAccessorView(model, primitive, 5);
    REQUIRE(
        std::visit(StatusFromAccessor{}, colorAccessor5) ==
        AccessorViewStatus::Valid);
    REQUIRE(std::visit(CountFromAccessor{}, colorAccessor5) == colors5.size());
  }
}

TEST_CASE("Test ColorFromAccessor") {
  Model model;
  MeshPrimitive& primitive =
      model.meshes.emplace_back().primitives.emplace_back();

  std::vector<glm::u8vec3> colors0{glm::u8vec3(0, 128, 255)};
  std::vector<glm::u8vec4> colors1{glm::u8vec4(0, 128, 255, 128)};
  std::vector<glm::u16vec3> colors2{glm::u16vec3(0, 32767, 65535)};
  std::vector<glm::u16vec4> colors3{glm::u16vec4(0, 32767, 65535, 32767)};
  std::vector<glm::vec3> colors4{glm::fvec3(0, 0.5, 1.0)};
  std::vector<glm::vec4> colors5{glm::fvec4(0, 0.5, 1.0, 0.5)};

  addColorAttribute(
      model,
      primitive,
      colors0,
      CesiumGltf::Accessor::Type::VEC3,
      CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE,
      true,
      0);

  addColorAttribute(
      model,
      primitive,
      colors1,
      CesiumGltf::Accessor::Type::VEC4,
      CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE,
      true,
      1);

  addColorAttribute(
      model,
      primitive,
      colors2,
      CesiumGltf::Accessor::Type::VEC3,
      CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT,
      true,
      2);

  addColorAttribute(
      model,
      primitive,
      colors3,
      CesiumGltf::Accessor::Type::VEC4,
      CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT,
      true,
      3);

  addColorAttribute(
      model,
      primitive,
      colors4,
      CesiumGltf::Accessor::Type::VEC3,
      CesiumGltf::Accessor::ComponentType::FLOAT,
      false,
      4);

  addColorAttribute(
      model,
      primitive,
      colors5,
      CesiumGltf::Accessor::Type::VEC4,
      CesiumGltf::Accessor::ComponentType::FLOAT,
      false,
      5);

  SUBCASE("Handles invalid accessor") {
    ColorAccessorType colorAccessor;
    REQUIRE(!std::visit(ColorFromAccessor{0}, colorAccessor));
  }

  SUBCASE("Handles invalid index") {
    ColorAccessorType colorAccessor = getColorAccessorView(model, primitive, 0);
    REQUIRE(!std::visit(ColorFromAccessor{-1}, colorAccessor));
    REQUIRE(!std::visit(ColorFromAccessor{10}, colorAccessor));
  }

  SUBCASE("Retrieves from valid accessor: UNSIGNED_BYTE, VEC3, normalized") {
    ColorAccessorType colorAccessor = getColorAccessorView(model, primitive, 0);

    for (size_t i = 0; i < colors0.size(); i++) {
      auto maybeColor =
          std::visit(ColorFromAccessor{int64_t(i)}, colorAccessor);
      REQUIRE(maybeColor);
      auto expected = glm::dvec4(
          colors0[i][0] / 255.0,
          colors0[i][1] / 255.0,
          colors0[i][2] / 255.0,
          1.0);
      CHECK(*maybeColor == expected);
    }
  }

  SUBCASE("Retrieves from valid accessor: UNSIGNED_BYTE, VEC4, normalized") {
    ColorAccessorType colorAccessor = getColorAccessorView(model, primitive, 1);

    for (size_t i = 0; i < colors1.size(); i++) {
      auto maybeColor =
          std::visit(ColorFromAccessor{int64_t(i)}, colorAccessor);
      REQUIRE(maybeColor);
      auto expected = glm::dvec4(
          colors1[i][0] / 255.0,
          colors1[i][1] / 255.0,
          colors1[i][2] / 255.0,
          colors1[i][3] / 255.0);
      CHECK(*maybeColor == expected);
    }
  }

  SUBCASE("Retrieves from valid accessor: UNSIGNED_SHORT, VEC3, normalized") {
    ColorAccessorType colorAccessor = getColorAccessorView(model, primitive, 2);

    for (size_t i = 0; i < colors2.size(); i++) {
      auto maybeColor =
          std::visit(ColorFromAccessor{int64_t(i)}, colorAccessor);
      REQUIRE(maybeColor);
      auto expected = glm::dvec4(
          colors2[i][0] / 65535.0,
          colors2[i][1] / 65535.0,
          colors2[i][2] / 65535.0,
          1.0);
      CHECK(*maybeColor == expected);
    }
  }

  SUBCASE("Retrieves from valid accessor: UNSIGNED_SHORT, VEC4, normalized") {
    ColorAccessorType colorAccessor = getColorAccessorView(model, primitive, 3);

    for (size_t i = 0; i < colors3.size(); i++) {
      auto maybeColor =
          std::visit(ColorFromAccessor{int64_t(i)}, colorAccessor);
      REQUIRE(maybeColor);
      auto expected = glm::dvec4(
          colors3[i][0] / 65535.0,
          colors3[i][1] / 65535.0,
          colors3[i][2] / 65535.0,
          colors3[i][3] / 65535.0);
      CHECK(*maybeColor == expected);
    }
  }

  SUBCASE("Retrieves from valid accessor: FLOAT, VEC3") {
    ColorAccessorType colorAccessor = getColorAccessorView(model, primitive, 4);

    for (size_t i = 0; i < colors4.size(); i++) {
      auto maybeColor =
          std::visit(ColorFromAccessor{int64_t(i)}, colorAccessor);
      REQUIRE(maybeColor);
      auto expected =
          glm::dvec4(colors4[i][0], colors4[i][1], colors4[i][2], 1.0);
      CHECK(*maybeColor == expected);
    }
  }

  SUBCASE("Retrieves from valid accessor: FLOAT, VEC4") {
    ColorAccessorType colorAccessor = getColorAccessorView(model, primitive, 5);

    for (size_t i = 0; i < colors5.size(); i++) {
      auto maybeColor =
          std::visit(ColorFromAccessor{int64_t(i)}, colorAccessor);
      REQUIRE(maybeColor);
      auto expected = glm::dvec4(
          colors5[i][0],
          colors5[i][1],
          colors5[i][2],
          colors5[i][3]);
      CHECK(*maybeColor == expected);
    }
  }
}
