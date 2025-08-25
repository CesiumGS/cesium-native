#include <CesiumGeometry/clipTriangleAtAxisAlignedThreshold.h>

#include <doctest/doctest.h>

#include <vector>

using namespace CesiumGeometry;

TEST_CASE("clipTriangleAtAxisAlignedThreshold") {
  struct TestCase {
    double threshold;
    bool keepAbove;
    int i0;
    int i1;
    int i2;
    double u0;
    double u1;
    double u2;
    std::vector<TriangleClipVertex> calculatedResult;
    std::vector<TriangleClipVertex> expectedResult;
  };

  std::vector<TestCase> testCases{
      // eliminates a triangle that is entirely on the wrong side of the
      // threshold
      TestCase{0.1, false, 0, 1, 2, 0.2, 0.3, 0.4, {}, {}},
      // keeps a triangle that is entirely on the correct side of the threshold
      TestCase{0.1, true, 0, 1, 2, 0.2, 0.3, 0.4, {}, {0, 1, 2}},
      // adds two vertices on threshold when point 0 is on the wrong side and
      // above
      TestCase{
          0.5,
          false,
          0,
          1,
          2,
          0.6,
          0.4,
          0.2,
          {},
          {1,
           2,
           InterpolatedVertex{0, 2, 0.25},
           InterpolatedVertex{0, 1, 0.5}}},
      // adds two vertices on threshold when point 0 is on the wrong side and
      // below
      TestCase{
          0.5,
          true,
          0,
          1,
          2,
          0.4,
          0.6,
          0.8,
          {},
          {1,
           2,
           InterpolatedVertex{0, 2, 0.25},
           InterpolatedVertex{0, 1, 0.5}}},
      // adds two vertices on threshold when point 1 is on the wrong side and
      // above
      TestCase{
          0.5,
          false,
          0,
          1,
          2,
          0.2,
          0.6,
          0.4,
          {},
          {2,
           0,
           InterpolatedVertex{1, 0, 0.25},
           InterpolatedVertex{1, 2, 0.5}}},
      // adds two vertices on threshold when point 1 is on the wrong side and
      // below
      TestCase{
          0.5,
          true,
          0,
          1,
          2,
          0.8,
          0.4,
          0.6,
          {},
          {2,
           0,
           InterpolatedVertex{1, 0, 0.25},
           InterpolatedVertex{1, 2, 0.5}}},
      // adds two vertices on threshold when point 2 is on the wrong side and
      // above
      TestCase{
          0.5,
          false,
          0,
          1,
          2,
          0.4,
          0.2,
          0.6,
          {},
          {0,
           1,
           InterpolatedVertex{2, 1, 0.25},
           InterpolatedVertex{2, 0, 0.5}}},
      // adds two vertices on threshold when point 2 is on the wrong side and
      // below
      TestCase{
          0.5,
          true,
          0,
          1,
          2,
          0.6,
          0.8,
          0.4,
          {},
          {0,
           1,
           InterpolatedVertex{2, 1, 0.25},
           InterpolatedVertex{2, 0, 0.5}}},
      // adds two vertices on threshold when point 1 is on the wrong side and
      // below
      TestCase{
          0.5,
          true,
          0,
          1,
          2,
          0.8,
          0.4,
          0.6,
          {},
          {2,
           0,
           InterpolatedVertex{1, 0, 0.25},
           InterpolatedVertex{1, 2, 0.5}}},
      // adds two vertices on threshold when only point 0 is on the right side
      // and below
      TestCase{
          0.5,
          false,
          0,
          1,
          2,
          0.4,
          0.6,
          0.8,
          {},
          {0, InterpolatedVertex{1, 0, 0.5}, InterpolatedVertex{2, 0, 0.75}}},
      // adds two vertices on threshold when only point 0 is on the right side
      // and above
      TestCase{
          0.5,
          true,
          0,
          1,
          2,
          0.6,
          0.4,
          0.2,
          {},
          {0, InterpolatedVertex{1, 0, 0.5}, InterpolatedVertex{2, 0, 0.75}}},
      // adds two vertices on threshold when only point 1 is on the right side
      // and below
      TestCase{
          0.5,
          false,
          0,
          1,
          2,
          0.8,
          0.4,
          0.6,
          {},
          {1, InterpolatedVertex{2, 1, 0.5}, InterpolatedVertex{0, 1, 0.75}}},
      // adds two vertices on threshold when only point 1 is on the right side
      // and above
      TestCase{
          0.5,
          true,
          0,
          1,
          2,
          0.2,
          0.6,
          0.4,
          {},
          {1, InterpolatedVertex{2, 1, 0.5}, InterpolatedVertex{0, 1, 0.75}}},
      // adds two vertices on threshold when only point 2 is on the right side
      // and below
      TestCase{
          0.5,
          false,
          0,
          1,
          2,
          0.6,
          0.8,
          0.4,
          {},
          {2, InterpolatedVertex{0, 2, 0.5}, InterpolatedVertex{1, 2, 0.75}}},
      // adds two vertices on threshold when only point 2 is on the right side
      // and above
      TestCase{
          0.5,
          true,
          0,
          1,
          2,
          0.4,
          0.2,
          0.6,
          {},
          {2, InterpolatedVertex{0, 2, 0.5}, InterpolatedVertex{1, 2, 0.75}}}};

  for (auto& testCase : testCases) {
    clipTriangleAtAxisAlignedThreshold(
        testCase.threshold,
        testCase.keepAbove,
        testCase.i0,
        testCase.i1,
        testCase.i2,
        testCase.u0,
        testCase.u1,
        testCase.u2,
        testCase.calculatedResult);

    CHECK(testCase.calculatedResult == testCase.expectedResult);
  }
}
