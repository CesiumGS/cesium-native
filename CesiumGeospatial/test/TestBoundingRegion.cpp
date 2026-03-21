#include <CesiumGeometry/CullingResult.h>
#include <CesiumGeometry/Plane.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>

#include <cmath>
#include <vector>

using namespace CesiumUtility;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;

TEST_CASE("BoundingRegion") {
  SUBCASE("computeDistanceSquaredToPosition") {
    struct TestCase {
      double longitude;
      double latitude;
      double height;
      double expectedDistance;
    };

    const double offset = 0.0001;

    auto updateDistance = [](TestCase testCase,
                             double longitude,
                             double latitude,
                             double height) -> TestCase {
      glm::dvec3 boxPosition = Ellipsoid::WGS84.cartographicToCartesian(
          Cartographic(longitude, latitude, height));
      glm::dvec3 position = Ellipsoid::WGS84.cartographicToCartesian(
          Cartographic(testCase.longitude, testCase.latitude, testCase.height));
      testCase.expectedDistance = glm::distance(boxPosition, position);
      return testCase;
    };

    BoundingRegion region(
        GlobeRectangle(-0.001, -0.001, 0.001, 0.001),
        0.0,
        10.0,
        Ellipsoid::WGS84);

    const GlobeRectangle& rectangle = region.getRectangle();

    std::vector<TestCase> testCases{// Inside bounding region
                                    TestCase{
                                        rectangle.getWest() + Math::Epsilon6,
                                        rectangle.getSouth(),
                                        region.getMinimumHeight(),
                                        0.0},
                                    // Outside bounding region
                                    TestCase{
                                        rectangle.getWest(),
                                        rectangle.getSouth(),
                                        region.getMaximumHeight() + 1.0,
                                        1.0},
                                    // Inside rectangle, above height
                                    TestCase{0.0, 0.0, 20.0, 10.0},
                                    // Inside rectangle, below height
                                    TestCase{0.0, 0.0, 5.0, 0.0},
                                    // From southwest
                                    updateDistance(
                                        TestCase{
                                            rectangle.getEast() + offset,
                                            rectangle.getNorth() + offset,
                                            0.0,
                                            0.0},
                                        rectangle.getEast(),
                                        rectangle.getNorth(),
                                        0.0),
                                    // From northeast
                                    updateDistance(
                                        TestCase{
                                            rectangle.getWest() - offset,
                                            rectangle.getSouth() - offset,
                                            0.0,
                                            0.0},
                                        rectangle.getWest(),
                                        rectangle.getSouth(),
                                        0.0)};

    for (auto& testCase : testCases) {
      glm::dvec3 position = Ellipsoid::WGS84.cartographicToCartesian(
          Cartographic(testCase.longitude, testCase.latitude, testCase.height));
      CHECK(Math::equalsEpsilon(
          sqrt(region.computeDistanceSquaredToPosition(
              position,
              Ellipsoid::WGS84)),
          testCase.expectedDistance,
          Math::Epsilon6));
    }
  }

  SUBCASE("computeDistanceSquaredToPosition with degenerate region") {
    struct TestCase {
      double longitude;
      double latitude;
      double height;
      double expectedDistance;
    };

    auto updateDistance = [](TestCase testCase,
                             double longitude,
                             double latitude,
                             double height) -> TestCase {
      glm::dvec3 boxPosition = Ellipsoid::WGS84.cartographicToCartesian(
          Cartographic(longitude, latitude, height));
      glm::dvec3 position = Ellipsoid::WGS84.cartographicToCartesian(
          Cartographic(testCase.longitude, testCase.latitude, testCase.height));
      testCase.expectedDistance = glm::distance(boxPosition, position);
      return testCase;
    };

    BoundingRegion region(
        GlobeRectangle(-1.03, 0.2292, -1.03, 0.2292),
        0.0,
        3.0,
        Ellipsoid::WGS84);

    std::vector<TestCase> testCases{
        TestCase{-1.03, 0.2292, 4.0, 1.0},
        TestCase{-1.03, 0.2292, 3.0, 0.0},
        TestCase{-1.03, 0.2292, 2.0, 0.0},
        updateDistance(TestCase{-1.02, 0.2291, 2.0, 0.0}, -1.03, 0.2292, 2.0)};

    for (auto& testCase : testCases) {
      glm::dvec3 position = Ellipsoid::WGS84.cartographicToCartesian(
          Cartographic(testCase.longitude, testCase.latitude, testCase.height));
      CHECK(Math::equalsEpsilon(
          sqrt(region.computeDistanceSquaredToPosition(
              position,
              Ellipsoid::WGS84)),
          testCase.expectedDistance,
          Math::Epsilon6));
    }
  }

  SUBCASE("intersectPlane") {
    BoundingRegion region(
        GlobeRectangle(0.0, 0.0, 1.0, 1.0),
        0.0,
        1.0,
        Ellipsoid::WGS84);

    Plane plane(
        glm::normalize(Ellipsoid::WGS84.cartographicToCartesian(
            Cartographic(0.0, 0.0, 1.0))),
        -glm::distance(
            glm::dvec3(0.0),
            Ellipsoid::WGS84.cartographicToCartesian(
                Cartographic(0.0, 0.0, 0.0))));

    CHECK(region.intersectPlane(plane) == CullingResult::Intersecting);
  }
}
