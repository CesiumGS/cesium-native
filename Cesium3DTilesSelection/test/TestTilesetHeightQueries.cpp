#include <Cesium3DTilesContent/registerAllTileContentTypes.h>
#include <Cesium3DTilesSelection/EllipsoidTilesetLoader.h>
#include <Cesium3DTilesSelection/SampleHeightResult.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumNativeTests/FileAccessor.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumUtility/Math.h>
#include <CesiumUtility/StringHelpers.h>
#include <CesiumUtility/Uri.h>

#include <doctest/doctest.h>

#include <filesystem>
#include <memory>
#include <string>

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

  SUBCASE("Additive-refined tileset") {
    std::string url =
        "file://" +
        Uri::nativePathToUriPath(StringHelpers::toStringUtf8(
            (testDataPath / "Tileset" / "tileset.json").u8string()));

    Tileset tileset(externals, url);

    Future<SampleHeightResult> future = tileset.sampleHeightMostDetailed(
        // A point on geometry in "parent.b3dm", which should only be included
        // because this tileset is additive-refined.
        {Cartographic::fromDegrees(-75.612088, 40.042526, 0.0),

         // A point on geometry in a leaf tile.
         Cartographic::fromDegrees(-75.612025, 40.041684, 0.0)});

    while (!future.isReady()) {
      tileset.updateView({});
    }

    SampleHeightResult results = future.waitInMainThread();
    CHECK(results.warnings.empty());
    REQUIRE(results.positions.size() == 2);

    CHECK(results.sampleSuccess[0]);
    CHECK(Math::equalsEpsilon(
        results.positions[0].height,
        78.155809,
        0.0,
        Math::Epsilon4));

    CHECK(results.sampleSuccess[1]);
    CHECK(Math::equalsEpsilon(
        results.positions[1].height,
        7.837332,
        0.0,
        Math::Epsilon4));
  }

  SUBCASE("Replace-refined tileset") {
    std::string url =
        "file://" +
        Uri::nativePathToUriPath(StringHelpers::toStringUtf8(
            (testDataPath / "ReplaceTileset" / "tileset.json").u8string()));

    Tileset tileset(externals, url);

    Future<SampleHeightResult> future = tileset.sampleHeightMostDetailed(
        // A point on geometry in "parent.b3dm", which should not be
        // included because this tileset is replace-refined.
        {Cartographic::fromDegrees(-75.612088, 40.042526, 0.0),

         // A point on geometry in a leaf tile.
         Cartographic::fromDegrees(-75.612025, 40.041684, 0.0)});

    while (!future.isReady()) {
      tileset.updateView({});
    }

    SampleHeightResult results = future.waitInMainThread();
    CHECK(results.warnings.empty());
    REQUIRE(results.positions.size() == 2);

    CHECK(!results.sampleSuccess[0]);

    CHECK(results.sampleSuccess[1]);
    CHECK(Math::equalsEpsilon(
        results.positions[1].height,
        7.837332,
        0.0,
        Math::Epsilon4));
  }

  SUBCASE("External tileset") {
    std::string url =
        "file://" +
        Uri::nativePathToUriPath(StringHelpers::toStringUtf8(
            (testDataPath / "AddTileset" / "tileset.json").u8string()));

    Tileset tileset(externals, url);

    Future<SampleHeightResult> future = tileset.sampleHeightMostDetailed(
        // A point on geometry in "0/0/0.b3dm", which should only be
        // included because this tileset is additive-refined.
        {Cartographic::fromDegrees(-75.612088, 40.042526, 0.0),

         // A point on geometry in a leaf tile.
         Cartographic::fromDegrees(-75.612025, 40.041684, 0.0)});

    while (!future.isReady()) {
      tileset.updateView({});
    }

    SampleHeightResult results = future.waitInMainThread();
    CHECK(results.warnings.empty());
    REQUIRE(results.positions.size() == 2);

    CHECK(results.sampleSuccess[0]);
    CHECK(Math::equalsEpsilon(
        results.positions[0].height,
        78.155809,
        0.0,
        Math::Epsilon4));

    CHECK(results.sampleSuccess[1]);
    CHECK(Math::equalsEpsilon(
        results.positions[1].height,
        7.837332,
        0.0,
        Math::Epsilon4));
  }

  SUBCASE("Implicit tileset") {
    std::string url =
        "file://" + Uri::nativePathToUriPath(StringHelpers::toStringUtf8(
                        (testDataPath / "ImplicitTileset" / "tileset_1.1.json")
                            .u8string()));

    Tileset tileset(externals, url);

    Future<SampleHeightResult> future = tileset.sampleHeightMostDetailed(
        // A point on geometry in "0/0/0.b3dm", which should only be
        // included because this tileset is additive-refined.
        {Cartographic::fromDegrees(-75.612088, 40.042526, 0.0),

         // A point on geometry in a leaf tile.
         Cartographic::fromDegrees(-75.612025, 40.041684, 0.0)});

    while (!future.isReady()) {
      tileset.updateView({});
    }

    SampleHeightResult results = future.waitInMainThread();
    CHECK(results.warnings.empty());
    REQUIRE(results.positions.size() == 2);

    CHECK(results.sampleSuccess[0]);
    CHECK(Math::equalsEpsilon(
        results.positions[0].height,
        78.155809,
        0.0,
        Math::Epsilon4));

    CHECK(results.sampleSuccess[1]);
    CHECK(Math::equalsEpsilon(
        results.positions[1].height,
        7.837332,
        0.0,
        Math::Epsilon4));
  }

  SUBCASE("Instanced model is not yet supported") {
    std::string url =
        "file://" +
        Uri::nativePathToUriPath(StringHelpers::toStringUtf8(
            (testDataPath / "i3dm" / "InstancedWithBatchTable" / "tileset.json")
                .u8string()));

    Tileset tileset(externals, url);

    Future<SampleHeightResult> future = tileset.sampleHeightMostDetailed(
        {Cartographic::fromDegrees(-75.612559, 40.042183, 0.0)});

    while (!future.isReady()) {
      tileset.updateView({});
    }

    SampleHeightResult results = future.waitInMainThread();
    REQUIRE(results.warnings.size() == 1);
    REQUIRE(results.positions.size() == 1);
    CHECK(!results.sampleSuccess[0]);
    CHECK(
        results.warnings[0].find("EXT_mesh_gpu_instancing") !=
        std::string::npos);
  }

  SUBCASE("broken tileset") {
    Tileset tileset(externals, "http://localhost/notgonnawork");

    Future<SampleHeightResult> future = tileset.sampleHeightMostDetailed(
        {Cartographic::fromDegrees(-75.612559, 40.042183, 0.0)});

    while (!future.isReady()) {
      tileset.updateView({});
    }

    SampleHeightResult results = future.waitInMainThread();
    REQUIRE(results.warnings.size() == 1);
    REQUIRE(results.positions.size() == 1);
    REQUIRE(results.sampleSuccess.size() == 1);
    CHECK(!results.sampleSuccess[0]);
    CHECK(results.warnings[0].find("failed to load") != std::string::npos);
  }

  SUBCASE("ellipsoid tileset") {
    std::unique_ptr<Tileset> pTileset =
        EllipsoidTilesetLoader::createTileset(externals);

    Future<SampleHeightResult> future = pTileset->sampleHeightMostDetailed(
        {Cartographic::fromDegrees(-75.612559, 40.042183, 1.0)});

    while (!future.isReady()) {
      pTileset->updateView({});
    }

    SampleHeightResult results = future.waitInMainThread();

    REQUIRE(results.warnings.size() == 0);
    REQUIRE(results.positions.size() == 1);
    REQUIRE(results.sampleSuccess.size() == 1);
    CHECK(results.sampleSuccess[0]);
    CHECK(Math::equalsEpsilon(
        results.positions[0].longitude,
        Math::degreesToRadians(-75.612559),
        0.0,
        Math::Epsilon4));
    CHECK(Math::equalsEpsilon(
        results.positions[0].latitude,
        Math::degreesToRadians(40.042183),
        0.0,
        Math::Epsilon4));
    CHECK(Math::equalsEpsilon(
        results.positions[0].height,
        0.0,
        0.0,
        Math::Epsilon4));
  }

  SUBCASE("stacked-cubes") {
    // This tileset has two cubes on top of each other, each in a different
    // tile, so we can test that the height of the top one is returned.
    // The bottom cube has a height of 78.0 meters, the upper cube has a height
    // of 83.0 meters.
    std::string url =
        "file://" +
        Uri::nativePathToUriPath(StringHelpers::toStringUtf8(
            (testDataPath / "stacked-cubes" / "tileset.json").u8string()));

    Tileset tileset(externals, url);

    Future<SampleHeightResult> future = tileset.sampleHeightMostDetailed(
        {Cartographic::fromDegrees(10.0, 45.0, 0.0)});

    while (!future.isReady()) {
      tileset.updateView({});
    }

    SampleHeightResult results = future.waitInMainThread();
    CHECK(results.warnings.empty());
    REQUIRE(results.positions.size() == 1);

    CHECK(results.sampleSuccess[0]);
    CHECK(Math::equalsEpsilon(
        results.positions[0].height,
        83.0,
        0.0,
        Math::Epsilon1));
  }
}
