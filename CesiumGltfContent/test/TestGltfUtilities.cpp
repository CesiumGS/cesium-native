#include <CesiumGltf/Model.h>
#include <CesiumGltf/Node.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumUtility/Math.h>

#include <catch2/catch.hpp>
#include <glm/gtx/quaternion.hpp>

using namespace CesiumGltf;
using namespace CesiumGltfContent;
using namespace CesiumUtility;

TEST_CASE("GltfUtilities::getNodeTransform") {
  Node node;

  SECTION("gets matrix if it has 16 elements") {
    // clang-format off
    node.matrix = {
      1.0, 2.0, 3.0, 4.0,
      5.0, 6.0, 7.0, 8.0,
      9.0, 10.0, 11.0, 12.0,
      13.0, 14.0, 15.0, 16.0};
    // clang-format on

    std::optional<glm::dmat4x4> maybeMatrix =
        GltfUtilities::getNodeTransform(node);
    REQUIRE(maybeMatrix);

    glm::dmat4x4 m = *maybeMatrix;
    CHECK(m[0] == glm::dvec4(1.0, 2.0, 3.0, 4.0));
    CHECK(m[1] == glm::dvec4(5.0, 6.0, 7.0, 8.0));
    CHECK(m[2] == glm::dvec4(9.0, 10.0, 11.0, 12.0));
    CHECK(m[3] == glm::dvec4(13.0, 14.0, 15.0, 16.0));
  }

  SECTION("gets matrix if it has more than 16 elements (ignoring extras)") {
    // clang-format off
    node.matrix = {
      1.0, 2.0, 3.0, 4.0,
      5.0, 6.0, 7.0, 8.0,
      9.0, 10.0, 11.0, 12.0,
      13.0, 14.0, 15.0, 16.0,
      17.0};
    // clang-format on

    std::optional<glm::dmat4x4> maybeMatrix =
        GltfUtilities::getNodeTransform(node);
    REQUIRE(maybeMatrix);

    glm::dmat4x4 m = *maybeMatrix;
    CHECK(m[0] == glm::dvec4(1.0, 2.0, 3.0, 4.0));
    CHECK(m[1] == glm::dvec4(5.0, 6.0, 7.0, 8.0));
    CHECK(m[2] == glm::dvec4(9.0, 10.0, 11.0, 12.0));
    CHECK(m[3] == glm::dvec4(13.0, 14.0, 15.0, 16.0));
  }

  SECTION("return std::nullopt if matrix has too few elements") {
    // clang-format off
    node.matrix = {
      1.0, 2.0, 3.0, 4.0,
      5.0, 6.0, 7.0, 8.0,
      9.0, 10.0, 11.0, 12.0,
      13.0, 14.0, 15.0};
    // clang-format on

    std::optional<glm::dmat4x4> maybeMatrix =
        GltfUtilities::getNodeTransform(node);
    REQUIRE(!maybeMatrix);
  }

  SECTION("gets translation/rotation/scale if matrix is not specified") {
    glm::dquat ninetyDegreesAboutX =
        glm::angleAxis(Math::degreesToRadians(90.0), glm::dvec3(1.0, 0.0, 0.0));

    node.translation = {1.0, 2.0, 3.0};
    node.rotation = {
        ninetyDegreesAboutX.x,
        ninetyDegreesAboutX.y,
        ninetyDegreesAboutX.z,
        ninetyDegreesAboutX.w};
    node.scale = {2.0, 4.0, 8.0};

    std::optional<glm::dmat4x4> maybeMatrix =
        GltfUtilities::getNodeTransform(node);
    REQUIRE(maybeMatrix);

    glm::dmat4x4 m = *maybeMatrix;

    // glTF Spec section 5.25:
    // TRS properties are converted to matrices and postmultiplied in the
    // `T * R * S` order to compose the transformation matrix; first the scale
    // is applied to the vertices, then the rotation, and then the translation.
    glm::dvec4 someVector(10.0, 20.0, 30.0, 1.0);
    glm::dvec4 transformed = m * someVector;

    glm::dvec4 expectedAfterScaling(
        someVector.x * 2.0,
        someVector.y * 4.0,
        someVector.z * 8.0,
        1.0);
    glm::dvec4 expectedAfterRotating(
        expectedAfterScaling.x,
        -expectedAfterScaling.z,
        expectedAfterScaling.y,
        1.0);
    glm::dvec4 expectedAfterTranslating(
        expectedAfterRotating.x + 1.0,
        expectedAfterRotating.y + 2.0,
        expectedAfterRotating.z + 3.0,
        1.0);

    CHECK(Math::equalsEpsilon(
        transformed,
        expectedAfterTranslating,
        Math::Epsilon14));
  }

  SECTION("returns std::nullopt if translation has too few elements") {
    node.translation = {1.0, 2.0};
    std::optional<glm::dmat4x4> maybeMatrix =
        GltfUtilities::getNodeTransform(node);
    REQUIRE(!maybeMatrix);
  }

  SECTION("returns std::nullopt if rotation has too few elements") {
    node.rotation = {1.0, 2.0, 3.0};
    std::optional<glm::dmat4x4> maybeMatrix =
        GltfUtilities::getNodeTransform(node);
    REQUIRE(!maybeMatrix);
  }

  SECTION("returns std::nullopt if scale has too few elements") {
    node.scale = {1.0, 2.0};
    std::optional<glm::dmat4x4> maybeMatrix =
        GltfUtilities::getNodeTransform(node);
    REQUIRE(!maybeMatrix);
  }
}

TEST_CASE("GltfUtilities::setNodeTransform") {
  Node node;

  SECTION("sets matrix") {
    glm::dmat4x4 m(
        glm::dvec4(1.0, 2.0, 3.0, 4.0),
        glm::dvec4(5.0, 6.0, 7.0, 8.0),
        glm::dvec4(9.0, 10.0, 11.0, 12.0),
        glm::dvec4(13.0, 14.0, 15.0, 16.0));

    GltfUtilities::setNodeTransform(node, m);

    REQUIRE(node.matrix.size() == 16);
    CHECK(node.matrix[0] == 1.0);
    CHECK(node.matrix[1] == 2.0);
    CHECK(node.matrix[2] == 3.0);
    CHECK(node.matrix[3] == 4.0);
    CHECK(node.matrix[4] == 5.0);
    CHECK(node.matrix[5] == 6.0);
    CHECK(node.matrix[6] == 7.0);
    CHECK(node.matrix[7] == 8.0);
    CHECK(node.matrix[8] == 9.0);
    CHECK(node.matrix[9] == 10.0);
    CHECK(node.matrix[10] == 11.0);
    CHECK(node.matrix[11] == 12.0);
    CHECK(node.matrix[12] == 13.0);
    CHECK(node.matrix[13] == 14.0);
    CHECK(node.matrix[14] == 15.0);
    CHECK(node.matrix[15] == 16.0);
  }

  SECTION("resets translation/rotation/scale to identity") {
    node.translation = {1.0, 2.0, 3.0};
    node.rotation = {3.0, 6.0, 9.0, 12.0};
    node.scale = {2.0, 4.0, 8.0};

    glm::dmat4x4 m(
        glm::dvec4(1.0, 2.0, 3.0, 4.0),
        glm::dvec4(5.0, 6.0, 7.0, 8.0),
        glm::dvec4(9.0, 10.0, 11.0, 12.0),
        glm::dvec4(13.0, 14.0, 15.0, 16.0));

    GltfUtilities::setNodeTransform(node, m);
    REQUIRE(node.matrix.size() == 16);
    REQUIRE(node.translation.size() == 3);
    REQUIRE(node.rotation.size() == 4);
    REQUIRE(node.scale.size() == 3);

    CHECK(node.translation[0] == 0.0);
    CHECK(node.translation[1] == 0.0);
    CHECK(node.translation[2] == 0.0);

    CHECK(node.rotation[0] == 0.0);
    CHECK(node.rotation[1] == 0.0);
    CHECK(node.rotation[2] == 0.0);
    CHECK(node.rotation[3] == 1.0);

    CHECK(node.scale[0] == 1.0);
    CHECK(node.scale[1] == 1.0);
    CHECK(node.scale[2] == 1.0);
  }
}

TEST_CASE("GltfUtilities::removeUnusedTextures") {
  Model m;

  SECTION("removes unused") {
    m.textures.emplace_back();
    GltfUtilities::removeUnusedTextures(m);
    CHECK(m.textures.empty());
  }

  SECTION("does not remove used") {
    m.textures.emplace_back();
    m.materials.emplace_back()
        .pbrMetallicRoughness.emplace()
        .baseColorTexture.emplace()
        .index = 0;
    GltfUtilities::removeUnusedTextures(m);
    CHECK(!m.textures.empty());
  }

  SECTION("updates indices when removing") {
    m.textures.emplace_back();
    m.textures.emplace_back();

    m.materials.emplace_back()
        .pbrMetallicRoughness.emplace()
        .baseColorTexture.emplace()
        .index = 1;

    GltfUtilities::removeUnusedTextures(m);
    CHECK(m.textures.size() == 1);

    REQUIRE(m.materials.size() == 1);
    REQUIRE(m.materials[0].pbrMetallicRoughness);
    REQUIRE(m.materials[0].pbrMetallicRoughness->baseColorTexture);
    CHECK(m.materials[0].pbrMetallicRoughness->baseColorTexture->index == 0);
  }
}

TEST_CASE("GltfUtilities::removeUnusedSamplers") {
  Model m;

  SECTION("removes unused") {
    m.samplers.emplace_back();
    GltfUtilities::removeUnusedSamplers(m);
    CHECK(m.samplers.empty());
  }

  SECTION("does not removed used") {
    m.samplers.emplace_back();
    m.textures.emplace_back().sampler = 0;
    GltfUtilities::removeUnusedSamplers(m);
    CHECK(!m.samplers.empty());
  }

  SECTION("updates indices when removing") {
    m.samplers.emplace_back();
    m.samplers.emplace_back();

    m.textures.emplace_back().sampler = 1;

    GltfUtilities::removeUnusedSamplers(m);
    CHECK(m.samplers.size() == 1);

    REQUIRE(m.textures.size() == 1);
    CHECK(m.textures[0].sampler == 0);
  }
}

TEST_CASE("GltfUtilities::removeUnusedImages") {
  Model m;

  SECTION("removes unused") {
    m.images.emplace_back();
    GltfUtilities::removeUnusedImages(m);
    CHECK(m.images.empty());
  }

  SECTION("does not removed used") {
    m.images.emplace_back();
    m.textures.emplace_back().source = 0;
    GltfUtilities::removeUnusedImages(m);
    CHECK(!m.images.empty());
  }

  SECTION("updates indices when removing") {
    m.images.emplace_back();
    m.images.emplace_back();

    m.textures.emplace_back().source = 1;

    GltfUtilities::removeUnusedImages(m);
    CHECK(m.images.size() == 1);

    REQUIRE(m.textures.size() == 1);
    CHECK(m.textures[0].source == 0);
  }
}

TEST_CASE("GltfUtilities::removeUnusedAccessors") {
  Model m;

  SECTION("removes unused") {
    m.accessors.emplace_back();
    GltfUtilities::removeUnusedAccessors(m);
    CHECK(m.accessors.empty());
  }

  SECTION("does not removed used") {
    m.accessors.emplace_back();
    m.meshes.emplace_back().primitives.emplace_back().attributes["POSITION"] =
        0;
    GltfUtilities::removeUnusedAccessors(m);
    CHECK(!m.accessors.empty());
  }

  SECTION("updates indices when removing") {
    m.accessors.emplace_back();
    m.accessors.emplace_back();

    m.meshes.emplace_back().primitives.emplace_back().attributes["POSITION"] =
        1;

    GltfUtilities::removeUnusedAccessors(m);
    CHECK(m.accessors.size() == 1);

    REQUIRE(m.meshes.size() == 1);
    REQUIRE(m.meshes[0].primitives.size() == 1);

    auto it = m.meshes[0].primitives[0].attributes.find("POSITION");
    REQUIRE(it != m.meshes[0].primitives[0].attributes.end());
    CHECK(it->second == 0);
  }
}

TEST_CASE("GltfUtilities::removeUnusedBufferViews") {
  Model m;

  SECTION("removes unused") {
    m.bufferViews.emplace_back();
    GltfUtilities::removeUnusedBufferViews(m);
    CHECK(m.bufferViews.empty());
  }

  SECTION("does not removed used") {
    m.bufferViews.emplace_back();
    m.accessors.emplace_back().bufferView = 0;
    GltfUtilities::removeUnusedBufferViews(m);
    CHECK(!m.bufferViews.empty());
  }

  SECTION("updates indices when removing") {
    m.bufferViews.emplace_back();
    m.bufferViews.emplace_back();

    m.accessors.emplace_back().bufferView = 1;

    GltfUtilities::removeUnusedBufferViews(m);
    CHECK(m.bufferViews.size() == 1);

    REQUIRE(m.accessors.size() == 1);
    CHECK(m.accessors[0].bufferView == 0);
  }
}

TEST_CASE("GltfUtilities::compactBuffers") {
  Model m;

  Buffer& buffer = m.buffers.emplace_back();
  buffer.byteLength = 123;
  buffer.cesium.data.resize(123);

  for (size_t i = 0; i < buffer.cesium.data.size(); ++i) {
    buffer.cesium.data[i] = std::byte(i);
  }

  SECTION("removes unused bytes at the beginning of the buffer") {
    BufferView& bv = m.bufferViews.emplace_back();
    bv.buffer = 0;
    bv.byteOffset = 10;
    bv.byteLength = buffer.byteLength - bv.byteOffset;

    GltfUtilities::compactBuffers(m);

    CHECK(buffer.byteLength == 113);
    REQUIRE(buffer.cesium.data.size() == 113);
    CHECK(bv.byteOffset == 0);

    for (size_t i = 0; i < buffer.cesium.data.size(); ++i) {
      CHECK(buffer.cesium.data[i] == std::byte(i + 10));
    }
  }

  SECTION("removes unused bytes at the end of the buffer") {
    BufferView& bv = m.bufferViews.emplace_back();
    bv.buffer = 0;
    bv.byteOffset = 0;
    bv.byteLength = 113;

    GltfUtilities::compactBuffers(m);

    CHECK(buffer.byteLength == 113);
    REQUIRE(buffer.cesium.data.size() == 113);

    for (size_t i = 0; i < buffer.cesium.data.size(); ++i) {
      CHECK(buffer.cesium.data[i] == std::byte(i));
    }
  }

  SECTION("removes unused bytes in the middle of the buffer") {
    BufferView& bv = m.bufferViews.emplace_back();
    bv.buffer = 0;
    bv.byteOffset = 10;
    bv.byteLength = buffer.byteLength - bv.byteOffset - 10;

    GltfUtilities::compactBuffers(m);

    CHECK(buffer.byteLength == 103);
    REQUIRE(buffer.cesium.data.size() == 103);
    CHECK(bv.byteOffset == 0);

    for (size_t i = 0; i < buffer.cesium.data.size(); ++i) {
      CHECK(buffer.cesium.data[i] == std::byte(i + 10));
    }
  }
}

TEST_CASE("GltfUtilities::parseGltfCopyright") {
  SECTION("properly parses multiple copyright entries") {
    Model model;
    model.asset.copyright = "Test;a;b;c";
    std::vector<std::string_view> result =
        GltfUtilities::parseGltfCopyright(model);

    REQUIRE(result.size() == 4);
    CHECK(result[0] == "Test");
    CHECK(result[1] == "a");
    CHECK(result[2] == "b");
    CHECK(result[3] == "c");
  }

  SECTION("properly parses a single copyright entry") {
    Model model;
    model.asset.copyright = "Test";
    std::vector<std::string_view> result =
        GltfUtilities::parseGltfCopyright(model);

    REQUIRE(result.size() == 1);
    CHECK(result[0] == "Test");
  }

  SECTION("properly parses an entry with a trailing semicolon") {
    Model model;
    model.asset.copyright = "Test;a;b;c;";
    std::vector<std::string_view> result =
        GltfUtilities::parseGltfCopyright(model);

    REQUIRE(result.size() == 4);
    CHECK(result[0] == "Test");
    CHECK(result[1] == "a");
    CHECK(result[2] == "b");
    CHECK(result[3] == "c");
  }

  SECTION("properly parses entries with whitespace") {
    Model model;
    model.asset.copyright = "\tTest;a\t ;\tb;\t \tc\t;\t ";
    std::vector<std::string_view> result =
        GltfUtilities::parseGltfCopyright(model);

    REQUIRE(result.size() == 4);
    CHECK(result[0] == "Test");
    CHECK(result[1] == "a");
    CHECK(result[2] == "b");
    CHECK(result[3] == "c");
  }

  SECTION("properly parses an empty string") {
    Model model;
    model.asset.copyright = "";
    std::vector<std::string_view> result =
        GltfUtilities::parseGltfCopyright(model);

    REQUIRE(result.size() == 0);
  }

  SECTION("properly parses whitespace only") {
    Model model;
    model.asset.copyright = " \t  \t";
    std::vector<std::string_view> result =
        GltfUtilities::parseGltfCopyright(model);

    REQUIRE(result.size() == 0);
  }

  SECTION("properly parses empty parts in the middle") {
    Model model;
    model.asset.copyright = "a;;b";
    std::vector<std::string_view> result =
        GltfUtilities::parseGltfCopyright(model);

    REQUIRE(result.size() == 2);
    CHECK(result[0] == "a");
    CHECK(result[1] == "b");
  }

  SECTION("properly parses whitespace parts in the middle") {
    Model model;
    model.asset.copyright = "a;\t;b";
    std::vector<std::string_view> result =
        GltfUtilities::parseGltfCopyright(model);

    REQUIRE(result.size() == 2);
    CHECK(result[0] == "a");
    CHECK(result[1] == "b");
  }

  SECTION("properly parses empty parts at the start") {
    Model model;
    model.asset.copyright = ";a;b";
    std::vector<std::string_view> result =
        GltfUtilities::parseGltfCopyright(model);

    REQUIRE(result.size() == 2);
    CHECK(result[0] == "a");
    CHECK(result[1] == "b");
  }

  SECTION("properly parses whitespace parts at the start") {
    Model model;
    model.asset.copyright = "\t;a;b";
    std::vector<std::string_view> result =
        GltfUtilities::parseGltfCopyright(model);

    REQUIRE(result.size() == 2);
    CHECK(result[0] == "a");
    CHECK(result[1] == "b");
  }

  SECTION("properly parses empty parts at the end") {
    Model model;
    model.asset.copyright = "a;b;";
    std::vector<std::string_view> result =
        GltfUtilities::parseGltfCopyright(model);

    REQUIRE(result.size() == 2);
    CHECK(result[0] == "a");
    CHECK(result[1] == "b");
  }

  SECTION("properly parses whitespace parts at the end") {
    Model model;
    model.asset.copyright = "a;b;\t";
    std::vector<std::string_view> result =
        GltfUtilities::parseGltfCopyright(model);

    REQUIRE(result.size() == 2);
    CHECK(result[0] == "a");
    CHECK(result[1] == "b");
  }
}
