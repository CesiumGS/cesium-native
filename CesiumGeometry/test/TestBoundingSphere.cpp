#include "CesiumGeometry/BoundingSphere.h"
#include "CesiumGeometry/Plane.h"
#include "CesiumUtility/Math.h"

#include <catch2/catch.hpp>
#include <glm/mat3x3.hpp>

using namespace CesiumGeometry;

TEST_CASE("BoundingSphere::intersectPlane") {
  struct TestCase {
    BoundingSphere sphere;
    Plane plane;
    CullingResult expectedResult;
  };

  auto testCase = GENERATE(
      // sphere on the positive side of a plane
      TestCase{
          BoundingSphere(glm::dvec3(0.0), 0.5),
          Plane(glm::dvec3(-1.0, 0.0, 0.0), 1.0),
          CullingResult::Inside},
      // sphere on the negative side of a plane
      TestCase{
          BoundingSphere(glm::dvec3(0.0), 0.5),
          Plane(glm::dvec3(1.0, 0.0, 0.0), -1.0),
          CullingResult::Outside},
      // sphere intersection a plane
      TestCase{
          BoundingSphere(glm::dvec3(1.0, 0.0, 0.0), 0.5),
          Plane(glm::dvec3(1.0, 0.0, 0.0), -1.0),
          CullingResult::Intersecting});

  CHECK(
      testCase.sphere.intersectPlane(testCase.plane) ==
      testCase.expectedResult);
}

TEST_CASE(
    "BoundingSphere::computeDistanceSquaredToPosition test outside sphere") {
  BoundingSphere bs(glm::dvec3(0.0), 1.0);
  glm::dvec3 position(-2.0, 1.0, 0.0);
  double expected = 1.52786405;
  CHECK(CesiumUtility::Math::equalsEpsilon(
      bs.computeDistanceSquaredToPosition(position),
      expected,
      CesiumUtility::Math::Epsilon6));
}

TEST_CASE(
    "BoundingSphere::computeDistanceSquaredToPosition test inside sphere") {
  BoundingSphere bs(glm::dvec3(0.0), 1.0);
  glm::dvec3 position(-.5, 0.5, 0.0);
  CHECK(bs.computeDistanceSquaredToPosition(position) == 0);
}

TEST_CASE("BoundingSphere::computeDistanceSquaredToPosition example") {
  auto anyOldSphereArray = []() {
    return std::vector<BoundingSphere>{
        {glm::dvec3(1.0, 0.0, 0.0), 1.0},
        {glm::dvec3(2.0, 0.0, 0.0), 1.0}};
  };
  auto anyOldCameraPosition = []() { return glm::dvec3(0.0, 0.0, 0.0); };

  //! [distanceSquaredTo]
  // Sort bounding spheres from back to front
  glm::dvec3 cameraPosition = anyOldCameraPosition();
  std::vector<BoundingSphere> spheres = anyOldSphereArray();
  std::sort(
      spheres.begin(),
      spheres.end(),
      [&cameraPosition](auto& a, auto& b) {
        return a.computeDistanceSquaredToPosition(cameraPosition) >
               b.computeDistanceSquaredToPosition(cameraPosition);
      });
  //! [distanceSquaredTo]

  CHECK(spheres[0].getCenter().x == 2.0);
  CHECK(spheres[1].getCenter().x == 1.0);
}
