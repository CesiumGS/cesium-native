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
