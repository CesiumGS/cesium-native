#include <Cesium3DTilesContent/registerAllTileContentTypes.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumNativeTests/FileAccessor.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumUtility/Uri.h>

#include <catch2/catch.hpp>

#include <filesystem>

using namespace Cesium3DTilesContent;
using namespace Cesium3DTilesSelection;
using namespace CesiumAsync;
using namespace CesiumGeospatial;
using namespace CesiumNativeTests;
using namespace CesiumUtility;

namespace {

std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;

}

TEST_CASE("Tileset height queries") {
  // The coordinates and expected heights in this file were determined in Cesium
  // for Unreal Engine by adding the tileset, putting a cube above the location
  // of interest, adding a CesiumGlobeAnchor to it, and pressing the "End" key
  // to drop it onto terrain. The coordinates were then copied out of the globe
  // anchor, subtracting 0.5 from the height to account for "End" placing the
  // bottom of the cube on the surface instead of its center.

  registerAllTileContentTypes();

  std::shared_ptr<IAssetAccessor> pAccessor =
      std::make_shared<CesiumNativeTests::FileAccessor>();
  AsyncSystem asyncSystem(std::make_shared<SimpleTaskProcessor>());

  TilesetExternals externals{pAccessor, nullptr, asyncSystem, nullptr};

  SECTION("Additive-refined tileset") {
    std::string url =
        "file://" + Uri::nativePathToUriPath(
                        (testDataPath / "Tileset" / "tileset.json").u8string());

    Tileset tileset(externals, url);

    Future<Tileset::HeightResults> future = tileset.getHeightsAtCoordinates(
        // A point on geometry in "parent.b3dm", which should only be included
        // because this tileset is additive-refine.
        {Cartographic::fromDegrees(-75.612088, 40.042526, 0.0),

         // A point on geometry in a leaf tile.
         Cartographic::fromDegrees(-75.612025, 40.041684, 0.0)});

    while (!future.isReady()) {
      tileset.updateView({});
    }

    Tileset::HeightResults results = future.waitInMainThread();
    CHECK(results.warnings.empty());
    REQUIRE(results.coordinateResults.size() == 2);

    CHECK(results.coordinateResults[0].heightAvailable);
    CHECK(Math::equalsEpsilon(
        results.coordinateResults[0].coordinate.height,
        78.155809,
        0.0,
        Math::Epsilon4));

    CHECK(results.coordinateResults[1].heightAvailable);
    CHECK(Math::equalsEpsilon(
        results.coordinateResults[1].coordinate.height,
        7.837332,
        0.0,
        Math::Epsilon4));
  }

  SECTION("Replace-refined tileset") {
    std::string url =
        "file://" +
        Uri::nativePathToUriPath(
            (testDataPath / "ReplaceTileset" / "tileset.json").u8string());

    Tileset tileset(externals, url);

    Future<Tileset::HeightResults> future = tileset.getHeightsAtCoordinates(
        // A point on geometry in "parent.b3dm", which should not be included
        // because this tileset is replace-refine.
        {Cartographic::fromDegrees(-75.612088, 40.042526, 0.0),

         // A point on geometry in a leaf tile.
         Cartographic::fromDegrees(-75.612025, 40.041684, 0.0)});

    while (!future.isReady()) {
      tileset.updateView({});
    }

    Tileset::HeightResults results = future.waitInMainThread();
    CHECK(results.warnings.empty());
    REQUIRE(results.coordinateResults.size() == 2);

    CHECK(!results.coordinateResults[0].heightAvailable);

    CHECK(results.coordinateResults[1].heightAvailable);
    CHECK(Math::equalsEpsilon(
        results.coordinateResults[1].coordinate.height,
        7.837332,
        0.0,
        Math::Epsilon4));
  }
}
