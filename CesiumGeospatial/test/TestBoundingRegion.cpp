#include "catch2/catch.hpp"
#include "CesiumGeospatial/BoundingRegion.h"
#include "CesiumGeospatial/Rectangle.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumUtility/Math.h"
#include "CesiumGeometry/Plane.h"

using namespace CesiumUtility;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;

TEST_CASE("BoundingRegion") {
    SECTION("computeDistanceSquaredToPosition") {
        struct TestCase {
            double longitude;
            double latitude;
            double height;
            double expectedDistance;
        };

        const double offset = 0.0001;

        auto updateDistance = [](TestCase testCase, double longitude, double latitude, double height) -> TestCase {
            glm::dvec3 boxPosition = Ellipsoid::WGS84.cartographicToCartesian(Cartographic(longitude, latitude, height));
            glm::dvec3 position = Ellipsoid::WGS84.cartographicToCartesian(Cartographic(testCase.longitude, testCase.latitude, testCase.height));
            testCase.expectedDistance = glm::distance(boxPosition, position);
            return testCase;
        };

        BoundingRegion region(
            Rectangle(-0.001, -0.001, 0.001, 0.001),
            0.0,
            10.0
        );

        const Rectangle& rectangle = region.getRectangle();

        auto testCase = GENERATE_COPY(
            // Inside bounding region
            TestCase { rectangle.getWest() + Math::EPSILON6, rectangle.getSouth(), region.getMinimumHeight(), 0.0 },
            // Outside bounding region
            TestCase { rectangle.getWest(), rectangle.getSouth(), region.getMaximumHeight() + 1.0, 1.0 },
            // Inside rectangle, above height
            TestCase { 0.0, 0.0, 20.0, 10.0 },
            // Inside rectangle, below height
            TestCase { 0.0, 0.0, 5.0, 0.0 },
            // From southwest
            updateDistance(TestCase {
                rectangle.getEast() + offset,
                rectangle.getNorth() + offset,
                0.0,
                0.0
            }, rectangle.getEast(), rectangle.getNorth(), 0.0),
            // From northeast
            updateDistance(TestCase {
                rectangle.getWest() - offset,
                rectangle.getSouth() - offset,
                0.0,
                0.0
            }, rectangle.getWest(), rectangle.getSouth(), 0.0)
        );

        glm::dvec3 position = Ellipsoid::WGS84.cartographicToCartesian(
            Cartographic(testCase.longitude, testCase.latitude, testCase.height)
        );
        CHECK(Math::equalsEpsilon(sqrt(region.computeDistanceSquaredToPosition(position)), testCase.expectedDistance, Math::EPSILON6));
    }

    SECTION("intersectPlane") {
        BoundingRegion region(
            Rectangle(0.0, 0.0, 1.0, 1.0),
            0.0,
            1.0
        );

        Plane plane(
            glm::normalize(Ellipsoid::WGS84.cartographicToCartesian(Cartographic(0.0, 0.0, 1.0))),
            -glm::distance(glm::dvec3(0.0), Ellipsoid::WGS84.cartographicToCartesian(Cartographic(0.0, 0.0, 0.0)))
        );

        CHECK(region.intersectPlane(plane) == CullingResult::Intersecting);
    }
}
