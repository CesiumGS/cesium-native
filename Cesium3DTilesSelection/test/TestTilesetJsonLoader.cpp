#include "SimpleAssetAccessor.h"
#include "SimpleAssetRequest.h"
#include "SimpleAssetResponse.h"
#include "SimplePrepareRendererResource.h"
#include "SimpleTaskProcessor.h"
#include "TilesetJsonLoader.h"
#include "readFile.h"

#include <catch2/catch.hpp>

#include <cstddef>
#include <memory>
#include <string>

using namespace CesiumAsync;
using namespace Cesium3DTilesSelection;

namespace {
TilesetExternals
createMockTilesetExternals(std::vector<std::byte>&& tilesetContent) {
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
  mockCompletedRequests.insert(
      {"tileset.json", std::move(pMockCompletedRequest)});

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
} // namespace

TEST_CASE("Test creating tileset json loader") {
  std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;

  SECTION("Create valid tileset json with REPLACE refinement") {
    auto tilesetData =
        readFile(testDataPath / "ReplaceTileset" / "tileset.json");

    auto loaderResult = TilesetJsonLoader::createLoader(
                            createMockTilesetExternals(std::move(tilesetData)),
                            "tileset.json",
                            {})
                            .wait();

    CHECK(!loaderResult.errors.hasErrors());
    CHECK(loaderResult.gltfUpAxis == CesiumGeometry::Axis::Y);

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
  }

  SECTION("Create valid tileset json with ADD refinement") {
    auto tilesetData = readFile(testDataPath / "AddTileset" / "tileset2.json");

    auto loaderResult = TilesetJsonLoader::createLoader(
                            createMockTilesetExternals(std::move(tilesetData)),
                            "tileset.json",
                            {})
                            .wait();

    CHECK(!loaderResult.errors.hasErrors());
    CHECK(loaderResult.gltfUpAxis == CesiumGeometry::Axis::Y);

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
  }

  SECTION("Tileset has tile with no bounding volume field") {
    auto tilesetData = readFile(
        testDataPath / "ErrorTilesets" / "NoBoundingVolumeTileset.json");

    auto loaderResult = TilesetJsonLoader::createLoader(
                            createMockTilesetExternals(std::move(tilesetData)),
                            "tileset.json",
                            {})
                            .wait();

    CHECK(!loaderResult.errors.hasErrors());
    CHECK(loaderResult.gltfUpAxis == CesiumGeometry::Axis::Y);
    CHECK(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->getChildren().empty());
  }

  SECTION("Tileset has tile with no geometric error field") {
  }

  SECTION("Tileset has tile with no capitalized Refinement field") {}
}

TEST_CASE("Test loading individual tile of tileset json") {
  SECTION("Load tile that has render content") {}

  SECTION("Load tile that has external content") {}

  SECTION("Load tile that has external content with implicit extensions") {}

  SECTION("Load tile that has unknown content") {}
}
