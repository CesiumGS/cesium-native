#include <CesiumNativeTests/RandomVector.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>

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
      CesiumUtility::Math::Epsilon2); // true
  bool b = CesiumUtility::Math::equalsEpsilon(
      0.0,
      0.1,
      CesiumUtility::Math::Epsilon2); // false
  bool c = CesiumUtility::Math::equalsEpsilon(
      3699175.1634344,
      3699175.2,
      CesiumUtility::Math::Epsilon7); // true
  bool d = CesiumUtility::Math::equalsEpsilon(
      3699175.1634344,
      3699175.2,
      CesiumUtility::Math::Epsilon9); // false
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

TEST_CASE("Math::roundUp and roundDown") {
  CHECK(Math::roundUp(1.0, 0.01) == 1.0);
  CHECK(Math::roundDown(1.0, 0.01) == 1.0);

  CHECK(Math::roundUp(1.01, 0.01) == 2.0);
  CHECK(Math::roundDown(1.99, 0.01) == 1.0);

  CHECK(Math::roundUp(1.005, 0.01) == 1.0);
  CHECK(Math::roundDown(1.995, 0.01) == 2.0);

  CHECK(Math::roundUp(-1.0, 0.01) == -1.0);
  CHECK(Math::roundDown(-1.0, 0.01) == -1.0);

  CHECK(Math::roundUp(-1.99, 0.01) == -1.0);
  CHECK(Math::roundDown(-1.01, 0.01) == -2.0);

  CHECK(Math::roundUp(-1.995, 0.01) == -2.0);
  CHECK(Math::roundDown(-1.005, 0.01) == -1.0);
}

TEST_CASE("Math::negativePitoPi") {
  CHECK(Math::negativePiToPi(0.0) == 0.0);
  CHECK(Math::negativePiToPi(+Math::OnePi) == +Math::OnePi);
  CHECK(Math::negativePiToPi(-Math::OnePi) == -Math::OnePi);
  CHECK(Math::negativePiToPi(+Math::OnePi - 1.0) == (+Math::OnePi - 1.0));
  CHECK(Math::negativePiToPi(-Math::OnePi + 1.0) == (-Math::OnePi + 1.0));
  CHECK(Math::negativePiToPi(+Math::OnePi - 0.1) == (+Math::OnePi - 0.1));
  CHECK(Math::negativePiToPi(-Math::OnePi + 0.1) == (-Math::OnePi + 0.1));
  CHECK(Math::equalsEpsilon(
      Math::negativePiToPi(+Math::OnePi + 0.1),
      -Math::OnePi + 0.1,
      Math::Epsilon15));
  CHECK(Math::negativePiToPi(+2.0 * Math::OnePi) == 0.0);
  CHECK(Math::negativePiToPi(-2.0 * Math::OnePi) == 0.0);
  CHECK(Math::negativePiToPi(+3.0 * Math::OnePi) == Math::OnePi);
  CHECK(Math::negativePiToPi(-3.0 * Math::OnePi) == Math::OnePi);
  CHECK(Math::negativePiToPi(+4.0 * Math::OnePi) == 0.0);
  CHECK(Math::negativePiToPi(-4.0 * Math::OnePi) == 0.0);
  CHECK(Math::negativePiToPi(+5.0 * Math::OnePi) == Math::OnePi);
  CHECK(Math::negativePiToPi(-5.0 * Math::OnePi) == Math::OnePi);
  CHECK(Math::negativePiToPi(+6.0 * Math::OnePi) == 0.0);
  CHECK(Math::negativePiToPi(-6.0 * Math::OnePi) == 0.0);
}

TEST_CASE("Math::zeroToTwoPi") {
  CHECK(Math::zeroToTwoPi(0.0) == 0.0);
  CHECK(Math::zeroToTwoPi(+Math::OnePi) == +Math::OnePi);
  CHECK(Math::zeroToTwoPi(-Math::OnePi) == +Math::OnePi);
  CHECK(Math::zeroToTwoPi(+Math::OnePi - 1.0) == (+Math::OnePi - 1.0));
  CHECK(Math::equalsEpsilon(
      Math::zeroToTwoPi(-Math::OnePi + 1.0),
      +Math::OnePi + 1.0,
      Math::Epsilon15));
  CHECK(Math::zeroToTwoPi(+Math::OnePi - 0.1) == (+Math::OnePi - 0.1));
  CHECK(Math::equalsEpsilon(
      Math::zeroToTwoPi(-Math::OnePi + 0.1),
      +Math::OnePi + 0.1,
      Math::Epsilon15));
  CHECK(Math::zeroToTwoPi(+2.0 * Math::OnePi) == (2.0 * Math::OnePi));
  CHECK(Math::zeroToTwoPi(-2.0 * Math::OnePi) == (2.0 * Math::OnePi));
  CHECK(Math::zeroToTwoPi(+3.0 * Math::OnePi) == Math::OnePi);
  CHECK(Math::zeroToTwoPi(-3.0 * Math::OnePi) == Math::OnePi);
  CHECK(Math::zeroToTwoPi(+4.0 * Math::OnePi) == (2.0 * Math::OnePi));
  CHECK(Math::zeroToTwoPi(-4.0 * Math::OnePi) == (2.0 * Math::OnePi));
  CHECK(Math::zeroToTwoPi(+5.0 * Math::OnePi) == Math::OnePi);
  CHECK(Math::zeroToTwoPi(-5.0 * Math::OnePi) == Math::OnePi);
  CHECK(Math::zeroToTwoPi(+6.0 * Math::OnePi) == (2.0 * Math::OnePi));
  CHECK(Math::zeroToTwoPi(-6.0 * Math::OnePi) == (2.0 * Math::OnePi));
}

TEST_CASE("Math::mod") {
  CHECK(Math::mod(0.0, 1.0) == 0.0);
  CHECK(Math::mod(0.1, 1.0) == 0.1);
  CHECK(Math::mod(0.5, 1.0) == 0.5);
  CHECK(Math::mod(1.0, 1.0) == 0.0);
  CHECK(Math::equalsEpsilon(Math::mod(1.1, 1.0), 0.1, Math::Epsilon15));
  CHECK(Math::mod(-0.0, 1.0) == 0.0);
  CHECK(Math::mod(-0.1, 1.0) == 0.9);
  CHECK(Math::mod(-0.5, 1.0) == 0.5);
  CHECK(Math::mod(-1.0, 1.0) == 0.0);
  CHECK(Math::equalsEpsilon(Math::mod(-1.1, 1.0), 0.9, Math::Epsilon15));
  CHECK(Math::mod(0.0, -1.0) == -0.0);
  CHECK(Math::mod(0.1, -1.0) == -0.9);
  CHECK(Math::mod(0.5, -1.0) == -0.5);
  CHECK(Math::mod(1.0, -1.0) == -0.0);
  CHECK(Math::equalsEpsilon(Math::mod(1.1, -1.0), -0.9, Math::Epsilon15));
  CHECK(Math::mod(-0.0, -1.0) == -0.0);
  CHECK(Math::mod(-0.1, -1.0) == -0.1);
  CHECK(Math::mod(-0.5, -1.0) == -0.5);
  CHECK(Math::mod(-1.0, -1.0) == -0.0);
  CHECK(Math::equalsEpsilon(Math::mod(-1.1, -1.0), -0.1, Math::Epsilon15));
}
