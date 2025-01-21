#include <CesiumGeometry/CullingResult.h>
#include <CesiumGeometry/Plane.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>
#include <CesiumGeospatial/S2CellID.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/exponential.hpp>
#include <glm/ext/vector_double3.hpp>

#include <span>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

TEST_CASE("S2CellBoundingVolume") {
  S2CellBoundingVolume tileS2Cell(
      S2CellID::fromToken("1"),
      0.0,
      100000.0,
      Ellipsoid::WGS84);

  SUBCASE("distance-squared to position is 0 when camera is inside bounding "
          "volume") {
    CHECK(
        tileS2Cell.computeDistanceSquaredToPosition(tileS2Cell.getCenter()) ==
        0.0);
  }

  SUBCASE(
      "Case I - distanceToCamera works when camera is facing only one plane") {
    const double testDistance = 100.0;

    std::span<const Plane> bvPlanes = tileS2Cell.getBoundingPlanes();

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
        Math::Epsilon7));

    // Test against the first side plane.
    Plane sidePlane0(
        bvPlanes[2].getNormal(),
        bvPlanes[2].getDistance() - testDistance);

    std::span<const glm::dvec3> vertices = tileS2Cell.getVertices();
    glm::dvec3 faceCenter = ((vertices[0] + vertices[1]) * 0.5 +
                             (vertices[4] + vertices[5]) * 0.5) *
                            0.5;
    position = sidePlane0.projectPointOntoPlane(faceCenter);
    CHECK(Math::equalsEpsilon(
        glm::sqrt(tileS2Cell.computeDistanceSquaredToPosition(position)),
        testDistance,
        0.0,
        Math::Epsilon7));
  }

  SUBCASE("Case II - distanceToCamera works when camera is facing two planes") {
    const double testDistance = 5.0;

    // Test with the top plane and the first side plane.
    glm::dvec3 position =
        (tileS2Cell.getVertices()[0] + tileS2Cell.getVertices()[1]) * 0.5;
    position.z -= testDistance;
    CHECK(Math::equalsEpsilon(
        glm::sqrt(tileS2Cell.computeDistanceSquaredToPosition(position)),
        testDistance,
        0.0,
        Math::Epsilon7));

    // Test with first and second side planes.
    position =
        (tileS2Cell.getVertices()[0] + tileS2Cell.getVertices()[4]) * 0.5;
    position.x -= 1;
    position.z -= 1;
    CHECK(Math::equalsEpsilon(
        tileS2Cell.computeDistanceSquaredToPosition(position),
        2.0,
        0.0,
        Math::Epsilon7));

    // Test with bottom plane and second side plane. Handles the obtuse dihedral
    // angle case.
    position =
        (tileS2Cell.getVertices()[5] + tileS2Cell.getVertices()[6]) * 0.5;
    position.x -= 10000;
    position.y -= 1;
    CHECK(Math::equalsEpsilon(
        glm::sqrt(tileS2Cell.computeDistanceSquaredToPosition(position)),
        10000.0,
        0.0,
        Math::Epsilon7));
  }

  SUBCASE(
      "Case III - distanceToCamera works when camera is facing three planes") {
    glm::dvec3 position = tileS2Cell.getVertices()[2] + glm::dvec3(1.0);
    CHECK(Math::equalsEpsilon(
        tileS2Cell.computeDistanceSquaredToPosition(position),
        3.0,
        0.0,
        Math::Epsilon7));
  }

  SUBCASE("Case IV - distanceToCamera works when camera is facing more than "
          "three planes") {
    glm::dvec3 position(-Ellipsoid::WGS84.getMaximumRadius(), 0.0, 0.0);
    CHECK(Math::equalsEpsilon(
        glm::sqrt(tileS2Cell.computeDistanceSquaredToPosition(position)),
        Ellipsoid::WGS84.getMaximumRadius() +
            tileS2Cell.getBoundingPlanes()[1].getDistance(),
        0.0,
        Math::Epsilon7));
  }

  SUBCASE("intersect plane") {
    CHECK(
        tileS2Cell.intersectPlane(Plane::ORIGIN_ZX_PLANE) ==
        CullingResult::Intersecting);

    Plane outsidePlane(
        Plane::ORIGIN_YZ_PLANE.getNormal(),
        Plane::ORIGIN_YZ_PLANE.getDistance() -
            2 * Ellipsoid::WGS84.getMaximumRadius());
    CHECK(tileS2Cell.intersectPlane(outsidePlane) == CullingResult::Outside);

    CHECK(
        tileS2Cell.intersectPlane(Plane::ORIGIN_YZ_PLANE) ==
        CullingResult::Inside);
  }

  SUBCASE("can construct face 2 (North pole)") {
    S2CellBoundingVolume face2Root(
        S2CellID::fromToken("5"),
        1000.0,
        2000.0,
        Ellipsoid::WGS84);
    CHECK(face2Root.getCellID().isValid());
    CHECK(face2Root.getCellID().getID() == 5764607523034234880U);
  }
}
