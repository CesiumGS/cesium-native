#include "catch2/catch.hpp"
#include "CesiumGeospatial/GlobeRectangle.h"

TEST_CASE("GlobeRectangle::fromDegrees example") {
    //! [fromDegrees]
    CesiumGeospatial::GlobeRectangle rectangle = CesiumGeospatial::GlobeRectangle::fromDegrees(0.0, 20.0, 10.0, 30.0);
    //! [fromDegrees]
    (void)rectangle;
}