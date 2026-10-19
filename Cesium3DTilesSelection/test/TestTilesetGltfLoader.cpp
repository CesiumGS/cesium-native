#include "ImplicitQuadtreeLoader.h"
#include "SimplePrepareRendererResource.h"
#include "TilesetJsonLoader.h"
#include "TilesetLoaderTestUtility.h"

#include <Cesium3DTiles/TileRefine.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetMetadata.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/PropertyTablePropertyView.h>
#include <CesiumGltf/PropertyTableView.h>
#include <CesiumGltfConverters/registerAllConverterTypes.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumTilesetJson/ExtensionContent3dTilesContentVoxels.h>
#include <CesiumTilesetJson/Schema.h>
#include <CesiumUtility/CreditSystem.h>

#include <doctest/doctest.h>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/spdlog.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace doctest;
using namespace CesiumAsync;
using namespace Cesium3DTiles;
using namespace Cesium3DTilesSelection;
using namespace CesiumGltf;
using namespace CesiumNativeTests;
using namespace CesiumUtility;

namespace {
std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;
} // namespace

TEST_CASE("Test creating tileset gltf loader") {
  CesiumGltfConverters::registerAllConverterTypes();

  SUBCASE("Create valid tileset json with ADD refinement") {
    auto loaderResult = createTilesetGltfLoader(
        testDataPath / "2.0" / "AdditiveTileset" / "sample.tileset.gltf");

    CHECK(!loaderResult.errors.hasErrors());

    // check root tile
    auto pTilesetJson = loaderResult.pRootTile.get();
    REQUIRE(pTilesetJson);
    CHECK_EQ(pTilesetJson->getParent(), nullptr);
    REQUIRE_EQ(pTilesetJson->getChildren().size(), 1);
    auto pRootTile = &pTilesetJson->getChildren()[0];
    CHECK_EQ(pRootTile->getParent(), pTilesetJson);
    CHECK_EQ(pRootTile->getChildren().size(), 4);
    CHECK_EQ(pRootTile->getGeometricError(), 70.0);
    CHECK_EQ(pRootTile->getRefine(), TileRefine::Add);
    CHECK_EQ(std::get<std::string>(pRootTile->getTileID()), "parent.gltf");

    const auto& boundingVolume = pRootTile->getBoundingVolume();
    const auto* pRegion =
        std::get_if<CesiumGeospatial::BoundingRegion>(&boundingVolume);
    REQUIRE(pRegion);
    CHECK_EQ(pRegion->getMinimumHeight(), Approx(0.0));
    CHECK_EQ(pRegion->getMaximumHeight(), Approx(88.0));
    CHECK_EQ(pRegion->getRectangle().getWest(), Approx(-1.3197209591796106));
    CHECK_EQ(pRegion->getRectangle().getEast(), Approx(-1.3196390408203893));
    CHECK_EQ(pRegion->getRectangle().getSouth(), Approx(0.6988424218));
    CHECK_EQ(pRegion->getRectangle().getNorth(), Approx(0.6989055782));

    // check children
    std::array<std::string, 4> expectedUrl{
        {"ll.gltf", "lr.gltf", "ul.gltf", "ur.gltf"}};
    auto expectedUrlIt = expectedUrl.begin();
    for (const Tile& child : pRootTile->getChildren()) {
      CHECK_EQ(child.getParent(), pRootTile);
      CHECK_EQ(child.getChildren().size(), 0);
      CHECK_EQ(child.getGeometricError(), 0.0);
      CHECK_EQ(child.getRefine(), TileRefine::Add);
      CHECK_EQ(std::get<std::string>(child.getTileID()), *expectedUrlIt);
      CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
          child.getBoundingVolume()));
      ++expectedUrlIt;
    }

    // check loader up axis
    CHECK_EQ(loaderResult.pLoader->getUpAxis(), CesiumGeometry::Axis::Y);
  }
}
