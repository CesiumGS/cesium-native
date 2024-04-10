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
using namespace CesiumUtility;

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

TEST_CASE("Model::addExtensionUsed") {
  SECTION("adds a new extension") {
    Model m;

    m.addExtensionUsed("Foo");
    m.addExtensionUsed("Bar");

    CHECK(m.extensionsUsed.size() == 2);
    CHECK(
        std::find(m.extensionsUsed.begin(), m.extensionsUsed.end(), "Foo") !=
        m.extensionsUsed.end());
    CHECK(
        std::find(m.extensionsUsed.begin(), m.extensionsUsed.end(), "Bar") !=
        m.extensionsUsed.end());
  }

  SECTION("does not add a duplicate extension") {
    Model m;

    m.addExtensionUsed("Foo");
    m.addExtensionUsed("Bar");
    m.addExtensionUsed("Foo");

    CHECK(m.extensionsUsed.size() == 2);
    CHECK(
        std::find(m.extensionsUsed.begin(), m.extensionsUsed.end(), "Foo") !=
        m.extensionsUsed.end());
    CHECK(
        std::find(m.extensionsUsed.begin(), m.extensionsUsed.end(), "Bar") !=
        m.extensionsUsed.end());
  }

  SECTION("does not also add the extension to extensionsRequired") {
    Model m;
    m.addExtensionUsed("Foo");
    CHECK(m.extensionsRequired.empty());
  }
}

TEST_CASE("Model::addExtensionRequired") {
  SECTION("adds a new extension") {
    Model m;

    m.addExtensionRequired("Foo");
    m.addExtensionRequired("Bar");

    CHECK(m.extensionsRequired.size() == 2);
    CHECK(
        std::find(
            m.extensionsRequired.begin(),
            m.extensionsRequired.end(),
            "Foo") != m.extensionsRequired.end());
    CHECK(
        std::find(
            m.extensionsRequired.begin(),
            m.extensionsRequired.end(),
            "Bar") != m.extensionsRequired.end());
  }

  SECTION("does not add a duplicate extension") {
    Model m;

    m.addExtensionRequired("Foo");
    m.addExtensionRequired("Bar");
    m.addExtensionRequired("Foo");

    CHECK(m.extensionsRequired.size() == 2);
    CHECK(
        std::find(
            m.extensionsRequired.begin(),
            m.extensionsRequired.end(),
            "Foo") != m.extensionsRequired.end());
    CHECK(
        std::find(
            m.extensionsRequired.begin(),
            m.extensionsRequired.end(),
            "Bar") != m.extensionsRequired.end());
  }

  SECTION("also adds the extension to extensionsUsed if not already present") {
    Model m;

    m.addExtensionUsed("Bar");
    m.addExtensionRequired("Foo");
    m.addExtensionRequired("Bar");

    CHECK(m.extensionsUsed.size() == 2);
    CHECK(
        std::find(m.extensionsUsed.begin(), m.extensionsUsed.end(), "Foo") !=
        m.extensionsUsed.end());
    CHECK(
        std::find(m.extensionsUsed.begin(), m.extensionsUsed.end(), "Bar") !=
        m.extensionsUsed.end());
  }
}

TEST_CASE("Model::merge") {
  SECTION("performs a simple merge") {
    Model m1;
    m1.accessors.emplace_back().name = "m1";
    m1.animations.emplace_back().name = "m1";
    m1.buffers.emplace_back().name = "m1";
    m1.bufferViews.emplace_back().name = "m1";
    m1.cameras.emplace_back().name = "m1";
    m1.images.emplace_back().name = "m1";
    m1.materials.emplace_back().name = "m1";
    m1.meshes.emplace_back().name = "m1";
    m1.nodes.emplace_back().name = "m1";
    m1.samplers.emplace_back().name = "m1";
    m1.scenes.emplace_back().name = "m1";
    m1.skins.emplace_back().name = "m1";
    m1.textures.emplace_back().name = "m1";

    Model m2;
    m2.accessors.emplace_back().name = "m2";
    m2.animations.emplace_back().name = "m2";
    m2.buffers.emplace_back().name = "m2";
    m2.bufferViews.emplace_back().name = "m2";
    m2.cameras.emplace_back().name = "m2";
    m2.images.emplace_back().name = "m2";
    m2.materials.emplace_back().name = "m2";
    m2.meshes.emplace_back().name = "m2";
    m2.nodes.emplace_back().name = "m2";
    m2.samplers.emplace_back().name = "m2";
    m2.scenes.emplace_back().name = "m2";
    m2.skins.emplace_back().name = "m2";
    m2.textures.emplace_back().name = "m2";

    ErrorList errors = m1.merge(std::move(m2));
    CHECK(errors.errors.empty());
    CHECK(errors.warnings.empty());

    REQUIRE(m1.accessors.size() == 2);
    CHECK(m1.accessors[0].name == "m1");
    CHECK(m1.accessors[1].name == "m2");
    REQUIRE(m1.animations.size() == 2);
    CHECK(m1.animations[0].name == "m1");
    CHECK(m1.animations[1].name == "m2");
    REQUIRE(m1.buffers.size() == 2);
    CHECK(m1.buffers[0].name == "m1");
    CHECK(m1.buffers[1].name == "m2");
    REQUIRE(m1.bufferViews.size() == 2);
    CHECK(m1.bufferViews[0].name == "m1");
    CHECK(m1.bufferViews[1].name == "m2");
    REQUIRE(m1.cameras.size() == 2);
    CHECK(m1.cameras[0].name == "m1");
    CHECK(m1.cameras[1].name == "m2");
    REQUIRE(m1.images.size() == 2);
    CHECK(m1.images[0].name == "m1");
    CHECK(m1.images[1].name == "m2");
    REQUIRE(m1.materials.size() == 2);
    CHECK(m1.materials[0].name == "m1");
    CHECK(m1.materials[1].name == "m2");
    REQUIRE(m1.meshes.size() == 2);
    CHECK(m1.meshes[0].name == "m1");
    CHECK(m1.meshes[1].name == "m2");
    REQUIRE(m1.nodes.size() == 2);
    CHECK(m1.nodes[0].name == "m1");
    CHECK(m1.nodes[1].name == "m2");
    REQUIRE(m1.samplers.size() == 2);
    CHECK(m1.samplers[0].name == "m1");
    CHECK(m1.samplers[1].name == "m2");
    REQUIRE(m1.scenes.size() == 2);
    CHECK(m1.scenes[0].name == "m1");
    CHECK(m1.scenes[1].name == "m2");
    REQUIRE(m1.skins.size() == 2);
    CHECK(m1.skins[0].name == "m1");
    CHECK(m1.skins[1].name == "m2");
    REQUIRE(m1.textures.size() == 2);
    CHECK(m1.textures[0].name == "m1");
    CHECK(m1.textures[1].name == "m2");
  }

  SECTION("merges default scenes") {
    Model m1;
    m1.nodes.emplace_back().name = "node1";
    m1.nodes.emplace_back().name = "node2";
    Scene& scene1 = m1.scenes.emplace_back();
    scene1.name = "scene1";
    scene1.nodes.push_back(1);
    Scene& scene2 = m1.scenes.emplace_back();
    scene2.name = "scene2";
    scene2.nodes.push_back(1);
    scene2.nodes.push_back(0);
    m1.scene = 1;

    Model m2;
    m2.nodes.emplace_back().name = "node3";
    m2.nodes.emplace_back().name = "node4";
    Scene& scene3 = m2.scenes.emplace_back();
    scene3.name = "scene3";
    scene3.nodes.push_back(1);
    scene3.nodes.push_back(0);
    Scene& scene4 = m2.scenes.emplace_back();
    scene4.name = "scene4";
    scene4.nodes.push_back(1);
    m2.scene = 0;

    ErrorList errors = m1.merge(std::move(m2));
    CHECK(errors.errors.empty());
    CHECK(errors.warnings.empty());

    REQUIRE(m1.scene >= 0);
    REQUIRE(m1.scene < m1.scenes.size());

    Scene& defaultScene = m1.scenes[m1.scene];
    REQUIRE(defaultScene.nodes.size() == 4);
    CHECK(m1.nodes[defaultScene.nodes[0]].name == "node2");
    CHECK(m1.nodes[defaultScene.nodes[1]].name == "node1");
    CHECK(m1.nodes[defaultScene.nodes[2]].name == "node4");
    CHECK(m1.nodes[defaultScene.nodes[3]].name == "node3");
  }
}
