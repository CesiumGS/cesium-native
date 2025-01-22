#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/Transforms.h>

#include <doctest/doctest.h>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/vector_double4.hpp>

using namespace CesiumGeometry;

TEST_CASE("Transforms convert the axes correctly") {

  glm::dvec4 X_AXIS{1.0, 0.0, 0.0, 0.0};
  glm::dvec4 Y_AXIS{0.0, 1.0, 0.0, 0.0};
  glm::dvec4 Z_AXIS{0.0, 0.0, 1.0, 0.0};

  SUBCASE("Y_UP_TO_Z_UP transforms  X to  X,  Y to -Z, and  Z to Y") {
    REQUIRE(X_AXIS * Transforms::Y_UP_TO_Z_UP == X_AXIS);
    REQUIRE(Y_AXIS * Transforms::Y_UP_TO_Z_UP == -Z_AXIS);
    REQUIRE(Z_AXIS * Transforms::Y_UP_TO_Z_UP == Y_AXIS);
  }
  SUBCASE("Z_UP_TO_Y_UP transforms  X to  X,  Y to  Z, and  Z to -Y") {
    REQUIRE(X_AXIS * Transforms::Z_UP_TO_Y_UP == X_AXIS);
    REQUIRE(Y_AXIS * Transforms::Z_UP_TO_Y_UP == Z_AXIS);
    REQUIRE(Z_AXIS * Transforms::Z_UP_TO_Y_UP == -Y_AXIS);
  }

  SUBCASE("X_UP_TO_Z_UP transforms  X to -Z,  Y to  Y, and  Z to  X") {
    REQUIRE(X_AXIS * Transforms::X_UP_TO_Z_UP == -Z_AXIS);
    REQUIRE(Y_AXIS * Transforms::X_UP_TO_Z_UP == Y_AXIS);
    REQUIRE(Z_AXIS * Transforms::X_UP_TO_Z_UP == X_AXIS);
  }
  SUBCASE("Z_UP_TO_X_UP transforms  X to  Z,  Y to  Y, and  Z to -X") {
    REQUIRE(X_AXIS * Transforms::Z_UP_TO_X_UP == Z_AXIS);
    REQUIRE(Y_AXIS * Transforms::Z_UP_TO_X_UP == Y_AXIS);
    REQUIRE(Z_AXIS * Transforms::Z_UP_TO_X_UP == -X_AXIS);
  }

  SUBCASE("X_UP_TO_Y_UP transforms  X to -Y,  Y to  X, and  Z to  Z") {
    REQUIRE(X_AXIS * Transforms::X_UP_TO_Y_UP == -Y_AXIS);
    REQUIRE(Y_AXIS * Transforms::X_UP_TO_Y_UP == X_AXIS);
    REQUIRE(Z_AXIS * Transforms::X_UP_TO_Y_UP == Z_AXIS);
  }
  SUBCASE("Y_UP_TO_X_UP transforms  X to  Y,  Y to -X, and  Z to  Z") {
    REQUIRE(X_AXIS * Transforms::Y_UP_TO_X_UP == Y_AXIS);
    REQUIRE(Y_AXIS * Transforms::Y_UP_TO_X_UP == -X_AXIS);
    REQUIRE(Z_AXIS * Transforms::Y_UP_TO_X_UP == Z_AXIS);
  }
}

TEST_CASE("Gets up axis transform") {
  const glm::dmat4 Identity(1.0);

  SUBCASE("Gets X-up to X-up transform") {
    CHECK(Transforms::getUpAxisTransform(Axis::X, Axis::X) == Identity);
  }

  SUBCASE("Gets X-up to Y-up transform") {
    CHECK(
        Transforms::getUpAxisTransform(Axis::X, Axis::Y) ==
        Transforms::X_UP_TO_Y_UP);
  }

  SUBCASE("Gets X-up to Z-up transform") {
    CHECK(
        Transforms::getUpAxisTransform(Axis::X, Axis::Z) ==
        Transforms::X_UP_TO_Z_UP);
  }

  SUBCASE("Gets Y-up to X-up transform") {
    CHECK(
        Transforms::getUpAxisTransform(Axis::Y, Axis::X) ==
        Transforms::Y_UP_TO_X_UP);
  }

  SUBCASE("Gets Y-up to Y-up transform") {
    CHECK(Transforms::getUpAxisTransform(Axis::Y, Axis::Y) == Identity);
  }

  SUBCASE("Gets Y-up to Z-up transform") {
    CHECK(
        Transforms::getUpAxisTransform(Axis::Y, Axis::Z) ==
        Transforms::Y_UP_TO_Z_UP);
  }

  SUBCASE("Gets Z-up to X-up transform") {
    CHECK(
        Transforms::getUpAxisTransform(Axis::Z, Axis::X) ==
        Transforms::Z_UP_TO_X_UP);
  }

  SUBCASE("Gets Z-up to Y-up transform") {
    CHECK(
        Transforms::getUpAxisTransform(Axis::Z, Axis::Y) ==
        Transforms::Z_UP_TO_Y_UP);
  }

  SUBCASE("Gets Z-up to Z-up transform") {
    CHECK(Transforms::getUpAxisTransform(Axis::Z, Axis::Z) == Identity);
  }
}
