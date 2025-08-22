#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/Transforms.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/vector_double4.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

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

namespace {
bool pointInClipVolume(const glm::dvec4& point) {
  const double w = point.w;
  return -w <= point.x && point.x <= w && -w <= point.y && point.y <= w &&
         0.0 <= point.z && point.z <= w;
}
} // namespace
TEST_CASE("Test perspective projection matrices") {
  const double horizontalFieldOfView =
      CesiumUtility::Math::degreesToRadians(60.0);
  const double verticalFieldOfView =
      CesiumUtility::Math::degreesToRadians(45.0);
  const double zNear = 1.0;
  const double zFar = 20000.0;
  const glm::dmat4 projMat = Transforms::createPerspectiveMatrix(
      horizontalFieldOfView,
      verticalFieldOfView,
      zNear,
      zFar);
  std::vector<glm::dvec4> testPoints;
  testPoints.reserve(1000);
  for (int i = 0; i < 11; ++i) {
    double hRad =
        -horizontalFieldOfView / 2.0 + i * horizontalFieldOfView / 10.0;
    hRad = CesiumUtility::Math::clamp(
        hRad,
        -horizontalFieldOfView + 0.1,
        horizontalFieldOfView - 0.1);
    double sinH = std::sin(hRad);
    for (int j = 0; j < 10; ++j) {
      double vRad = -verticalFieldOfView / 2.0 + j * verticalFieldOfView / 10.0;
      vRad = CesiumUtility::Math::clamp(
          vRad,
          -verticalFieldOfView + 0.1,
          verticalFieldOfView - 0.1);
      double sinV = std::sin(vRad);
      for (int k = 0; k < 10; ++k) {
        double z = zNear + k * (zFar - zNear) / 10.0;
        z = CesiumUtility::Math::clamp(z, zNear + .1, zFar - .1);
        testPoints.emplace_back(sinH * z, sinV * z, -z, 1.0);
      }
    }
  }
  SUBCASE("Check that points lie in clipping volume") {
    CHECK(std::all_of(
        testPoints.begin(),
        testPoints.end(),
        [&projMat](const glm::dvec4& p) {
          glm::dvec4 projected = projMat * p;
          return pointInClipVolume(projected);
        }));
  }
  double hDim = std::tan(horizontalFieldOfView / 2.0) * zNear;
  double vDim = std::tan(verticalFieldOfView / 2.0) * zNear;
  glm::dmat4 corners = Transforms::createPerspectiveMatrix(
      -hDim,
      hDim,
      -vDim,
      vDim,
      zNear,
      zFar);
  SUBCASE("Check that different perspective constructions are equivalent") {
    CHECK(CesiumUtility::Math::equalsEpsilon(projMat, corners, 1e-14, 1e-14));
  }

  SUBCASE("Check skewed projection") {
    glm::dmat4 skewed =
        Transforms::createPerspectiveMatrix(0.0, hDim, 0.0, vDim, zNear, zFar);
    for (auto& point : testPoints) {
      glm::dvec4 skewProjected = skewed * point;
      if (pointInClipVolume(skewProjected)) {
        glm::dvec4 symProjected = corners * point;
        skewProjected /= skewProjected.w;
        symProjected /= symProjected.w;
        CHECK(CesiumUtility::Math::equalsEpsilon(
            skewProjected.x / 2.0 + .5,
            symProjected.x,
            1e-14));
        CHECK(CesiumUtility::Math::equalsEpsilon(
            skewProjected.y / 2.0 - .5,
            symProjected.y,
            1e-14));
        CHECK(CesiumUtility::Math::equalsEpsilon(
            skewProjected.z,
            symProjected.z,
            1e-14));
      }
    }
  }

  SUBCASE("Check that points are contained in orthgraphic projection too") {
    double orthoHDim = hDim / zNear * zFar;
    double orthoVDim = vDim / zNear * zFar;
    glm::dmat4 ortho = Transforms::createOrthographicMatrix(
        -orthoHDim,
        orthoHDim,
        -orthoVDim,
        orthoVDim,
        zNear,
        zFar);
    CHECK(std::all_of(
        testPoints.begin(),
        testPoints.end(),
        [&ortho](const glm::dvec4& p) {
          glm::dvec4 projected = ortho * p;
          return pointInClipVolume(projected);
        }));
  }
}
