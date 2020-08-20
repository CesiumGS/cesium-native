#include "catch2/catch.hpp"
#include "CesiumUtility/Math.h"

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
