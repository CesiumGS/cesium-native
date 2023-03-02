#include "CesiumGeometry/Transforms.h"

#include <CesiumUtility/Math.h>

#include <catch2/catch.hpp>

TEST_CASE("Transforms convert the axes correctly") {

  glm::dvec4 X_AXIS{1.0, 0.0, 0.0, 0.0};
  glm::dvec4 Y_AXIS{0.0, 1.0, 0.0, 0.0};
  glm::dvec4 Z_AXIS{0.0, 0.0, 1.0, 0.0};

  SECTION("Y_UP_TO_Z_UP transforms  X to  X,  Y to -Z, and  Z to Y") {
    REQUIRE(X_AXIS * CesiumGeometry::Transforms::Y_UP_TO_Z_UP == X_AXIS);
    REQUIRE(Y_AXIS * CesiumGeometry::Transforms::Y_UP_TO_Z_UP == -Z_AXIS);
    REQUIRE(Z_AXIS * CesiumGeometry::Transforms::Y_UP_TO_Z_UP == Y_AXIS);
  }
  SECTION("Z_UP_TO_Y_UP transforms  X to  X,  Y to  Z, and  Z to -Y") {
    REQUIRE(X_AXIS * CesiumGeometry::Transforms::Z_UP_TO_Y_UP == X_AXIS);
    REQUIRE(Y_AXIS * CesiumGeometry::Transforms::Z_UP_TO_Y_UP == Z_AXIS);
    REQUIRE(Z_AXIS * CesiumGeometry::Transforms::Z_UP_TO_Y_UP == -Y_AXIS);
  }

  SECTION("X_UP_TO_Z_UP transforms  X to -Z,  Y to  Y, and  Z to  X") {
    REQUIRE(X_AXIS * CesiumGeometry::Transforms::X_UP_TO_Z_UP == -Z_AXIS);
    REQUIRE(Y_AXIS * CesiumGeometry::Transforms::X_UP_TO_Z_UP == Y_AXIS);
    REQUIRE(Z_AXIS * CesiumGeometry::Transforms::X_UP_TO_Z_UP == X_AXIS);
  }
  SECTION("Z_UP_TO_X_UP transforms  X to  Z,  Y to  Y, and  Z to -X") {
    REQUIRE(X_AXIS * CesiumGeometry::Transforms::Z_UP_TO_X_UP == Z_AXIS);
    REQUIRE(Y_AXIS * CesiumGeometry::Transforms::Z_UP_TO_X_UP == Y_AXIS);
    REQUIRE(Z_AXIS * CesiumGeometry::Transforms::Z_UP_TO_X_UP == -X_AXIS);
  }

  SECTION("X_UP_TO_Y_UP transforms  X to -Y,  Y to  X, and  Z to  Z") {
    REQUIRE(X_AXIS * CesiumGeometry::Transforms::X_UP_TO_Y_UP == -Y_AXIS);
    REQUIRE(Y_AXIS * CesiumGeometry::Transforms::X_UP_TO_Y_UP == X_AXIS);
    REQUIRE(Z_AXIS * CesiumGeometry::Transforms::X_UP_TO_Y_UP == Z_AXIS);
  }
  SECTION("Y_UP_TO_X_UP transforms  X to  Y,  Y to -X, and  Z to  Z") {
    REQUIRE(X_AXIS * CesiumGeometry::Transforms::Y_UP_TO_X_UP == Y_AXIS);
    REQUIRE(Y_AXIS * CesiumGeometry::Transforms::Y_UP_TO_X_UP == -X_AXIS);
    REQUIRE(Z_AXIS * CesiumGeometry::Transforms::Y_UP_TO_X_UP == Z_AXIS);
  }
}
