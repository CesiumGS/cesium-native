#include <CesiumGeospatial/S2CellBoundingVolume.h>
#include <CesiumGeospatial/S2CellID.h>

#include <catch2/catch.hpp>

using namespace CesiumGeospatial;

TEST_CASE("S2CellBoundingVolume") {
  S2CellBoundingVolume foo(S2CellID::fromToken("3"), 0.0, 10.0);
}
