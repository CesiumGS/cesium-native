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

TEST_CASE("IntersectionTests::rayTriangle") {

  glm::dvec3 V0 = glm::dvec3(-1.0, 0.0, 0.0);
  glm::dvec3 V1 = glm::dvec3(1.0, 0.0, 0.0);
  glm::dvec3 V2 = glm::dvec3(0.0, 1.0, 0.0);

  struct TestCase {
    Ray ray;
    bool cullBackFaces;
    std::optional<glm::dvec3> expectedIntersectionPoint;
  };

  auto testCase = GENERATE(
      // rayTriangle intersects front face
      TestCase{
          Ray(glm::dvec3(0.0, 0.0, 1.0), glm::dvec3(0.0, 0.0, -1.0)),
          false,
          glm::dvec3(0.0, 0.0, 0.0)},
      // rayTriangle intersects back face without culling
      TestCase{
          Ray(glm::dvec3(0.0, 0.0, -1.0), glm::dvec3(0.0, 0.0, 1.0)),
          false,
          glm::dvec3(0.0, 0.0, 0.0)},
      // rayTriangle does not intersect back face with culling
      TestCase{
          Ray(glm::dvec3(0.0, 0.0, -1.0), glm::dvec3(0.0, 0.0, 1.0)),
          true,
          std::nullopt},
      // rayTriangle does not intersect outside the 0-1 edge
      TestCase{
          Ray(glm::dvec3(0.0, -1.0, 1.0), glm::dvec3(0.0, 0.0, -1.0)),
          false,
          std::nullopt},
      // rayTriangle does not intersect outside the 1-2 edge
      TestCase{
          Ray(glm::dvec3(1.0, 1.0, 10.0), glm::dvec3(0.0, 0.0, -1.0)),
          false,
          std::nullopt},
      // rayTriangle does not intersect outside the 2-0 edge
      TestCase{
          Ray(glm::dvec3(2.0, 0.0, 0.0), glm::dvec3(0.0, 1.0, 0.0)),
          false,
          std::nullopt},
      // rayTriangle does not intersect parallel ray and triangle
      TestCase{
          Ray(glm::dvec3(-1.0, 1.0, 1.0), glm::dvec3(0.0, 0.0, -1.0)),
          false,
          std::nullopt},
      // rayTriangle does not intersect parallel ray and triangle
      TestCase{
          Ray(glm::dvec3(-1.0, 0.0, 1.0), glm::dvec3(1.0, 0.0, 0.0)),
          false,
          std::nullopt},
      // rayTriangle does not intersect behind the ray origin
      TestCase{
          Ray(glm::dvec3(0.0, 0.0, 1.0), glm::dvec3(0.0, 0.0, 1.0)),
          false,
          std::nullopt});

  std::optional<glm::dvec3> intersectionPoint = IntersectionTests::rayTriangle(
      testCase.ray,
      V0,
      V1,
      V2,
      testCase.cullBackFaces);
  CHECK(intersectionPoint == testCase.expectedIntersectionPoint);
}
