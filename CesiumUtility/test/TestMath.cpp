#include "CesiumNativeTests/RandomVector.h"
#include "CesiumUtility/Math.h"

#include <catch2/catch.hpp>
#include <glm/glm.hpp>

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

TEST_CASE("Math::perpVec") {
  glm::vec3 v0(.2f, .3f, .4f);
  glm::vec3 perp = Math::perpVec(v0);
  // perp is normalized
  glm::vec3 mutual = glm::cross(v0, perp);
  CHECK(Math::equalsEpsilon(length(v0), length(mutual), Math::Epsilon5));
  glm::vec3 v1(.3f, .2f, -1.0f);
  glm::vec3 perp1 = Math::perpVec(v1);
  glm::vec3 mutual1 = glm::cross(v1, perp1);
  CHECK(Math::equalsEpsilon(length(v1), length(mutual1), Math::Epsilon5));
}

TEST_CASE("Math::rotation") {
  CesiumNativeTests::RandomUnitVectorGenerator<glm::vec3> generator;
  for (int i = 0; i < 100; ++i) {
    glm::vec3 vec1 = generator();
    glm::vec3 vec2 = generator();
    glm::quat rotation = Math::rotation(vec1, vec2);
    // Not a unit vector!
    glm::vec3 axis(rotation.x, rotation.y, rotation.z);
    // Is the rotation axis perpendicular to vec1 and vec2?
    CHECK(Math::equalsEpsilon(dot(vec1, axis), 0.0f, Math::Epsilon5));
    CHECK(Math::equalsEpsilon(dot(vec2, axis), 0.0f, Math::Epsilon5));
    // Does the quaternion match the trig values we get with dot and cross?
    const float c = dot(vec1, vec2);
    const float s = length(cross(vec1, vec2));
    const float qc = rotation.w;
    const float qs = length(axis);
    // Double angle formulae
    float testSin = 2.0f * qs * qc;
    float testCos = qc * qc - qs * qs;
    CHECK(Math::equalsEpsilon(s, testSin, Math::Epsilon5));
    CHECK(Math::equalsEpsilon(c, testCos, Math::Epsilon5));
  }
  for (int i = 0; i < 100; ++i) {
    glm::vec3 vec = generator();
    glm::quat rotation = Math::rotation(vec, vec);
    CHECK(Math::equalsEpsilon(rotation.w, 1.0f, Math::Epsilon5));
  }
  for (int i = 0; i < 100; ++i) {
    glm::vec3 vec1 = generator();
    glm::vec3 vec2 = vec1 * -1.0f;
    glm::quat rotation = Math::rotation(vec1, vec2);
    glm::vec3 axis(rotation.x, rotation.y, rotation.z);
    CHECK(Math::equalsEpsilon(rotation.w, 0.0f, Math::Epsilon5));
    CHECK(Math::equalsEpsilon(dot(vec1, axis), 0.0f, Math::Epsilon5));
  }
}
