#include <CesiumGeometry/AxisAlignedBox.h>
#include <CesiumGeometry/IntersectionTests.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeometry/Plane.h>
#include <CesiumGeometry/Ray.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vector_relational.hpp>

#include <array>
#include <optional>
#include <vector>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

TEST_CASE("IntersectionTests::rayPlane") {
  struct TestCase {
    Ray ray;
    Plane plane;
    std::optional<glm::dvec3> expectedIntersectionPoint;
  };

  std::vector<TestCase> testCases{
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
          std::nullopt}};

  for (auto& testCase : testCases) {
    std::optional<glm::dvec3> intersectionPoint =
        IntersectionTests::rayPlane(testCase.ray, testCase.plane);
    CHECK(intersectionPoint == testCase.expectedIntersectionPoint);
  }
}

namespace {
const glm::dvec3 unitRadii(1.0, 1.0, 1.0);
const glm::dvec3 wgs84Radii = Ellipsoid::WGS84.getRadii();
} // namespace

TEST_CASE("IntersectionTests::rayEllipsoid") {
  struct TestCase {
    Ray ray;
    glm::dvec3 inverseRadii;
    std::optional<glm::dvec2> expectedIntersection;
  };

  std::vector<TestCase> testCases{
      // Degenerate ellipsoid
      TestCase{
          Ray(glm::dvec3(2.0, 0.0, 0.0), glm::dvec3(-1.0, 0.0, 0)),
          glm::dvec3(0),
          std::nullopt},
      // RayEllipsoid outside intersections
      TestCase{
          Ray(glm::dvec3(2.0, 0.0, 0.0), glm::dvec3(-1.0, 0.0, 0.0)),
          unitRadii,
          glm::dvec2(1.0, 3.0)},
      TestCase{
          Ray(glm::dvec3(0.0, 2.0, 0.0), glm::dvec3(0.0, -1.0, 0.0)),
          unitRadii,
          glm::dvec2(1.0, 3.0)},
      TestCase{
          Ray(glm::dvec3(0.0, 0.0, 2.0), glm::dvec3(0.0, 0.0, -1.0)),
          unitRadii,
          glm::dvec2{1.0, 3.0}},
      TestCase{
          Ray(glm::dvec3(-2.0, 0.0, 0.0), glm::dvec3(1.0, 0.0, 0.0)),
          unitRadii,
          glm::dvec2(1.0, 3.0)},
      TestCase{
          Ray(glm::dvec3(0.0, -2.0, 0.0), glm::dvec3(0.0, 1.0, 0.0)),
          unitRadii,
          glm::dvec2(1.0, 3.0)},
      TestCase{
          Ray(glm::dvec3(0.0, 0.0, -2.0), glm::dvec3(0.0, 0.0, 1.0)),
          unitRadii,
          glm::dvec2(1.0, 3.0)},

      TestCase{
          Ray(glm::dvec3(-2.0, 0.0, 0.0), glm::dvec3(-1.0, 0.0, 0.0)),
          unitRadii,
          std::nullopt},
      TestCase{
          Ray(glm::dvec3(0.0, -2.0, 0.0), glm::dvec3(0.0, -1.0, 0.0)),
          unitRadii,
          std::nullopt},
      TestCase{
          Ray(glm::dvec3(0.0, 0.0, -2.0), glm::dvec3(0.0, 0.0, -1.0)),
          unitRadii,
          std::nullopt},

      // rayEllipsoid ray inside pointing in intersection
      TestCase{
          Ray(glm::dvec3(20000.0, 0.0, 0.0),
              glm::normalize(glm::dvec3(20000.0, 0.0, 0.0))),
          wgs84Radii,
          glm::dvec2(0.0, wgs84Radii.x - 20000.0)},

      // rayEllipsoid tangent intersections
      TestCase{
          Ray(glm::dvec3(1.0, 0.0, 0.0),
              glm::normalize(glm::dvec3(0.0, 0.0, 1.0))),
          unitRadii,
          std::nullopt},

      // rayEllipsoid no intersections
      TestCase{
          Ray(glm::dvec3(2.0, 0.0, 0.0), glm::dvec3(0.0, 0.0, 1.0)),
          unitRadii,
          std::nullopt},
      TestCase{
          Ray(glm::dvec3(2.0, 0.0, 0.0), glm::dvec3(0.0, 0.0, -1.0)),
          unitRadii,
          std::nullopt},
      TestCase{
          Ray(glm::dvec3(2.0, 0.0, 0.0), glm::dvec3(0.0, 1.0, 0.0)),
          unitRadii,
          std::nullopt},
      TestCase{
          Ray(glm::dvec3(2.0, 0.0, 0.0), glm::dvec3(0.0, -1.0, 0.0)),
          unitRadii,
          std::nullopt}

  };

  for (auto& testCase : testCases) {
    std::optional<glm::dvec2> intersection =
        IntersectionTests::rayEllipsoid(testCase.ray, testCase.inverseRadii);
    CHECK(intersection == testCase.expectedIntersection);
  }
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

  std::vector<TestCase> testCases{
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
          std::nullopt}};

  for (auto& testCase : testCases) {
    std::optional<glm::dvec3> intersectionPoint =
        IntersectionTests::rayTriangle(
            testCase.ray,
            V0,
            V1,
            V2,
            testCase.cullBackFaces);
    CHECK(intersectionPoint == testCase.expectedIntersectionPoint);
  }
}

TEST_CASE("IntersectionTests::rayAABB") {

  struct TestCase {
    Ray ray;
    AxisAlignedBox aabb;
    std::optional<glm::dvec3> expectedIntersectionPoint;
  };

  std::vector<TestCase> testCases{
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
          glm::dvec3(0.0, -1.0, 0.0)}};

  for (auto& testCase : testCases) {
    std::optional<glm::dvec3> intersectionPoint =
        IntersectionTests::rayAABB(testCase.ray, testCase.aabb);
    CHECK(intersectionPoint == testCase.expectedIntersectionPoint);
  }
}

TEST_CASE("IntersectionTests::rayOBB") {
  struct TestCase {
    Ray ray;
    OrientedBoundingBox obb;
    std::optional<glm::dvec3> expectedIntersectionPoint;
  };

  std::vector<TestCase> testCases{
      // 2x2x2 obb at origin that is rotated -45 degrees on the x-axis.
      TestCase{
          Ray(glm::dvec3(0.0, 0.0, 10.0), glm::dvec3(0.0, 0.0, -1.0)),
          OrientedBoundingBox(
              glm::dvec3(0.0, 0.0, 0.0),
              glm::dmat3(
                  glm::rotate(glm::radians(-45.0), glm::dvec3(1.0, 0.0, 0.0)))),
          glm::dvec3(0.0, 0.0, glm::sqrt(2.0))},
      // 2x2x2 obb at (10,10,10) that is rotated -45 degrees on the x-axis.
      TestCase{
          Ray(glm::dvec3(10.0, 10.0, 20.0), glm::dvec3(0.0, 0.0, -1.0)),
          OrientedBoundingBox(
              glm::dvec3(10.0, 10.0, 10.0),
              glm::dmat3(
                  glm::rotate(glm::radians(-45.0), glm::dvec3(1.0, 0.0, 0.0)))),
          glm::dvec3(10.0, 10.0, 10.0 + glm::sqrt(2.0))},
      // 2x2x2 obb at (10,20,30) that is rotated -45 degrees on the x-axis and
      // hit from an angle.
      TestCase{
          Ray(glm::dvec3(10.0, 20.0 + 2.0, 30.0 + 1.0 + glm::sqrt(2)),
              glm::normalize(glm::dvec3(0.0, -2.0, -1.0))),
          OrientedBoundingBox(
              glm::dvec3(10.0, 20.0, 30.0),
              glm::dmat3(
                  glm::rotate(glm::radians(-45.0), glm::dvec3(1.0, 0.0, 0.0)))),
          glm::dvec3(10.0, 20.0, 30.0 + glm::sqrt(2.0))},
      // 4x4x4 obb at (10,10,10) that is rotated -45 degrees on the x-axis.
      TestCase{
          Ray(glm::dvec3(10.0, 10.0, 20.0), glm::dvec3(0.0, 0.0, -1.0)),
          OrientedBoundingBox(
              glm::dvec3(10.0, 10.0, 10.0),
              2.0 * glm::dmat3(glm::rotate(
                        glm::radians(-45.0),
                        glm::dvec3(1.0, 0.0, 0.0)))),
          glm::dvec3(10.0, 10.0, 10.0 + glm::sqrt(8.0))},
      // 4x4x4 obb at (10,20,30) that is rotated -45 degrees on the x-axis and
      // hit from an angle
      TestCase{
          Ray(glm::dvec3(10.0, 20.0 + 10.0, 30.0 + 20.0 + glm::sqrt(8.0)),
              glm::normalize(glm::dvec3(0.0, -1.0, -2.0))),
          OrientedBoundingBox(
              glm::dvec3(10.0, 20.0, 30.0),
              2.0 * glm::dmat3(glm::rotate(
                        glm::radians(-45.0),
                        glm::dvec3(1.0, 0.0, 0.0)))),
          glm::dvec3(10.0, 20.0, 30.0 + glm::sqrt(8.0))},
      // 4x4x2 obb at (10,10,10) that is not rotated.
      TestCase{
          Ray(glm::dvec3(10.0, 10.0, 20.0), glm::dvec3(0.0, 0.0, -1.0)),
          OrientedBoundingBox(
              glm::dvec3(10.0, 10.0, 10.0),
              glm::dmat3(glm::scale(glm::dvec3(2.0, 2.0, 1.0)))),
          glm::dvec3(10.0, 10.0, 10.0 + 1.0)},
      // 4x2x4 obb at (10,20,30) that is not rotated.
      TestCase{
          Ray(glm::dvec3(10.0, 20.0, 40.0), glm::dvec3(0.0, 0.0, -1.0)),
          OrientedBoundingBox(
              glm::dvec3(10.0, 20.0, 30.0),
              glm::dmat3(glm::scale(glm::dvec3(2.0, 1.0, 2.0)))),
          glm::dvec3(10.0, 20.0, 30.0 + 2.0)},
      // 2x4x2 obb at (10,20,30) that is rotated 45 degrees on the Y-axis.
      TestCase{
          Ray(glm::dvec3(10.0, 20.0, 40.0), glm::dvec3(0.0, 0.0, -1.0)),
          OrientedBoundingBox(
              glm::dvec3(10.0, 20.0, 30.0),
              glm::dmat3(glm::scale(glm::dvec3(1.0, 2.0, 1.0))) *
                  glm::dmat3(glm::rotate(
                      glm::radians(45.0),
                      glm::dvec3(0.0, 1.0, 0.0)))),
          glm::dvec3(10.0, 20.0, 30.0 + glm::sqrt(2.0))},
      // 2x4x2 obb at (10,20,30) that is rotated 45 degrees on the X-axis.
      TestCase{
          Ray(glm::dvec3(10.0, 20.0, 40.0), glm::dvec3(0.0, 0.0, -1.0)),
          OrientedBoundingBox(
              glm::dvec3(10.0, 20.0, 30.0),
              glm::dmat3(
                  glm::rotate(glm::radians(45.0), glm::dvec3(1.0, 0.0, 0.0))) *
                  glm::dmat3(glm::scale(glm::dvec3(1.0, 2.0, 1.0)))),
          glm::dvec3(10.0, 20.0, 30.0 + 1.0 / glm::cos(glm::radians(45.0)))},
      // 2x4x2 obb at (10,20,30) that is rotated 225 degrees on the Y-axis.
      TestCase{
          Ray(glm::dvec3(10.0, 20.0, 40.0), glm::dvec3(0.0, 0.0, -1.0)),
          OrientedBoundingBox(
              glm::dvec3(10.0, 20.0, 30.0),
              glm::dmat3(glm::scale(glm::dvec3(1.0, 2.0, 1.0))) *
                  glm::dmat3(glm::rotate(
                      glm::radians(225.0),
                      glm::dvec3(0.0, 1.0, 0.0)))),
          glm::dvec3(10.0, 20.0, 30.0 + glm::sqrt(2.0))},
      // 2x2x4 obb at (10,20,30) that is rotated 90 degrees on the X-axis and
      // hit from an angle.
      TestCase{
          Ray(glm::dvec3(10.0, 20.0 + 2.0, 30.0 + 1.0 + 1.0),
              glm::normalize(glm::dvec3(0.0, -2.0, -1.0))),
          OrientedBoundingBox(
              glm::dvec3(10.0, 20.0, 30.0),
              glm::dmat3(
                  glm::rotate(glm::radians(90.0), glm::dvec3(1.0, 0.0, 0.0))) *
                  glm::dmat3(glm::scale(glm::dvec3(1.0, 1.0, 2.0)))),
          glm::dvec3(10.0, 20.0, 30.0 + 1.0)},
      // 2x2x2 obb at (10,20,30) that is rotated 45 degrees on the X- and
      // Y-axis.
      TestCase{
          Ray(glm::dvec3(10.0, 20.0, 40.0), glm::dvec3(0.0, 0.0, -1.0)),
          OrientedBoundingBox(
              glm::dvec3(10.0, 20.0, 30.0),
              (glm::dmat3(glm::rotate(
                  glm::atan(1.0 / 2.0, glm::sqrt(2) / 2.0),
                  glm::dvec3(1.0, 0.0, 0.0)))) *
                  glm::dmat3(glm::rotate(
                      glm::radians(45.0),
                      glm::dvec3(0.0, 1.0, 0.0)))),
          glm::dvec3(10.0, 20.0, 30.0 + glm::sqrt(3.0))}};

  for (auto& testCase : testCases) {
    std::optional<glm::dvec3> intersectionPoint =
        IntersectionTests::rayOBB(testCase.ray, testCase.obb);
    CHECK(glm::all(glm::lessThan(
        glm::abs(*intersectionPoint - *testCase.expectedIntersectionPoint),
        glm::dvec3(CesiumUtility::Math::Epsilon6))));
  }
}

TEST_CASE("IntersectionTests::raySphere") {
  struct TestCase {
    Ray ray;
    BoundingSphere sphere;
    double t;
  };

  std::vector<TestCase> testCases{
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
          -1.0}};

  for (auto& testCase : testCases) {
    std::optional<double> t =
        IntersectionTests::raySphereParametric(testCase.ray, testCase.sphere);
    if (!t)
      t = -1.0;

    CHECK(CesiumUtility::Math::equalsEpsilon(
        t.value(),
        testCase.t,
        CesiumUtility::Math::Epsilon6));
  }
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

  std::vector<TestCase> testCases{
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
      // Point "inside" degenerate triangle returns true.
      TestCase{
          rightTriangle[0],
          rightTriangle[0],
          rightTriangle[0],
          rightTriangle[2],
          true}};

  for (auto& testCase : testCases) {
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

  std::vector<TestCase> testCases{
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
          false}};

  for (auto& testCase : testCases) {
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

  std::vector<TestCase> testCases{
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
          glm::dvec3()}};

  for (auto& testCase : testCases) {
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
}
