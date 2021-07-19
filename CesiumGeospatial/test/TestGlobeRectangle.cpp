#include "CesiumGeospatial/GlobeRectangle.h"
#include "catch2/catch.hpp"

TEST_CASE("GlobeRectangle::fromDegrees example") {
  //! [fromDegrees]
  CesiumGeospatial::GlobeRectangle rectangle =
      CesiumGeospatial::GlobeRectangle::fromDegrees(0.0, 20.0, 10.0, 30.0);
  //! [fromDegrees]
  (void)rectangle;
}