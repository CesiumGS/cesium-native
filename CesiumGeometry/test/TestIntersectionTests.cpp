#include "CesiumGeometry/IntersectionTests.h"
#include "CesiumGeometry/Plane.h"
#include "CesiumGeometry/Ray.h"

#include <catch2/catch.hpp>
#include <glm/glm.hpp>

#include <array>

using namespace CesiumGeometry;

TEST_CASE("IntersectionTests::rayPlane") {
  struct TestCase {
    Ray ray;
    Plane plane;
    std::optional<glm::dvec3> expectedIntersectionPoint;
  };

  auto testCase = GENERATE(
      // intersects
      TestCase{
          Ray(glm::dvec3(2.0, 0.0, 0.0), glm::dvec3(-1.0, 0.0, 0.0)),
          Plane(glm::dvec3(1.0, 0.0, 0.0), -1.0),
          glm::dvec3(1.0, 0.0, 0.0)},
      // misses
      TestCase{
          Ray(glm::dvec3(2.0, 0.0, 0.0), glm::dvec3(1.0, 0.0, 0.0)),
          Plane(glm::dvec3(1.0, 0.0, 0.0), -1.0),
          std::nullopt},
      // misses (parallel)
      TestCase{
          Ray(glm::dvec3(2.0, 0.0, 0.0), glm::dvec3(0.0, 1.0, 0.0)),
          Plane(glm::dvec3(1.0, 0.0, 0.0), -1.0),
          std::nullopt});

  std::optional<glm::dvec3> intersectionPoint =
      IntersectionTests::rayPlane(testCase.ray, testCase.plane);
  CHECK(intersectionPoint == testCase.expectedIntersectionPoint);
}

TEST_CASE("IntersectionTests::pointInTriangle (2D overload)") {
  struct TestCase {
    glm::dvec2 point;
    glm::dvec2 triangleVert1;
    glm::dvec2 triangleVert2;
    glm::dvec2 triangleVert3;
    bool expected;
  };

  std::array<glm::dvec2, 3> rightTriangle{
      glm::dvec2(-1.0, 0.0),
      glm::dvec2(0.0, 1.0),
      glm::dvec2(1.0, 0.0)};

  std::array<glm::dvec2, 3> obtuseTriangle{
      glm::dvec2(2.0, 0.0),
      glm::dvec2(4.0, 1.0),
      glm::dvec2(6.0, 0.0)};

  auto testCase = GENERATE_REF(
      // Corner of triangle returns true.
      TestCase{
          rightTriangle[2],
          rightTriangle[0],
          rightTriangle[1],
          rightTriangle[2],
          true},
      // Point on triangle edge returns true.
      TestCase{
          glm::dvec2(0.0, 0.0),
          rightTriangle[0],
          rightTriangle[1],
          rightTriangle[2],
          true},
      // Point inside triangle returns true. (right)
      TestCase{
          glm::dvec2(0.2, 0.5),
          rightTriangle[0],
          rightTriangle[1],
          rightTriangle[2],
          true},
      // Point inside triangle returns true. (obtuse)
      TestCase{
          glm::dvec2(4.0, 0.3),
          obtuseTriangle[0],
          obtuseTriangle[1],
          obtuseTriangle[2],
          true},
      // Point outside triangle returns false. (right)
      TestCase{
          glm::dvec2(-2.0, 0.5),
          rightTriangle[0],
          rightTriangle[1],
          rightTriangle[2],
          false},
      // Point outside triangle returns false. (obtuse)
      TestCase{
          glm::dvec2(3.0, -0.5),
          obtuseTriangle[0],
          obtuseTriangle[1],
          obtuseTriangle[2],
          false},
      // Point "inside" degenerate triangle returns false.
      TestCase{
          rightTriangle[0],
          rightTriangle[0],
          rightTriangle[0],
          rightTriangle[2],
          false});

  bool result = IntersectionTests::pointInTriangle(
      testCase.point,
      testCase.triangleVert1,
      testCase.triangleVert2,
      testCase.triangleVert3);
  CHECK(result == testCase.expected);

  // Do same test but with reverse winding
  bool reverseResult = IntersectionTests::pointInTriangle(
      testCase.point,
      testCase.triangleVert3,
      testCase.triangleVert2,
      testCase.triangleVert1);
  CHECK(reverseResult == testCase.expected);
}

TEST_CASE("IntersectionTests::pointInTriangle (3D overload)") {
  struct TestCase {
    glm::dvec3 point;
    glm::dvec3 triangleVert1;
    glm::dvec3 triangleVert2;
    glm::dvec3 triangleVert3;
    bool expected;
  };

  std::array<glm::dvec3, 3> rightTriangle{
      glm::dvec3(-1.0, 0.0, 0.0),
      glm::dvec3(0.0, 1.0, 0.0),
      glm::dvec3(1.0, 0.0, 0.0)};

  std::array<glm::dvec3, 3> equilateralTriangle{
      glm::dvec3(1.0, 0.0, 0.0),
      glm::dvec3(0.0, 1.0, 0.0),
      glm::dvec3(0.0, 0.0, 1.0)};

  auto testCase = GENERATE_REF(
      // Corner of triangle returns true.
      TestCase{
          rightTriangle[2],
          rightTriangle[0],
          rightTriangle[1],
          rightTriangle[2],
          true},
      // Point on triangle edge returns true.
      TestCase{
          glm::dvec3(0.0, 0.0, 0.0),
          rightTriangle[0],
          rightTriangle[1],
          rightTriangle[2],
          true},
      // Point inside triangle returns true. (right)
      TestCase{
          glm::dvec3(0.2, 0.5, 0.0),
          rightTriangle[0],
          rightTriangle[1],
          rightTriangle[2],
          true},
      // Point inside triangle returns true. (equilateral)
      TestCase{
          glm::dvec3(0.25, 0.25, 0.5),
          equilateralTriangle[0],
          equilateralTriangle[1],
          equilateralTriangle[2],
          true},
      // Point outside triangle on same plane returns false. (right)
      TestCase{
          glm::dvec3(-2.0, 0.5, 0.0),
          rightTriangle[0],
          rightTriangle[1],
          rightTriangle[2],
          false},
      // Point outside triangle on different plane returns false. (right)
      TestCase{
          glm::dvec3(0.2, 0.5, 1.0),
          rightTriangle[0],
          rightTriangle[1],
          rightTriangle[2],
          false},
      // Point outside triangle on same plane returns false. (equilateral)
      TestCase{
          glm::dvec3(-1.0, 1.5, 0.5),
          equilateralTriangle[0],
          equilateralTriangle[1],
          equilateralTriangle[2],
          false},
      // Point outside triangle on different plane returns false. (equilateral)
      TestCase{
          glm::dvec3(0.0, 0.0, 0.0),
          equilateralTriangle[0],
          equilateralTriangle[1],
          equilateralTriangle[2],
          false},
      // Point "inside" degenerate triangle returns false.
      TestCase{
          rightTriangle[0],
          rightTriangle[0],
          rightTriangle[0],
          rightTriangle[2],
          false});

  bool result = IntersectionTests::pointInTriangle(
      testCase.point,
      testCase.triangleVert1,
      testCase.triangleVert2,
      testCase.triangleVert3);
  CHECK(result == testCase.expected);

  // Do same test but with reverse winding
  bool reverseResult = IntersectionTests::pointInTriangle(
      testCase.point,
      testCase.triangleVert3,
      testCase.triangleVert2,
      testCase.triangleVert1);
  CHECK(reverseResult == testCase.expected);
}

TEST_CASE("IntersectionTests::pointInTriangle (3D overload with barycentric "
          "coordinates)") {
  struct TestCase {
    glm::dvec3 point;
    glm::dvec3 triangleVert1;
    glm::dvec3 triangleVert2;
    glm::dvec3 triangleVert3;
    bool expected;
    glm::dvec3 expectedCoordinates;
  };

  std::array<glm::dvec3, 3> rightTriangle{
      glm::dvec3(-1.0, 0.0, 0.0),
      glm::dvec3(0.0, 1.0, 0.0),
      glm::dvec3(1.0, 0.0, 0.0)};

  std::array<glm::dvec3, 3> equilateralTriangle{
      glm::dvec3(1.0, 0.0, 0.0),
      glm::dvec3(0.0, 1.0, 0.0),
      glm::dvec3(0.0, 0.0, 1.0)};

  auto testCase = GENERATE_REF(
      // Corner of triangle returns true.
      TestCase{
          rightTriangle[2],
          rightTriangle[0],
          rightTriangle[1],
          rightTriangle[2],
          true,
          glm::dvec3(0.0, 0.0, 1.0)},
      // Point on triangle edge returns true.
      TestCase{
          glm::dvec3(0.0, 0.0, 0.0),
          rightTriangle[0],
          rightTriangle[1],
          rightTriangle[2],
          true,
          glm::dvec3(0.5, 0.0, 0.5)},
      // Point inside triangle returns true. (right)
      TestCase{
          glm::dvec3(0.0, 0.5, 0.0),
          rightTriangle[0],
          rightTriangle[1],
          rightTriangle[2],
          true,
          glm::dvec3(0.25, 0.5, 0.25)},
      // Point inside triangle returns true. (equilateral)
      TestCase{
          glm::dvec3(0.25, 0.25, 0.5),
          equilateralTriangle[0],
          equilateralTriangle[1],
          equilateralTriangle[2],
          true,
          glm::dvec3(0.25, 0.25, 0.5)},
      // Point outside triangle on same plane returns false. (right)
      TestCase{
          glm::dvec3(-2.0, 0.5, 0.0),
          rightTriangle[0],
          rightTriangle[1],
          rightTriangle[2],
          false,
          glm::dvec3()},
      // Point outside triangle on different plane returns false. (right)
      TestCase{
          glm::dvec3(0.2, 0.5, 1.0),
          rightTriangle[0],
          rightTriangle[1],
          rightTriangle[2],
          false,
          glm::dvec3()},
      // Point outside triangle on same plane returns false. (equilateral)
      TestCase{
          glm::dvec3(-1.0, 1.5, 0.5),
          equilateralTriangle[0],
          equilateralTriangle[1],
          equilateralTriangle[2],
          false,
          glm::dvec3()},
      // Point outside triangle on different plane returns false. (equilateral)
      TestCase{
          glm::dvec3(0.0, 0.0, 0.0),
          equilateralTriangle[0],
          equilateralTriangle[1],
          equilateralTriangle[2],
          false,
          glm::dvec3()},
      // Point "inside" degenerate triangle returns false.
      TestCase{
          rightTriangle[0],
          rightTriangle[0],
          rightTriangle[0],
          rightTriangle[2],
          false,
          glm::dvec3()});

  glm::dvec3 barycentricCoordinates = glm::dvec3();
  bool result = IntersectionTests::pointInTriangle(
      testCase.point,
      testCase.triangleVert1,
      testCase.triangleVert2,
      testCase.triangleVert3,
      barycentricCoordinates);

  REQUIRE(result == testCase.expected);
  REQUIRE(
      (barycentricCoordinates.x == testCase.expectedCoordinates.x &&
       barycentricCoordinates.y == testCase.expectedCoordinates.y &&
       barycentricCoordinates.z == testCase.expectedCoordinates.z));

  // Do same test but with reverse winding
  bool reverseResult = IntersectionTests::pointInTriangle(
      testCase.point,
      testCase.triangleVert3,
      testCase.triangleVert2,
      testCase.triangleVert1,
      barycentricCoordinates);

  CHECK(reverseResult == testCase.expected);
  REQUIRE(
      (barycentricCoordinates.z == testCase.expectedCoordinates.x &&
       barycentricCoordinates.y == testCase.expectedCoordinates.y &&
       barycentricCoordinates.x == testCase.expectedCoordinates.z));
}
