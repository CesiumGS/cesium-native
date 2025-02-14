#include "CesiumGeometry/BoundingCylinderRegion.h"

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

TEST_CASE("BoundingCylinderRegion constructor example") {
  //! [Constructor]
  // Create an Bounding Cylinder Region using a translation, rotation,
  // height, and both radial and angular bounds.
  glm::dvec3 translation(1.0, 2.0, 3.0);
  glm::dquat rotation(1.0, 0.0, 0.0, 0.0);
  double height = 2.0;
  glm::dvec2 radialBounds(0.5, 1.0);
  glm::dvec2 angularBounds(-Math::PiOverTwo, 0.0);
  auto cylinder = BoundingCylinderRegion(
      translation,
      rotation,
      height,
      radialBounds,
      angularBounds);
  //! [Constructor]
  (void)cylinder;

  CHECK(cylinder.getTranslation() == translation);
  CHECK(cylinder.getRotation() == rotation);
  CHECK(cylinder.getHeight() == height);
  CHECK(cylinder.getRadialBounds() == radialBounds);
  CHECK(cylinder.getAngularBounds() == angularBounds);
}

TEST_CASE("BoundingCylinderRegion::toOrientedBoundingBox test") {
  SUBCASE("whole cylinder") {
    BoundingCylinderRegion region(
        glm::dvec3(0.0),
        glm::dquat(1.0, 0.0, 0.0, 0.0),
        3.0,
        glm::dvec2(0.0, 2.0));
    CesiumGeometry::OrientedBoundingBox box = region.toOrientedBoundingBox();

    glm::dvec3 expectedCenter(0.0);
    glm::dmat3 expectedHalfAxes(2.0, 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 1.5);

    CHECK(box.getCenter() == expectedCenter);
    CHECK(box.getHalfAxes() == expectedHalfAxes);
  }

  SUBCASE("partial cylinder") {
    BoundingCylinderRegion region(
        glm::dvec3(0.0),
        glm::dquat(1.0, 0.0, 0.0, 0.0),
        3.0,
        glm::dvec2(1.0, 2.0),
        glm::dvec2(0.0, CesiumUtility::Math::PiOverTwo));
    CesiumGeometry::OrientedBoundingBox box = region.toOrientedBoundingBox();

    glm::dvec3 expectedCenter(-1.0, 1.0, 0.0);
    glm::dmat3 expectedHalfAxes(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.5);

    CHECK(box.getCenter() == expectedCenter);
    CHECK(box.getHalfAxes() == expectedHalfAxes);
  }

  SUBCASE("partial cylinder with reversed min-max") {
    BoundingCylinderRegion region(
        glm::dvec3(0.0),
        glm::dquat(1.0, 0.0, 0.0, 0.0),
        3.0,
        glm::dvec2(1.0, 2.0),
        glm::dvec2(
            CesiumUtility::Math::PiOverTwo,
            -CesiumUtility::Math::PiOverTwo));
    CesiumGeometry::OrientedBoundingBox box = region.toOrientedBoundingBox();

    glm::dvec3 expectedCenter(0.0, -1.0, 0.0);
    glm::dmat3 expectedHalfAxes(2.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.5);

    CHECK(CesiumUtility::Math::equalsEpsilon(
        box.getCenter(),
        expectedCenter,
        CesiumUtility::Math::Epsilon6));
    CHECK(box.getHalfAxes() == expectedHalfAxes);
  }

  SUBCASE("transformed partial cylinder") {
    // Rotate 90 degrees clockwise around the Z-axis.
    glm::dquat rotation(glm::rotate(
        glm::dmat4(1.0),
        CesiumUtility::Math::PiOverTwo,
        glm::dvec3(0.0, 0.0, 1.0)));
    glm::dvec3 translation(1.0, 2.0, 3.0);

    BoundingCylinderRegion region(
        translation,
        rotation,
        3.0,
        glm::dvec2(1.0, 2.0),
        glm::dvec2(0.0, CesiumUtility::Math::PiOverTwo));

    CesiumGeometry::OrientedBoundingBox box = region.toOrientedBoundingBox();

    glm::dvec3 expectedCenter(0.0, 1.0, 3.0);
    glm::dmat3 expectedHalfAxes(1.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 1.5);

    CHECK(box.getCenter() == expectedCenter);
    CHECK(box.getHalfAxes() == expectedHalfAxes);
  }
}
