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

// The coordinates and expected heights in this file were determined in Cesium
// for Unreal Engine by adding the tileset, putting a cube above the location
// of interest, adding a CesiumGlobeAnchor to it, and pressing the "End" key
// to drop it onto terrain. The coordinates were then copied out of the globe
// anchor, subtracting 0.5 from the height to account for "End" placing the
// bottom of the cube on the surface instead of its center.

TEST_CASE("Tileset most detailed height queries") {
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
      externals.asyncSystem.dispatchMainThreadTasks();
      tileset.loadTiles();
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
      externals.asyncSystem.dispatchMainThreadTasks();
      tileset.loadTiles();
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
      externals.asyncSystem.dispatchMainThreadTasks();
      tileset.loadTiles();
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
      externals.asyncSystem.dispatchMainThreadTasks();
      tileset.loadTiles();
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
      externals.asyncSystem.dispatchMainThreadTasks();
      tileset.loadTiles();
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
      externals.asyncSystem.dispatchMainThreadTasks();
      tileset.loadTiles();
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
      externals.asyncSystem.dispatchMainThreadTasks();
      pTileset->loadTiles();
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
      externals.asyncSystem.dispatchMainThreadTasks();
      tileset.loadTiles();
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

  SUBCASE("stacked-cubes on custom ellipsoid") {
    // This tileset has two cubes on top of each other, each in a different
    // tile, so we can test that the height of the top one is returned.
    // Relative to the WGS84 ellipsoid, the bottom cube has a height of 78.0
    // meters, and the upper cube has a height of 83.0 meters.
    std::string url =
        "file://" +
        Uri::nativePathToUriPath(StringHelpers::toStringUtf8(
            (testDataPath / "stacked-cubes" / "tileset.json").u8string()));

    CesiumGeospatial::Ellipsoid ellipsoid(
        CesiumGeospatial::Ellipsoid::WGS84.getRadii() - glm::dvec3(15.0));

    TilesetOptions options;
    options.ellipsoid = ellipsoid;

    Tileset tileset(externals, url, options);

    Cartographic samplePosition = Cartographic::fromDegrees(10.0, 45.0, 0.0);
    Future<SampleHeightResult> future =
        tileset.sampleHeightMostDetailed({samplePosition});

    while (!future.isReady()) {
      externals.asyncSystem.dispatchMainThreadTasks();
      tileset.loadTiles();
    }

    SampleHeightResult results = future.waitInMainThread();
    CHECK(results.warnings.empty());
    REQUIRE(results.positions.size() == 1);

    glm::dvec3 rayDirection = -ellipsoid.geodeticSurfaceNormal(samplePosition);
    glm::dvec3 difference = glm::dvec3(15.0) * rayDirection;

    CHECK(results.sampleSuccess[0]);
    CHECK(Math::equalsEpsilon(
        results.positions[0].height,
        83.0 + glm::length(difference),
        0.0,
        Math::Epsilon1));
  }
}

namespace {

/**
 * @brief Creates a ViewState looking down at a given cartographic position.
 */
ViewState createViewState(
    const Cartographic& target,
    const CesiumGeospatial::Ellipsoid& ellipsoid =
        CesiumGeospatial::Ellipsoid::WGS84) {
  // Position the camera 200m above the target, looking straight down.
  Cartographic cameraPosition(target.longitude, target.latitude, 200.0);
  glm::dvec3 position = ellipsoid.cartographicToCartesian(cameraPosition);
  glm::dvec3 target3D = ellipsoid.cartographicToCartesian(
      Cartographic(target.longitude, target.latitude, 0.0));
  glm::dvec3 direction = glm::normalize(target3D - position);
  glm::dvec3 up{0.0, 0.0, 1.0};
  glm::dvec2 viewportSize{500.0, 500.0};
  double aspectRatio = viewportSize.x / viewportSize.y;
  double horizontalFieldOfView = Math::degreesToRadians(60.0);
  double verticalFieldOfView =
      std::atan(std::tan(horizontalFieldOfView * 0.5) / aspectRatio) * 2.0;
  return ViewState(
      position,
      direction,
      up,
      viewportSize,
      horizontalFieldOfView,
      verticalFieldOfView,
      ellipsoid);
}

/**
 * @brief Loads a tileset to completion for a given view using
 * updateViewGroupOffline.
 */
void loadTilesetForView(Tileset& tileset, const ViewState& viewState) {
  tileset.updateViewGroupOffline(tileset.getDefaultViewGroup(), {viewState});
}

} // namespace

TEST_CASE("Tileset current detail height queries") {
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

    std::vector<Cartographic> positions = {
        // A point on geometry in "parent.b3dm", which should only be included
        // because this tileset is additive-refined.
        Cartographic::fromDegrees(-75.612088, 40.042526, 0.0),

        // A point on geometry in a leaf tile.
        Cartographic::fromDegrees(-75.612025, 40.041684, 0.0)};

    // Fully load the tileset at this location
    ViewState viewState = createViewState(positions[0]);
    loadTilesetForView(tileset, viewState);

    SampleHeightResult results = tileset.sampleHeightCurrentDetail(positions);
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

    std::vector<Cartographic> positions = {
        // A point on geometry in "parent.b3dm", which should not be
        // included because this tileset is replace-refined.
        Cartographic::fromDegrees(-75.612088, 40.042526, 0.0),

        // A point on geometry in a leaf tile.
        Cartographic::fromDegrees(-75.612025, 40.041684, 0.0)};

    // Fully load the tileset at this location
    ViewState viewState = createViewState(positions[0]);
    loadTilesetForView(tileset, viewState);

    SampleHeightResult results = tileset.sampleHeightCurrentDetail(positions);
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

  SUBCASE("Partially-loaded additive-refined tileset") {
    // Use a very high maximumScreenSpaceError so that only the root tile is
    // loaded (children are never requested because the root already meets SSE).
    std::string url =
        "file://" +
        Uri::nativePathToUriPath(StringHelpers::toStringUtf8(
            (testDataPath / "Tileset" / "tileset.json").u8string()));

    TilesetOptions options;
    options.maximumScreenSpaceError = 999999.0;

    Tileset tileset(externals, url, options);

    std::vector<Cartographic> positions = {
        // A point on geometry in "parent.b3dm" (the root tile content).
        Cartographic::fromDegrees(-75.612088, 40.042526, 0.0),

        // A point on geometry in a leaf tile, which should NOT be loaded.
        Cartographic::fromDegrees(-75.612025, 40.041684, 0.0)};

    ViewState viewState = createViewState(positions[0]);
    loadTilesetForView(tileset, viewState);

    SampleHeightResult results = tileset.sampleHeightCurrentDetail(positions);
    REQUIRE(results.positions.size() == 2);

    // The root tile's geometry should be found.
    CHECK(results.sampleSuccess[0]);
    CHECK(Math::equalsEpsilon(
        results.positions[0].height,
        78.155809,
        0.0,
        Math::Epsilon4));

    // The leaf tile geometry should NOT be found because only the root is
    // loaded.
    CHECK(!results.sampleSuccess[1]);
  }

  SUBCASE("Partially-loaded replace-refined tileset") {
    // Use a very high maximumScreenSpaceError so that only the root tile is
    // loaded (children are never requested because the root already meets SSE).
    std::string url =
        "file://" +
        Uri::nativePathToUriPath(StringHelpers::toStringUtf8(
            (testDataPath / "ReplaceTileset" / "tileset.json").u8string()));

    TilesetOptions options;
    options.maximumScreenSpaceError = 999999.0;

    Tileset tileset(externals, url, options);

    std::vector<Cartographic> positions = {
        // A point on geometry in "parent.b3dm" (the root tile content).
        // With replace refinement and only the root loaded, this geometry
        // should be found (since no children have replaced it).
        Cartographic::fromDegrees(-75.612088, 40.042526, 0.0),

        // A point on geometry in a leaf tile, which should NOT be loaded.
        Cartographic::fromDegrees(-75.612025, 40.041684, 0.0)};

    ViewState viewState = createViewState(positions[0]);
    loadTilesetForView(tileset, viewState);

    SampleHeightResult results = tileset.sampleHeightCurrentDetail(positions);
    REQUIRE(results.positions.size() == 2);

    // The root tile's geometry should be found because children haven't
    // replaced it yet.
    CHECK(results.sampleSuccess[0]);
    CHECK(Math::equalsEpsilon(
        results.positions[0].height,
        78.155809,
        0.0,
        Math::Epsilon4));

    // The leaf tile geometry should NOT be found because only the root is
    // loaded.
    CHECK(!results.sampleSuccess[1]);
  }

  SUBCASE("Convenience overload") {
    std::string url =
        "file://" +
        Uri::nativePathToUriPath(StringHelpers::toStringUtf8(
            (testDataPath / "ReplaceTileset" / "tileset.json").u8string()));

    Tileset tileset(externals, url);

    Cartographic position =
        Cartographic::fromDegrees(-75.612025, 40.041684, 0.0);

    // Fully load the tileset at this location
    ViewState viewState = createViewState({position});
    loadTilesetForView(tileset, viewState);

    std::optional<double> height = tileset.sampleHeightCurrentDetail(position);

    CHECK(height.has_value());
    CHECK(Math::equalsEpsilon(*height, 7.837332, 0.0, Math::Epsilon4));
  }
}