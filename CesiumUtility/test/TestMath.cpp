#include "CesiumUtility/Math.h"
#include "catch2/catch.hpp"

using namespace CesiumUtility;

TEST_CASE("Math::lerp") {
  CHECK(Math::lerp(1.0, 2.0, 0.0) == 1.0);
  CHECK(Math::lerp(1.0, 2.0, 0.5) == 1.5);
  CHECK(Math::lerp(1.0, 2.0, 1.0) == 2.0);
}

TEST_CASE("Math::lerp example") {
  //! [lerp]
  double n = CesiumUtility::Math::lerp(0.0, 2.0, 0.5); // returns 1.0
  //! [lerp]
  (void)n;
}

TEST_CASE("Math::equalsEpsilon example") {
  //! [equalsEpsilon]
  bool a = CesiumUtility::Math::equalsEpsilon(
      0.0,
      0.01,
      CesiumUtility::Math::EPSILON2); // true
  bool b = CesiumUtility::Math::equalsEpsilon(
      0.0,
      0.1,
      CesiumUtility::Math::EPSILON2); // false
  bool c = CesiumUtility::Math::equalsEpsilon(
      3699175.1634344,
      3699175.2,
      CesiumUtility::Math::EPSILON7); // true
  bool d = CesiumUtility::Math::equalsEpsilon(
      3699175.1634344,
      3699175.2,
      CesiumUtility::Math::EPSILON9); // false
  //! [equalsEpsilon]

  CHECK(a == true);
  CHECK(b == false);
  CHECK(c == true);
  CHECK(d == false);
}

TEST_CASE("Math::convertLongitudeRange example") {
  //! [convertLongitudeRange]
  // Convert 270 degrees to -90 degrees longitude
  double longitude = CesiumUtility::Math::convertLongitudeRange(
      CesiumUtility::Math::degreesToRadians(270.0));
  //! [convertLongitudeRange]
  CHECK(longitude == CesiumUtility::Math::degreesToRadians(-90.0));
}