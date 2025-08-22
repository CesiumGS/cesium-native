#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Class.h>
#include <CesiumGltf/ExtensionBufferViewExtMeshoptCompression.h>
#include <CesiumGltf/ExtensionCesiumPrimitiveOutline.h>
#include <CesiumGltf/ExtensionCesiumTileEdges.h>
#include <CesiumGltf/ExtensionExtMeshFeatures.h>
#include <CesiumGltf/ExtensionExtMeshGpuInstancing.h>
#include <CesiumGltf/ExtensionKhrTextureBasisu.h>
#include <CesiumGltf/ExtensionMeshPrimitiveExtStructuralMetadata.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/ExtensionTextureWebp.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Node.h>
#include <CesiumGltf/PropertyAttribute.h>
#include <CesiumGltf/PropertyTable.h>
#include <CesiumGltf/PropertyTableProperty.h>
#include <CesiumGltf/PropertyTexture.h>
#include <CesiumGltf/PropertyTextureProperty.h>
#include <CesiumGltf/Scene.h>
#include <CesiumGltf/Texture.h>
#include <CesiumUtility/ErrorList.h>

#include <doctest/doctest.h>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/vector_relational.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
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
  std::memcpy(node3.matrix.data(), &childNodeMatrix, sizeof(glm::dmat4));

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

  SUBCASE("Check that the correct primitives are iterated over.") {
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

  SUBCASE("Check the node transform") {
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

  size_t vertexByteStride = sizeof(glm::vec3);
  size_t vertexByteLength = 8 * vertexByteStride;

  Buffer& vertexBuffer = model.buffers.emplace_back();
  vertexBuffer.byteLength = static_cast<int64_t>(vertexByteLength);
  vertexBuffer.cesium.data.resize(vertexByteLength);
  std::memcpy(
      vertexBuffer.cesium.data.data(),
      &cubeVertices[0],
      vertexByteLength);

  BufferView& vertexBufferView = model.bufferViews.emplace_back();
  vertexBufferView.buffer = 0;
  vertexBufferView.byteLength = vertexBuffer.byteLength;
  vertexBufferView.byteOffset = 0;
  vertexBufferView.byteStride = static_cast<int64_t>(vertexByteStride);
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
  SUBCASE("Test normal generation TRIANGLES") {
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

  SUBCASE("Test normal generation for TRIANGLE_STRIP") {
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

  SUBCASE("Test normal generation for TRIANGLE_STRIP") {
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
  SUBCASE("adds a new extension") {
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

  SUBCASE("does not add a duplicate extension") {
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

  SUBCASE("does not also add the extension to extensionsRequired") {
    Model m;
    m.addExtensionUsed("Foo");
    CHECK(m.extensionsRequired.empty());
  }
}

TEST_CASE("Model::addExtensionRequired") {
  SUBCASE("adds a new extension") {
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

  SUBCASE("does not add a duplicate extension") {
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

  SUBCASE("also adds the extension to extensionsUsed if not already present") {
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

TEST_CASE("Model::removeExtensionUsed") {
  SUBCASE("removes an extension") {
    Model m;
    m.extensionsUsed = {"Foo", "Bar"};

    m.removeExtensionUsed("Foo");

    CHECK(m.extensionsUsed == std::vector<std::string>{"Bar"});

    m.removeExtensionUsed("Bar");

    CHECK(m.extensionsUsed.empty());

    m.removeExtensionUsed("Other");

    CHECK(m.extensionsUsed.empty());
  }

  SUBCASE("does not also remove the extension from extensionsRequired") {
    Model m;
    m.extensionsUsed = {"Foo"};
    m.extensionsRequired = {"Foo"};

    m.removeExtensionUsed("Foo");

    CHECK(m.extensionsUsed.empty());
    CHECK(!m.extensionsRequired.empty());
  }
}

TEST_CASE("Model::removeExtensionRequired") {
  SUBCASE("removes an extension") {
    Model m;
    m.extensionsRequired = {"Foo", "Bar"};

    m.removeExtensionRequired("Foo");

    CHECK(m.extensionsRequired == std::vector<std::string>{"Bar"});

    m.removeExtensionRequired("Bar");

    CHECK(m.extensionsRequired.empty());

    m.removeExtensionRequired("Other");

    CHECK(m.extensionsRequired.empty());
  }

  SUBCASE("also removes the extension from extensionsUsed if present") {
    Model m;
    m.extensionsUsed = {"Foo"};
    m.extensionsRequired = {"Foo"};

    m.removeExtensionRequired("Foo");

    CHECK(m.extensionsUsed.empty());
    CHECK(m.extensionsRequired.empty());
  }
}

TEST_CASE("Model::isExtensionUsed") {
  Model m;
  m.extensionsUsed = {"Foo", "Bar"};

  CHECK(m.isExtensionUsed("Foo"));
  CHECK(m.isExtensionUsed("Bar"));
  CHECK_FALSE(m.isExtensionUsed("Baz"));
}

TEST_CASE("Model::isExtensionRequired") {
  Model m;
  m.extensionsRequired = {"Foo", "Bar"};

  CHECK(m.isExtensionRequired("Foo"));
  CHECK(m.isExtensionRequired("Bar"));
  CHECK_FALSE(m.isExtensionRequired("Baz"));
}

TEST_CASE("Model::merge") {
  SUBCASE("performs a simple merge") {
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

  SUBCASE("merges default scenes") {
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
    REQUIRE(size_t(m1.scene) < m1.scenes.size());

    Scene& defaultScene = m1.scenes[size_t(m1.scene)];
    REQUIRE(defaultScene.nodes.size() == 4);
    CHECK(m1.nodes[size_t(defaultScene.nodes[0])].name == "node2");
    CHECK(m1.nodes[size_t(defaultScene.nodes[1])].name == "node1");
    CHECK(m1.nodes[size_t(defaultScene.nodes[2])].name == "node4");
    CHECK(m1.nodes[size_t(defaultScene.nodes[3])].name == "node3");
  }

  SUBCASE("merges metadata") {
    Model m1;
    Model m2;

    SUBCASE("when only this has the extension") {
      ExtensionModelExtStructuralMetadata& metadata1 =
          m1.addExtension<ExtensionModelExtStructuralMetadata>();
      metadata1.schema.emplace().name = "test";

      ErrorList errors = m1.merge(std::move(m2));
      CHECK(errors.errors.empty());
      CHECK(errors.warnings.empty());

      ExtensionModelExtStructuralMetadata* pExtension =
          m1.getExtension<ExtensionModelExtStructuralMetadata>();
      REQUIRE(pExtension);
      REQUIRE(pExtension->schema);
      CHECK(pExtension->schema->name == "test");
    }

    SUBCASE("when only rhs has the extension") {
      ExtensionModelExtStructuralMetadata& metadata2 =
          m2.addExtension<ExtensionModelExtStructuralMetadata>();
      metadata2.schema.emplace().name = "test";

      ErrorList errors = m1.merge(std::move(m2));
      CHECK(errors.errors.empty());
      CHECK(errors.warnings.empty());

      ExtensionModelExtStructuralMetadata* pExtension =
          m1.getExtension<ExtensionModelExtStructuralMetadata>();
      REQUIRE(pExtension);
      REQUIRE(pExtension->schema);
      CHECK(pExtension->schema->name == "test");
    }

    SUBCASE("when both have the extension") {
      ExtensionModelExtStructuralMetadata& metadata1 =
          m1.addExtension<ExtensionModelExtStructuralMetadata>();

      ExtensionModelExtStructuralMetadata& metadata2 =
          m2.addExtension<ExtensionModelExtStructuralMetadata>();

      SUBCASE("and only this has a schema") {
        metadata1.schema.emplace().name = "test";

        ErrorList errors = m1.merge(std::move(m2));
        CHECK(errors.errors.empty());
        CHECK(errors.warnings.empty());

        ExtensionModelExtStructuralMetadata* pExtension =
            m1.getExtension<ExtensionModelExtStructuralMetadata>();
        REQUIRE(pExtension);
        REQUIRE(pExtension->schema);
        CHECK(pExtension->schema->name == "test");
      }

      SUBCASE("and only rhs has a schema") {
        metadata2.schema.emplace().name = "test";

        ErrorList errors = m1.merge(std::move(m2));
        CHECK(errors.errors.empty());
        CHECK(errors.warnings.empty());

        ExtensionModelExtStructuralMetadata* pExtension =
            m1.getExtension<ExtensionModelExtStructuralMetadata>();
        REQUIRE(pExtension);
        REQUIRE(pExtension->schema);
        CHECK(pExtension->schema->name == "test");
      }

      SUBCASE("and both have a schema with different classes") {
        Schema& schema1 = metadata1.schema.emplace();
        Class& class1 = schema1.classes["foo"];
        class1.name = "foo";

        Schema& schema2 = metadata2.schema.emplace();
        Class& class2 = schema2.classes["bar"];
        class2.name = "bar";

        ErrorList errors = m1.merge(std::move(m2));
        CHECK(errors.errors.empty());
        CHECK(errors.warnings.empty());

        // Check that both classes are included in the merged schema.
        ExtensionModelExtStructuralMetadata* pExtension =
            m1.getExtension<ExtensionModelExtStructuralMetadata>();
        REQUIRE(pExtension);
        REQUIRE(pExtension->schema);
        CHECK(pExtension->schema->classes.size() == 2);

        auto it1 = pExtension->schema->classes.find("foo");
        REQUIRE(it1 != pExtension->schema->classes.end());

        auto it2 = pExtension->schema->classes.find("bar");
        REQUIRE(it2 != pExtension->schema->classes.end());
      }

      SUBCASE("and both have a schema with a class with the same name") {
        Schema& schema1 = metadata1.schema.emplace();
        Class& class1 = schema1.classes["foo"];
        class1.name = "foo";

        Schema& schema2 = metadata2.schema.emplace();
        Class& class2 = schema2.classes["foo"];
        class2.name = "foo";

        SUBCASE("it renames the duplicate class") {
          ErrorList errors = m1.merge(std::move(m2));
          CHECK(errors.errors.empty());
          CHECK(errors.warnings.empty());

          ExtensionModelExtStructuralMetadata* pExtension =
              m1.getExtension<ExtensionModelExtStructuralMetadata>();
          REQUIRE(pExtension);
          REQUIRE(pExtension->schema);
          CHECK(pExtension->schema->classes.size() == 2);

          auto it1 = pExtension->schema->classes.find("foo");
          REQUIRE(it1 != pExtension->schema->classes.end());
          CHECK(it1->second.name == "foo");

          auto it2 = pExtension->schema->classes.find("foo_1");
          REQUIRE(it2 != pExtension->schema->classes.end());
          CHECK(it2->second.name == "foo");
        }

        SUBCASE("it updates PropertyTables to reference the renamed class") {
          PropertyTable& propertyTable1 =
              metadata1.propertyTables.emplace_back();
          propertyTable1.classProperty = "foo";

          PropertyTable& propertyTable2 =
              metadata2.propertyTables.emplace_back();
          propertyTable2.classProperty = "foo";

          ErrorList errors = m1.merge(std::move(m2));
          CHECK(errors.errors.empty());
          CHECK(errors.warnings.empty());

          ExtensionModelExtStructuralMetadata* pExtension =
              m1.getExtension<ExtensionModelExtStructuralMetadata>();
          REQUIRE(pExtension);
          REQUIRE(pExtension->schema);

          REQUIRE(pExtension->propertyTables.size() == 2);
          CHECK(pExtension->propertyTables[0].classProperty == "foo");
          CHECK(pExtension->propertyTables[1].classProperty == "foo_1");
        }

        SUBCASE(
            "it updates PropertyAttributes to reference the renamed class") {
          PropertyAttribute& propertyAttribute1 =
              metadata1.propertyAttributes.emplace_back();
          propertyAttribute1.classProperty = "foo";

          PropertyAttribute& propertyAttribute2 =
              metadata2.propertyAttributes.emplace_back();
          propertyAttribute2.classProperty = "foo";

          ErrorList errors = m1.merge(std::move(m2));
          CHECK(errors.errors.empty());
          CHECK(errors.warnings.empty());

          ExtensionModelExtStructuralMetadata* pExtension =
              m1.getExtension<ExtensionModelExtStructuralMetadata>();
          REQUIRE(pExtension);
          REQUIRE(pExtension->schema);

          REQUIRE(pExtension->propertyAttributes.size() == 2);
          CHECK(pExtension->propertyAttributes[0].classProperty == "foo");
          CHECK(pExtension->propertyAttributes[1].classProperty == "foo_1");
        }

        SUBCASE("it updates PropertyTextures to reference the renamed class") {
          PropertyTexture& propertyTexture1 =
              metadata1.propertyTextures.emplace_back();
          propertyTexture1.classProperty = "foo";

          PropertyTexture& propertyTexture2 =
              metadata2.propertyTextures.emplace_back();
          propertyTexture2.classProperty = "foo";

          ErrorList errors = m1.merge(std::move(m2));
          CHECK(errors.errors.empty());
          CHECK(errors.warnings.empty());

          ExtensionModelExtStructuralMetadata* pExtension =
              m1.getExtension<ExtensionModelExtStructuralMetadata>();
          REQUIRE(pExtension);
          REQUIRE(pExtension->schema);

          REQUIRE(pExtension->propertyTextures.size() == 2);
          CHECK(pExtension->propertyTextures[0].classProperty == "foo");
          CHECK(pExtension->propertyTextures[1].classProperty == "foo_1");
        }
      }

      SUBCASE("it updates BufferView indices in PropertyTableProperties") {
        m1.bufferViews.emplace_back().name = "bufferView1";
        m2.bufferViews.emplace_back().name = "bufferView2";

        PropertyTable& propertyTable1 = metadata1.propertyTables.emplace_back();
        PropertyTableProperty& property1 = propertyTable1.properties["foo"];
        property1.values = 0;
        property1.arrayOffsets = 0;
        property1.stringOffsets = 0;

        PropertyTable& propertyTable2 = metadata2.propertyTables.emplace_back();
        PropertyTableProperty& property2 = propertyTable2.properties["foo"];
        property2.values = 0;
        property2.arrayOffsets = 0;
        property2.stringOffsets = 0;

        ErrorList errors = m1.merge(std::move(m2));
        CHECK(errors.errors.empty());
        CHECK(errors.warnings.empty());

        ExtensionModelExtStructuralMetadata* pExtension =
            m1.getExtension<ExtensionModelExtStructuralMetadata>();
        REQUIRE(pExtension);
        REQUIRE(pExtension->propertyTables.size() == 2);
        REQUIRE(m1.bufferViews.size() == 2);
        REQUIRE(m1.bufferViews[0].name == "bufferView1");
        REQUIRE(m1.bufferViews[1].name == "bufferView2");

        REQUIRE(pExtension->propertyTables[0].properties.size() == 1);
        auto it1 = pExtension->propertyTables[0].properties.find("foo");
        REQUIRE(it1 != pExtension->propertyTables[0].properties.end());
        CHECK(it1->second.values == 0);
        CHECK(it1->second.arrayOffsets == 0);
        CHECK(it1->second.stringOffsets == 0);

        REQUIRE(pExtension->propertyTables[1].properties.size() == 1);
        auto it2 = pExtension->propertyTables[1].properties.find("foo");
        REQUIRE(it2 != pExtension->propertyTables[1].properties.end());
        CHECK(it2->second.values == 1);
        CHECK(it2->second.arrayOffsets == 1);
        CHECK(it2->second.stringOffsets == 1);
      }

      SUBCASE("it updates Texture indices in PropertyTextureProperties") {
        m1.textures.emplace_back().name = "texture1";
        m2.textures.emplace_back().name = "texture2";

        PropertyTexture& propertyTexture1 =
            metadata1.propertyTextures.emplace_back();
        PropertyTextureProperty& property1 = propertyTexture1.properties["foo"];
        property1.index = 0;

        PropertyTexture& propertyTexture2 =
            metadata2.propertyTextures.emplace_back();
        PropertyTextureProperty& property2 = propertyTexture2.properties["foo"];
        property2.index = 0;

        ErrorList errors = m1.merge(std::move(m2));
        CHECK(errors.errors.empty());
        CHECK(errors.warnings.empty());

        ExtensionModelExtStructuralMetadata* pExtension =
            m1.getExtension<ExtensionModelExtStructuralMetadata>();
        REQUIRE(pExtension);
        REQUIRE(pExtension->propertyTextures.size() == 2);
        REQUIRE(m1.textures.size() == 2);
        REQUIRE(m1.textures[0].name == "texture1");
        REQUIRE(m1.textures[1].name == "texture2");

        REQUIRE(pExtension->propertyTextures[0].properties.size() == 1);
        auto it1 = pExtension->propertyTextures[0].properties.find("foo");
        REQUIRE(it1 != pExtension->propertyTextures[0].properties.end());
        CHECK(it1->second.index == 0);

        REQUIRE(pExtension->propertyTextures[1].properties.size() == 1);
        auto it2 = pExtension->propertyTextures[1].properties.find("foo");
        REQUIRE(it2 != pExtension->propertyTextures[1].properties.end());
        CHECK(it2->second.index == 1);
      }

      SUBCASE("it updates PropertyTexture indices in primitives") {
        metadata1.propertyTextures.emplace_back().name = "propertyTexture1";
        metadata2.propertyTextures.emplace_back().name = "propertyTexture2";

        Mesh& mesh1 = m1.meshes.emplace_back();
        MeshPrimitive& primitive1 = mesh1.primitives.emplace_back();
        ExtensionMeshPrimitiveExtStructuralMetadata& primitiveMetadata1 =
            primitive1
                .addExtension<ExtensionMeshPrimitiveExtStructuralMetadata>();
        primitiveMetadata1.propertyTextures.push_back(0);

        Mesh& mesh2 = m2.meshes.emplace_back();
        MeshPrimitive& primitive2 = mesh2.primitives.emplace_back();
        ExtensionMeshPrimitiveExtStructuralMetadata& primitiveMetadata2 =
            primitive2
                .addExtension<ExtensionMeshPrimitiveExtStructuralMetadata>();
        primitiveMetadata2.propertyTextures.push_back(0);

        ErrorList errors = m1.merge(std::move(m2));
        CHECK(errors.errors.empty());
        CHECK(errors.warnings.empty());

        REQUIRE(m1.meshes.size() == 2);
        REQUIRE(m1.meshes[0].primitives.size() == 1);
        REQUIRE(m1.meshes[1].primitives.size() == 1);

        ExtensionMeshPrimitiveExtStructuralMetadata* pPrimitiveMetadata1 =
            m1.meshes[0]
                .primitives[0]
                .getExtension<ExtensionMeshPrimitiveExtStructuralMetadata>();
        REQUIRE(pPrimitiveMetadata1);
        REQUIRE(pPrimitiveMetadata1->propertyTextures.size() == 1);
        CHECK(pPrimitiveMetadata1->propertyTextures[0] == 0);

        ExtensionMeshPrimitiveExtStructuralMetadata* pPrimitiveMetadata2 =
            m1.meshes[1]
                .primitives[0]
                .getExtension<ExtensionMeshPrimitiveExtStructuralMetadata>();
        REQUIRE(pPrimitiveMetadata2);
        REQUIRE(pPrimitiveMetadata2->propertyTextures.size() == 1);
        CHECK(pPrimitiveMetadata2->propertyTextures[0] == 1);
      }

      SUBCASE("it updates PropertyAttribute indices in primitives") {
        metadata1.propertyAttributes.emplace_back().name = "propertyAttribute1";
        metadata2.propertyAttributes.emplace_back().name = "propertyAttribute2";

        Mesh& mesh1 = m1.meshes.emplace_back();
        MeshPrimitive& primitive1 = mesh1.primitives.emplace_back();
        ExtensionMeshPrimitiveExtStructuralMetadata& primitiveMetadata1 =
            primitive1
                .addExtension<ExtensionMeshPrimitiveExtStructuralMetadata>();
        primitiveMetadata1.propertyAttributes.push_back(0);

        Mesh& mesh2 = m2.meshes.emplace_back();
        MeshPrimitive& primitive2 = mesh2.primitives.emplace_back();
        ExtensionMeshPrimitiveExtStructuralMetadata& primitiveMetadata2 =
            primitive2
                .addExtension<ExtensionMeshPrimitiveExtStructuralMetadata>();
        primitiveMetadata2.propertyAttributes.push_back(0);

        ErrorList errors = m1.merge(std::move(m2));
        CHECK(errors.errors.empty());
        CHECK(errors.warnings.empty());

        REQUIRE(m1.meshes.size() == 2);
        REQUIRE(m1.meshes[0].primitives.size() == 1);
        REQUIRE(m1.meshes[1].primitives.size() == 1);

        ExtensionMeshPrimitiveExtStructuralMetadata* pPrimitiveMetadata1 =
            m1.meshes[0]
                .primitives[0]
                .getExtension<ExtensionMeshPrimitiveExtStructuralMetadata>();
        REQUIRE(pPrimitiveMetadata1);
        REQUIRE(pPrimitiveMetadata1->propertyAttributes.size() == 1);
        CHECK(pPrimitiveMetadata1->propertyAttributes[0] == 0);

        ExtensionMeshPrimitiveExtStructuralMetadata* pPrimitiveMetadata2 =
            m1.meshes[1]
                .primitives[0]
                .getExtension<ExtensionMeshPrimitiveExtStructuralMetadata>();
        REQUIRE(pPrimitiveMetadata2);
        REQUIRE(pPrimitiveMetadata2->propertyAttributes.size() == 1);
        CHECK(pPrimitiveMetadata2->propertyAttributes[0] == 1);
      }

      SUBCASE("it updates PropertyTable indices in EXT_mesh_features attached "
              "to a primitive") {
        metadata1.propertyTables.emplace_back().name = "propertyTables1";
        metadata2.propertyTables.emplace_back().name = "propertyTables2";

        Mesh& mesh1 = m1.meshes.emplace_back();
        MeshPrimitive& primitive1 = mesh1.primitives.emplace_back();
        ExtensionExtMeshFeatures& meshFeatures1 =
            primitive1.addExtension<ExtensionExtMeshFeatures>();
        meshFeatures1.featureIds.emplace_back().propertyTable = 0;

        Mesh& mesh2 = m2.meshes.emplace_back();
        MeshPrimitive& primitive2 = mesh2.primitives.emplace_back();
        ExtensionExtMeshFeatures& meshFeatures2 =
            primitive2.addExtension<ExtensionExtMeshFeatures>();
        meshFeatures2.featureIds.emplace_back().propertyTable = 0;

        ErrorList errors = m1.merge(std::move(m2));
        CHECK(errors.errors.empty());
        CHECK(errors.warnings.empty());

        REQUIRE(m1.meshes.size() == 2);
        REQUIRE(m1.meshes[0].primitives.size() == 1);
        REQUIRE(m1.meshes[1].primitives.size() == 1);

        ExtensionExtMeshFeatures* pMeshFeatures1 =
            m1.meshes[0].primitives[0].getExtension<ExtensionExtMeshFeatures>();
        REQUIRE(pMeshFeatures1);
        REQUIRE(pMeshFeatures1->featureIds.size() == 1);
        CHECK(pMeshFeatures1->featureIds[0].propertyTable == 0);

        ExtensionExtMeshFeatures* pMeshFeatures2 =
            m1.meshes[1].primitives[0].getExtension<ExtensionExtMeshFeatures>();
        REQUIRE(pMeshFeatures2);
        REQUIRE(pMeshFeatures2->featureIds.size() == 1);
        CHECK(pMeshFeatures2->featureIds[0].propertyTable == 1);
      }

      SUBCASE("it updates Textures indices in EXT_mesh_features attached to a "
              "primitive") {
        m1.textures.emplace_back().name = "texture1";
        m2.textures.emplace_back().name = "texture2";

        Mesh& mesh1 = m1.meshes.emplace_back();
        MeshPrimitive& primitive1 = mesh1.primitives.emplace_back();
        ExtensionExtMeshFeatures& meshFeatures1 =
            primitive1.addExtension<ExtensionExtMeshFeatures>();
        meshFeatures1.featureIds.emplace_back().texture.emplace().index = 0;

        Mesh& mesh2 = m2.meshes.emplace_back();
        MeshPrimitive& primitive2 = mesh2.primitives.emplace_back();
        ExtensionExtMeshFeatures& meshFeatures2 =
            primitive2.addExtension<ExtensionExtMeshFeatures>();
        meshFeatures2.featureIds.emplace_back().texture.emplace().index = 0;

        ErrorList errors = m1.merge(std::move(m2));
        CHECK(errors.errors.empty());
        CHECK(errors.warnings.empty());

        REQUIRE(m1.meshes.size() == 2);
        REQUIRE(m1.meshes[0].primitives.size() == 1);
        REQUIRE(m1.meshes[1].primitives.size() == 1);

        ExtensionExtMeshFeatures* pMeshFeatures1 =
            m1.meshes[0].primitives[0].getExtension<ExtensionExtMeshFeatures>();
        REQUIRE(pMeshFeatures1);
        REQUIRE(pMeshFeatures1->featureIds.size() == 1);
        REQUIRE(pMeshFeatures1->featureIds[0].texture);
        CHECK(pMeshFeatures1->featureIds[0].texture->index == 0);

        ExtensionExtMeshFeatures* pMeshFeatures2 =
            m1.meshes[1].primitives[0].getExtension<ExtensionExtMeshFeatures>();
        REQUIRE(pMeshFeatures2);
        REQUIRE(pMeshFeatures2->featureIds.size() == 1);
        REQUIRE(pMeshFeatures2->featureIds[0].texture);
        CHECK(pMeshFeatures2->featureIds[0].texture->index == 1);
      }
    }
  }

  SUBCASE("updates image index in KHR_texture_basisu") {
    Model m1;
    m1.images.emplace_back();

    Model m2;
    m2.images.emplace_back();
    Texture& texture = m2.textures.emplace_back();
    ExtensionKhrTextureBasisu& extension =
        texture.addExtension<ExtensionKhrTextureBasisu>();
    extension.source = 0;

    m1.merge(std::move(m2));

    REQUIRE(m1.images.size() == 2);
    REQUIRE(m1.textures.size() == 1);

    ExtensionKhrTextureBasisu* pMerged =
        m1.textures[0].getExtension<ExtensionKhrTextureBasisu>();
    REQUIRE(pMerged);
    CHECK(pMerged->source == 1);
  }

  SUBCASE("updates image index in EXT_texture_webp") {
    Model m1;
    m1.images.emplace_back();

    Model m2;
    m2.images.emplace_back();
    Texture& texture = m2.textures.emplace_back();
    ExtensionTextureWebp& extension =
        texture.addExtension<ExtensionTextureWebp>();
    extension.source = 0;

    m1.merge(std::move(m2));

    REQUIRE(m1.images.size() == 2);
    REQUIRE(m1.textures.size() == 1);

    ExtensionTextureWebp* pMerged =
        m1.textures[0].getExtension<ExtensionTextureWebp>();
    REQUIRE(pMerged);
    CHECK(pMerged->source == 1);
  }

  SUBCASE("updates accessor index in CESIUM_primitive_outline") {
    Model m1;
    m1.accessors.emplace_back();

    Model m2;
    m2.accessors.emplace_back();
    MeshPrimitive& primitive =
        m2.meshes.emplace_back().primitives.emplace_back();
    ExtensionCesiumPrimitiveOutline& extension =
        primitive.addExtension<ExtensionCesiumPrimitiveOutline>();
    extension.indices = 0;

    m1.merge(std::move(m2));

    REQUIRE(m1.accessors.size() == 2);
    REQUIRE(m1.meshes.size() == 1);
    REQUIRE(m1.meshes[0].primitives.size() == 1);

    ExtensionCesiumPrimitiveOutline* pMerged =
        m1.meshes[0]
            .primitives[0]
            .getExtension<ExtensionCesiumPrimitiveOutline>();
    REQUIRE(pMerged);
    CHECK(pMerged->indices == 1);
  }

  SUBCASE("updates accessor indices in CESIUM_tile_edges") {
    Model m1;
    m1.accessors.emplace_back();

    Model m2;
    m2.accessors.emplace_back();
    MeshPrimitive& primitive =
        m2.meshes.emplace_back().primitives.emplace_back();
    ExtensionCesiumTileEdges& extension =
        primitive.addExtension<ExtensionCesiumTileEdges>();
    extension.left = 0;
    extension.bottom = 0;
    extension.right = 0;
    extension.top = 0;

    m1.merge(std::move(m2));

    REQUIRE(m1.accessors.size() == 2);
    REQUIRE(m1.meshes.size() == 1);
    REQUIRE(m1.meshes[0].primitives.size() == 1);

    ExtensionCesiumTileEdges* pMerged =
        m1.meshes[0].primitives[0].getExtension<ExtensionCesiumTileEdges>();
    REQUIRE(pMerged);
    CHECK(pMerged->left == 1);
    CHECK(pMerged->bottom == 1);
    CHECK(pMerged->right == 1);
    CHECK(pMerged->top == 1);
  }

  SUBCASE("updates accessor indices in EXT_mesh_gpu_instancing") {
    Model m1;
    m1.accessors.emplace_back();

    Model m2;
    m2.accessors.emplace_back();
    Node& node = m2.nodes.emplace_back();

    ExtensionExtMeshGpuInstancing& extension =
        node.addExtension<ExtensionExtMeshGpuInstancing>();
    extension.attributes["foo"] = 0;

    m1.merge(std::move(m2));

    REQUIRE(m1.accessors.size() == 2);
    REQUIRE(m1.nodes.size() == 1);

    ExtensionExtMeshGpuInstancing* pMerged =
        m1.nodes[0].getExtension<ExtensionExtMeshGpuInstancing>();
    REQUIRE(pMerged);

    auto it = pMerged->attributes.find("foo");
    REQUIRE(it != pMerged->attributes.end());
    CHECK(it->second == 1);
  }

  SUBCASE("updates buffer indices in EXT_meshopt_compression") {
    Model m1;
    m1.buffers.emplace_back();

    Model m2;
    m2.buffers.emplace_back();
    BufferView& bufferView = m2.bufferViews.emplace_back();

    ExtensionBufferViewExtMeshoptCompression& extension =
        bufferView.addExtension<ExtensionBufferViewExtMeshoptCompression>();
    extension.buffer = 0;

    m1.merge(std::move(m2));

    REQUIRE(m1.buffers.size() == 2);
    REQUIRE(m1.bufferViews.size() == 1);

    ExtensionBufferViewExtMeshoptCompression* pMerged =
        m1.bufferViews[0]
            .getExtension<ExtensionBufferViewExtMeshoptCompression>();
    REQUIRE(pMerged);

    CHECK(pMerged->buffer == 1);
  }
}

TEST_CASE("Model::forEachRootNodeInScene") {
  Model m;

  SUBCASE("with scenes and nodes") {
    m.scenes.emplace_back();
    m.scenes.emplace_back();
    m.nodes.emplace_back();
    m.nodes.emplace_back();
    m.nodes.emplace_back();

    m.scenes.front().nodes.push_back(0);
    m.scenes.front().nodes.push_back(2);
    m.scenes.back().nodes.push_back(1);
    m.scenes.back().nodes.push_back(2);

    m.scene = 0;

    SUBCASE("it enumerates a specified scene") {
      std::vector<Node*> visited;
      m.forEachRootNodeInScene(1, [&visited, &m](Model& model, Node& node) {
        CHECK(&m == &model);
        visited.push_back(&node);
      });

      REQUIRE(visited.size() == 2);
      CHECK(visited[0] == &m.nodes[1]);
      CHECK(visited[1] == &m.nodes[2]);
    }

    SUBCASE("it enumerates the default scene") {
      std::vector<Node*> visited;
      m.forEachRootNodeInScene(-1, [&visited, &m](Model& model, Node& node) {
        CHECK(&m == &model);
        visited.push_back(&node);
      });

      REQUIRE(visited.size() == 2);
      CHECK(visited[0] == &m.nodes[0]);
      CHECK(visited[1] == &m.nodes[2]);
    }

    SUBCASE("it enumerates the first scene if there is no default") {
      m.scene = -1;

      std::vector<Node*> visited;
      m.forEachRootNodeInScene(-1, [&visited, &m](Model& model, Node& node) {
        CHECK(&m == &model);
        visited.push_back(&node);
      });

      REQUIRE(visited.size() == 2);
      CHECK(visited[0] == &m.nodes[0]);
      CHECK(visited[1] == &m.nodes[2]);
    }
  }

  SUBCASE("with nodes only") {
    m.nodes.emplace_back();
    m.nodes.emplace_back();
    m.nodes.emplace_back();

    // Check that it enumerates the first node.
    std::vector<Node*> visited;
    m.forEachRootNodeInScene(-1, [&visited, &m](Model& model, Node& node) {
      CHECK(&m == &model);
      visited.push_back(&node);
    });

    REQUIRE(visited.size() == 1);
    CHECK(visited[0] == &m.nodes[0]);
  }

  SUBCASE("with no scenes or nodes") {
    // Check that it enumerates nothing.
    m.forEachRootNodeInScene(-1, [](Model& /* model */, Node& /* node */) {
      // This should not be called.
      CHECK(false);
    });
  }
}

TEST_CASE("Model::forEachNodeInScene") {
  Model m;

  SUBCASE("with scenes and nodes") {
    m.scenes.emplace_back();
    m.scenes.emplace_back();
    m.nodes.emplace_back();
    m.nodes.emplace_back();
    m.nodes.emplace_back();
    m.nodes.emplace_back();

    m.scenes.front().nodes.push_back(0);
    m.scenes.front().nodes.push_back(1);
    m.scenes.back().nodes.push_back(2);

    m.nodes[2].children.push_back(3);

    m.scene = 0;

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

    std::memcpy(
        m.nodes[2].matrix.data(),
        &parentNodeMatrix,
        sizeof(glm::dmat4));
    std::memcpy(m.nodes[3].matrix.data(), &childNodeMatrix, sizeof(glm::dmat4));

    SUBCASE("it enumerates a specified scene") {
      std::vector<Node*> visited;
      m.forEachNodeInScene(
          1,
          [&visited,
           &m](Model& model, Node& node, const glm::dmat4& /* transform */) {
            CHECK(&m == &model);
            visited.push_back(&node);
          });

      REQUIRE(visited.size() == 2);
      CHECK(visited[0] == &m.nodes[2]);
      CHECK(visited[1] == &m.nodes[3]);
    }

    SUBCASE("it enumerates the default scene") {
      std::vector<Node*> visited;
      m.forEachNodeInScene(
          -1,
          [&visited,
           &m](Model& model, Node& node, const glm::dmat4& /* transform */) {
            CHECK(&m == &model);
            visited.push_back(&node);
          });

      REQUIRE(visited.size() == 2);
      CHECK(visited[0] == &m.nodes[0]);
      CHECK(visited[1] == &m.nodes[1]);
    }

    SUBCASE("it enumerates the first scene if there is no default") {
      m.scene = -1;

      std::vector<Node*> visited;
      m.forEachNodeInScene(
          -1,
          [&visited,
           &m](Model& model, Node& node, const glm::dmat4& /* transform */) {
            CHECK(&m == &model);
            visited.push_back(&node);
          });

      REQUIRE(visited.size() == 2);
      CHECK(visited[0] == &m.nodes[0]);
      CHECK(visited[1] == &m.nodes[1]);
    }

    SUBCASE("check the node transforms") {
      std::vector<glm::dmat4> transforms;
      m.forEachNodeInScene(
          1,
          [&transforms](
              Model& /* model */,
              Node& /* node */,
              const glm::dmat4& transform) {
            transforms.push_back(transform);
          });

      REQUIRE(transforms.size() == 2);
      CHECK(transforms[0] == parentNodeMatrix);
      CHECK(transforms[1] == expectedNodeTransform);
    }
  }

  SUBCASE("with nodes only") {
    m.nodes.emplace_back();
    m.nodes.emplace_back();
    m.nodes.emplace_back();

    // Check that it enumerates the first node.
    std::vector<Node*> visited;
    m.forEachNodeInScene(
        -1,
        [&visited,
         &m](Model& model, Node& node, const glm::dmat4& /* transform */) {
          CHECK(&m == &model);
          visited.push_back(&node);
        });

    REQUIRE(visited.size() == 1);
    CHECK(visited[0] == &m.nodes[0]);
  }

  SUBCASE("with no scenes or nodes") {
    // Check that it enumerates nothing.
    m.forEachNodeInScene(
        -1,
        [](Model& /* model */,
           Node& /* node */,
           const glm::dmat4& /* transform */) {
          // This should not be called.
          CHECK(false);
        });
  }
}
