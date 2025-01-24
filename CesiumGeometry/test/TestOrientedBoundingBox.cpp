#include <CesiumGeometry/AxisAlignedBox.h>
#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/CullingResult.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeometry/Plane.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/string_cast.hpp>

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

using namespace doctest;
using namespace CesiumGeometry;
using namespace CesiumUtility;

struct TestCase {
  glm::dvec3 center;
  glm::dmat3 axes;
};

void testIntersectPlane(const TestCase& testCase) {
  CAPTURE(testCase);
  const double SQRT1_2 = pow(1.0 / 2.0, 1 / 2.0);
  const double SQRT3_4 = pow(3.0 / 4.0, 1 / 2.0);

  auto box = OrientedBoundingBox(testCase.center, testCase.axes * 0.5);

  std::string s = glm::to_string(box.getHalfAxes());

  auto planeNormXform =
      [&testCase](double nx, double ny, double nz, double dist) {
        auto n = glm::dvec3(nx, ny, nz);
        auto arb = glm::dvec3(357, 924, 258);
        auto p0 = glm::normalize(n) * -dist;
        auto tang = glm::normalize(glm::cross(n, arb));
        auto binorm = glm::normalize(glm::cross(n, tang));

        p0 = testCase.axes * p0;
        tang = testCase.axes * tang;
        binorm = testCase.axes * binorm;

        n = glm::cross(tang, binorm);
        if (glm::length(n) == 0.0) {
          return std::optional<Plane>();
        }
        n = glm::normalize(n);

        p0 += testCase.center;
        double d = -glm::dot(p0, n);
        if (glm::abs(d) > 0.0001 && glm::dot(n, n) > 0.0001) {
          return std::optional(Plane(n, d));
        }

        return std::optional<Plane>();
      };

  std::optional<Plane> pl;

  // Tests against faces

  pl = planeNormXform(+1.0, +0.0, +0.0, 0.50001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(-1.0, +0.0, +0.0, 0.50001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(+0.0, +1.0, +0.0, 0.50001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(+0.0, -1.0, +0.0, 0.50001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(+0.0, +0.0, +1.0, 0.50001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(+0.0, +0.0, -1.0, 0.50001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));

  pl = planeNormXform(+1.0, +0.0, +0.0, 0.49999);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(-1.0, +0.0, +0.0, 0.49999);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+0.0, +1.0, +0.0, 0.49999);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+0.0, -1.0, +0.0, 0.49999);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+0.0, +0.0, +1.0, 0.49999);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+0.0, +0.0, -1.0, 0.49999);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));

  pl = planeNormXform(+1.0, +0.0, +0.0, -0.49999);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(-1.0, +0.0, +0.0, -0.49999);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+0.0, +1.0, +0.0, -0.49999);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+0.0, -1.0, +0.0, -0.49999);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+0.0, +0.0, +1.0, -0.49999);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+0.0, +0.0, -1.0, -0.49999);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));

  pl = planeNormXform(+1.0, +0.0, +0.0, -0.50001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(-1.0, +0.0, +0.0, -0.50001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(+0.0, +1.0, +0.0, -0.50001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(+0.0, -1.0, +0.0, -0.50001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(+0.0, +0.0, +1.0, -0.50001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(+0.0, +0.0, -1.0, -0.50001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));

  // Tests against edges

  pl = planeNormXform(+1.0, +1.0, +0.0, SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(+1.0, -1.0, +0.0, SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(-1.0, +1.0, +0.0, SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(-1.0, -1.0, +0.0, SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(+1.0, +0.0, +1.0, SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(+1.0, +0.0, -1.0, SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(-1.0, +0.0, +1.0, SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(-1.0, +0.0, -1.0, SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(+0.0, +1.0, +1.0, SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(+0.0, +1.0, -1.0, SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(+0.0, -1.0, +1.0, SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(+0.0, -1.0, -1.0, SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));

  pl = planeNormXform(+1.0, +1.0, +0.0, SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+1.0, -1.0, +0.0, SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(-1.0, +1.0, +0.0, SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(-1.0, -1.0, +0.0, SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+1.0, +0.0, +1.0, SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+1.0, +0.0, -1.0, SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(-1.0, +0.0, +1.0, SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(-1.0, +0.0, -1.0, SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+0.0, +1.0, +1.0, SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+0.0, +1.0, -1.0, SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+0.0, -1.0, +1.0, SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+0.0, -1.0, -1.0, SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));

  pl = planeNormXform(+1.0, +1.0, +0.0, -SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+1.0, -1.0, +0.0, -SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(-1.0, +1.0, +0.0, -SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(-1.0, -1.0, +0.0, -SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+1.0, +0.0, +1.0, -SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+1.0, +0.0, -1.0, -SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(-1.0, +0.0, +1.0, -SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(-1.0, +0.0, -1.0, -SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+0.0, +1.0, +1.0, -SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+0.0, +1.0, -1.0, -SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+0.0, -1.0, +1.0, -SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+0.0, -1.0, -1.0, -SQRT1_2 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));

  pl = planeNormXform(+1.0, +1.0, +0.0, -SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(+1.0, -1.0, +0.0, -SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(-1.0, +1.0, +0.0, -SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(-1.0, -1.0, +0.0, -SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(+1.0, +0.0, +1.0, -SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(+1.0, +0.0, -1.0, -SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(-1.0, +0.0, +1.0, -SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(-1.0, +0.0, -1.0, -SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(+0.0, +1.0, +1.0, -SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(+0.0, +1.0, -1.0, -SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(+0.0, -1.0, +1.0, -SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(+0.0, -1.0, -1.0, -SQRT1_2 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));

  // Tests against corners

  pl = planeNormXform(+1.0, +1.0, +1.0, SQRT3_4 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(+1.0, +1.0, -1.0, SQRT3_4 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(+1.0, -1.0, +1.0, SQRT3_4 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(+1.0, -1.0, -1.0, SQRT3_4 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(-1.0, +1.0, +1.0, SQRT3_4 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(-1.0, +1.0, -1.0, SQRT3_4 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(-1.0, -1.0, +1.0, SQRT3_4 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
  pl = planeNormXform(-1.0, -1.0, -1.0, SQRT3_4 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));

  pl = planeNormXform(+1.0, +1.0, +1.0, SQRT3_4 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+1.0, +1.0, -1.0, SQRT3_4 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+1.0, -1.0, +1.0, SQRT3_4 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+1.0, -1.0, -1.0, SQRT3_4 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(-1.0, +1.0, +1.0, SQRT3_4 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(-1.0, +1.0, -1.0, SQRT3_4 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(-1.0, -1.0, +1.0, SQRT3_4 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(-1.0, -1.0, -1.0, SQRT3_4 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));

  pl = planeNormXform(+1.0, +1.0, +1.0, -SQRT3_4 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+1.0, +1.0, -1.0, -SQRT3_4 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+1.0, -1.0, +1.0, -SQRT3_4 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(+1.0, -1.0, -1.0, -SQRT3_4 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(-1.0, +1.0, +1.0, -SQRT3_4 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(-1.0, +1.0, -1.0, -SQRT3_4 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(-1.0, -1.0, +1.0, -SQRT3_4 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
  pl = planeNormXform(-1.0, -1.0, -1.0, -SQRT3_4 + 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));

  pl = planeNormXform(+1.0, +1.0, +1.0, -SQRT3_4 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(+1.0, +1.0, -1.0, -SQRT3_4 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(+1.0, -1.0, +1.0, -SQRT3_4 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(+1.0, -1.0, -1.0, -SQRT3_4 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(-1.0, +1.0, +1.0, -SQRT3_4 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(-1.0, +1.0, -1.0, -SQRT3_4 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(-1.0, -1.0, +1.0, -SQRT3_4 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
  pl = planeNormXform(-1.0, -1.0, -1.0, -SQRT3_4 - 0.00001);
  CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
}

TEST_CASE("OrientedBoundingBox::intersectPlane") {
  std::vector<TestCase> testCases{
      // untransformed
      TestCase{glm::dvec3(0.0), glm::dmat3(1.0)},
      // off-center
      TestCase{glm::dvec3(1.0, 0.0, 0.0), glm::dmat3(1.0)},
      TestCase{glm::dvec3(0.7, -1.8, 12.0), glm::dmat3(1.0)},
      // rotated
      TestCase{
          glm::dvec3(0.0),
          glm::dmat3(
              glm::rotate(glm::dmat4(), 1.2, glm::dvec3(0.5, 1.5, -1.2)))},
      // scaled
      TestCase{
          glm::dvec3(0.0),
          glm::dmat3(glm::scale(glm::dmat4(), glm::dvec3(1.5, 0.4, 20.6)))},
      TestCase{
          glm::dvec3(0.0),
          glm::dmat3(glm::scale(glm::dmat4(), glm::dvec3(0.0, 0.4, 20.6)))},
      TestCase{
          glm::dvec3(0.0),
          glm::dmat3(glm::scale(glm::dmat4(), glm::dvec3(1.5, 0.0, 20.6)))},
      TestCase{
          glm::dvec3(0.0),
          glm::dmat3(glm::scale(glm::dmat4(), glm::dvec3(1.5, 0.4, 0.0)))},
      TestCase{
          glm::dvec3(0.0),
          glm::dmat3(glm::scale(glm::dmat4(), glm::dvec3(0.0, 0.0, 0.0)))},
      // arbitrary box
      TestCase{
          glm::dvec3(-5.1, 0.0, 0.1),
          glm::dmat3(glm::rotate(
              glm::scale(glm::dmat4(), glm::dvec3(1.5, 80.4, 2.6)),
              1.2,
              glm::dvec3(0.5, 1.5, -1.2)))}};

  for (auto& testCase : testCases) {
    testIntersectPlane(testCase);
  }
}

TEST_CASE("OrientedBoundingBox constructor example") {
  //! [Constructor]
  // Create an OrientedBoundingBox using a transformation matrix, a position
  // where the box will be translated, and a scale.
  glm::dvec3 center = glm::dvec3(1.0, 0.0, 0.0);
  glm::dmat3 halfAxes = glm::dmat3(
      glm::dvec3(1.0, 0.0, 0.0),
      glm::dvec3(0.0, 3.0, 0.0),
      glm::dvec3(0.0, 0.0, 2.0));

  auto obb = OrientedBoundingBox(center, halfAxes);
  //! [Constructor]
  (void)obb;
}

TEST_CASE("OrientedBoundingBox::computeDistanceSquaredToPosition example") {
  auto anyOldBoxArray = []() {
    return std::vector<OrientedBoundingBox>{
        {glm::dvec3(1.0, 0.0, 0.0), glm::dmat3(1.0)},
        {glm::dvec3(2.0, 0.0, 0.0), glm::dmat3(1.0)}};
  };
  auto anyOldCameraPosition = []() { return glm::dvec3(0.0, 0.0, 0.0); };

  //! [distanceSquaredTo]
  // Sort bounding boxes from back to front
  glm::dvec3 cameraPosition = anyOldCameraPosition();
  std::vector<OrientedBoundingBox> boxes = anyOldBoxArray();
  std::sort(boxes.begin(), boxes.end(), [&cameraPosition](auto& a, auto& b) {
    return a.computeDistanceSquaredToPosition(cameraPosition) >
           b.computeDistanceSquaredToPosition(cameraPosition);
  });
  //! [distanceSquaredTo]

  CHECK(boxes[0].getCenter().x == 2.0);
  CHECK(boxes[1].getCenter().x == 1.0);
}

TEST_CASE("OrientedBoundingBox::toAxisAligned") {
  SUBCASE("simple box that is already axis-aligned") {
    OrientedBoundingBox obb(
        glm::dvec3(1.0, 2.0, 3.0),
        glm::dmat3(
            glm::dvec3(10.0, 0.0, 0.0),
            glm::dvec3(0.0, 20.0, 0.0),
            glm::dvec3(0.0, 0.0, 30.0)));
    AxisAlignedBox aabb = obb.toAxisAligned();
    CHECK(aabb.minimumX == -9.0);
    CHECK(aabb.maximumX == 11.0);
    CHECK(aabb.minimumY == -18.0);
    CHECK(aabb.maximumY == 22.0);
    CHECK(aabb.minimumZ == -27.0);
    CHECK(aabb.maximumZ == 33.0);
  }

  SUBCASE("a truly oriented box") {
    // Rotate the OBB 45 degrees around the Y-axis
    double fortyFiveDegrees = Math::OnePi / 4.0;
    glm::dmat3 rotation = glm::dmat3(glm::eulerAngleY(fortyFiveDegrees));
    OrientedBoundingBox obb(glm::dvec3(1.0, 2.0, 3.0), rotation);

    AxisAlignedBox aabb = obb.toAxisAligned();
    CHECK(Math::equalsEpsilon(aabb.minimumX, 1.0 - glm::sqrt(2.0), 0.0, 1e-14));
    CHECK(Math::equalsEpsilon(aabb.maximumX, 1.0 + glm::sqrt(2.0), 0.0, 1e-14));
    CHECK(Math::equalsEpsilon(aabb.minimumY, 2.0 - 1.0, 0.0, 1e-14));
    CHECK(Math::equalsEpsilon(aabb.maximumY, 2.0 + 1.0, 0.0, 1e-14));
    CHECK(Math::equalsEpsilon(aabb.minimumZ, 3.0 - glm::sqrt(2.0), 0.0, 1e-14));
    CHECK(Math::equalsEpsilon(aabb.maximumZ, 3.0 + glm::sqrt(2.0), 0.0, 1e-14));
  }
}

TEST_CASE("OrientedBoundingBox::toSphere") {
  SUBCASE("axis-aligned box with identity half-axes") {
    OrientedBoundingBox obb(
        glm::dvec3(1.0, 2.0, 3.0),
        glm::dmat3(
            glm::dvec3(1.0, 0.0, 0.0),
            glm::dvec3(0.0, 1.0, 0.0),
            glm::dvec3(0.0, 0.0, 1.0)));

    BoundingSphere sphere = obb.toSphere();
    CHECK(sphere.getCenter().x == Approx(1.0));
    CHECK(sphere.getCenter().y == Approx(2.0));
    CHECK(sphere.getCenter().z == Approx(3.0));

    CHECK(sphere.getRadius() == Approx(glm::sqrt(3.0)));
  }

  SUBCASE("rotating the box does not change the bounding sphere") {
    // Rotate the OBB 45 degrees around the Y-axis.
    // This shouldn't change the bounding sphere at all.
    double fortyFiveDegrees = Math::OnePi / 4.0;
    glm::dmat3 rotation = glm::dmat3(glm::eulerAngleY(fortyFiveDegrees));
    OrientedBoundingBox obb(glm::dvec3(1.0, 2.0, 3.0), rotation);

    BoundingSphere sphere = obb.toSphere();
    CHECK(sphere.getCenter().x == Approx(1.0));
    CHECK(sphere.getCenter().y == Approx(2.0));
    CHECK(sphere.getCenter().z == Approx(3.0));

    CHECK(sphere.getRadius() == Approx(glm::sqrt(3.0)));
  }

  SUBCASE("a scaled axis-aligned box") {
    OrientedBoundingBox obb(
        glm::dvec3(1.0, 2.0, 3.0),
        glm::dmat3(
            glm::dvec3(10.0, 0.0, 0.0),
            glm::dvec3(0.0, 20.0, 0.0),
            glm::dvec3(0.0, 0.0, 30.0)));

    BoundingSphere sphere = obb.toSphere();
    CHECK(sphere.getCenter().x == Approx(1.0));
    CHECK(sphere.getCenter().y == Approx(2.0));
    CHECK(sphere.getCenter().z == Approx(3.0));

    CHECK(
        sphere.getRadius() ==
        Approx(glm::sqrt(10.0 * 10.0 + 20.0 * 20.0 + 30.0 * 30.0)));
  }
}

TEST_CASE("OrientedBoundingBox::contains") {
  SUBCASE("axis-aligned") {
    OrientedBoundingBox obb(
        glm::dvec3(10.0, 20.0, 30.0),
        glm::dmat3(
            glm::dvec3(2.0, 0.0, 0.0),
            glm::dvec3(0.0, 3.0, 0.0),
            glm::dvec3(0.0, 0.0, 4.0)));
    CHECK(!obb.contains(glm::dvec3(0.0, 0.0, 0.0)));
    CHECK(obb.contains(glm::dvec3(10.0, 20.0, 30.0)));
    CHECK(obb.contains(glm::dvec3(12.0, 23.0, 34.0)));
    CHECK(obb.contains(glm::dvec3(8.0, 17.0, 26.0)));
    CHECK(!obb.contains(glm::dvec3(13.0, 20.0, 30.0)));
    CHECK(!obb.contains(glm::dvec3(10.0, 24.0, 30.0)));
    CHECK(!obb.contains(glm::dvec3(10.0, 20.0, 35.0)));
  }

  SUBCASE("rotated") {
    // Rotate the OBB 45 degrees around the Y-axis
    double fortyFiveDegrees = Math::OnePi / 4.0;
    glm::dmat3 halfAngles(
        glm::dvec3(2.0, 0.0, 0.0),
        glm::dvec3(0.0, 3.0, 0.0),
        glm::dvec3(0.0, 0.0, 4.0));
    glm::dmat3 rotation = glm::dmat3(glm::eulerAngleY(fortyFiveDegrees));
    glm::dmat3 transformed = rotation * halfAngles;
    glm::dvec3 center(10.0, 20.0, 30.0);
    OrientedBoundingBox obb(center, transformed);

    CHECK(!obb.contains(glm::dvec3(0.0, 0.0, 0.0)));
    CHECK(obb.contains(center));
    CHECK(obb.contains(center + rotation * glm::dvec3(2.0, 3.0, 4.0)));
    CHECK(obb.contains(center + rotation * glm::dvec3(-2.0, -3.0, -4.0)));
    CHECK(!obb.contains(center + rotation * glm::dvec3(3.0, 0.0, 0.0)));
    CHECK(!obb.contains(center + rotation * glm::dvec3(0.0, 4.0, 0.0)));
    CHECK(!obb.contains(center + rotation * glm::dvec3(0.0, 0.0, 5.0)));
  }
}
