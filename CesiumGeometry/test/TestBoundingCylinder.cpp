#include "CesiumGeometry/BoundingCylinder.h"

#include <Cesium3DTilesSelection/ViewState.h>
#include <CesiumUtility/Math.h>

#include <catch2/catch.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/string_cast.hpp>

#include <optional>

using namespace CesiumGeometry;
using namespace Cesium3DTilesSelection;
using namespace CesiumUtility;

TEST_CASE("BoundingCylinder constructor example") {
  //! [Constructor]
  // Create an Bounding Cylinder using a transformation matrix, a position
  // where the cylinder will be translated, and a scale.
  glm::dvec3 center = glm::dvec3(1.0, 0.0, 0.0);
  glm::dmat3 halfAxes = glm::dmat3(
      glm::dvec3(2.0, 0.0, 0.0),
      glm::dvec3(0.0, 2.0, 0.0),
      glm::dvec3(0.0, 0.0, 1.5));

  auto cylinder = BoundingCylinder(center, halfAxes);
  //! [Constructor]
  (void)cylinder;

  CHECK(cylinder.getRadius() == 2.0);
  CHECK(cylinder.getHeight() == 3.0);
}
