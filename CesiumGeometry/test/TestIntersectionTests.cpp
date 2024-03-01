#include "CesiumGeometry/IntersectionTests.h"
#include "CesiumGeometry/Plane.h"
#include "CesiumGeometry/Ray.h"

#include <catch2/catch.hpp>
#include <glm/mat3x3.hpp>

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

TEST_CASE("IntersectionTests::pointInTriangle2D") {
  struct TestCase {
    glm::dvec2 point;
    glm::dvec2 triangleVert1;
    glm::dvec2 triangleVert2;
    glm::dvec2 triangleVert3;
    bool expected;
  };

  auto testCase = GENERATE(
      // is in triangle (corner)
      TestCase{
          glm::dvec2(1.0, 0.0),
          glm::dvec2(-1.0, 0.0),
          glm::dvec2(0.0, 1.0),
          glm::dvec2(1.0, 0.0),
          true},
      // is in triangle (edge)
      TestCase{
          glm::dvec2(0.0, 0.0),
          glm::dvec2(-1.0, 0.0),
          glm::dvec2(0.0, 1.0),
          glm::dvec2(1.0, 0.0),
          true},
      // is in triangle (middle)
      TestCase{
          glm::dvec2(0.2, 0.5),
          glm::dvec2(-1.0, 0.0),
          glm::dvec2(0.0, 1.0),
          glm::dvec2(1.0, 0.0),
          true},
      // is in triangle (regardless of winding order)
      TestCase{
          glm::dvec2(0.2, 0.5),
          glm::dvec2(-1.0, 0.0),
          glm::dvec2(1.0, 0.0),
          glm::dvec2(0.0, 1.0),
          true},
      // is not in triangle
      TestCase{
          glm::dvec2(-2.0, 0.5),
          glm::dvec2(-1.0, 0.0),
          glm::dvec2(1.0, 0.0),
          glm::dvec2(0.0, 1.0),
          false},
      // is not in triangle (regardless of winding order)
      TestCase{
          glm::dvec2(-2.0, 0.5),
          glm::dvec2(-1.0, 0.0),
          glm::dvec2(1.0, 0.0),
          glm::dvec2(0.0, 1.0),
          false});

  bool result = IntersectionTests::pointInTriangle2D(
      testCase.point,
      testCase.triangleVert1,
      testCase.triangleVert2,
      testCase.triangleVert3);
  CHECK(result == testCase.expected);
}

TEST_CASE("IntersectionTests::pointInTriangle") {
  struct TestCase {
    glm::dvec3 point;
    glm::dvec3 triangleVert1;
    glm::dvec3 triangleVert2;
    glm::dvec3 triangleVert3;
    bool expected;
  };

  auto testCase = GENERATE(
      // is in triangle (corner)
      TestCase{
          glm::dvec3(1.0, 0.0, 0.0),
          glm::dvec3(-1.0, 0.0, 0.0),
          glm::dvec3(0.0, 1.0, 0.0),
          glm::dvec3(1.0, 0.0, 0.0),
          true},
      // is in triangle (edge)
      TestCase{
          glm::dvec3(0.0, 0.0, 0.0),
          glm::dvec3(-1.0, 0.0, 0.0),
          glm::dvec3(0.0, 1.0, 0.0),
          glm::dvec3(1.0, 0.0, 0.0),
          true},
      // is in triangle (middle)
      TestCase{
          glm::dvec3(0.2, 0.5, 0.0),
          glm::dvec3(-1.0, 0.0, 0.0),
          glm::dvec3(0.0, 1.0, 0.0),
          glm::dvec3(1.0, 0.0, 0.0),
          true},
      // is in triangle (regardless of winding order)
      TestCase{
          glm::dvec3(0.2, 0.5, 0.0),
          glm::dvec3(-1.0, 0.0, 0.0),
          glm::dvec3(1.0, 0.0, 0.0),
          glm::dvec3(0.0, 1.0, 0.0),
          true},
      // is not in triangle (same plane)
      TestCase{
          glm::dvec3(-2.0, 0.5, 0.0),
          glm::dvec3(-1.0, 0.0, 0.0),
          glm::dvec3(1.0, 0.0, 0.0),
          glm::dvec3(0.0, 1.0, 0.0),
          false},
      // is not in triangle (different plane)
      TestCase{
          glm::dvec3(0.2, 0.5, 1.0),
          glm::dvec3(-1.0, 0.0, 0.0),
          glm::dvec3(1.0, 0.0, 0.0),
          glm::dvec3(0.0, 1.0, 0.0),
          false},
      // is not in triangle (regardless of winding order)
      TestCase{
          glm::dvec3(-2.0, 0.5, 0.0),
          glm::dvec3(-1.0, 0.0, 0.0),
          glm::dvec3(1.0, 0.0, 0.0),
          glm::dvec3(0.0, 1.0, 0.0),
          false});

  bool result = IntersectionTests::pointInTriangle(
      testCase.point,
      testCase.triangleVert1,
      testCase.triangleVert2,
      testCase.triangleVert3);
  CHECK(result == testCase.expected);
}

TEST_CASE("IntersectionTests::pointInTriangle with barycentric coordinates") {
  struct TestCase {
    glm::dvec3 point;
    glm::dvec3 triangleVert1;
    glm::dvec3 triangleVert2;
    glm::dvec3 triangleVert3;
    bool expected;
    glm::dvec3 expectedCoordinates;
  };

  auto testCase = GENERATE(
      // is in triangle (corner)
      TestCase{
          glm::dvec3(1.0, 0.0, 0.0),
          glm::dvec3(-1.0, 0.0, 0.0),
          glm::dvec3(0.0, 1.0, 0.0),
          glm::dvec3(1.0, 0.0, 0.0),
          true,
          glm::dvec3(0.0, 0.0, 1.0)},
      // is in triangle (edge)
      TestCase{
          glm::dvec3(0.0, 0.0, 0.0),
          glm::dvec3(-1.0, 0.0, 0.0),
          glm::dvec3(0.0, 1.0, 0.0),
          glm::dvec3(1.0, 0.0, 0.0),
          true,
          glm::dvec3(0.5, 0.0, 0.5)},
      // is in triangle (middle)
      TestCase{
          glm::dvec3(0.0, 0.5, 0.0),
          glm::dvec3(-1.0, 0.0, 0.0),
          glm::dvec3(0.0, 1.0, 0.0),
          glm::dvec3(1.0, 0.0, 0.0),
          true,
          glm::dvec3(0.25, 0.5, 0.25)},
      // is in triangle (regardless of winding order)
      TestCase{
          glm::dvec3(0.0, 0.5, 0.0),
          glm::dvec3(-1.0, 0.0, 0.0),
          glm::dvec3(1.0, 0.0, 0.0),
          glm::dvec3(0.0, 1.0, 0.0),
          true,
          glm::dvec3(0.25, 0.25, 0.5)},
      // is not in triangle (same plane)
      TestCase{
          glm::dvec3(-2.0, 0.5, 0.0),
          glm::dvec3(-1.0, 0.0, 0.0),
          glm::dvec3(1.0, 0.0, 0.0),
          glm::dvec3(0.0, 1.0, 0.0),
          false,
          glm::dvec3()},
      // is not in triangle (different plane)
      TestCase{
          glm::dvec3(0.2, 0.5, 1.0),
          glm::dvec3(-1.0, 0.0, 0.0),
          glm::dvec3(1.0, 0.0, 0.0),
          glm::dvec3(0.0, 1.0, 0.0),
          false,
          glm::dvec3()},
      // is not in triangle (regardless of winding order)
      TestCase{
          glm::dvec3(-2.0, 0.5, 0.0),
          glm::dvec3(-1.0, 0.0, 0.0),
          glm::dvec3(1.0, 0.0, 0.0),
          glm::dvec3(0.0, 1.0, 0.0),
          false,
          glm::dvec3()});

  glm::dvec3 barycentricCoordinates;
  bool result = IntersectionTests::pointInTriangle(
      testCase.point,
      testCase.triangleVert1,
      testCase.triangleVert2,
      testCase.triangleVert3,
      barycentricCoordinates);

  REQUIRE(result == testCase.expected);
  if (result) {
    CHECK(barycentricCoordinates == testCase.expectedCoordinates);
  }
}
