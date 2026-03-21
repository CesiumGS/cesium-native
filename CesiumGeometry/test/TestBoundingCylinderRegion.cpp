#include <Cesium3DTilesSelection/ViewState.h>
#include <CesiumGeometry/BoundingCylinderRegion.h>
#include <CesiumGeometry/Transforms.h>
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

    glm::dvec3 expectedCenter(1.0, 1.0, 0.0);
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

    glm::dvec3 expectedCenter(-1.0, 0.0, 0.0);
    glm::dmat3 expectedHalfAxes(1.0, 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 1.5);

    CHECK(CesiumUtility::Math::equalsEpsilon(
        box.getCenter(),
        expectedCenter,
        CesiumUtility::Math::Epsilon6));
    CHECK(box.getHalfAxes() == expectedHalfAxes);
  }

  SUBCASE("transformed partial cylinder") {
    // Rotate 90 degrees counter-clockwise around the Z-axis.
    glm::dquat rotation(CesiumGeometry::Transforms::X_UP_TO_Y_UP);
    glm::dvec3 translation(1.0, 2.0, 3.0);

    BoundingCylinderRegion region(
        translation,
        rotation,
        3.0,
        glm::dvec2(1.0, 2.0),
        glm::dvec2(0.0, CesiumUtility::Math::PiOverTwo));

    CesiumGeometry::OrientedBoundingBox box = region.toOrientedBoundingBox();

    glm::dvec3 expectedCenter(0.0, 3.0, 3.0);
    glm::dmat3 expectedHalfAxes(0.0, 1.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 1.5);

    CHECK(CesiumUtility::Math::equalsEpsilon(
        box.getCenter(),
        expectedCenter,
        CesiumUtility::Math::Epsilon6));

    const glm::dmat3& halfAxes = box.getHalfAxes();
    for (glm::length_t i = 0; i < 3; i++) {
      CHECK(CesiumUtility::Math::equalsEpsilon(
          halfAxes[i],
          expectedHalfAxes[i],
          CesiumUtility::Math::Epsilon6));
    }
  }
}

TEST_CASE("BoundingCylinderRegion::transform test") {
  glm::dmat4 transform(CesiumGeometry::Transforms::Z_UP_TO_Y_UP);
  transform[3] = glm::dvec4(1.0, 2.0, 3.0, 1.0);

  SUBCASE("solid cylinder") {
    BoundingCylinderRegion region(
        glm::dvec3(0.0),
        glm::dquat(1.0, 0.0, 0.0, 0.0),
        3.0,
        glm::dvec2(0.0, 2.0));

    BoundingCylinderRegion transformedRegion = region.transform(transform);
    CHECK(transformedRegion.getTranslation() == glm::dvec3(1.0, 2.0, 3.0));
    CHECK(
        transformedRegion.getRotation() ==
        glm::dquat(CesiumGeometry::Transforms::Z_UP_TO_Y_UP));

    CesiumGeometry::OrientedBoundingBox box =
        transformedRegion.toOrientedBoundingBox();

    glm::dvec3 expectedCenter(1.0, 2.0, 3.0);
    glm::dmat3 expectedHalfAxes(2.0, 0.0, 0.0, 0.0, 0.0, -2.0, 0.0, 1.5, 0.0);

    CHECK(box.getCenter() == expectedCenter);

    const glm::dmat3& halfAxes = box.getHalfAxes();
    for (glm::length_t i = 0; i < 3; i++) {
      CHECK(CesiumUtility::Math::equalsEpsilon(
          halfAxes[i],
          expectedHalfAxes[i],
          CesiumUtility::Math::Epsilon6));
    }
  }

  SUBCASE("partial cylinder") {
    BoundingCylinderRegion region(
        glm::dvec3(0.0),
        glm::dquat(1.0, 0.0, 0.0, 0.0),
        3.0,
        glm::dvec2(1.0, 2.0),
        glm::dvec2(0.0, CesiumUtility::Math::PiOverTwo));

    BoundingCylinderRegion transformedRegion = region.transform(transform);
    CHECK(transformedRegion.getTranslation() == glm::dvec3(1.0, 2.0, 3.0));
    CHECK(
        transformedRegion.getRotation() ==
        glm::dquat(CesiumGeometry::Transforms::Z_UP_TO_Y_UP));

    CesiumGeometry::OrientedBoundingBox box =
        transformedRegion.toOrientedBoundingBox();

    glm::dvec3 expectedCenter(2.0, 2.0, 2.0);
    glm::dmat3 expectedHalfAxes(1.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 1.5, 0.0);

    CHECK(box.getCenter() == expectedCenter);

    const glm::dmat3& halfAxes = box.getHalfAxes();
    for (glm::length_t i = 0; i < 3; i++) {
      CHECK(CesiumUtility::Math::equalsEpsilon(
          halfAxes[i],
          expectedHalfAxes[i],
          CesiumUtility::Math::Epsilon6));
    }
  }

  SUBCASE("transformed partial cylinder") {
    glm::dquat rotation(CesiumGeometry::Transforms::X_UP_TO_Z_UP);
    glm::dvec3 translation(-1.0, 0.0, 1.0);

    BoundingCylinderRegion region(
        translation,
        rotation,
        3.0,
        glm::dvec2(1.0, 2.0),
        glm::dvec2(0.0, CesiumUtility::Math::PiOverTwo));

    // Verify construction before the additional transform.
    {
      CesiumGeometry::OrientedBoundingBox box = region.toOrientedBoundingBox();
      glm::dvec3 expectedCenter(-1.0, 1.0, 2.0);
      glm::dmat3 expectedHalfAxes(0.0, 0.0, 1.0, 0.0, 1.0, 0.0, -1.5, 0.0, 0);

      CHECK(CesiumUtility::Math::equalsEpsilon(
          box.getCenter(),
          expectedCenter,
          CesiumUtility::Math::Epsilon6));

      const glm::dmat3& halfAxes = box.getHalfAxes();
      for (glm::length_t i = 0; i < 3; i++) {
        CHECK(CesiumUtility::Math::equalsEpsilon(
            halfAxes[i],
            expectedHalfAxes[i],
            CesiumUtility::Math::Epsilon6));
      }
    }

    BoundingCylinderRegion transformedRegion = region.transform(transform);

    {
      glm::dmat4 finalTransform =
          transform * glm::translate(glm::dmat4(1.0), translation) *
          glm::dmat4(CesiumGeometry::Transforms::X_UP_TO_Z_UP);

      glm::dvec3 expectedTranslation(0.0);
      glm::dquat expectedRotation(1.0, 0.0, 0.0, 0.0);

      CesiumGeometry::Transforms::computeTranslationRotationScaleFromMatrix(
          finalTransform,
          &expectedTranslation,
          &expectedRotation,
          nullptr);

      CHECK(transformedRegion.getTranslation() == expectedTranslation);
      glm::dquat transformedRotation = transformedRegion.getRotation();
      for (glm::length_t i = 0; i < 3; i++) {
        CHECK(CesiumUtility::Math::equalsEpsilon(
            transformedRotation[i],
            expectedRotation[i],
            CesiumUtility::Math::Epsilon6));
      }
      CesiumGeometry::OrientedBoundingBox box =
          transformedRegion.toOrientedBoundingBox();
      glm::dvec3 expectedCenter(0.0, 4.0, 2.0);
      glm::dmat3
          expectedHalfAxes(0.0, 1.0, 0.0, 0.0, 0.0, -1.0, -1.5, 0.0, 0.0);

      CHECK(CesiumUtility::Math::equalsEpsilon(
          box.getCenter(),
          expectedCenter,
          CesiumUtility::Math::Epsilon6));

      const glm::dmat3& halfAxes = box.getHalfAxes();
      for (glm::length_t i = 0; i < 3; i++) {
        CHECK(CesiumUtility::Math::equalsEpsilon(
            halfAxes[i],
            expectedHalfAxes[i],
            CesiumUtility::Math::Epsilon6));
      }
    }
  }
}
