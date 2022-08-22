#include "SimpleAssetAccessor.h"
#include "SimpleAssetRequest.h"
#include "SimpleAssetResponse.h"
#include "SimplePrepareRendererResource.h"
#include "SimpleTaskProcessor.h"
#include "TilesetJsonLoader.h"
#include "readFile.h"

#include <Cesium3DTilesSelection/registerAllTileContentTypes.h>

#include <catch2/catch.hpp>

#include <cstddef>
#include <memory>
#include <string>

using namespace CesiumAsync;
using namespace Cesium3DTilesSelection;

namespace {
std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;

TilesetExternals createMockTilesetExternals(const std::string& tilesetPath) {
  auto tilesetContent = readFile(tilesetPath);
  auto pMockCompletedResponse = std::make_unique<SimpleAssetResponse>(
      static_cast<uint16_t>(200),
      "doesn't matter",
      CesiumAsync::HttpHeaders{},
      std::move(tilesetContent));

  auto pMockCompletedRequest = std::make_shared<SimpleAssetRequest>(
      "GET",
      "tileset.json",
      CesiumAsync::HttpHeaders{},
      std::move(pMockCompletedResponse));

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;
  mockCompletedRequests.insert({tilesetPath, std::move(pMockCompletedRequest)});

  std::shared_ptr<SimpleAssetAccessor> pMockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));

  auto pMockPrepareRendererResource =
      std::make_shared<SimplePrepareRendererResource>();

  auto pMockCreditSystem = std::make_shared<CreditSystem>();

  AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};

  return TilesetExternals{
      std::move(pMockAssetAccessor),
      std::move(pMockPrepareRendererResource),
      std::move(asyncSystem),
      std::move(pMockCreditSystem)};
}

TilesetContentLoaderResult<TilesetJsonLoader>
createLoader(const std::filesystem::path& tilesetPath) {
  std::string tilesetPathStr = tilesetPath.string();
  auto externals = createMockTilesetExternals(tilesetPathStr);
  auto loaderResultFuture =
      TilesetJsonLoader::createLoader(externals, tilesetPathStr, {});
  externals.asyncSystem.dispatchMainThreadTasks();

  return loaderResultFuture.wait();
}

TileLoadResult loadTileContent(
    const std::filesystem::path& tilePath,
    TilesetContentLoader& loader,
    Tile& tile) {
  auto pMockCompletedResponse = std::make_unique<SimpleAssetResponse>(
      static_cast<uint16_t>(200),
      "doesn't matter",
      CesiumAsync::HttpHeaders{},
      readFile(tilePath));
  auto pMockCompletedRequest = std::make_shared<SimpleAssetRequest>(
      "GET",
      "doesn't matter",
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
  Cesium3DTilesSelection::registerAllTileContentTypes();

  SECTION("Create valid tileset json with REPLACE refinement") {
    auto loaderResult =
        createLoader(testDataPath / "ReplaceTileset" / "tileset.json");

    CHECK(!loaderResult.errors.hasErrors());

    // check root tile
    auto pRootTile = loaderResult.pRootTile.get();
    CHECK(pRootTile != nullptr);
    CHECK(pRootTile->getParent() == nullptr);
    CHECK(pRootTile->getChildren().size() == 4);
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

  SECTION("Create valid tileset json with ADD refinement") {
    auto loaderResult =
        createLoader(testDataPath / "AddTileset" / "tileset2.json");

    CHECK(!loaderResult.errors.hasErrors());

    // check root tile
    auto pRootTile = loaderResult.pRootTile.get();
    CHECK(pRootTile != nullptr);
    CHECK(pRootTile->getParent() == nullptr);
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

  SECTION("Tileset has tile with sphere bounding volume") {
    auto loaderResult = createLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "SphereBoundingVolumeTileset.json");

    CHECK(loaderResult.pRootTile);

    const Tile& rootTile = *loaderResult.pRootTile;
    const CesiumGeometry::BoundingSphere& sphere =
        std::get<CesiumGeometry::BoundingSphere>(rootTile.getBoundingVolume());
    CHECK(sphere.getCenter() == glm::dvec3(0.0, 0.0, 10.0));
    CHECK(sphere.getRadius() == 141.4214);

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SECTION("Tileset has tile with box bounding volume") {
    auto loaderResult = createLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "BoxBoundingVolumeTileset.json");

    CHECK(loaderResult.pRootTile);

    const Tile& rootTile = *loaderResult.pRootTile;
    const CesiumGeometry::OrientedBoundingBox& box =
        std::get<CesiumGeometry::OrientedBoundingBox>(
            rootTile.getBoundingVolume());
    const glm::dmat3& halfAxis = box.getHalfAxes();
    CHECK(halfAxis[0] == glm::dvec3(100.0, 0.0, 0.0));
    CHECK(halfAxis[1] == glm::dvec3(0.0, 100.0, 0.0));
    CHECK(halfAxis[2] == glm::dvec3(0.0, 0.0, 10.0));
    CHECK(box.getCenter() == glm::dvec3(0.0, 0.0, 10.0));
  }

  SECTION("Tileset has tile with no bounding volume field") {
    auto loaderResult = createLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "NoBoundingVolumeTileset.json");

    CHECK(!loaderResult.errors.hasErrors());
    CHECK(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->getChildren().empty());

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SECTION("Tileset has tile with no geometric error field") {
    auto loaderResult = createLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "NoGeometricErrorTileset.json");

    CHECK(!loaderResult.errors.hasErrors());
    CHECK(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->getGeometricError() == Approx(70.0));
    CHECK(loaderResult.pRootTile->getChildren().size() == 4);
    for (const Tile& child : loaderResult.pRootTile->getChildren()) {
      CHECK(child.getGeometricError() == Approx(35.0));
    }

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SECTION("Tileset has tile with no capitalized Refinement field") {
    auto loaderResult = createLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "NoCapitalizedRefineTileset.json");

    CHECK(!loaderResult.errors.hasErrors());
    CHECK(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->getGeometricError() == Approx(70.0));
    CHECK(loaderResult.pRootTile->getRefine() == TileRefine::Add);
    CHECK(loaderResult.pRootTile->getChildren().size() == 4);
    for (const Tile& child : loaderResult.pRootTile->getChildren()) {
      CHECK(child.getGeometricError() == Approx(5.0));
      CHECK(child.getRefine() == TileRefine::Replace);
    }

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SECTION("Scale geometric error along with tile transform") {
    auto loaderResult = createLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "ScaleGeometricErrorTileset.json");

    CHECK(!loaderResult.errors.hasErrors());
    CHECK(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->getGeometricError() == Approx(210.0));
    CHECK(loaderResult.pRootTile->getChildren().size() == 4);
    for (const Tile& child : loaderResult.pRootTile->getChildren()) {
      CHECK(child.getGeometricError() == Approx(15.0));
    }

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SECTION("Tileset with empty tile") {
    auto loaderResult = createLoader(
        testDataPath / "MultipleKindsOfTilesets" / "EmptyTileTileset.json");
    CHECK(!loaderResult.errors.hasErrors());
    CHECK(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->getGeometricError() == Approx(70.0));
    CHECK(loaderResult.pRootTile->getChildren().size() == 1);

    const Tile& child = loaderResult.pRootTile->getChildren().front();
    CHECK(child.isEmptyContent());

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SECTION("Tileset with quadtree implicit tile") {
    auto loaderResult = createLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "QuadtreeImplicitTileset.json");
    CHECK(!loaderResult.errors.hasErrors());
    CHECK(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->isExternalContent());
    CHECK(loaderResult.pRootTile->getChildren().size() == 1);
    CHECK(
        loaderResult.pRootTile->getTransform() == glm::dmat4(glm::dmat3(2.0)));

    const Tile& child = loaderResult.pRootTile->getChildren().front();
    CHECK(child.getLoader() != loaderResult.pLoader.get());
    CHECK(
        child.getGeometricError() ==
        loaderResult.pRootTile->getGeometricError());
    CHECK(child.getRefine() == loaderResult.pRootTile->getRefine());
    CHECK(child.getTransform() == loaderResult.pRootTile->getTransform());
    CHECK(
        std::get<CesiumGeometry::QuadtreeTileID>(child.getTileID()) ==
        CesiumGeometry::QuadtreeTileID(0, 0, 0));
  }

  SECTION("Tileset with octree implicit tile") {
    auto loaderResult = createLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "OctreeImplicitTileset.json");
    CHECK(!loaderResult.errors.hasErrors());
    CHECK(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->isExternalContent());
    CHECK(loaderResult.pRootTile->getChildren().size() == 1);
    CHECK(
        loaderResult.pRootTile->getTransform() == glm::dmat4(glm::dmat3(2.0)));

    const Tile& child = loaderResult.pRootTile->getChildren().front();
    CHECK(child.getLoader() != loaderResult.pLoader.get());
    CHECK(
        child.getGeometricError() ==
        loaderResult.pRootTile->getGeometricError());
    CHECK(child.getRefine() == loaderResult.pRootTile->getRefine());
    CHECK(child.getTransform() == loaderResult.pRootTile->getTransform());
    CHECK(
        std::get<CesiumGeometry::OctreeTileID>(child.getTileID()) ==
        CesiumGeometry::OctreeTileID(0, 0, 0, 0));
  }
}

TEST_CASE("Test loading individual tile of tileset json") {
  Cesium3DTilesSelection::registerAllTileContentTypes();

  SECTION("Load tile that has render content") {
    auto loaderResult =
        createLoader(testDataPath / "ReplaceTileset" / "tileset.json");
    CHECK(loaderResult.pRootTile);

    const auto& tileID =
        std::get<std::string>(loaderResult.pRootTile->getTileID());
    CHECK(tileID == "parent.b3dm");

    // check tile content
    auto tileLoadResult = loadTileContent(
        testDataPath / "ReplaceTileset" / tileID,
        *loaderResult.pLoader,
        *loaderResult.pRootTile);
    CHECK(
        std::holds_alternative<CesiumGltf::Model>(tileLoadResult.contentKind));
    CHECK(tileLoadResult.updatedBoundingVolume == std::nullopt);
    CHECK(tileLoadResult.updatedContentBoundingVolume == std::nullopt);
    CHECK(tileLoadResult.state == TileLoadResultState::Success);
    CHECK(!tileLoadResult.tileInitializer);
  }

  SECTION("Load tile that has external content") {
    auto loaderResult =
        createLoader(testDataPath / "AddTileset" / "tileset.json");

    CHECK(loaderResult.pRootTile);

    const auto& tileID =
        std::get<std::string>(loaderResult.pRootTile->getTileID());
    CHECK(tileID == "tileset2.json");

    // check tile content
    auto tileLoadResult = loadTileContent(
        testDataPath / "AddTileset" / tileID,
        *loaderResult.pLoader,
        *loaderResult.pRootTile);
    CHECK(tileLoadResult.updatedBoundingVolume == std::nullopt);
    CHECK(tileLoadResult.updatedContentBoundingVolume == std::nullopt);
    CHECK(std::holds_alternative<TileExternalContent>(
        tileLoadResult.contentKind));
    CHECK(tileLoadResult.state == TileLoadResultState::Success);
    CHECK(tileLoadResult.tileInitializer);

    // check tile is really an external tile
    loaderResult.pRootTile->getContent().setContentKind(
        std::get<TileExternalContent>(tileLoadResult.contentKind));
    tileLoadResult.tileInitializer(*loaderResult.pRootTile);
    const auto& children = loaderResult.pRootTile->getChildren();
    CHECK(children.size() == 1);

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

  SECTION("Load tile that has external content with implicit extensions") {
    auto loaderResult =
        createLoader(testDataPath / "ImplicitTileset" / "tileset.json");

    CHECK(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->isExternalContent());
    CHECK(loaderResult.pRootTile->getChildren().size() == 1);

    auto& implicitTile = loaderResult.pRootTile->getChildren().front();
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
}
