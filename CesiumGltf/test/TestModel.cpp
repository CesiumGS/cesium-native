#include "CesiumGltf/AccessorView.h"
#include "CesiumGltf/Model.h"

#include <catch2/catch.hpp>
#include <glm/common.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <utility>
#include <vector>

using namespace CesiumGltf;

#define DEFAULT_EPSILON 1e-6f

TEST_CASE("Test forEachPrimitive") {

  Model model;

  model.scenes.resize(2);
  model.scene = 0;

  Scene& scene0 = model.scenes[0];
  Scene& scene1 = model.scenes[1];

  glm::dmat4 parentNodeMatrix(
      1.0,
      6.0,
      23.1,
      10.3,
      0.0,
      3.0,
      2.0,
      1.0,
      0.0,
      4.5,
      1.0,
      0.0,
      3.7,
      0.0,
      0.0,
      1.0);

  glm::dmat4 childNodeMatrix(
      4.0,
      0.0,
      0.0,
      3.0,
      2.8,
      2.0,
      3.0,
      2.0,
      0.0,
      1.0,
      0.0,
      0.0,
      0.0,
      5.3,
      0.0,
      1.0);

  glm::dmat4 expectedNodeTransform = parentNodeMatrix * childNodeMatrix;

  model.nodes.resize(4);
  Node& node0 = model.nodes[0];
  Node& node1 = model.nodes[1];
  Node& node2 = model.nodes[2];
  Node& node3 = model.nodes[3];

  scene0.nodes = {0, 1};
  scene1.nodes = {2};
  node2.children = {3};

  std::memcpy(node2.matrix.data(), &parentNodeMatrix, sizeof(glm::dmat4));
  scene1.nodes = {2};
  std::memcpy(node3.matrix.data(), &childNodeMatrix, sizeof(glm::dmat4));
  node2.children = {3};

  model.meshes.resize(3);
  Mesh& mesh0 = model.meshes[0];
  Mesh& mesh1 = model.meshes[1];
  Mesh& mesh2 = model.meshes[2];

  node0.mesh = 0;
  node1.mesh = 1;
  node3.mesh = 2;

  MeshPrimitive& primitive0 = mesh0.primitives.emplace_back();

  mesh1.primitives.resize(2);
  MeshPrimitive& primitive1 = mesh1.primitives[0];
  MeshPrimitive& primitive2 = mesh1.primitives[1];

  MeshPrimitive& primitive3 = mesh2.primitives.emplace_back();

  SECTION("Check that the correct primitives are iterated over.") {

    std::vector<MeshPrimitive*> iteratedPrimitives;

    model.forEachPrimitiveInScene(
        -1,
        [&iteratedPrimitives](
            Model& /*model*/,
            Node& /*node*/,
            Mesh& /*mesh*/,
            MeshPrimitive& primitive,
            const glm::dmat4& /*transform*/) {
          iteratedPrimitives.push_back(&primitive);
        });

    REQUIRE(iteratedPrimitives.size() == 3);
    REQUIRE(
        std::find(
            iteratedPrimitives.begin(),
            iteratedPrimitives.end(),
            &primitive0) != iteratedPrimitives.end());
    REQUIRE(
        std::find(
            iteratedPrimitives.begin(),
            iteratedPrimitives.end(),
            &primitive1) != iteratedPrimitives.end());
    REQUIRE(
        std::find(
            iteratedPrimitives.begin(),
            iteratedPrimitives.end(),
            &primitive2) != iteratedPrimitives.end());

    iteratedPrimitives.clear();

    model.forEachPrimitiveInScene(
        0,
        [&iteratedPrimitives](
            Model& /*model*/,
            Node& /*node*/,
            Mesh& /*mesh*/,
            MeshPrimitive& primitive,
            const glm::dmat4& /*transform*/) {
          iteratedPrimitives.push_back(&primitive);
        });

    REQUIRE(iteratedPrimitives.size() == 3);
    REQUIRE(
        std::find(
            iteratedPrimitives.begin(),
            iteratedPrimitives.end(),
            &primitive0) != iteratedPrimitives.end());
    REQUIRE(
        std::find(
            iteratedPrimitives.begin(),
            iteratedPrimitives.end(),
            &primitive1) != iteratedPrimitives.end());
    REQUIRE(
        std::find(
            iteratedPrimitives.begin(),
            iteratedPrimitives.end(),
            &primitive2) != iteratedPrimitives.end());

    iteratedPrimitives.clear();

    model.forEachPrimitiveInScene(
        1,
        [&iteratedPrimitives](
            Model& /*model*/,
            Node& /*node*/,
            Mesh& /*mesh*/,
            MeshPrimitive& primitive,
            const glm::dmat4& /*transform*/) {
          iteratedPrimitives.push_back(&primitive);
        });

    REQUIRE(iteratedPrimitives.size() == 1);
    REQUIRE(iteratedPrimitives[0] == &primitive3);
  }

  SECTION("Check the node transform") {

    std::vector<glm::dmat4> nodeTransforms;

    model.forEachPrimitiveInScene(
        1,
        [&nodeTransforms](
            Model& /*model*/,
            Node& /*node*/,
            Mesh& /*mesh*/,
            MeshPrimitive& /*primitive*/,
            const glm::dmat4& transform) {
          nodeTransforms.push_back(transform);
        });

    REQUIRE(nodeTransforms.size() == 1);
    REQUIRE(nodeTransforms[0] == expectedNodeTransform);
  }
}

static Model createCubeGltf() {
  Model model;

  std::vector<glm::vec3> cubeVertices = {
      glm::vec3(0.0f, 0.0f, 0.0f),
      glm::vec3(1.0f, 0.0f, 0.0f),
      glm::vec3(1.0f, 0.0f, 1.0f),
      glm::vec3(0.0f, 0.0f, 1.0f),

      glm::vec3(0.0f, 1.0f, 0.0f),
      glm::vec3(1.0f, 1.0f, 0.0f),
      glm::vec3(1.0f, 1.0f, 1.0f),
      glm::vec3(0.0f, 1.0f, 1.0f)};

  // TODO: generalize type so each index type can be tested?
  std::vector<uint8_t> cubeIndices = {0, 1, 2, 0, 2, 3,

                                      4, 6, 5, 4, 7, 6,

                                      0, 5, 1, 0, 4, 5,

                                      0, 7, 4, 0, 3, 7,

                                      1, 5, 6, 1, 6, 2,

                                      3, 2, 6, 3, 6, 7};

  size_t vertexbyteStride = sizeof(glm::vec3);
  size_t vertexbyteLength = 8 * vertexbyteStride;

  Buffer& vertexBuffer = model.buffers.emplace_back();
  vertexBuffer.byteLength = static_cast<int64_t>(vertexbyteLength);
  vertexBuffer.cesium.data.resize(vertexbyteLength);
  std::memcpy(
      vertexBuffer.cesium.data.data(),
      &cubeVertices[0],
      vertexbyteLength);

  BufferView& vertexBufferView = model.bufferViews.emplace_back();
  vertexBufferView.buffer = 0;
  vertexBufferView.byteLength = vertexBuffer.byteLength;
  vertexBufferView.byteOffset = 0;
  vertexBufferView.byteStride = static_cast<int64_t>(vertexbyteStride);
  vertexBufferView.target = BufferView::Target::ARRAY_BUFFER;

  Accessor& vertexAccessor = model.accessors.emplace_back();
  vertexAccessor.bufferView = 0;
  vertexAccessor.byteOffset = 0;
  vertexAccessor.componentType = Accessor::ComponentType::FLOAT;
  vertexAccessor.count = 8;
  vertexAccessor.type = Accessor::Type::VEC3;

  Buffer& indexBuffer = model.buffers.emplace_back();
  indexBuffer.byteLength = 36;
  indexBuffer.cesium.data.resize(36);
  std::memcpy(indexBuffer.cesium.data.data(), &cubeIndices[0], 36);

  BufferView& indexBufferView = model.bufferViews.emplace_back();
  indexBufferView.buffer = 1;
  indexBufferView.byteLength = 36;
  indexBufferView.byteOffset = 0;
  indexBufferView.byteStride = 1;
  indexBufferView.target = BufferView::Target::ELEMENT_ARRAY_BUFFER;

  Accessor& indexAccessor = model.accessors.emplace_back();
  indexAccessor.bufferView = 1;
  indexAccessor.byteOffset = 0;
  indexAccessor.componentType = Accessor::ComponentType::UNSIGNED_BYTE;
  indexAccessor.count = 36;
  indexAccessor.type = Accessor::Type::SCALAR;

  Scene& scene = model.scenes.emplace_back();
  Node& node = model.nodes.emplace_back();
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  primitive.attributes.emplace("POSITION", 0);
  primitive.indices = 1;
  primitive.mode = MeshPrimitive::Mode::TRIANGLES;

  model.scene = 0;
  scene.nodes = {0};
  node.mesh = 0;

  return model;
}

static Model createTriangleStrip() {
  Model model;

  std::vector<glm::vec3> vertices = {
      glm::vec3(0.0f, 1.0f, 0.0f),
      glm::vec3(1.0f, 0.0f, 0.0f),
      glm::vec3(0.0f, 0.0f, -1.0f),
      glm::vec3(1.0f, 1.0f, -1.0f)};

  size_t byteStride = sizeof(glm::vec3);
  size_t byteLength = 4 * byteStride;

  Buffer& vertexBuffer = model.buffers.emplace_back();
  vertexBuffer.byteLength = static_cast<int64_t>(byteLength);
  vertexBuffer.cesium.data.resize(byteLength);
  std::memcpy(vertexBuffer.cesium.data.data(), &vertices[0], byteLength);

  BufferView& vertexBufferView = model.bufferViews.emplace_back();
  vertexBufferView.buffer = 0;
  vertexBufferView.byteLength = vertexBuffer.byteLength;
  vertexBufferView.byteOffset = 0;
  vertexBufferView.byteStride = static_cast<int64_t>(byteStride);
  vertexBufferView.target = BufferView::Target::ARRAY_BUFFER;

  Accessor& vertexAccessor = model.accessors.emplace_back();
  vertexAccessor.bufferView = 0;
  vertexAccessor.byteOffset = 0;
  vertexAccessor.componentType = Accessor::ComponentType::FLOAT;
  vertexAccessor.count = 4;
  vertexAccessor.type = Accessor::Type::VEC3;

  Scene& scene = model.scenes.emplace_back();
  Node& node = model.nodes.emplace_back();
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  primitive.attributes.emplace("POSITION", 0);
  primitive.mode = MeshPrimitive::Mode::TRIANGLE_STRIP;

  model.scene = 0;
  scene.nodes = {0};
  node.mesh = 0;

  return model;
}

static Model createTriangleFan() {
  Model model;

  std::vector<glm::vec3> vertices = {
      glm::vec3(0.5f, 1.0f, -0.5f),
      glm::vec3(0.0f, 0.0f, 0.0f),
      glm::vec3(1.0f, 0.0f, 0.0f),
      glm::vec3(1.0f, 0.0f, -1.0f),
      glm::vec3(0.0f, 0.0f, -1.0f),
      glm::vec3(0.0f, 0.0f, 0.0f)};

  size_t byteStride = sizeof(glm::vec3);
  size_t byteLength = 6 * byteStride;

  Buffer& vertexBuffer = model.buffers.emplace_back();
  vertexBuffer.byteLength = static_cast<int64_t>(byteLength);
  vertexBuffer.cesium.data.resize(byteLength);
  std::memcpy(vertexBuffer.cesium.data.data(), &vertices[0], byteLength);

  BufferView& vertexBufferView = model.bufferViews.emplace_back();
  vertexBufferView.buffer = 0;
  vertexBufferView.byteLength = vertexBuffer.byteLength;
  vertexBufferView.byteOffset = 0;
  vertexBufferView.byteStride = static_cast<int64_t>(byteStride);
  vertexBufferView.target = BufferView::Target::ARRAY_BUFFER;

  Accessor& vertexAccessor = model.accessors.emplace_back();
  vertexAccessor.bufferView = 0;
  vertexAccessor.byteOffset = 0;
  vertexAccessor.componentType = Accessor::ComponentType::FLOAT;
  vertexAccessor.count = 6;
  vertexAccessor.type = Accessor::Type::VEC3;

  Scene& scene = model.scenes.emplace_back();
  Node& node = model.nodes.emplace_back();
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  primitive.attributes.emplace("POSITION", 0);
  primitive.mode = MeshPrimitive::Mode::TRIANGLE_FAN;

  model.scene = 0;
  scene.nodes = {0};
  node.mesh = 0;

  return model;
}

TEST_CASE("Test smooth normal generation") {
  SECTION("Test normal generation TRIANGLES") {
    Model model = createCubeGltf();

    model.generateMissingNormalsSmooth();

    REQUIRE(model.scene == 0);
    REQUIRE(model.scenes.size() == 1);
    REQUIRE(model.scenes[0].nodes.size() == 1);
    REQUIRE(model.scenes[0].nodes[0] == 0);
    REQUIRE(model.nodes.size() == 1);
    REQUIRE(model.nodes[0].mesh == 0);
    REQUIRE(model.meshes.size() == 1);
    REQUIRE(model.meshes[0].primitives.size() == 1);

    MeshPrimitive& primitive = model.meshes[0].primitives[0];
    auto normalIt = primitive.attributes.find("NORMAL");
    REQUIRE(normalIt != primitive.attributes.end());

    AccessorView<glm::vec3> normalView(model, normalIt->second);
    REQUIRE(normalView.status() == AccessorViewStatus::Valid);
    REQUIRE(normalView.size() == 8);

    const glm::vec3& vertex0Normal = normalView[0];
    glm::vec3 expectedNormal(-1.0f, -1.0f, -1.0f);
    expectedNormal = glm::normalize(expectedNormal);

    REQUIRE(glm::all(
        glm::epsilonEqual(vertex0Normal, expectedNormal, DEFAULT_EPSILON)));

    const glm::vec3& vertex6Normal = normalView[6];
    expectedNormal = glm::vec3(1.0f, 1.0f, 1.0f);
    expectedNormal = glm::normalize(expectedNormal);

    REQUIRE(glm::all(
        glm::epsilonEqual(vertex6Normal, expectedNormal, DEFAULT_EPSILON)));
  }

  SECTION("Test normal generation for TRIANGLE_STRIP") {
    Model model = createTriangleStrip();

    model.generateMissingNormalsSmooth();

    REQUIRE(model.scene == 0);
    REQUIRE(model.scenes.size() == 1);
    REQUIRE(model.scenes[0].nodes.size() == 1);
    REQUIRE(model.scenes[0].nodes[0] == 0);
    REQUIRE(model.nodes.size() == 1);
    REQUIRE(model.nodes[0].mesh == 0);
    REQUIRE(model.meshes.size() == 1);
    REQUIRE(model.meshes[0].primitives.size() == 1);

    MeshPrimitive& primitive = model.meshes[0].primitives[0];
    auto normalIt = primitive.attributes.find("NORMAL");
    REQUIRE(normalIt != primitive.attributes.end());

    AccessorView<glm::vec3> normalView(model, normalIt->second);
    REQUIRE(normalView.status() == AccessorViewStatus::Valid);
    REQUIRE(normalView.size() == 4);

    const glm::vec3& vertex1Normal = normalView[1];
    const glm::vec3& vertex2Normal = normalView[2];

    glm::vec3 expectedNormal(0.0f, 1.0f, 0.0f);
    expectedNormal = glm::normalize(expectedNormal);

    REQUIRE(glm::all(
        glm::epsilonEqual(vertex1Normal, expectedNormal, DEFAULT_EPSILON)));
    REQUIRE(glm::all(
        glm::epsilonEqual(vertex2Normal, expectedNormal, DEFAULT_EPSILON)));
  }

  SECTION("Test normal generation for TRIANGLE_STRIP") {
    Model model = createTriangleFan();

    model.generateMissingNormalsSmooth();

    REQUIRE(model.scene == 0);
    REQUIRE(model.scenes.size() == 1);
    REQUIRE(model.scenes[0].nodes.size() == 1);
    REQUIRE(model.scenes[0].nodes[0] == 0);
    REQUIRE(model.nodes.size() == 1);
    REQUIRE(model.nodes[0].mesh == 0);
    REQUIRE(model.meshes.size() == 1);
    REQUIRE(model.meshes[0].primitives.size() == 1);

    MeshPrimitive& primitive = model.meshes[0].primitives[0];
    auto normalIt = primitive.attributes.find("NORMAL");
    REQUIRE(normalIt != primitive.attributes.end());

    AccessorView<glm::vec3> normalView(model, normalIt->second);
    REQUIRE(normalView.status() == AccessorViewStatus::Valid);
    REQUIRE(normalView.size() == 6);

    const glm::vec3& vertex0Normal = normalView[0];

    glm::vec3 expectedNormal(0.0f, 1.0f, 0.0f);
    expectedNormal = glm::normalize(expectedNormal);

    REQUIRE(glm::all(
        glm::epsilonEqual(vertex0Normal, expectedNormal, DEFAULT_EPSILON)));
  }
}
