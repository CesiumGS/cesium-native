#include "CesiumGeometry/AxisAlignedBox.h"
#include "CesiumGeometry/IntersectionTests.h"
#include "CesiumGeometry/OrientedBoundingBox.h"
#include "CesiumGeometry/Plane.h"
#include "CesiumGeometry/Ray.h"
#include "CesiumUtility/Math.h"

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

TEST_CASE("IntersectionTests::rayAABB") {

  struct TestCase {
    Ray ray;
    AxisAlignedBox aabb;
    std::optional<glm::dvec3> expectedIntersectionPoint;
  };

  auto testCase = GENERATE(
      // basic intersection works
      TestCase{
          Ray(glm::dvec3(-1.0, 0.5, 0.5), glm::dvec3(1.0, 0.0, 0.0)),
          AxisAlignedBox(0.0, 0.0, 0.0, 1.0, 1.0, 1.0),
          glm::dvec3(0.0, 0.5, 0.5)},
      // intersects with angled ray
      TestCase{
          Ray(glm::dvec3(-1.0, 0.0, 1.0),
              glm::dvec3(1.0 / glm::sqrt(2), 0.0, -1.0 / glm::sqrt(2))),
          AxisAlignedBox(-0.5, -0.5, -0.5, 0.5, 0.5, 0.5),
          glm::dvec3(-0.5, 0.0, 0.5)},
      // no intersection when ray is pointing away from box
      TestCase{
          Ray(glm::dvec3(-1.0, 0.5, 0.5), glm::dvec3(-1.0, 0.0, 0.0)),
          AxisAlignedBox(0.0, 0.0, 0.0, 1.0, 1.0, 1.0),
          std::nullopt},
      // ray inside of box intersects
      TestCase{
          Ray(glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(0.0, -1.0, 0.0)),
          AxisAlignedBox(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0),
          glm::dvec3(0.0, -1.0, 0.0)});
  std::optional<glm::dvec3> intersectionPoint =
      IntersectionTests::rayAABB(testCase.ray, testCase.aabb);
  CHECK(intersectionPoint == testCase.expectedIntersectionPoint);
}

TEST_CASE("IntersectionTests::rayOBB") {
  struct TestCase {
    Ray ray;
    glm::dvec3 xHalf;
    glm::dvec3 yHalf;
    glm::dvec3 obbOrigin;
    std::optional<glm::dvec3> expectedIntersectionPoint;
  };

  auto testCase = GENERATE(
      // 2x2x2 obb at origin that is rotated -45 degrees on the x-axis.
      TestCase{
          Ray(glm::dvec3(0.0, 0.0, 10.0), glm::dvec3(0.0, 0.0, -1.0)),
          glm::dvec3(-1.0 / glm::sqrt(2), 0.0, 1.0 / glm::sqrt(2)),
          glm::dvec3(0.0, 1.0, 0.0),
          glm::dvec3(0.0, 0.0, 0.0),
          glm::dvec3(0.0, 0.0, 2.0 / glm::sqrt(2))},
      // 2x2x2 obb at (10,10,10) that is rotated -45 degrees on the x-axis.
      TestCase{
          Ray(glm::dvec3(10.0, 10.0, 20.0), glm::dvec3(0.0, 0.0, -1.0)),
          glm::dvec3(-1.0 / glm::sqrt(2), 0.0, 1.0 / glm::sqrt(2)),
          glm::dvec3(0.0, 1.0, 0.0),
          glm::dvec3(10.0, 10.0, 10.0),
          glm::dvec3(10.0, 10.0, 10.0 + 2.0 / glm::sqrt(2))});
  std::optional<glm::dvec3> intersectionPoint = IntersectionTests::rayOBB(
      testCase.ray,
      OrientedBoundingBox(
          testCase.obbOrigin,
          glm::dmat3x3(
              testCase.xHalf,
              testCase.yHalf,
              glm::cross(testCase.xHalf, testCase.yHalf))));
  CHECK(glm::all(glm::lessThan(
      glm::abs(*intersectionPoint - *testCase.expectedIntersectionPoint),
      glm::dvec3(CesiumUtility::Math::Epsilon6))));
}

TEST_CASE("IntersectionTests::raySphere") {
  struct TestCase {
    Ray ray;
    BoundingSphere sphere;
    double t;
  };

  auto testCase = GENERATE(
      // raySphere outside intersections
      TestCase{
          Ray(glm::dvec3(2.0, 0.0, 0.0), glm::dvec3(-1.0, 0.0, 0.0)),
          BoundingSphere(glm::dvec3(0.0, 0.0, 0.0), 1.0),
          1.0},
      TestCase{
          Ray(glm::dvec3(0.0, 2.0, 0.0), glm::dvec3(0.0, -1.0, 0.0)),
          BoundingSphere(glm::dvec3(0.0, 0.0, 0.0), 1.0),
          1.0},
      TestCase{
          Ray(glm::dvec3(0.0, 0.0, 2.0), glm::dvec3(0.0, 0.0, -1.0)),
          BoundingSphere(glm::dvec3(0.0, 0.0, 0.0), 1.0),
          1.0},
      TestCase{
          Ray(glm::dvec3(1.0, 1.0, 0.0), glm::dvec3(-1.0, 0.0, 0.0)),
          BoundingSphere(glm::dvec3(0.0, 0.0, 0.0), 1.0),
          1.0},
      TestCase{
          Ray(glm::dvec3(-2.0, 0.0, 0.0), glm::dvec3(1.0, 0.0, 0.0)),
          BoundingSphere(glm::dvec3(0.0, 0.0, 0.0), 1.0),
          1.0},
      TestCase{
          Ray(glm::dvec3(0.0, -2.0, 0.0), glm::dvec3(0.0, 1.0, 0.0)),
          BoundingSphere(glm::dvec3(0.0, 0.0, 0.0), 1.0),
          1.0},
      TestCase{
          Ray(glm::dvec3(0.0, 0.0, -2.0), glm::dvec3(0.0, 0.0, 1.0)),
          BoundingSphere(glm::dvec3(0.0, 0.0, 0.0), 1.0),
          1.0},
      TestCase{
          Ray(glm::dvec3(-1.0, -1.0, 0.0), glm::dvec3(1.0, 0.0, 0.0)),
          BoundingSphere(glm::dvec3(0.0, 0.0, 0.0), 1.0),
          1.0},
      TestCase{
          Ray(glm::dvec3(-2.0, 0.0, 0.0), glm::dvec3(-1.0, 0.0, 0.0)),
          BoundingSphere(glm::dvec3(0.0, 0.0, 0.0), 1.0),
          -1.0},
      TestCase{
          Ray(glm::dvec3(0.0, -2.0, 0.0), glm::dvec3(0.0, -1.0, 0.0)),
          BoundingSphere(glm::dvec3(0.0, 0.0, 0.0), 1.0),
          -1.0},
      TestCase{
          Ray(glm::dvec3(0.0, 0.0, -2.0), glm::dvec3(0.0, 0.0, -1.0)),
          BoundingSphere(glm::dvec3(0.0, 0.0, 0.0), 1.0),
          -1.0},
      // raySphere ray inside pointing in intersection
      TestCase{
          Ray(glm::dvec3(200.0, 0.0, 0.0),
              -glm::normalize(glm::dvec3(200.0, 0.0, 0.0))),
          BoundingSphere(glm::dvec3(0.0, 0.0, 0.0), 5000.0),
          5000.0 + 200.0},
      // raySphere ray inside pointing out intersection
      TestCase{
          Ray(glm::dvec3(200.0, 0.0, 0.0),
              glm::normalize(glm::dvec3(200.0, 0.0, 0.0))),
          BoundingSphere(glm::dvec3(0.0, 0.0, 0.0), 5000.0),
          5000.0 - 200.0},
      // raySphere tangent intersections
      TestCase{
          Ray(glm::dvec3(1.0, 0.0, 0.0), glm::dvec3(0.0, 0.0, 1.0)),
          BoundingSphere(glm::dvec3(0.0, 0.0, 0.0), 1.0),
          -1.0},
      // raySphere no intersections
      TestCase{
          Ray(glm::dvec3(2.0, 0.0, 0.0), glm::dvec3(0.0, 0.0, 1.0)),
          BoundingSphere(glm::dvec3(0.0, 0.0, 0.0), 1.0),
          -1.0},
      TestCase{
          Ray(glm::dvec3(2.0, 0.0, 0.0), glm::dvec3(0.0, 0.0, -1.0)),
          BoundingSphere(glm::dvec3(0.0, 0.0, 0.0), 1.0),
          -1.0},
      TestCase{
          Ray(glm::dvec3(2.0, 0.0, 0.0), glm::dvec3(0.0, 1.0, 0.0)),
          BoundingSphere(glm::dvec3(0.0, 0.0, 0.0), 1.0),
          -1.0},
      TestCase{
          Ray(glm::dvec3(2.0, 0.0, 0.0), glm::dvec3(0.0, -1.0, 0.0)),
          BoundingSphere(glm::dvec3(0.0, 0.0, 0.0), 1.0),
          -1.0},
      // raySphere intersection with sphere center not the origin
      TestCase{
          Ray(glm::dvec3(202.0, 0.0, 0.0), glm::dvec3(-1.0, 0.0, 0.0)),
          BoundingSphere(glm::dvec3(200.0, 0.0, 0.0), 1.0),
          1.0},
      TestCase{
          Ray(glm::dvec3(200.0, 2.0, 0.0), glm::dvec3(0.0, -1.0, 0.0)),
          BoundingSphere(glm::dvec3(200.0, 0.0, 0.0), 1.0),
          1.0},
      TestCase{
          Ray(glm::dvec3(200.0, 0.0, 2.0), glm::dvec3(0.0, 0.0, -1.0)),
          BoundingSphere(glm::dvec3(200.0, 0.0, 0.0), 1.0),
          1.0},
      TestCase{
          Ray(glm::dvec3(201.0, 1.0, 0.0), glm::dvec3(-1.0, 0.0, 0.0)),
          BoundingSphere(glm::dvec3(200.0, 0.0, 0.0), 1.0),
          1.0},
      TestCase{
          Ray(glm::dvec3(198.0, 0.0, 0.0), glm::dvec3(1.0, 0.0, 0.0)),
          BoundingSphere(glm::dvec3(200.0, 0.0, 0.0), 1.0),
          1.0},
      TestCase{
          Ray(glm::dvec3(200.0, -2.0, 0.0), glm::dvec3(0.0, 1.0, 0.0)),
          BoundingSphere(glm::dvec3(200.0, 0.0, 0.0), 1.0),
          1.0},
      TestCase{
          Ray(glm::dvec3(200.0, 0.0, -2.0), glm::dvec3(0.0, 0.0, 1.0)),
          BoundingSphere(glm::dvec3(200.0, 0.0, 0.0), 1.0),
          1.0},
      TestCase{
          Ray(glm::dvec3(199.0, -1.0, 0.0), glm::dvec3(1.0, 0.0, 0.0)),
          BoundingSphere(glm::dvec3(200.0, 0.0, 0.0), 1.0),
          1.0},
      TestCase{
          Ray(glm::dvec3(198.0, 0.0, 0.0), glm::dvec3(-1.0, 0.0, 0.0)),
          BoundingSphere(glm::dvec3(200.0, 0.0, 0.0), 1.0),
          -1.0},
      TestCase{
          Ray(glm::dvec3(200.0, -2.0, 0.0), glm::dvec3(0.0, -1.0, 0.0)),
          BoundingSphere(glm::dvec3(200.0, 0.0, 0.0), 1.0),
          -1.0},
      TestCase{
          Ray(glm::dvec3(200.0, 0.0, -2.0), glm::dvec3(0.0, 0.0, -1.0)),
          BoundingSphere(glm::dvec3(200.0, 0.0, 0.0), 1.0),
          -1.0});

  double t =
      IntersectionTests::raySphereParametric(testCase.ray, testCase.sphere);

  CHECK(CesiumUtility::Math::equalsEpsilon(
      t,
      testCase.t,
      CesiumUtility::Math::Epsilon6));
}
