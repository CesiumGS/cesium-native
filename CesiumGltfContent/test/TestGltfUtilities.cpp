#include <CesiumGltf/ExtensionBufferExtMeshoptCompression.h>
#include <CesiumGltf/ExtensionBufferViewExtMeshoptCompression.h>
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

TEST_CASE("GltfUtilities::removeUnusedBuffers") {
  Model m;

  SECTION("removes unused") {
    m.buffers.emplace_back();
    GltfUtilities::removeUnusedBuffers(m);
    CHECK(m.buffers.empty());
  }

  SECTION("does not removed used") {
    m.buffers.emplace_back();
    m.bufferViews.emplace_back().buffer = 0;
    GltfUtilities::removeUnusedBuffers(m);
    CHECK(!m.buffers.empty());
  }

  SECTION("does not remove buffer used by EXT_meshopt_compression") {
    m.buffers.emplace_back();
    m.bufferViews.emplace_back()
        .addExtension<ExtensionBufferViewExtMeshoptCompression>()
        .buffer = 0;
    GltfUtilities::removeUnusedBuffers(m);
    CHECK(!m.buffers.empty());
  }

  SECTION("updates indices when removing") {
    m.buffers.emplace_back();
    m.buffers.emplace_back();

    m.bufferViews.emplace_back().buffer = 1;

    GltfUtilities::removeUnusedBuffers(m);
    CHECK(m.buffers.size() == 1);

    REQUIRE(m.bufferViews.size() == 1);
    CHECK(m.bufferViews[0].buffer == 0);
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

    CHECK(buffer.byteLength == 123 - 8);
    REQUIRE(buffer.cesium.data.size() == 123 - 8);
    CHECK(bv.byteOffset == 2);

    for (size_t i = size_t(bv.byteOffset); i < buffer.cesium.data.size(); ++i) {
      CHECK(buffer.cesium.data[i] == std::byte(i + 8));
    }
  }

  SECTION("removes unused bytes at the end of the buffer") {
    BufferView& bv = m.bufferViews.emplace_back();
    bv.buffer = 0;
    bv.byteOffset = 0;
    bv.byteLength = 113;

    GltfUtilities::compactBuffers(m);

    // Any number of bytes can be removed from the end (no alignment impact)
    CHECK(buffer.byteLength == 123 - 10);
    REQUIRE(buffer.cesium.data.size() == 123 - 10);

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

    CHECK(buffer.byteLength == 123 - 8 - 10);
    REQUIRE(buffer.cesium.data.size() == 123 - 8 - 10);
    CHECK(bv.byteOffset == 2);

    for (size_t i = 2; i < buffer.cesium.data.size(); ++i) {
      CHECK(buffer.cesium.data[i] == std::byte(i + 8));
    }
  }

  SECTION("does not remove gaps less than 8 bytes") {
    BufferView& bv1 = m.bufferViews.emplace_back();
    bv1.buffer = 0;
    bv1.byteOffset = 1;
    bv1.byteLength = 99;

    BufferView& bv2 = m.bufferViews.emplace_back();
    bv2.buffer = 0;
    bv2.byteOffset = 105;
    bv2.byteLength = 10;

    GltfUtilities::compactBuffers(m);

    CHECK(buffer.byteLength == 115);
    REQUIRE(buffer.cesium.data.size() == 115);
    CHECK(m.bufferViews[0].byteOffset == 1);
    CHECK(m.bufferViews[1].byteOffset == 105);

    for (size_t i = 0; i < buffer.cesium.data.size(); ++i) {
      CHECK(buffer.cesium.data[i] == std::byte(i));
    }
  }

  SECTION("counts meshopt bufferViews when determining used byte ranges") {
    BufferView& bv = m.bufferViews.emplace_back();
    bv.buffer = 0;
    bv.byteOffset = 0;
    bv.byteLength = 100;

    ExtensionBufferViewExtMeshoptCompression& extension =
        bv.addExtension<ExtensionBufferViewExtMeshoptCompression>();
    extension.buffer = 0;
    extension.byteOffset = 100;
    extension.byteLength = 13;

    GltfUtilities::compactBuffers(m);

    // Any number of bytes can be removed from the end (no alignment impact)
    CHECK(buffer.byteLength == 123 - 10);
    REQUIRE(buffer.cesium.data.size() == 123 - 10);

    for (size_t i = 0; i < buffer.cesium.data.size(); ++i) {
      CHECK(buffer.cesium.data[i] == std::byte(i));
    }
  }
}

TEST_CASE("GltfUtilities::collapseToSingleBuffer") {
  SECTION("merges two buffers into one") {
    Model m;
    m.buffers.emplace_back();
    m.buffers.emplace_back();
    m.bufferViews.emplace_back();
    m.bufferViews.emplace_back();

    Buffer& buffer1 = m.buffers[0];
    buffer1.byteLength = 10;
    buffer1.cesium.data.resize(10, std::byte('1'));

    Buffer& buffer2 = m.buffers[1];
    buffer2.byteLength = 12;
    buffer2.cesium.data.resize(12, std::byte('2'));

    BufferView& bufferView1 = m.bufferViews[0];
    bufferView1.buffer = 1;
    bufferView1.byteLength = 12;
    BufferView& bufferView2 = m.bufferViews[1];
    bufferView2.buffer = 0;
    bufferView2.byteLength = 10;

    GltfUtilities::collapseToSingleBuffer(m);

    REQUIRE(m.buffers.size() == 1);
    CHECK(m.bufferViews[0].buffer == 0);
    // expect 8-byte alignment
    CHECK(m.bufferViews[0].byteOffset == 16);
    CHECK(m.bufferViews[0].byteLength == 12);
    CHECK(m.bufferViews[1].buffer == 0);
    CHECK(m.bufferViews[1].byteOffset == 0);
    CHECK(m.bufferViews[1].byteLength == 10);
  }

  SECTION("leaves buffer with a URI and no data intact") {
    Model m;
    m.buffers.emplace_back();
    m.buffers.emplace_back();
    m.buffers.emplace_back();
    m.bufferViews.emplace_back();
    m.bufferViews.emplace_back();
    m.bufferViews.emplace_back();

    Buffer& buffer1 = m.buffers[0];
    buffer1.byteLength = 10;
    buffer1.cesium.data.resize(10, std::byte('1'));

    Buffer& buffer2 = m.buffers[1];
    buffer2.byteLength = 100;
    buffer2.uri = "foo";

    Buffer& buffer3 = m.buffers[2];
    buffer3.byteLength = 12;
    buffer3.cesium.data.resize(12, std::byte('2'));

    BufferView& bufferView1 = m.bufferViews[0];
    bufferView1.buffer = 2;
    bufferView1.byteLength = 12;
    BufferView& bufferView2 = m.bufferViews[1];
    bufferView2.buffer = 0;
    bufferView2.byteLength = 10;
    BufferView& bufferView3 = m.bufferViews[2];
    bufferView3.buffer = 1;
    bufferView3.byteLength = 100;

    GltfUtilities::collapseToSingleBuffer(m);

    REQUIRE(m.buffers.size() == 2);
    CHECK(m.bufferViews[0].buffer == 0);
    // expect 8-byte alignment
    CHECK(m.bufferViews[0].byteOffset == 16);
    CHECK(m.bufferViews[0].byteLength == 12);
    CHECK(m.bufferViews[1].buffer == 0);
    CHECK(m.bufferViews[1].byteOffset == 0);
    CHECK(m.bufferViews[1].byteLength == 10);
    CHECK(m.bufferViews[2].buffer == 1);
    CHECK(m.bufferViews[2].byteLength == 100);
  }

  SECTION("leaves a meshopt fallback buffer with no data intact even if it has "
          "no URI") {
    Model m;
    m.buffers.emplace_back();
    m.buffers.emplace_back();
    m.buffers.emplace_back();
    m.bufferViews.emplace_back();
    m.bufferViews.emplace_back();
    m.bufferViews.emplace_back();

    Buffer& buffer1 = m.buffers[0];
    buffer1.byteLength = 10;
    buffer1.cesium.data.resize(10, std::byte('1'));

    Buffer& buffer2 = m.buffers[1];
    buffer2.byteLength = 100;
    ExtensionBufferExtMeshoptCompression& extension =
        buffer2.addExtension<ExtensionBufferExtMeshoptCompression>();
    extension.fallback = true;

    Buffer& buffer3 = m.buffers[2];
    buffer3.byteLength = 12;
    buffer3.cesium.data.resize(12, std::byte('2'));

    BufferView& bufferView1 = m.bufferViews[0];
    bufferView1.buffer = 2;
    bufferView1.byteLength = 12;
    BufferView& bufferView2 = m.bufferViews[1];
    bufferView2.buffer = 0;
    bufferView2.byteLength = 10;
    BufferView& bufferView3 = m.bufferViews[2];
    bufferView3.buffer = 1;
    bufferView3.byteLength = 100;

    GltfUtilities::collapseToSingleBuffer(m);

    REQUIRE(m.buffers.size() == 2);
    CHECK(m.buffers[1].hasExtension<ExtensionBufferExtMeshoptCompression>());
    CHECK(m.bufferViews[0].buffer == 0);
    // expect 8-byte alignment
    CHECK(m.bufferViews[0].byteOffset == 16);
    CHECK(m.bufferViews[0].byteLength == 12);
    CHECK(m.bufferViews[1].buffer == 0);
    CHECK(m.bufferViews[1].byteOffset == 0);
    CHECK(m.bufferViews[1].byteLength == 10);
    CHECK(m.bufferViews[2].buffer == 1);
    CHECK(m.bufferViews[2].byteLength == 100);
  }
}
