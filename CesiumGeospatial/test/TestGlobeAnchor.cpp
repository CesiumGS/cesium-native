#include <CesiumGeospatial/GlobeAnchor.h>
#include <CesiumGeospatial/LocalHorizontalCoordinateSystem.h>

#include <catch2/catch.hpp>

using namespace CesiumGeospatial;
using namespace CesiumUtility;

TEST_CASE("GlobeAnchor") {
  Cartographic nullIsland = Cartographic(0.0, 0.0, 0.0);

  LocalHorizontalCoordinateSystem leftHandedEastUpNorth(
      nullIsland,
      LocalDirection::East,
      LocalDirection::Up,
      LocalDirection::North);

  SECTION("Identity transform in local is equivalent to the local") {
    GlobeAnchor anchor = GlobeAnchor::fromAnchorToLocalTransform(
        leftHandedEastUpNorth,
        glm::dmat4(1.0));
    CHECK(
        anchor.getAnchorToFixedTransform() ==
        leftHandedEastUpNorth.getLocalToEcefTransformation());
  }

  SECTION("Translation in local is represented correctly in ECEF") {
    GlobeAnchor anchor = GlobeAnchor::fromAnchorToLocalTransform(
        leftHandedEastUpNorth,
        glm::dmat4(
            glm::dvec4(0.0),
            glm::dvec4(0.0),
            glm::dvec4(0.0),
            glm::dvec4(1.0, 2.0, 3.0, 1.0)));
    glm::dvec3 positionInLocal =
        glm::dvec3(anchor.getAnchorToFixedTransform()[3]);
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

  SECTION("Scale in local is represented correctly in ECEF") {
    GlobeAnchor anchor = GlobeAnchor::fromAnchorToLocalTransform(
        leftHandedEastUpNorth,
        glm::dmat4(
            glm::dvec4(0.0),
            glm::dvec4(0.0),
            glm::dvec4(0.0),
            glm::dvec4(1.0, 2.0, 3.0, 1.0)));
    glm::dvec3 positionInLocal =
        glm::dvec3(anchor.getAnchorToFixedTransform()[3]);
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
}
