#include "TestTilesetJsonLoader.h"

#include "ImplicitQuadtreeLoader.h"
#include "SimplePrepareRendererResource.h"
#include "TilesetContentLoaderResult.h"
#include "TilesetJsonLoader.h"

#include <Cesium3DTiles/Schema.h>
#include <Cesium3DTilesContent/registerAllTileContentTypes.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TileRefine.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetMetadata.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGltf/Model.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>
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
using namespace Cesium3DTilesSelection;
using namespace CesiumNativeTests;
using namespace CesiumUtility;

namespace {
std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;

TileLoadResult loadTileContent(
    const std::filesystem::path& tilePath,
    TilesetContentLoader& loader,
    Tile& tile) {
  std::unique_ptr<SimpleAssetResponse> pMockCompletedResponse;
  if (std::filesystem::exists(tilePath)) {
    pMockCompletedResponse = std::make_unique<SimpleAssetResponse>(
        static_cast<uint16_t>(200),
        "doesn't matter",
        CesiumAsync::HttpHeaders{},
        readFile(tilePath));
  } else {
    pMockCompletedResponse = std::make_unique<SimpleAssetResponse>(
        static_cast<uint16_t>(404),
        "doesn't matter",
        CesiumAsync::HttpHeaders{},
        std::vector<std::byte>{});
  }
  auto pMockCompletedRequest = std::make_shared<SimpleAssetRequest>(
      "GET",
      tilePath.filename().string(),
      CesiumAsync::HttpHeaders{},
      std::move(pMockCompletedResponse));

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;
  mockCompletedRequests.insert(
      {tilePath.filename().string(), std::move(pMockCompletedRequest)});

  std::shared_ptr<SimpleAssetAccessor> pMockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));

  AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};

  TileLoadInput loadInput{
      tile,
      {},
      asyncSystem,
      pMockAssetAccessor,
      spdlog::default_logger(),
      {}};

  auto tileLoadResultFuture = loader.loadTileContent(loadInput);

  asyncSystem.dispatchMainThreadTasks();

  return tileLoadResultFuture.wait();
}
} // namespace

TEST_CASE("Test creating tileset json loader") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  SUBCASE("Create valid tileset json with REPLACE refinement") {
    auto loaderResult = createTilesetJsonLoader(
        testDataPath / "ReplaceTileset" / "tileset.json");

    CHECK(!loaderResult.errors.hasErrors());

    // check root tile
    auto pTilesetJson = loaderResult.pRootTile.get();
    REQUIRE(pTilesetJson);
    REQUIRE(pTilesetJson->getChildren().size() == 1);
    CHECK(pTilesetJson->getParent() == nullptr);
    auto pRootTile = &pTilesetJson->getChildren()[0];
    CHECK(pRootTile->getParent() == pTilesetJson);
    REQUIRE(pRootTile->getChildren().size() == 4);
    CHECK(pRootTile->getGeometricError() == 70.0);
    CHECK(pRootTile->getRefine() == TileRefine::Replace);
    CHECK(std::get<std::string>(pRootTile->getTileID()) == "parent.b3dm");

    const auto& boundingVolume = pRootTile->getBoundingVolume();
    const auto* pRegion =
        std::get_if<CesiumGeospatial::BoundingRegion>(&boundingVolume);
    CHECK(pRegion != nullptr);
    CHECK(pRegion->getMinimumHeight() == Approx(0.0));
    CHECK(pRegion->getMaximumHeight() == Approx(88.0));
    CHECK(pRegion->getRectangle().getWest() == Approx(-1.3197209591796106));
    CHECK(pRegion->getRectangle().getEast() == Approx(-1.3196390408203893));
    CHECK(pRegion->getRectangle().getSouth() == Approx(0.6988424218));
    CHECK(pRegion->getRectangle().getNorth() == Approx(0.6989055782));

    // check root children
    auto children = pRootTile->getChildren();
    CHECK(children[0].getParent() == pRootTile);
    CHECK(children[0].getChildren().size() == 1);
    CHECK(children[0].getGeometricError() == 5.0);
    CHECK(children[0].getRefine() == TileRefine::Replace);
    CHECK(std::get<std::string>(children[0].getTileID()) == "ll.b3dm");
    CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
        children[0].getBoundingVolume()));

    CHECK(children[1].getParent() == pRootTile);
    CHECK(children[1].getChildren().size() == 0);
    CHECK(children[1].getGeometricError() == 0.0);
    CHECK(children[1].getRefine() == TileRefine::Replace);
    CHECK(std::get<std::string>(children[1].getTileID()) == "lr.b3dm");
    CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
        children[1].getBoundingVolume()));

    CHECK(children[2].getParent() == pRootTile);
    CHECK(children[2].getChildren().size() == 0);
    CHECK(children[2].getGeometricError() == 0.0);
    CHECK(children[2].getRefine() == TileRefine::Replace);
    CHECK(std::get<std::string>(children[2].getTileID()) == "ur.b3dm");
    CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
        children[2].getBoundingVolume()));

    CHECK(children[3].getParent() == pRootTile);
    CHECK(children[3].getChildren().size() == 0);
    CHECK(children[3].getGeometricError() == 0.0);
    CHECK(children[3].getRefine() == TileRefine::Replace);
    CHECK(std::get<std::string>(children[3].getTileID()) == "ul.b3dm");
    CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
        children[3].getBoundingVolume()));

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SUBCASE("Create valid tileset json with ADD refinement") {
    auto loaderResult =
        createTilesetJsonLoader(testDataPath / "AddTileset" / "tileset2.json");

    CHECK(!loaderResult.errors.hasErrors());

    // check root tile
    auto pTilesetJson = loaderResult.pRootTile.get();
    REQUIRE(pTilesetJson != nullptr);
    CHECK(pTilesetJson->getParent() == nullptr);
    REQUIRE(pTilesetJson->getChildren().size() == 1);
    auto pRootTile = &pTilesetJson->getChildren()[0];
    CHECK(pRootTile->getParent() == pTilesetJson);
    CHECK(pRootTile->getChildren().size() == 4);
    CHECK(pRootTile->getGeometricError() == 70.0);
    CHECK(pRootTile->getRefine() == TileRefine::Add);
    CHECK(std::get<std::string>(pRootTile->getTileID()) == "parent.b3dm");

    const auto& boundingVolume = pRootTile->getBoundingVolume();
    const auto* pRegion =
        std::get_if<CesiumGeospatial::BoundingRegion>(&boundingVolume);
    CHECK(pRegion != nullptr);
    CHECK(pRegion->getMinimumHeight() == Approx(0.0));
    CHECK(pRegion->getMaximumHeight() == Approx(88.0));
    CHECK(pRegion->getRectangle().getWest() == Approx(-1.3197209591796106));
    CHECK(pRegion->getRectangle().getEast() == Approx(-1.3196390408203893));
    CHECK(pRegion->getRectangle().getSouth() == Approx(0.6988424218));
    CHECK(pRegion->getRectangle().getNorth() == Approx(0.6989055782));

    // check children
    std::array<std::string, 4> expectedUrl{
        {"tileset3/tileset3.json", "lr.b3dm", "ur.b3dm", "ul.b3dm"}};
    auto expectedUrlIt = expectedUrl.begin();
    for (const Tile& child : pRootTile->getChildren()) {
      CHECK(child.getParent() == pRootTile);
      CHECK(child.getChildren().size() == 0);
      CHECK(child.getGeometricError() == 0.0);
      CHECK(child.getRefine() == TileRefine::Add);
      CHECK(std::get<std::string>(child.getTileID()) == *expectedUrlIt);
      CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
          child.getBoundingVolume()));
      ++expectedUrlIt;
    }

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SUBCASE("Tileset has tile with sphere bounding volume") {
    auto loaderResult = createTilesetJsonLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "SphereBoundingVolumeTileset.json");

    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);

    const Tile& rootTile = loaderResult.pRootTile->getChildren()[0];
    const CesiumGeometry::BoundingSphere& sphere =
        std::get<CesiumGeometry::BoundingSphere>(rootTile.getBoundingVolume());
    CHECK(sphere.getCenter() == glm::dvec3(0.0, 0.0, 10.0));
    CHECK(sphere.getRadius() == 141.4214);

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SUBCASE("Tileset has tile with box bounding volume") {
    auto loaderResult = createTilesetJsonLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "BoxBoundingVolumeTileset.json");

    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);

    const Tile& rootTile = loaderResult.pRootTile->getChildren()[0];
    const CesiumGeometry::OrientedBoundingBox& box =
        std::get<CesiumGeometry::OrientedBoundingBox>(
            rootTile.getBoundingVolume());
    const glm::dmat3& halfAxis = box.getHalfAxes();
    CHECK(halfAxis[0] == glm::dvec3(100.0, 0.0, 0.0));
    CHECK(halfAxis[1] == glm::dvec3(0.0, 100.0, 0.0));
    CHECK(halfAxis[2] == glm::dvec3(0.0, 0.0, 10.0));
    CHECK(box.getCenter() == glm::dvec3(0.0, 0.0, 10.0));
  }

  SUBCASE("Tileset has tile with no bounding volume field") {
    auto loaderResult = createTilesetJsonLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "NoBoundingVolumeTileset.json");

    CHECK(!loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);
    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK(pRootTile->getChildren().empty());

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SUBCASE("Tileset has tile with no geometric error field") {
    auto loaderResult = createTilesetJsonLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "NoGeometricErrorTileset.json");

    CHECK(!loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);
    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK(pRootTile->getGeometricError() == Approx(70.0));
    CHECK(pRootTile->getChildren().size() == 4);
    for (const Tile& child : pRootTile->getChildren()) {
      CHECK(child.getGeometricError() == Approx(35.0));
    }

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SUBCASE("Tileset has tile with no capitalized Refinement field") {
    auto loaderResult = createTilesetJsonLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "NoCapitalizedRefineTileset.json");

    CHECK(!loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);
    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK(pRootTile->getGeometricError() == Approx(70.0));
    CHECK(pRootTile->getRefine() == TileRefine::Add);
    CHECK(pRootTile->getChildren().size() == 4);
    for (const Tile& child : pRootTile->getChildren()) {
      CHECK(child.getGeometricError() == Approx(5.0));
      CHECK(child.getRefine() == TileRefine::Replace);
    }

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SUBCASE("Scale geometric error along with tile transform") {
    auto loaderResult = createTilesetJsonLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "ScaleGeometricErrorTileset.json");

    CHECK(!loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);
    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK(pRootTile->getGeometricError() == Approx(210.0));
    CHECK(pRootTile->getChildren().size() == 4);
    for (const Tile& child : pRootTile->getChildren()) {
      CHECK(child.getGeometricError() == Approx(15.0));
    }

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SUBCASE("Tileset with empty tile") {
    auto loaderResult = createTilesetJsonLoader(
        testDataPath / "MultipleKindsOfTilesets" / "EmptyTileTileset.json");
    CHECK(!loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);
    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK(pRootTile->getGeometricError() == Approx(70.0));
    CHECK(pRootTile->getChildren().size() == 1);

    const Tile& child = pRootTile->getChildren().front();
    CHECK(child.isEmptyContent());

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SUBCASE("Tileset with quadtree implicit tile") {
    auto loaderResult = createTilesetJsonLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "QuadtreeImplicitTileset.json");
    CHECK(!loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->isExternalContent());
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);
    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK(pRootTile->isExternalContent());
    REQUIRE(pRootTile->getChildren().size() == 1);
    CHECK(pRootTile->getTransform() == glm::dmat4(glm::dmat3(2.0)));

    const Tile& child = pRootTile->getChildren().front();
    CHECK(child.getLoader() != loaderResult.pLoader.get());
    CHECK(child.getGeometricError() == pRootTile->getGeometricError());
    CHECK(child.getRefine() == pRootTile->getRefine());
    CHECK(child.getTransform() == pRootTile->getTransform());
    CHECK(
        std::get<CesiumGeometry::QuadtreeTileID>(child.getTileID()) ==
        CesiumGeometry::QuadtreeTileID(0, 0, 0));
  }

  SUBCASE("Tileset with octree implicit tile") {
    auto loaderResult = createTilesetJsonLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "OctreeImplicitTileset.json");
    CHECK(!loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->isExternalContent());
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);
    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK(pRootTile->isExternalContent());
    REQUIRE(pRootTile->getChildren().size() == 1);
    CHECK(pRootTile->getTransform() == glm::dmat4(glm::dmat3(2.0)));

    const Tile& child = pRootTile->getChildren().front();
    CHECK(child.getLoader() != loaderResult.pLoader.get());
    CHECK(child.getGeometricError() == pRootTile->getGeometricError());
    CHECK(child.getRefine() == pRootTile->getRefine());
    CHECK(child.getTransform() == pRootTile->getTransform());
    CHECK(
        std::get<CesiumGeometry::OctreeTileID>(child.getTileID()) ==
        CesiumGeometry::OctreeTileID(0, 0, 0, 0));
  }

  SUBCASE("Tileset with metadata") {
    auto loaderResult =
        createTilesetJsonLoader(testDataPath / "WithMetadata" / "tileset.json");

    CHECK(!loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pLoader);
    REQUIRE(loaderResult.pRootTile);

    TileExternalContent* pExternal =
        loaderResult.pRootTile->getContent().getExternalContent();
    REQUIRE(pExternal);

    const TilesetMetadata& metadata = pExternal->metadata;
    const std::optional<Cesium3DTiles::Schema>& schema = metadata.schema;
    REQUIRE(schema);
    CHECK(schema->id == "foo");
  }
}

TEST_CASE("Test loading individual tile of tileset json") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  SUBCASE("Load tile that has render content") {
    auto loaderResult = createTilesetJsonLoader(
        testDataPath / "ReplaceTileset" / "tileset.json");
    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);

    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];

    const auto& tileID = std::get<std::string>(pRootTile->getTileID());
    CHECK(tileID == "parent.b3dm");

    // check tile content
    auto tileLoadResult = loadTileContent(
        testDataPath / "ReplaceTileset" / tileID,
        *loaderResult.pLoader,
        *pRootTile);
    CHECK(
        std::holds_alternative<CesiumGltf::Model>(tileLoadResult.contentKind));
    CHECK(tileLoadResult.updatedBoundingVolume == std::nullopt);
    CHECK(tileLoadResult.updatedContentBoundingVolume == std::nullopt);
    CHECK(tileLoadResult.state == TileLoadResultState::Success);
    CHECK(!tileLoadResult.tileInitializer);
  }

  SUBCASE("Load tile that has external content") {
    auto loaderResult =
        createTilesetJsonLoader(testDataPath / "AddTileset" / "tileset.json");

    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);

    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];

    const auto& tileID = std::get<std::string>(pRootTile->getTileID());
    CHECK(tileID == "tileset2.json");

    // check tile content
    auto tileLoadResult = loadTileContent(
        testDataPath / "AddTileset" / tileID,
        *loaderResult.pLoader,
        *pRootTile);
    CHECK(tileLoadResult.updatedBoundingVolume == std::nullopt);
    CHECK(tileLoadResult.updatedContentBoundingVolume == std::nullopt);
    CHECK(std::holds_alternative<TileExternalContent>(
        tileLoadResult.contentKind));
    CHECK(tileLoadResult.state == TileLoadResultState::Success);
    CHECK(tileLoadResult.tileInitializer);

    // check tile is really an external tile
    pRootTile->getContent().setContentKind(
        std::make_unique<TileExternalContent>(
            std::get<TileExternalContent>(tileLoadResult.contentKind)));
    tileLoadResult.tileInitializer(*pRootTile);
    const auto& children = pRootTile->getChildren();
    REQUIRE(children.size() == 1);

    const Tile& parentB3dmTile = children[0];
    CHECK(std::get<std::string>(parentB3dmTile.getTileID()) == "parent.b3dm");
    CHECK(parentB3dmTile.getGeometricError() == Approx(70.0));

    std::vector<std::string> expectedChildUrls{
        "tileset3/tileset3.json",
        "lr.b3dm",
        "ur.b3dm",
        "ul.b3dm"};
    const auto& parentB3dmChildren = parentB3dmTile.getChildren();
    for (std::size_t i = 0; i < parentB3dmChildren.size(); ++i) {
      const Tile& child = parentB3dmChildren[i];
      CHECK(std::get<std::string>(child.getTileID()) == expectedChildUrls[i]);
      CHECK(child.getGeometricError() == Approx(0.0));
      CHECK(child.getRefine() == TileRefine::Add);
      CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
          child.getBoundingVolume()));
    }
  }

  SUBCASE("Load tile that has external content with implicit tiling") {
    auto loaderResult = createTilesetJsonLoader(
        testDataPath / "ImplicitTileset" / "tileset_1.1.json");

    REQUIRE(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->isExternalContent());
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);

    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    REQUIRE(pRootTile->getChildren().size() == 1);

    auto& implicitTile = pRootTile->getChildren().front();
    const auto& tileID =
        std::get<CesiumGeometry::QuadtreeTileID>(implicitTile.getTileID());
    CHECK(tileID == CesiumGeometry::QuadtreeTileID(0, 0, 0));

    // mock subtree content response
    auto subtreePath =
        testDataPath / "ImplicitTileset" / "subtrees" / "0.0.0.json";

    auto pMockSubtreeResponse = std::make_unique<SimpleAssetResponse>(
        static_cast<uint16_t>(200),
        "doesn't matter",
        CesiumAsync::HttpHeaders{},
        readFile(subtreePath));
    auto pMockSubtreeRequest = std::make_shared<SimpleAssetRequest>(
        "GET",
        "doesn't matter",
        CesiumAsync::HttpHeaders{},
        std::move(pMockSubtreeResponse));

    // mock tile content response
    auto tilePath =
        testDataPath / "ImplicitTileset" / "content" / "0" / "0" / "0.b3dm";

    auto pMockTileResponse = std::make_unique<SimpleAssetResponse>(
        static_cast<uint16_t>(200),
        "doesn't matter",
        CesiumAsync::HttpHeaders{},
        readFile(tilePath));
    auto pMockTileRequest = std::make_shared<SimpleAssetRequest>(
        "GET",
        "doesn't matter",
        CesiumAsync::HttpHeaders{},
        std::move(pMockTileResponse));

    // create mock asset accessor
    std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
        mockCompletedRequests{
            {"subtrees/0.0.0.json", std::move(pMockSubtreeRequest)},
            {"content/0/0/0.b3dm", std::move(pMockTileRequest)}};

    std::shared_ptr<SimpleAssetAccessor> pMockAssetAccessor =
        std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));

    AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};

    {
      // loader will tell to retry later since it needs subtree
      TileLoadInput loadInput{
          implicitTile,
          {},
          asyncSystem,
          pMockAssetAccessor,
          spdlog::default_logger(),
          {}};
      auto implicitContentResultFuture =
          loaderResult.pLoader->loadTileContent(loadInput);

      asyncSystem.dispatchMainThreadTasks();

      auto implicitContentResult = implicitContentResultFuture.wait();
      CHECK(implicitContentResult.state == TileLoadResultState::RetryLater);
    }

    {
      // loader will be able to load the tile the second time around
      TileLoadInput loadInput{
          implicitTile,
          {},
          asyncSystem,
          pMockAssetAccessor,
          spdlog::default_logger(),
          {}};
      auto implicitContentResultFuture =
          loaderResult.pLoader->loadTileContent(loadInput);

      asyncSystem.dispatchMainThreadTasks();

      auto implicitContentResult = implicitContentResultFuture.wait();
      CHECK(std::holds_alternative<CesiumGltf::Model>(
          implicitContentResult.contentKind));
      CHECK(!implicitContentResult.updatedBoundingVolume);
      CHECK(!implicitContentResult.updatedContentBoundingVolume);
      CHECK(implicitContentResult.state == TileLoadResultState::Success);
      CHECK(!implicitContentResult.tileInitializer);
    }
  }

  SUBCASE("Check that tile with legacy implicit tiling extension still works") {
    auto loaderResult = createTilesetJsonLoader(
        testDataPath / "ImplicitTileset" / "tileset_1.0.json");

    REQUIRE(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->isExternalContent());
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);

    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    REQUIRE(pRootTile->getChildren().size() == 1);

    auto& implicitTile = pRootTile->getChildren().front();
    const auto& tileID =
        std::get<CesiumGeometry::QuadtreeTileID>(implicitTile.getTileID());
    CHECK(tileID == CesiumGeometry::QuadtreeTileID(0, 0, 0));

    const auto pLoader =
        dynamic_cast<const ImplicitQuadtreeLoader*>(implicitTile.getLoader());
    CHECK(pLoader);
    CHECK(pLoader->getSubtreeLevels() == 2);
    CHECK(pLoader->getAvailableLevels() == 2);
  }

  SUBCASE("Tile with missing content") {
    auto pLog = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(3);
    spdlog::default_logger()->sinks().emplace_back(pLog);

    auto loaderResult = createTilesetJsonLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "ErrorMissingContentTileset.json");
    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);

    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];

    const auto& tileID = std::get<std::string>(pRootTile->getTileID());
    CHECK(tileID == "nonexistent.b3dm");

    // check tile content
    auto tileLoadResult = loadTileContent(
        testDataPath / "MultipleKindsOfTilesets" / tileID,
        *loaderResult.pLoader,
        *pRootTile);
    CHECK(tileLoadResult.state == TileLoadResultState::Failed);

    std::vector<std::string> logMessages = pLog->last_formatted();
    REQUIRE(logMessages.size() == 1);
    REQUIRE(
        logMessages.back()
            .substr(0, logMessages.back().find_last_not_of("\n\r") + 1)
            .ends_with(
                "Received status code 404 for tile content nonexistent.b3dm"));
  }
}
Cesium3DTilesSelection::TilesetContentLoaderResult<TilesetJsonLoader>
Cesium3DTilesSelection::createTilesetJsonLoader(
    const std::filesystem::path& tilesetPath) {
  std::string tilesetPathStr = tilesetPath.string();
  auto pAccessor = std::make_shared<SimpleAssetAccessor>(
      std::map<std::string, std::shared_ptr<SimpleAssetRequest>>());
  auto externals = createMockJsonTilesetExternals(tilesetPathStr, pAccessor);
  auto loaderResultFuture =
      TilesetJsonLoader::createLoader(externals, tilesetPathStr, {});
  externals.asyncSystem.dispatchMainThreadTasks();

  return loaderResultFuture.wait();
}
Cesium3DTilesSelection::TilesetExternals
Cesium3DTilesSelection::createMockJsonTilesetExternals(
    const std::string& tilesetPath,
    std::shared_ptr<CesiumNativeTests::SimpleAssetAccessor>& pAssetAccessor) {
  auto tilesetContent = readFile(tilesetPath);
  auto pMockCompletedResponse =
      std::make_unique<CesiumNativeTests::SimpleAssetResponse>(
          static_cast<uint16_t>(200),
          "doesn't matter",
          CesiumAsync::HttpHeaders{},
          std::move(tilesetContent));

  auto pMockCompletedRequest =
      std::make_shared<CesiumNativeTests::SimpleAssetRequest>(
          "GET",
          "tileset.json",
          CesiumAsync::HttpHeaders{},
          std::move(pMockCompletedResponse));

  pAssetAccessor->mockCompletedRequests.insert(
      {tilesetPath, std::move(pMockCompletedRequest)});

  auto pMockPrepareRendererResource =
      std::make_shared<SimplePrepareRendererResource>();

  auto pMockCreditSystem = std::make_shared<CesiumUtility::CreditSystem>();

  CesiumAsync::AsyncSystem asyncSystem{
      std::make_shared<CesiumNativeTests::SimpleTaskProcessor>()};

  return TilesetExternals{
      std::move(pAssetAccessor),
      std::move(pMockPrepareRendererResource),
      std::move(asyncSystem),
      std::move(pMockCreditSystem)};
}
