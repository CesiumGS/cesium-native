#include <CesiumGeometry/Rectangle.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/ext/vector_double2.hpp>

#include <cmath>
#include <vector>

using namespace CesiumGeometry;

TEST_CASE("Rectangle::computeSignedDistance") {
  struct TestCase {
    CesiumGeometry::Rectangle rectangle;
    glm::dvec2 position;
    double expectedResult;
  };

  CesiumGeometry::Rectangle positive(10.0, 20.0, 30.0, 40.0);
  CesiumGeometry::Rectangle negative(-30.0, -40.0, -10.0, -20.0);

  std::vector<TestCase> testCases{
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
          std::sqrt(5.0 * 5.0 + 5.0 * 5.0)}};

  for (auto& testCase : testCases) {
    CHECK(CesiumUtility::Math::equalsEpsilon(
        testCase.rectangle.computeSignedDistance(testCase.position),
        testCase.expectedResult,
        CesiumUtility::Math::Epsilon13));
  }
}

TEST_CASE("Rectangle::computeUnion") {
  Rectangle a(1.0, 2.0, 3.0, 4.0);
  Rectangle b(0.0, 0.0, 10.0, 10.0);
  Rectangle c(1.5, 2.5, 3.5, 4.5);
  Rectangle d(0.5, 1.5, 2.5, 3.5);
  Rectangle e(10.0, 11.0, 12.0, 13.0);

  // One rectangle entirely inside another.
  Rectangle intersectionAB = a.computeUnion(b);
  CHECK(intersectionAB.minimumX == 0.0);
  CHECK(intersectionAB.minimumY == 0.0);
  CHECK(intersectionAB.maximumX == 10.0);
  CHECK(intersectionAB.maximumY == 10.0);

  Rectangle intersectionBA = b.computeUnion(a);
  CHECK(intersectionBA.minimumX == 0.0);
  CHECK(intersectionBA.minimumY == 0.0);
  CHECK(intersectionBA.maximumX == 10.0);
  CHECK(intersectionBA.maximumY == 10.0);

  // One rectangle extends outside the other to the lower right
  Rectangle intersectionAC = a.computeUnion(c);
  CHECK(intersectionAC.minimumX == 1.0);
  CHECK(intersectionAC.minimumY == 2.0);
  CHECK(intersectionAC.maximumX == 3.5);
  CHECK(intersectionAC.maximumY == 4.5);

  Rectangle intersectionCA = c.computeUnion(a);
  CHECK(intersectionCA.minimumX == 1.0);
  CHECK(intersectionCA.minimumY == 2.0);
  CHECK(intersectionCA.maximumX == 3.5);
  CHECK(intersectionCA.maximumY == 4.5);

  // One rectangle extends outside the other to the upper left
  Rectangle intersectionAD = a.computeUnion(d);
  CHECK(intersectionAD.minimumX == 0.5);
  CHECK(intersectionAD.minimumY == 1.5);
  CHECK(intersectionAD.maximumX == 3.0);
  CHECK(intersectionAD.maximumY == 4.0);

  Rectangle intersectionDA = d.computeUnion(a);
  CHECK(intersectionDA.minimumX == 0.5);
  CHECK(intersectionDA.minimumY == 1.5);
  CHECK(intersectionDA.maximumX == 3.0);
  CHECK(intersectionDA.maximumY == 4.0);

  // Disjoint rectangles
  Rectangle intersectionAE = a.computeUnion(e);
  CHECK(intersectionAE.minimumX == 1.0);
  CHECK(intersectionAE.minimumY == 2.0);
  CHECK(intersectionAE.maximumX == 12.0);
  CHECK(intersectionAE.maximumY == 13.0);

  Rectangle intersectionEA = e.computeUnion(a);
  CHECK(intersectionEA.minimumX == 1.0);
  CHECK(intersectionEA.minimumY == 2.0);
  CHECK(intersectionEA.maximumX == 12.0);
  CHECK(intersectionEA.maximumY == 13.0);
}
