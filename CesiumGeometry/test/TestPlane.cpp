#include <CesiumGeometry/Plane.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>

#include <doctest/doctest.h>
#include <glm/geometric.hpp>

using namespace CesiumGeometry;

TEST_CASE("Plane constructor throws if a normal is not normalized") {
  CHECK_THROWS(Plane(glm::dvec3(1.0, 2.0, 3.0), 0.0));
  CHECK_THROWS(Plane(glm::dvec3(0.0), glm::dvec3(1.0, 2.0, 3.0)));
}

TEST_CASE("Plane::getPointDistance") {
  glm::dvec3 normal(1.0, 2.0, 3.0);
  normal = glm::normalize(normal);
  Plane plane(normal, 12.34);
  glm::dvec3 point(4.0, 5.0, 6.0);

  CHECK(
      plane.getPointDistance(point) ==
      glm::dot(plane.getNormal(), point) + plane.getDistance());
}

TEST_CASE("Plane::projectPointOntoPlane") {
  Plane plane(glm::dvec3(1.0, 0.0, 0.0), 0.0);
  glm::dvec3 point(1.0, 1.0, 0.0);
  glm::dvec3 result = plane.projectPointOntoPlane(point);
  CHECK(result == glm::dvec3(0.0, 1.0, 0.0));

  plane = Plane(glm::dvec3(0.0, 1.0, 0.0), 0.0);
  result = plane.projectPointOntoPlane(point);
  CHECK(result == glm::dvec3(1.0, 0.0, 0.0));
}

TEST_CASE("Plane constructor from normal and distance") {
  //! [constructor-normal-distance]
  // The plane x=0
  Plane plane(glm::dvec3(1.0, 0.0, 0.0), 0.0);
  //! [constructor-normal-distance]
}

TEST_CASE("Plane constructor from point and normal") {
  //! [constructor-point-normal]
  const CesiumGeospatial::Ellipsoid& ellipsoid =
      CesiumGeospatial::Ellipsoid::WGS84;
  glm::dvec3 point = ellipsoid.cartographicToCartesian(
      CesiumGeospatial::Cartographic::fromDegrees(-72.0, 40.0));
  glm::dvec3 normal = ellipsoid.geodeticSurfaceNormal(point);
  Plane tangentPlane(point, normal);
  //! [constructor-point-normal]
}
