#include <CesiumGeospatial/S2CellBoundingVolume.h>
#include <CesiumGeospatial/S2CellID.h>

#include <catch2/catch.hpp>
#include <glm/geometric.hpp>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

TEST_CASE("S2CellBoundingVolume") {
  SECTION("distance-squared to position is 0 when camera is inside bounding "
          "volume") {
    S2CellBoundingVolume bv(S2CellID::fromToken("1"), 0.0, 100000.0);
    CHECK(bv.computeDistanceSquaredToPosition(bv.getCenter()) == 0.0);
  }

  SECTION("distanceToCamera works when camera is facing only one plane") {
    S2CellBoundingVolume tileS2Cell(S2CellID::fromToken("1"), 0.0, 100000.0);
    double testDistance = 100.0;

    gsl::span<const Plane> bvPlanes = tileS2Cell.getBoundingPlanes();

    // Test against the top plane.
    Plane topPlane(
        bvPlanes[0].getNormal(),
        bvPlanes[0].getDistance() - testDistance);
    glm::dvec3 position =
        topPlane.projectPointOntoPlane(tileS2Cell.getCenter());
    CHECK(Math::equalsEpsilon(
        glm::sqrt(tileS2Cell.computeDistanceSquaredToPosition(position)),
        testDistance,
        0.0,
        Math::EPSILON7));

    // Test against the first side plane.
    Plane sidePlane0(
        bvPlanes[2].getNormal(),
        bvPlanes[2].getDistance() - testDistance);

    gsl::span<const glm::dvec3> vertices = tileS2Cell.getVertices();
    glm::dvec3 faceCenter = ((vertices[0] + vertices[1]) * 0.5 +
                             (vertices[4] + vertices[5]) * 0.5) *
                            0.5;
    position = sidePlane0.projectPointOntoPlane(faceCenter);
    CHECK(Math::equalsEpsilon(
        glm::sqrt(tileS2Cell.computeDistanceSquaredToPosition(position)),
        testDistance,
        0.0,
        Math::EPSILON7));
  }
}
