
#include "CesiumGltf/AccessorView.h"
#include "CesiumGltf/Model.h"
#include "catch2/catch.hpp"
#include <algorithm>
#include <cstdint>
#include <glm/common.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <utility>
#include <vector>

using namespace CesiumGltf;

TEST_CASE("Test forEachPrimitive") {

  Model model;

  Scene scene0;
  model.scene = 0;
  Scene scene1;

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

  Node node0;
  Node node1;
  scene0.nodes = {0, 1};
  Node node2;
  std::memcpy(node2.matrix.data(), &parentNodeMatrix, sizeof(glm::dmat4));
  scene1.nodes = {2};
  Node node3;
  std::memcpy(node3.matrix.data(), &childNodeMatrix, sizeof(glm::dmat4));
  node2.children = {3};

  Mesh mesh0;
  node0.mesh = 0;
  Mesh mesh1;
  node1.mesh = 1;
  Mesh mesh2;
  node3.mesh = 2;

  MeshPrimitive primitive0;
  MeshPrimitive primitive1;
  MeshPrimitive primitive2;
  MeshPrimitive primitive3;

  mesh0.primitives = {std::move(primitive0)};
  mesh1.primitives = {std::move(primitive1), std::move(primitive2)};
  mesh2.primitives = {std::move(primitive3)};

  model.meshes = {std::move(mesh0), std::move(mesh1), std::move(mesh2)};
  model.nodes =
      {std::move(node0), std::move(node1), std::move(node2), std::move(node3)};
  model.scenes = {std::move(scene0), std::move(scene1)};

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
            &model.meshes[0].primitives[0]) != iteratedPrimitives.end());
    REQUIRE(
        std::find(
            iteratedPrimitives.begin(),
            iteratedPrimitives.end(),
            &model.meshes[1].primitives[0]) != iteratedPrimitives.end());
    REQUIRE(
        std::find(
            iteratedPrimitives.begin(),
            iteratedPrimitives.end(),
            &model.meshes[1].primitives[1]) != iteratedPrimitives.end());

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
            &model.meshes[0].primitives[0]) != iteratedPrimitives.end());
    REQUIRE(
        std::find(
            iteratedPrimitives.begin(),
            iteratedPrimitives.end(),
            &model.meshes[1].primitives[0]) != iteratedPrimitives.end());
    REQUIRE(
        std::find(
            iteratedPrimitives.begin(),
            iteratedPrimitives.end(),
            &model.meshes[1].primitives[1]) != iteratedPrimitives.end());

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
    REQUIRE(iteratedPrimitives[0] == &model.meshes[2].primitives[0]);
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

  Buffer& vertexBuffer = model.buffers.emplace_back();
  vertexBuffer.byteLength = static_cast<int64_t>(8 * sizeof(glm::vec3));
  vertexBuffer.cesium.data.resize(vertexBuffer.byteLength);
  std::memcpy(
      vertexBuffer.cesium.data.data(),
      &cubeVertices[0],
      vertexBuffer.byteLength);

  BufferView& vertexBufferView = model.bufferViews.emplace_back();
  vertexBufferView.buffer = 0;
  vertexBufferView.byteLength = vertexBuffer.byteLength;
  vertexBufferView.byteOffset = 0;
  vertexBufferView.byteStride = static_cast<int64_t>(sizeof(glm::vec3));
  vertexBufferView.target = BufferView::Target::ARRAY_BUFFER;

  Accessor& vertexAccessor = model.accessors.emplace_back();
  vertexAccessor.bufferView = 0;
  vertexAccessor.byteOffset = 0;
  vertexAccessor.componentType = Accessor::ComponentType::FLOAT;
  vertexAccessor.count = 8;
  vertexAccessor.type = Accessor::Type::VEC3;

  Buffer& indexBuffer = model.buffers.emplace_back();
  indexBuffer.byteLength = 36;
  indexBuffer.cesium.data.resize(indexBuffer.byteLength);
  std::memcpy(
      indexBuffer.cesium.data.data(),
      &cubeIndices[0],
      indexBuffer.byteLength);

  BufferView& indexBufferView = model.bufferViews.emplace_back();
  indexBufferView.buffer = 1;
  indexBufferView.byteLength = indexBuffer.byteLength;
  indexBufferView.byteOffset = 0;
  indexBufferView.byteStride = 1;
  indexBufferView.target = BufferView::Target::ELEMENT_ARRAY_BUFFER;

  Accessor& indexAccessor = model.accessors.emplace_back();
  indexAccessor.bufferView = 1;
  indexAccessor.byteOffset = 0;
  indexAccessor.componentType = Accessor::ComponentType::UNSIGNED_BYTE;
  indexAccessor.count = 36;
  indexAccessor.type = Accessor::Type::SCALAR;

  Scene scene;
  Node node;
  Mesh mesh;
  MeshPrimitive primitive;

  primitive.attributes.emplace("POSITION", 0);
  primitive.indices = 1;
  primitive.mode = MeshPrimitive::Mode::TRIANGLES;

  mesh.primitives = {primitive};
  node.mesh = 0;
  scene.nodes = {0};

  model.scene = 0;

  model.scenes = {scene};
  model.nodes = {node};
  model.meshes = {mesh};

  return model;
}

TEST_CASE("Test smooth normal generation") {
  Model model = createCubeGltf();

  model.generateMissingNormalsSmooth();

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

  REQUIRE(glm::abs(glm::dot(vertex0Normal, expectedNormal) - 1.0f) < 1e-6f);

  const glm::vec3& vertex6Normal = normalView[6];
  expectedNormal = glm::vec3(1.0f, 1.0f, 1.0f);
  expectedNormal = glm::normalize(expectedNormal);

  REQUIRE(glm::abs(glm::dot(vertex6Normal, expectedNormal) - 1.0f) < 1e-6f);
}