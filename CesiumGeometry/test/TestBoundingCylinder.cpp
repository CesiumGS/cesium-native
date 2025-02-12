#include "CesiumGeometry/BoundingCylinder.h"

#include <Cesium3DTilesSelection/ViewState.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/string_cast.hpp>

#include <optional>

using namespace CesiumGeometry;
using namespace Cesium3DTilesSelection;
using namespace CesiumUtility;

TEST_CASE("BoundingCylinder constructor example") {
  //! [Constructor]
  // Create an Bounding Cylinder using a transformation matrix, a position
  // where the cylinder will be translated, and a scale.
  glm::dvec3 center = glm::dvec3(1.0, 0.0, 0.0);
  glm::dmat3 halfAxes = glm::dmat3(
      glm::dvec3(2.0, 0.0, 0.0),
      glm::dvec3(0.0, 2.0, 0.0),
      glm::dvec3(0.0, 0.0, 1.5));

  auto cylinder = BoundingCylinder(center, halfAxes);
  //! [Constructor]
  (void)cylinder;

  CHECK(cylinder.getRadius() == 2.0);
  CHECK(cylinder.getHeight() == 3.0);
}

TEST_CASE("BoundingCylinder::computeDistanceSquaredToPosition test") {
  glm::dvec3 center = glm::dvec3(1.0, 0.0, 0.0);
  glm::dmat3 halfAxes = glm::dmat3(
      glm::dvec3(2.0, 0.0, 0.0),
      glm::dvec3(0.0, 2.0, 0.0),
      glm::dvec3(0.0, 0.0, 1.5));
  BoundingCylinder cylinder(center, halfAxes);

  SUBCASE("inside cylinder") {
    glm::dvec3 position(-0.5, 0.5, 0.0);
    CHECK(cylinder.computeDistanceSquaredToPosition(position) == 0);
  }

  SUBCASE("outside cylinder along height") {
    glm::dvec3 position(-3.0, 0.0, 0.0);
    double expected = 4.0;
    CHECK(CesiumUtility::Math::equalsEpsilon(
        cylinder.computeDistanceSquaredToPosition(position),
        expected,
        CesiumUtility::Math::Epsilon6));
  }

  SUBCASE("outside cylinder above disc") {
    glm::dvec3 position(0.0, 0.0, 2.0);
    double expected = 0.25;
    CHECK(CesiumUtility::Math::equalsEpsilon(
        cylinder.computeDistanceSquaredToPosition(position),
        expected,
        CesiumUtility::Math::Epsilon6));
  }

  SUBCASE("outside cylinder 'corner'") {
    glm::dvec3 position(-3.0, 0.0, 2.0);
    double expected = 4.25;
    CHECK(CesiumUtility::Math::equalsEpsilon(
        cylinder.computeDistanceSquaredToPosition(position),
        expected,
        CesiumUtility::Math::Epsilon6));
  }
}

TEST_CASE("BoundingCylinder::contains test") {
  glm::dvec3 center = glm::dvec3(1.0, 0.0, 0.0);
  glm::dmat3 halfAxes = glm::dmat3(
      glm::dvec3(2.0, 0.0, 0.0),
      glm::dvec3(0.0, 2.0, 0.0),
      glm::dvec3(0.0, 0.0, 1.5));
  BoundingCylinder cylinder(center, halfAxes);

  SUBCASE("inside cylinder") {
    glm::dvec3 position(-0.5, 0.5, 0.0);
    CHECK(cylinder.contains(position));
  }

  SUBCASE("outside cylinder") {
    glm::dvec3 position(-3.0, 0.0, 2.0);
    CHECK(!cylinder.contains(position));
  }
}
