#include <Cesium3DTiles/TileRefine.h>
#include <CesiumGltf/ExtensionNode3dTilesTileset.h>
#include <CesiumUtility/Result.h>

#include <doctest/doctest.h>

#include <string>

using namespace Cesium3DTiles;
using namespace CesiumGltf;

TEST_CASE("getTileRefine") {
  SUBCASE("correctly parses tile refine values") {
    std::string replace = ExtensionNode3dTilesTileset::Refine::REPLACE;
    CesiumUtility::Result<TileRefine> result = getTileRefine(replace);
    CHECK_EQ(result.value, TileRefine::Replace);

    std::string add = ExtensionNode3dTilesTileset::Refine::ADD;
    result = getTileRefine(add);
    CHECK_EQ(result.value, TileRefine::Add);
  }

  SUBCASE("handles tile refine values with incorrect casing") {
    std::string replace = "replace";
    CesiumUtility::Result<TileRefine> result = getTileRefine(replace);
    CHECK_EQ(result.value, TileRefine::Replace);
    CHECK_FALSE(result.errors.warnings.empty());

    std::string add = "add";
    result = getTileRefine(add);
    CHECK_EQ(result.value, TileRefine::Add);
    CHECK_FALSE(result.errors.warnings.empty());
  }

  SUBCASE("returns empty result for nonsense values") {
    std::string badInput = "bad input";
    CesiumUtility::Result<TileRefine> result = getTileRefine(badInput);
    CHECK_UNARY_FALSE(result.value);
    CHECK_FALSE(result.errors.errors.empty());
  }
}
