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
