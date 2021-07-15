#include "CesiumGeometry/Rectangle.h"
#include "CesiumUtility/Math.h"
#include <catch2/catch.hpp>

TEST_CASE("Rectangle::computeSignedDistance") {
  struct TestCase {
    CesiumGeometry::Rectangle rectangle;
    glm::dvec2 position;
    double expectedResult;
  };

  CesiumGeometry::Rectangle positive(10.0, 20.0, 30.0, 40.0);
  CesiumGeometry::Rectangle negative(-30.0, -40.0, -10.0, -20.0);

  auto testCase = GENERATE_REF(
      TestCase{positive, glm::dvec2(20.0, 30.0), -10.0},
      TestCase{negative, glm::dvec2(-20.0, -30.0), -10.0},
      TestCase{positive, glm::dvec2(-5.0, 30.0), 15.0},
      TestCase{negative, glm::dvec2(5.0, -30.0), 15.0},
      TestCase{positive, glm::dvec2(45.0, 30.0), 15.0},
      TestCase{negative, glm::dvec2(-45.0, -30.0), 15.0},
      TestCase{positive, glm::dvec2(20.0, 5.0), 15.0},
      TestCase{negative, glm::dvec2(-20.0, -5.0), 15.0},
      TestCase{positive, glm::dvec2(20.0, 55.0), 15.0},
      TestCase{negative, glm::dvec2(-20.0, -55.0), 15.0},
      TestCase{
          positive,
          glm::dvec2(5.0, 15.0),
          std::sqrt(5.0 * 5.0 + 5.0 * 5.0)},
      TestCase{
          negative,
          glm::dvec2(-5.0, -15.0),
          std::sqrt(5.0 * 5.0 + 5.0 * 5.0)},
      TestCase{
          positive,
          glm::dvec2(5.0, 45.0),
          std::sqrt(5.0 * 5.0 + 5.0 * 5.0)},
      TestCase{
          negative,
          glm::dvec2(-5.0, -45.0),
          std::sqrt(5.0 * 5.0 + 5.0 * 5.0)},
      TestCase{
          positive,
          glm::dvec2(35.0, 15.0),
          std::sqrt(5.0 * 5.0 + 5.0 * 5.0)},
      TestCase{
          negative,
          glm::dvec2(-35.0, -15.0),
          std::sqrt(5.0 * 5.0 + 5.0 * 5.0)},
      TestCase{
          positive,
          glm::dvec2(35.0, 45.0),
          std::sqrt(5.0 * 5.0 + 5.0 * 5.0)},
      TestCase{
          negative,
          glm::dvec2(-35.0, -45.0),
          std::sqrt(5.0 * 5.0 + 5.0 * 5.0)});

  CHECK(CesiumUtility::Math::equalsEpsilon(
      testCase.rectangle.computeSignedDistance(testCase.position),
      testCase.expectedResult,
      CesiumUtility::Math::EPSILON13));
}
