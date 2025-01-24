#include <CesiumGeometry/Transforms.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeAnchor.h>
#include <CesiumGeospatial/LocalHorizontalCoordinateSystem.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/fwd.hpp>
#include <glm/gtx/quaternion.hpp>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

TEST_CASE("GlobeAnchor") {
  Cartographic nullIsland = Cartographic(0.0, 0.0, 0.0);

  LocalHorizontalCoordinateSystem leftHandedEastUpNorth(
      nullIsland,
      LocalDirection::East,
      LocalDirection::Up,
      LocalDirection::North,
      1.0,
      Ellipsoid::WGS84);

  LocalHorizontalCoordinateSystem leftHandedEastUpNorth90(
      Cartographic::fromDegrees(90.0, 0.0, 0.0),
      LocalDirection::East,
      LocalDirection::Up,
      LocalDirection::North,
      1.0,
      Ellipsoid::WGS84);

  SUBCASE("Identity transform in local is equivalent to the local") {
    GlobeAnchor anchor = GlobeAnchor::fromAnchorToLocalTransform(
        leftHandedEastUpNorth,
        glm::dmat4(1.0));
    CHECK(
        anchor.getAnchorToFixedTransform() ==
        leftHandedEastUpNorth.getLocalToEcefTransformation());
  }

  SUBCASE("Translation in local is represented correctly in ECEF") {
    GlobeAnchor anchor = GlobeAnchor::fromAnchorToLocalTransform(
        leftHandedEastUpNorth,
        glm::dmat4(
            glm::dvec4(1.0, 0.0, 0.0, 0.0),
            glm::dvec4(0.0, 1.0, 0.0, 0.0),
            glm::dvec4(0.0, 0.0, 1.0, 0.0),
            glm::dvec4(1.0, 2.0, 3.0, 1.0)));
    glm::dvec3 originInEcef =
        glm::dvec3(leftHandedEastUpNorth.getLocalToEcefTransformation()[3]);

    // +X in local is East, which is +Y in ECEF.
    // +Y in local is Up, which is +X in ECEF.
    // +Z in local is North, which is +Z in ECEF.
    glm::dvec3 expectedPositionInEcef =
        originInEcef + glm::dvec3(2.0, 1.0, 3.0);
    glm::dvec3 actualPositionInEcef =
        glm::dvec3(anchor.getAnchorToFixedTransform()[3]);

    CHECK(Math::equalsEpsilon(
        expectedPositionInEcef,
        actualPositionInEcef,
        0.0,
        Math::Epsilon10));
  }

  SUBCASE(
      "Translation-rotation-scale in local is represented correctly in ECEF") {
    glm::dquat ninetyDegreesAboutX =
        glm::angleAxis(Math::degreesToRadians(90.0), glm::dvec3(1.0, 0.0, 0.0));
    glm::dmat4 anchorToLocal = Transforms::createTranslationRotationScaleMatrix(
        glm::dvec3(1.0, 2.0, 3.0),
        ninetyDegreesAboutX,
        glm::dvec3(30.0, 20.0, 10.0));

    GlobeAnchor anchor = GlobeAnchor::fromAnchorToLocalTransform(
        leftHandedEastUpNorth,
        anchorToLocal);

    glm::dvec3 localPosition = glm::dvec3(7.0, 8.0, 9.0);
    glm::dvec3 actualPositionInEcef = glm::dvec3(
        anchor.getAnchorToFixedTransform() * glm::dvec4(localPosition, 1.0));
    glm::dvec3 expectedPositionInEcef = glm::dvec3(
        leftHandedEastUpNorth.getLocalToEcefTransformation() *
        (anchorToLocal * glm::dvec4(localPosition, 1.0)));

    CHECK(Math::equalsEpsilon(
        expectedPositionInEcef,
        actualPositionInEcef,
        0.0,
        Math::Epsilon10));
  }

  SUBCASE("Can transform between different local coordinate systems") {
    glm::dmat4 toLocal = glm::dmat4(
        glm::dvec4(1.0, 0.0, 0.0, 0.0),
        glm::dvec4(0.0, 1.0, 0.0, 0.0),
        glm::dvec4(0.0, 0.0, 1.0, 0.0),
        glm::dvec4(1.0, 2.0, 3.0, 1.0));
    GlobeAnchor anchor =
        GlobeAnchor::fromAnchorToLocalTransform(leftHandedEastUpNorth, toLocal);

    glm::dmat4 toLocal90 =
        anchor.getAnchorToLocalTransform(leftHandedEastUpNorth90);

    glm::dvec3 somePosition = glm::dvec3(123.0, 456.0, 789.0);
    glm::dvec3 positionInLocal90 =
        glm::dvec3(toLocal90 * glm::dvec4(somePosition, 1.0));

    // +X in old local is East, which is +Y in ECEF, which is +Y in new local.
    // +Y in old local is Up, which is +X in ECEF, which is -X in new local.
    // +Z in old local is North, which is +Z in ECEF, which is +Z in new local.
    glm::dvec3 oldOriginEcef =
        glm::dvec3(Ellipsoid::WGS84.getMaximumRadius(), 0.0, 0.0);
    glm::dvec3 newOriginEcef =
        glm::dvec3(0.0, Ellipsoid::WGS84.getMaximumRadius(), 0.0);
    glm::dvec3 offsetEcef = newOriginEcef - oldOriginEcef;
    glm::dvec3 offset90 = glm::dvec3(-offsetEcef.x, offsetEcef.y, offsetEcef.z);
    glm::dvec3 expectedPositionInLocal90 = -offset90 +
                                           glm::dvec3(-2.0, 1.0, 3.0) +
                                           glm::dvec3(-456.0, 123.0, 789.0);

    CHECK(Math::equalsEpsilon(
        expectedPositionInLocal90,
        positionInLocal90,
        0.0,
        Math::Epsilon10));
  }

  SUBCASE("Moving in ECEF adjusts orientation if requested") {
    glm::dmat4 toLocal = glm::dmat4(
        glm::dvec4(1.0, 0.0, 0.0, 0.0),
        glm::dvec4(0.0, 1.0, 0.0, 0.0),
        glm::dvec4(0.0, 0.0, 1.0, 0.0),
        glm::dvec4(0.0, 0.0, 0.0, 1.0));
    GlobeAnchor anchor =
        GlobeAnchor::fromAnchorToLocalTransform(leftHandedEastUpNorth, toLocal);

    // Moving without adjusting orientation should leave the orientation
    // unchanged.
    GlobeAnchor first = anchor;
    first.setAnchorToLocalTransform(
        leftHandedEastUpNorth90,
        toLocal,
        false,
        Ellipsoid::WGS84);
    glm::dmat3 rotationScaleAfter =
        glm::dmat3(first.getAnchorToLocalTransform(leftHandedEastUpNorth90));
    CHECK(Math::equalsEpsilon(
        rotationScaleAfter[0],
        glm::dmat3(1.0)[0],
        0.0,
        Math::Epsilon10));
    CHECK(Math::equalsEpsilon(
        rotationScaleAfter[1],
        glm::dmat3(1.0)[1],
        0.0,
        Math::Epsilon10));
    CHECK(Math::equalsEpsilon(
        rotationScaleAfter[2],
        glm::dmat3(1.0)[2],
        0.0,
        Math::Epsilon10));

    // But if we allow adjusting orientation, the object should stay upright.
    GlobeAnchor second = anchor;
    second.setAnchorToLocalTransform(
        leftHandedEastUpNorth90,
        toLocal,
        true,
        Ellipsoid::WGS84);
    rotationScaleAfter =
        glm::dmat3(second.getAnchorToLocalTransform(leftHandedEastUpNorth90));
    glm::dmat3 expected = glm::dmat3(
        glm::dvec3(0.0, -1.0, 0.0),
        glm::dvec3(1.0, 0.0, 0.0),
        glm::dvec3(0.0, 0.0, 1.0));
    CHECK(Math::equalsEpsilon(
        rotationScaleAfter[0],
        expected[0],
        0.0,
        Math::Epsilon10));
    CHECK(Math::equalsEpsilon(
        rotationScaleAfter[1],
        expected[1],
        0.0,
        Math::Epsilon10));
    CHECK(Math::equalsEpsilon(
        rotationScaleAfter[2],
        expected[2],
        0.0,
        Math::Epsilon10));
  }
}
