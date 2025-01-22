#include "SimplePrepareRendererResource.h"
#include "TestTilesetJsonLoader.h"
#include "TilesetContentManager.h"
#include "TilesetJsonLoader.h"

#include <Cesium3DTilesContent/registerAllTileContentTypes.h>
#include <Cesium3DTilesSelection/RasterOverlayCollection.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TileRefine.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/AccessorWriter.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Node.h>
#include <CesiumGltf/Scene.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumRasterOverlays/DebugColorizeTilesRasterOverlay.h>
#include <CesiumRasterOverlays/IPrepareRasterOverlayRendererResources.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/common.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/trigonometric.hpp>
#include <spdlog/logger.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

using namespace doctest;
using namespace Cesium3DTilesSelection;
using namespace CesiumGeospatial;
using namespace CesiumGeometry;
using namespace CesiumUtility;
using namespace CesiumNativeTests;
using namespace CesiumRasterOverlays;

namespace {
std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;

class SimpleTilesetContentLoader : public TilesetContentLoader {
public:
  CesiumAsync::Future<TileLoadResult>
  loadTileContent(const TileLoadInput& input) override {
    return input.asyncSystem.createResolvedFuture(
        std::move(mockLoadTileContent));
  }

  TileChildrenResult createTileChildren(
      [[maybe_unused]] const Tile& tile,
      [[maybe_unused]] const Ellipsoid& ellipsoid) override {
    return std::move(mockCreateTileChildren);
  }

  TileLoadResult mockLoadTileContent;
  TileChildrenResult mockCreateTileChildren;
};

std::shared_ptr<SimpleAssetRequest>
createMockRequest(const std::filesystem::path& path) {
  auto pMockCompletedResponse = std::make_unique<SimpleAssetResponse>(
      static_cast<uint16_t>(200),
      "doesn't matter",
      CesiumAsync::HttpHeaders{},
      readFile(path));

  auto pMockCompletedRequest = std::make_shared<SimpleAssetRequest>(
      "GET",
      "doesn't matter",
      CesiumAsync::HttpHeaders{},
      std::move(pMockCompletedResponse));

  return pMockCompletedRequest;
}

CesiumGltf::Model createGlobeGrid(
    const Cartographic& beginPoint,
    uint32_t width,
    uint32_t height,
    double dimension) {
  const auto& ellipsoid = Ellipsoid::WGS84;
  std::vector<uint32_t> indices;
  glm::dvec3 min = ellipsoid.cartographicToCartesian(beginPoint);
  glm::dvec3 max = min;

  std::vector<glm::dvec3> positions;
  indices.reserve(static_cast<size_t>(6 * (width - 1) * (height - 1)));
  positions.reserve(static_cast<size_t>(width * height));
  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      double longitude = beginPoint.longitude + x * dimension;
      double latitude = beginPoint.latitude + y * dimension;
      Cartographic currPoint{longitude, latitude, beginPoint.height};
      positions.emplace_back(ellipsoid.cartographicToCartesian(currPoint));
      min = glm::min(positions.back(), min);
      max = glm::max(positions.back(), max);
      if (y != height - 1 && x != width - 1) {
        indices.emplace_back(y * width + x);
        indices.emplace_back(y * width + x + 1);
        indices.emplace_back((y + 1) * width + x);

        indices.emplace_back(y * width + x + 1);
        indices.emplace_back((y + 1) * width + x + 1);
        indices.emplace_back((y + 1) * width + x);
      }
    }
  }

  glm::dvec3 center = (min + max) / 2.0;
  std::vector<glm::vec3> relToCenterPositions;
  relToCenterPositions.reserve(positions.size());
  for (const auto& position : positions) {
    relToCenterPositions.emplace_back(
        static_cast<glm::vec3>(position - center));
  }

  CesiumGltf::Model model;

  CesiumGltf::Mesh& mesh = model.meshes.emplace_back();
  CesiumGltf::MeshPrimitive& meshPrimitive = mesh.primitives.emplace_back();

  {
    CesiumGltf::Buffer& positionBuffer = model.buffers.emplace_back();
    positionBuffer.byteLength =
        static_cast<int64_t>(relToCenterPositions.size() * sizeof(glm::vec3));
    positionBuffer.cesium.data.resize(
        static_cast<size_t>(positionBuffer.byteLength));
    std::memcpy(
        positionBuffer.cesium.data.data(),
        relToCenterPositions.data(),
        static_cast<size_t>(positionBuffer.byteLength));

    CesiumGltf::BufferView& positionBufferView =
        model.bufferViews.emplace_back();
    positionBufferView.buffer = int32_t(model.buffers.size() - 1);
    positionBufferView.byteOffset = 0;
    positionBufferView.byteLength = positionBuffer.byteLength;
    positionBufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;

    CesiumGltf::Accessor& positionAccessor = model.accessors.emplace_back();
    positionAccessor.bufferView = int32_t(model.bufferViews.size() - 1);
    positionAccessor.byteOffset = 0;
    positionAccessor.componentType = CesiumGltf::Accessor::ComponentType::FLOAT;
    positionAccessor.count = int64_t(relToCenterPositions.size());
    positionAccessor.type = CesiumGltf::Accessor::Type::VEC3;

    meshPrimitive.attributes["POSITION"] = int32_t(model.accessors.size() - 1);
  }

  {
    CesiumGltf::Buffer& indicesBuffer = model.buffers.emplace_back();
    indicesBuffer.byteLength =
        static_cast<int64_t>(indices.size() * sizeof(uint32_t));
    indicesBuffer.cesium.data.resize(
        static_cast<size_t>(indicesBuffer.byteLength));
    std::memcpy(
        indicesBuffer.cesium.data.data(),
        indices.data(),
        static_cast<size_t>(indicesBuffer.byteLength));

    CesiumGltf::BufferView& indicesBufferView =
        model.bufferViews.emplace_back();
    indicesBufferView.buffer = int32_t(model.buffers.size() - 1);
    indicesBufferView.byteOffset = 0;
    indicesBufferView.byteLength = indicesBuffer.byteLength;
    indicesBufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;

    CesiumGltf::Accessor& indicesAccessor = model.accessors.emplace_back();
    indicesAccessor.bufferView = int32_t(model.bufferViews.size() - 1);
    indicesAccessor.byteOffset = 0;
    indicesAccessor.componentType =
        CesiumGltf::Accessor::ComponentType::UNSIGNED_INT;
    indicesAccessor.count = int64_t(indices.size());
    indicesAccessor.type = CesiumGltf::Accessor::Type::SCALAR;

    meshPrimitive.indices = int32_t(model.accessors.size() - 1);
  }

  CesiumGltf::Node& node = model.nodes.emplace_back();
  node.translation = {center.x, center.y, center.z};
  node.mesh = int32_t(model.meshes.size() - 1);

  CesiumGltf::Scene& scene = model.scenes.emplace_back();
  scene.nodes.emplace_back(int32_t(model.nodes.size() - 1));

  return model;
}

// Creates a model with two triangles in opposite corners of the given
// rectangle. The triangles extend slightly into the other two quadrants.
CesiumGltf::Model createSparseMesh(const GlobeRectangle& rectangle) {
  const auto& ellipsoid = Ellipsoid::WGS84;

  double width = rectangle.computeWidth();
  double height = rectangle.computeHeight();

  // First triangle in southwest corner
  glm::dvec3 t0p0 = ellipsoid.cartographicToCartesian(rectangle.getSouthwest());
  glm::dvec3 t0p1 = ellipsoid.cartographicToCartesian(Cartographic(
      rectangle.getWest() + width * 0.55,
      rectangle.getSouth() + height * 0.1,
      0.0));
  glm::dvec3 t0p2 = ellipsoid.cartographicToCartesian(Cartographic(
      rectangle.getWest(),
      rectangle.getSouth() + height * 0.2,
      0.0));

  // Second triangle in northeast corner
  glm::dvec3 t1p0 = ellipsoid.cartographicToCartesian(rectangle.getNortheast());
  glm::dvec3 t1p1 = ellipsoid.cartographicToCartesian(Cartographic(
      rectangle.getEast() - width * 0.55,
      rectangle.getNorth() - height * 0.1,
      0.0));
  glm::dvec3 t1p2 = ellipsoid.cartographicToCartesian(Cartographic(
      rectangle.getEast(),
      rectangle.getNorth() - height * 0.2,
      0.0));

  std::vector<glm::dvec3> positions{t0p0, t0p1, t0p2, t1p0, t1p1, t1p2};
  glm::dvec3 center = (t0p0 + t1p0) * 0.5;

  CesiumGltf::Model model;
  model.asset.version = "2.0";

  CesiumGltf::Mesh& mesh = model.meshes.emplace_back();
  CesiumGltf::MeshPrimitive& meshPrimitive = mesh.primitives.emplace_back();

  {
    CesiumGltf::Buffer& positionBuffer = model.buffers.emplace_back();
    positionBuffer.byteLength =
        static_cast<int64_t>(positions.size() * sizeof(glm::vec3));
    positionBuffer.cesium.data.resize(
        static_cast<size_t>(positionBuffer.byteLength));

    CesiumGltf::BufferView& positionBufferView =
        model.bufferViews.emplace_back();
    positionBufferView.buffer = int32_t(model.buffers.size() - 1);
    positionBufferView.byteOffset = 0;
    positionBufferView.byteLength = positionBuffer.byteLength;
    positionBufferView.target = CesiumGltf::BufferView::Target::ARRAY_BUFFER;

    CesiumGltf::Accessor& positionAccessor = model.accessors.emplace_back();
    positionAccessor.bufferView = int32_t(model.bufferViews.size() - 1);
    positionAccessor.byteOffset = 0;
    positionAccessor.componentType = CesiumGltf::Accessor::ComponentType::FLOAT;
    positionAccessor.count = int64_t(positions.size());
    positionAccessor.type = CesiumGltf::Accessor::Type::VEC3;

    CesiumGltf::AccessorWriter<glm::vec3> writer(model, positionAccessor);
    CHECK(writer.size() == int64_t(positions.size()));

    for (size_t i = 0; i < positions.size(); ++i) {
      writer[int64_t(i)] = glm::vec3(positions[i] - center);
    }

    meshPrimitive.attributes["POSITION"] = int32_t(model.accessors.size() - 1);
  }

  {
    CesiumGltf::Buffer& indicesBuffer = model.buffers.emplace_back();
    indicesBuffer.byteLength = static_cast<int64_t>(6 * sizeof(uint8_t));
    indicesBuffer.cesium.data.resize(
        static_cast<size_t>(indicesBuffer.byteLength));

    CesiumGltf::BufferView& indicesBufferView =
        model.bufferViews.emplace_back();
    indicesBufferView.buffer = int32_t(model.buffers.size() - 1);
    indicesBufferView.byteOffset = 0;
    indicesBufferView.byteLength = indicesBuffer.byteLength;
    indicesBufferView.target =
        CesiumGltf::BufferView::Target::ELEMENT_ARRAY_BUFFER;

    CesiumGltf::Accessor& indicesAccessor = model.accessors.emplace_back();
    indicesAccessor.bufferView = int32_t(model.bufferViews.size() - 1);
    indicesAccessor.byteOffset = 0;
    indicesAccessor.componentType =
        CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE;
    indicesAccessor.count = 6;
    indicesAccessor.type = CesiumGltf::Accessor::Type::SCALAR;

    CesiumGltf::AccessorWriter<uint8_t> writer(model, indicesAccessor);
    CHECK(writer.size() == 6);

    for (int64_t i = 0; i < writer.size(); ++i) {
      writer[i] = uint8_t(i);
    }

    meshPrimitive.indices = int32_t(model.accessors.size() - 1);
  }

  CesiumGltf::Node& node = model.nodes.emplace_back();
  node.translation = {center.x, center.y, center.z};
  node.mesh = int32_t(model.meshes.size() - 1);

  CesiumGltf::Scene& scene = model.scenes.emplace_back();
  scene.nodes.emplace_back(int32_t(model.nodes.size() - 1));

  return model;
}

} // namespace

TEST_CASE("Test the manager can be initialized with correct loaders") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  // create mock tileset externals
  auto pMockedAssetAccessor = std::make_shared<SimpleAssetAccessor>(
      std::map<std::string, std::shared_ptr<SimpleAssetRequest>>{});
  auto pMockedPrepareRendererResources =
      std::make_shared<SimplePrepareRendererResource>();
  CesiumAsync::AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};
  auto pMockedCreditSystem = std::make_shared<CreditSystem>();

  TilesetExternals externals{
      pMockedAssetAccessor,
      pMockedPrepareRendererResources,
      asyncSystem,
      pMockedCreditSystem};

  SUBCASE("Initialize manager with tileset.json url") {
    // create mock request
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"tileset.json",
         createMockRequest(testDataPath / "Tileset" / "tileset.json")});

    // construct manager with tileset.json format
    Tile::LoadedLinkedList loadedTiles;
    IntrusivePointer<TilesetContentManager> pManager =
        new TilesetContentManager(
            externals,
            {},
            RasterOverlayCollection{loadedTiles, externals},
            "tileset.json");
    TilesetContentManager& manager = *pManager;
    CHECK(manager.getNumberOfTilesLoading() == 1);

    manager.waitUntilIdle();
    CHECK(manager.getNumberOfTilesLoading() == 0);
    CHECK(manager.getNumberOfTilesLoaded() == 1);

    // check root
    const Tile* pTilesetJson = manager.getRootTile();
    REQUIRE(pTilesetJson);
    REQUIRE(pTilesetJson->getChildren().size() == 1);
    const Tile* pRootTile = &pTilesetJson->getChildren()[0];
    CHECK(std::get<std::string>(pRootTile->getTileID()) == "parent.b3dm");
    CHECK(pRootTile->getGeometricError() == 70.0);
    CHECK(pRootTile->getRefine() == TileRefine::Add);
  }

  SUBCASE("Initialize manager with layer.json url") {
    // create mock request
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json",
         createMockRequest(
             testDataPath / "CesiumTerrainTileJson" /
             "QuantizedMesh.tile.json")});

    // construct manager with tileset.json format
    Tile::LoadedLinkedList loadedTiles;
    IntrusivePointer<TilesetContentManager> pManager =
        new TilesetContentManager(
            externals,
            {},
            RasterOverlayCollection{loadedTiles, externals},
            "layer.json");
    TilesetContentManager& manager = *pManager;
    CHECK(manager.getNumberOfTilesLoading() == 1);

    manager.waitUntilIdle();
    CHECK(manager.getNumberOfTilesLoading() == 0);
    CHECK(manager.getNumberOfTilesLoaded() == 1);

    // check root
    const Tile* pRootTile = manager.getRootTile();
    CHECK(pRootTile);
    CHECK(pRootTile->getRefine() == TileRefine::Replace);

    const std::span<const Tile> children = pRootTile->getChildren();
    CHECK(
        std::get<QuadtreeTileID>(children[0].getTileID()) ==
        QuadtreeTileID(0, 0, 0));
    CHECK(
        std::get<QuadtreeTileID>(children[1].getTileID()) ==
        QuadtreeTileID(0, 1, 0));
  }

  SUBCASE("Initialize manager with wrong format") {
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json",
         createMockRequest(
             testDataPath / "CesiumTerrainTileJson" /
             "WithAttribution.tile.json")});

    // construct manager with tileset.json format
    Tile::LoadedLinkedList loadedTiles;
    IntrusivePointer<TilesetContentManager> pManager =
        new TilesetContentManager(
            externals,
            {},
            RasterOverlayCollection{loadedTiles, externals},
            "layer.json");
    TilesetContentManager& manager = *pManager;
    CHECK(manager.getNumberOfTilesLoading() == 1);

    manager.waitUntilIdle();
    CHECK(manager.getNumberOfTilesLoading() == 0);
    CHECK(manager.getNumberOfTilesLoaded() == 1);

    // check root
    const Tile* pRootTile = manager.getRootTile();
    CHECK(!pRootTile);
  }
}

TEST_CASE("Test tile state machine") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  // create mock tileset externals
  auto pMockedAssetAccessor = std::make_shared<SimpleAssetAccessor>(
      std::map<std::string, std::shared_ptr<SimpleAssetRequest>>{});
  auto pMockedPrepareRendererResources =
      std::make_shared<SimplePrepareRendererResource>();
  CesiumAsync::AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};
  auto pMockedCreditSystem = std::make_shared<CreditSystem>();

  TilesetExternals externals{
      pMockedAssetAccessor,
      pMockedPrepareRendererResources,
      asyncSystem,
      pMockedCreditSystem};

  SUBCASE("Load content successfully") {
    // create mock loader
    bool initializerCall = false;
    auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
    pMockedLoader->mockLoadTileContent = {
        CesiumGltf::Model(),
        CesiumGeometry::Axis::Y,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        nullptr,
        [&](Tile&) { initializerCall = true; },
        TileLoadResultState::Success,
        Ellipsoid::WGS84};
    pMockedLoader->mockCreateTileChildren = {{}, TileLoadResultState::Success};
    pMockedLoader->mockCreateTileChildren.children.emplace_back(
        pMockedLoader.get(),
        TileEmptyContent());

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());

    // create manager
    TilesetOptions options{};
    options.contentOptions.generateMissingNormalsSmooth = true;

    Tile::LoadedLinkedList loadedTiles;
    IntrusivePointer<TilesetContentManager> pManager =
        new TilesetContentManager{
            externals,
            options,
            RasterOverlayCollection{loadedTiles, externals},
            std::move(pMockedLoader),
            std::move(pRootTile)};

    // test manager loading
    Tile& tile = *pManager->getRootTile();
    pManager->loadTileContent(tile, options);

    SUBCASE("Load tile from ContentLoading -> Done") {
      // Unloaded -> ContentLoading
      // check the state of the tile before main thread get called
      CHECK(pManager->getNumberOfTilesLoading() == 1);
      CHECK(tile.getState() == TileLoadState::ContentLoading);
      CHECK(tile.getContent().isUnknownContent());
      CHECK(!tile.getContent().isRenderContent());
      CHECK(!tile.getContent().isExternalContent());
      CHECK(!tile.getContent().isEmptyContent());
      CHECK(!tile.getContent().getRenderContent());
      CHECK(!initializerCall);

      // ContentLoading -> ContentLoaded
      // check the state of the tile after main thread get called
      pManager->waitUntilIdle();
      CHECK(pManager->getNumberOfTilesLoading() == 0);
      CHECK(tile.getState() == TileLoadState::ContentLoaded);
      CHECK(tile.getContent().isRenderContent());
      CHECK(tile.getContent().getRenderContent()->getRenderResources());
      CHECK(initializerCall);

      // ContentLoaded -> Done
      // update tile content to move from ContentLoaded -> Done
      pManager->updateTileContent(tile, options);
      CHECK(tile.getState() == TileLoadState::Done);
      CHECK(tile.getChildren().size() == 1);
      CHECK(tile.getChildren().front().getContent().isEmptyContent());
      CHECK(tile.getContent().isRenderContent());
      CHECK(tile.getContent().getRenderContent()->getRenderResources());
      CHECK(initializerCall);

      // Done -> Unloaded
      pManager->unloadTileContent(tile);
      CHECK(tile.getState() == TileLoadState::Unloaded);
      CHECK(tile.getContent().isUnknownContent());
      CHECK(!tile.getContent().isRenderContent());
      CHECK(!tile.getContent().getRenderContent());
    }

    SUBCASE("Try to unload tile when it's still loading") {
      // unload tile to move from Done -> Unload
      pManager->unloadTileContent(tile);
      CHECK(pManager->getNumberOfTilesLoading() == 1);
      CHECK(tile.getState() == TileLoadState::ContentLoading);
      CHECK(tile.getContent().isUnknownContent());
      CHECK(!tile.getContent().isRenderContent());
      CHECK(!tile.getContent().isExternalContent());
      CHECK(!tile.getContent().isEmptyContent());
      CHECK(!tile.getContent().getRenderContent());

      pManager->waitUntilIdle();
      CHECK(pManager->getNumberOfTilesLoading() == 0);
      CHECK(tile.getState() == TileLoadState::ContentLoaded);
      CHECK(tile.getContent().isRenderContent());
      CHECK(tile.getContent().getRenderContent()->getRenderResources());

      pManager->unloadTileContent(tile);
      CHECK(pManager->getNumberOfTilesLoading() == 0);
      CHECK(tile.getState() == TileLoadState::Unloaded);
      CHECK(tile.getContent().isUnknownContent());
      CHECK(!tile.getContent().isRenderContent());
      CHECK(!tile.getContent().getRenderContent());
    }
  }

  SUBCASE("Loader requests retry later") {
    // create mock loader
    bool initializerCall = false;
    auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
    pMockedLoader->mockLoadTileContent = {
        CesiumGltf::Model(),
        CesiumGeometry::Axis::Y,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        nullptr,
        [&](Tile&) { initializerCall = true; },
        TileLoadResultState::RetryLater,
        Ellipsoid::WGS84};
    pMockedLoader->mockCreateTileChildren = {{}, TileLoadResultState::Success};
    pMockedLoader->mockCreateTileChildren.children.emplace_back(
        pMockedLoader.get(),
        TileEmptyContent());

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());

    // create manager
    TilesetOptions options{};
    options.contentOptions.generateMissingNormalsSmooth = true;

    Tile::LoadedLinkedList loadedTiles;
    IntrusivePointer<TilesetContentManager> pManager =
        new TilesetContentManager{
            externals,
            options,
            RasterOverlayCollection{loadedTiles, externals},
            std::move(pMockedLoader),
            std::move(pRootTile)};

    // test manager loading
    Tile& tile = *pManager->getRootTile();
    pManager->loadTileContent(tile, options);

    // Unloaded -> ContentLoading
    CHECK(pManager->getNumberOfTilesLoading() == 1);
    CHECK(tile.getState() == TileLoadState::ContentLoading);
    CHECK(tile.getChildren().empty());
    CHECK(tile.getContent().isUnknownContent());
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().getRenderContent());

    // ContentLoading -> FailedTemporarily
    pManager->waitUntilIdle();
    CHECK(pManager->getNumberOfTilesLoading() == 0);
    CHECK(tile.getChildren().empty());
    CHECK(tile.getState() == TileLoadState::FailedTemporarily);
    CHECK(tile.getContent().isUnknownContent());
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().getRenderContent());
    CHECK(!initializerCall);

    // FailedTemporarily -> FailedTemporarily
    // tile is failed temporarily but the loader can still add children to it
    pManager->updateTileContent(tile, options);
    CHECK(pManager->getNumberOfTilesLoading() == 0);
    CHECK(tile.getChildren().size() == 1);
    CHECK(tile.getChildren().front().isEmptyContent());
    CHECK(tile.getState() == TileLoadState::FailedTemporarily);
    CHECK(tile.getContent().isUnknownContent());
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().getRenderContent());
    CHECK(!initializerCall);

    // FailedTemporarily -> ContentLoading
    pManager->loadTileContent(tile, options);
    CHECK(pManager->getNumberOfTilesLoading() == 1);
    CHECK(tile.getState() == TileLoadState::ContentLoading);
  }

  SUBCASE("Loader requests failed") {
    // create mock loader
    bool initializerCall = false;
    auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
    pMockedLoader->mockLoadTileContent = {
        CesiumGltf::Model(),
        CesiumGeometry::Axis::Y,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        nullptr,
        [&](Tile&) { initializerCall = true; },
        TileLoadResultState::Failed,
        Ellipsoid::WGS84};
    pMockedLoader->mockCreateTileChildren = {{}, TileLoadResultState::Success};
    pMockedLoader->mockCreateTileChildren.children.emplace_back(
        pMockedLoader.get(),
        TileEmptyContent());

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());

    // create manager
    TilesetOptions options{};
    options.contentOptions.generateMissingNormalsSmooth = true;

    Tile::LoadedLinkedList loadedTiles;
    IntrusivePointer<TilesetContentManager> pManager =
        new TilesetContentManager{
            externals,
            options,
            RasterOverlayCollection{loadedTiles, externals},
            std::move(pMockedLoader),
            std::move(pRootTile)};

    // test manager loading
    Tile& tile = *pManager->getRootTile();
    pManager->loadTileContent(tile, options);

    // Unloaded -> ContentLoading
    CHECK(pManager->getNumberOfTilesLoading() == 1);
    CHECK(tile.getState() == TileLoadState::ContentLoading);
    CHECK(tile.getChildren().empty());
    CHECK(tile.getContent().isUnknownContent());
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().getRenderContent());

    // ContentLoading -> Failed
    pManager->waitUntilIdle();
    CHECK(pManager->getNumberOfTilesLoading() == 0);
    CHECK(tile.getChildren().empty());
    CHECK(tile.getState() == TileLoadState::Failed);
    CHECK(tile.getContent().isUnknownContent());
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().getRenderContent());
    CHECK(!initializerCall);

    // Failed -> Failed
    // tile is failed but the loader can still add children to it
    pManager->updateTileContent(tile, options);
    CHECK(pManager->getNumberOfTilesLoading() == 0);
    CHECK(tile.getChildren().size() == 1);
    CHECK(tile.getChildren().front().isEmptyContent());
    CHECK(tile.getState() == TileLoadState::Failed);
    CHECK(tile.getContent().isUnknownContent());
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().getRenderContent());
    CHECK(!initializerCall);

    // cannot transition from Failed -> ContentLoading
    pManager->loadTileContent(tile, options);
    CHECK(pManager->getNumberOfTilesLoading() == 0);
    CHECK(tile.getState() == TileLoadState::Failed);
    CHECK(tile.getContent().isUnknownContent());
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().isExternalContent());
    CHECK(!tile.getContent().isEmptyContent());
    CHECK(!tile.getContent().getRenderContent());

    // Failed -> Unloaded
    pManager->unloadTileContent(tile);
    CHECK(tile.getState() == TileLoadState::Unloaded);
    CHECK(tile.getContent().isUnknownContent());
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().isExternalContent());
    CHECK(!tile.getContent().isEmptyContent());
    CHECK(!tile.getContent().getRenderContent());
  }

  SUBCASE("Make sure the manager loads parent first before loading upsampled "
          "child") {
    // create mock loader
    bool initializerCall = false;
    auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
    auto pMockedLoaderRaw = pMockedLoader.get();

    pMockedLoader->mockLoadTileContent = {
        CesiumGltf::Model(),
        CesiumGeometry::Axis::Y,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        nullptr,
        [&](Tile&) { initializerCall = true; },
        TileLoadResultState::Success,
        Ellipsoid::WGS84};
    pMockedLoader->mockCreateTileChildren = {{}, TileLoadResultState::Failed};

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoaderRaw);
    pRootTile->setTileID(QuadtreeTileID(0, 0, 0));

    // create upsampled children. We put it in the scope since upsampledTile
    // will be moved to the parent afterward, and we don't want the tests below
    // to use it accidentally
    {
      std::vector<Tile> children;
      children.emplace_back(pMockedLoaderRaw);
      Tile& upsampledTile = children.back();
      upsampledTile.setTileID(UpsampledQuadtreeNode{QuadtreeTileID(1, 1, 1)});
      pRootTile->createChildTiles(std::move(children));
    }

    // create manager
    TilesetOptions options{};
    options.contentOptions.generateMissingNormalsSmooth = true;

    Tile::LoadedLinkedList loadedTiles;
    IntrusivePointer<TilesetContentManager> pManager =
        new TilesetContentManager{
            externals,
            options,
            RasterOverlayCollection{loadedTiles, externals},
            std::move(pMockedLoader),
            std::move(pRootTile)};

    Tile& tile = *pManager->getRootTile();
    Tile& upsampledTile = tile.getChildren().back();

    // test manager loading upsample tile
    pManager->loadTileContent(upsampledTile, options);

    // since parent is not yet loaded, it will load the parent first.
    // The upsampled tile will not be loaded at the moment
    CHECK(upsampledTile.getState() == TileLoadState::Unloaded);
    CHECK(tile.getState() == TileLoadState::ContentLoading);

    // parent moves from ContentLoading -> ContentLoaded
    pManager->waitUntilIdle();
    CHECK(tile.getState() == TileLoadState::ContentLoaded);
    CHECK(tile.isRenderContent());
    CHECK(initializerCall);

    // try again with upsample tile, but still not able to load it
    // because parent is not done yet
    pManager->loadTileContent(upsampledTile, options);
    CHECK(upsampledTile.getState() == TileLoadState::Unloaded);

    // parent moves from ContentLoaded -> Done
    pManager->updateTileContent(tile, options);
    CHECK(tile.getState() == TileLoadState::Done);
    CHECK(tile.getChildren().size() == 1);
    CHECK(&tile.getChildren().back() == &upsampledTile);
    CHECK(tile.isRenderContent());
    CHECK(initializerCall);

    // load the upsampled tile again: Unloaded -> ContentLoading
    initializerCall = false;
    pMockedLoaderRaw->mockLoadTileContent = {
        CesiumGltf::Model(),
        CesiumGeometry::Axis::Y,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        nullptr,
        [&](Tile&) { initializerCall = true; },
        TileLoadResultState::Success,
        Ellipsoid::WGS84};
    pMockedLoaderRaw->mockCreateTileChildren = {
        {},
        TileLoadResultState::Failed};
    pManager->loadTileContent(upsampledTile, options);
    CHECK(upsampledTile.getState() == TileLoadState::ContentLoading);

    // trying to unload parent while upsampled children is loading while put the
    // tile into the Unloading state but not unload the render content.
    CHECK(!pManager->unloadTileContent(tile));
    CHECK(tile.getState() == TileLoadState::Unloading);
    CHECK(tile.isRenderContent());

    // Unloading again will have the same result.
    CHECK(!pManager->unloadTileContent(tile));
    CHECK(tile.getState() == TileLoadState::Unloading);
    CHECK(tile.isRenderContent());

    // Attempting to load won't do anything - unloading must finish first.
    pManager->loadTileContent(tile, options);
    CHECK(tile.getState() == TileLoadState::Unloading);

    // upsampled tile: ContentLoading -> ContentLoaded
    pManager->waitUntilIdle();
    CHECK(upsampledTile.getState() == TileLoadState::ContentLoaded);
    CHECK(upsampledTile.isRenderContent());

    // trying to unload parent will work now since the upsampled tile is already
    // in the main thread
    CHECK(pManager->unloadTileContent(tile));
    CHECK(tile.getState() == TileLoadState::Unloaded);
    CHECK(!tile.isRenderContent());
    CHECK(!tile.getContent().getRenderContent());

    // unload upsampled tile: ContentLoaded -> Done
    CHECK(pManager->unloadTileContent(upsampledTile));
    CHECK(upsampledTile.getState() == TileLoadState::Unloaded);
    CHECK(!upsampledTile.isRenderContent());
    CHECK(!upsampledTile.getContent().getRenderContent());
  }
}

TEST_CASE("Test the tileset content manager's post processing for gltf") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  // create mock tileset externals
  auto pMockedAssetAccessor = std::make_shared<SimpleAssetAccessor>(
      std::map<std::string, std::shared_ptr<SimpleAssetRequest>>{});
  auto pMockedPrepareRendererResources =
      std::make_shared<SimplePrepareRendererResource>();
  CesiumAsync::AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};
  auto pMockedCreditSystem = std::make_shared<CreditSystem>();

  TilesetExternals externals{
      pMockedAssetAccessor,
      pMockedPrepareRendererResources,
      asyncSystem,
      pMockedCreditSystem};

  SUBCASE("Resolve external buffers") {
    // create mock loader
    CesiumGltfReader::GltfReader gltfReader;
    std::vector<std::byte> gltfBoxFile =
        readFile(testDataPath / "gltf" / "box" / "Box.gltf");
    auto modelReadResult = gltfReader.readGltf(gltfBoxFile);

    // check that this model has external buffer and it's not loaded
    {
      CHECK(modelReadResult.errors.empty());
      CHECK(modelReadResult.warnings.empty());
      CHECK(modelReadResult.model);
      const auto& buffers = modelReadResult.model->buffers;
      CHECK(buffers.size() == 1);

      const auto& buffer = buffers.front();
      CHECK(buffer.uri == "Box0.bin");
      CHECK(buffer.byteLength == 648);
      CHECK(buffer.cesium.data.size() == 0);
    }

    auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
    pMockedLoader->mockLoadTileContent = {
        std::move(*modelReadResult.model),
        CesiumGeometry::Axis::Y,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        pMockedAssetAccessor,
        nullptr,
        {},
        TileLoadResultState::Success,
        Ellipsoid::WGS84};
    pMockedLoader->mockCreateTileChildren = {{}, TileLoadResultState::Failed};

    // add external buffer to the completed request
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"Box0.bin",
         createMockRequest(testDataPath / "gltf" / "box" / "Box0.bin")});

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());

    // create manager
    Tile::LoadedLinkedList loadedTiles;
    IntrusivePointer<TilesetContentManager> pManager =
        new TilesetContentManager{
            externals,
            {},
            RasterOverlayCollection{loadedTiles, externals},
            std::move(pMockedLoader),
            std::move(pRootTile)};

    // test the gltf model
    Tile& tile = *pManager->getRootTile();
    pManager->loadTileContent(tile, {});
    pManager->waitUntilIdle();

    // check the buffer is already loaded
    {
      CHECK(tile.getState() == TileLoadState::ContentLoaded);
      CHECK(tile.isRenderContent());

      const auto& renderContent = tile.getContent().getRenderContent();
      const auto& buffers = renderContent->getModel().buffers;
      CHECK(buffers.size() == 1);

      const auto& buffer = buffers.front();
      CHECK(buffer.uri == std::nullopt);
      CHECK(buffer.cesium.data.size() == 648);
    }

    // unload the tile content
    pManager->unloadTileContent(tile);
  }

  SUBCASE("Ensure the loader generate smooth normal when the mesh doesn't have "
          "normal") {
    CesiumGltfReader::GltfReader gltfReader;
    std::vector<std::byte> gltfBoxFile =
        readFile(testDataPath / "gltf" / "embedded_box" / "Box.glb");
    auto modelReadResult = gltfReader.readGltf(gltfBoxFile);
    CHECK(modelReadResult.errors.empty());
    CHECK(modelReadResult.model);

    // retrieve expected accessor index and remove normal attribute
    CesiumGltf::Mesh& prevMesh = modelReadResult.model->meshes.front();
    CesiumGltf::MeshPrimitive& prevPrimitive = prevMesh.primitives.front();
    int32_t expectedAccessor = prevPrimitive.attributes.at("NORMAL");
    prevPrimitive.attributes.erase("NORMAL");

    // create mock loader
    auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
    pMockedLoader->mockLoadTileContent = {
        std::move(*modelReadResult.model),
        CesiumGeometry::Axis::Y,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        nullptr,
        {},
        TileLoadResultState::Success,
        Ellipsoid::WGS84};
    pMockedLoader->mockCreateTileChildren = {{}, TileLoadResultState::Failed};

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());

    // create manager
    TilesetOptions options;
    options.contentOptions.generateMissingNormalsSmooth = true;

    Tile::LoadedLinkedList loadedTiles;
    IntrusivePointer<TilesetContentManager> pManager =
        new TilesetContentManager{
            externals,
            options,
            RasterOverlayCollection{loadedTiles, externals},
            std::move(pMockedLoader),
            std::move(pRootTile)};

    // test the gltf model
    Tile& tile = *pManager->getRootTile();
    pManager->loadTileContent(tile, options);
    pManager->waitUntilIdle();

    // check that normal is generated
    CHECK(tile.getState() == TileLoadState::ContentLoaded);
    const auto& renderContent = tile.getContent().getRenderContent();
    CHECK(renderContent->getModel().meshes.size() == 1);
    const CesiumGltf::Mesh& mesh = renderContent->getModel().meshes.front();
    CHECK(mesh.primitives.size() == 1);
    const CesiumGltf::MeshPrimitive& primitive = mesh.primitives.front();
    CHECK(primitive.attributes.find("NORMAL") != primitive.attributes.end());

    CesiumGltf::AccessorView<glm::vec3> normalView{
        renderContent->getModel(),
        primitive.attributes.at("NORMAL")};
    CHECK(normalView.size() == 8);

    CesiumGltf::AccessorView<glm::vec3> expectedNormalView{
        renderContent->getModel(),
        expectedAccessor};
    CHECK(expectedNormalView.size() == 8);

    for (int64_t i = 0; i < expectedNormalView.size(); ++i) {
      const glm::vec3& expectedNorm = expectedNormalView[i];
      const glm::vec3& norm = normalView[i];
      CHECK(expectedNorm.x == Approx(norm.x).epsilon(1e-4));
      CHECK(expectedNorm.y == Approx(norm.y).epsilon(1e-4));
      CHECK(expectedNorm.z == Approx(norm.z).epsilon(1e-4));
    }

    // unload tile
    pManager->unloadTileContent(tile);
  }

  SUBCASE("Embed gltf up axis to extra") {
    // create mock loader
    auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
    pMockedLoader->mockLoadTileContent = {
        CesiumGltf::Model{},
        CesiumGeometry::Axis::Z,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        nullptr,
        {},
        TileLoadResultState::Success,
        Ellipsoid::WGS84};
    pMockedLoader->mockCreateTileChildren = {{}, TileLoadResultState::Failed};

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());

    // create manager
    Tile::LoadedLinkedList loadedTiles;
    IntrusivePointer<TilesetContentManager> pManager =
        new TilesetContentManager{
            externals,
            {},
            RasterOverlayCollection{loadedTiles, externals},
            std::move(pMockedLoader),
            std::move(pRootTile)};

    Tile& tile = *pManager->getRootTile();
    pManager->loadTileContent(tile, {});
    pManager->waitUntilIdle();

    const auto& renderContent = tile.getContent().getRenderContent();
    CHECK(renderContent);

    auto gltfUpAxisIt = renderContent->getModel().extras.find("gltfUpAxis");
    CHECK(gltfUpAxisIt != renderContent->getModel().extras.end());
    CHECK(gltfUpAxisIt->second.getInt64() == 2);

    pManager->unloadTileContent(tile);
  }

  SUBCASE("Generate raster overlay projections") {
    // add raster overlay
    Tile::LoadedLinkedList loadedTiles;
    RasterOverlayCollection rasterOverlayCollection{loadedTiles, externals};
    rasterOverlayCollection.add(
        new DebugColorizeTilesRasterOverlay("DebugOverlay"));
    asyncSystem.dispatchMainThreadTasks();

    // create mock loader
    auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
    Cartographic beginCarto{glm::radians(32.0), glm::radians(48.0), 100.0};
    pMockedLoader->mockLoadTileContent = {
        createGlobeGrid(beginCarto, 10, 10, 0.01),
        CesiumGeometry::Axis::Z,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        nullptr,
        {},
        TileLoadResultState::Success,
        Ellipsoid::WGS84};
    pMockedLoader->mockCreateTileChildren = {{}, TileLoadResultState::Failed};

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());

    // create manager
    IntrusivePointer<TilesetContentManager> pManager =
        new TilesetContentManager{
            externals,
            {},
            std::move(rasterOverlayCollection),
            std::move(pMockedLoader),
            std::move(pRootTile)};

    SUBCASE(
        "Generate raster overlay details when tile doesn't have loose region") {
      // test the gltf model
      Tile& tile = *pManager->getRootTile();
      pManager->loadTileContent(tile, {});
      pManager->waitUntilIdle();

      CHECK(tile.getState() == TileLoadState::ContentLoaded);
      const TileContent& tileContent = tile.getContent();
      CHECK(tileContent.isRenderContent());
      const RasterOverlayDetails& rasterOverlayDetails =
          tileContent.getRenderContent()->getRasterOverlayDetails();

      // ensure the raster overlay details has geographic projection
      GeographicProjection geographicProjection{Ellipsoid::WGS84};
      auto existingProjectionIt = std::find(
          rasterOverlayDetails.rasterOverlayProjections.begin(),
          rasterOverlayDetails.rasterOverlayProjections.end(),
          Projection{geographicProjection});
      CHECK(
          existingProjectionIt !=
          rasterOverlayDetails.rasterOverlayProjections.end());

      // check the rectangle
      const auto& projectionRectangle =
          rasterOverlayDetails.rasterOverlayRectangles.front();
      auto globeRectangle = geographicProjection.unproject(projectionRectangle);
      CHECK(globeRectangle.getWest() == Approx(beginCarto.longitude));
      CHECK(globeRectangle.getSouth() == Approx(beginCarto.latitude));
      CHECK(
          globeRectangle.getEast() == Approx(beginCarto.longitude + 9 * 0.01));
      CHECK(
          globeRectangle.getNorth() == Approx(beginCarto.latitude + 9 * 0.01));

      // check the UVs
      const auto& renderContent = tileContent.getRenderContent();
      const auto& mesh = renderContent->getModel().meshes.front();
      const auto& meshPrimitive = mesh.primitives.front();
      CesiumGltf::AccessorView<glm::vec2> uv{
          renderContent->getModel(),
          meshPrimitive.attributes.at("_CESIUMOVERLAY_0")};
      CHECK(uv.status() == CesiumGltf::AccessorViewStatus::Valid);
      int64_t uvIdx = 0;
      for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
          CHECK(CesiumUtility::Math::equalsEpsilon(
              uv[uvIdx].x,
              x * 0.01 / globeRectangle.computeWidth(),
              CesiumUtility::Math::Epsilon7));
          CHECK(CesiumUtility::Math::equalsEpsilon(
              uv[uvIdx].y,
              y * 0.01 / globeRectangle.computeHeight(),
              CesiumUtility::Math::Epsilon7));
          ++uvIdx;
        }
      }
    }

    SUBCASE("Generate raster overlay details when tile has loose region") {
      Tile& tile = *pManager->getRootTile();
      auto originalLooseRegion =
          BoundingRegionWithLooseFittingHeights{BoundingRegion{
              GeographicProjection::MAXIMUM_GLOBE_RECTANGLE,
              -1000.0,
              9000.0,
              Ellipsoid::WGS84}};
      tile.setBoundingVolume(originalLooseRegion);

      pManager->loadTileContent(tile, {});
      pManager->waitUntilIdle();

      CHECK(tile.getState() == TileLoadState::ContentLoaded);
      const TileContent& tileContent = tile.getContent();
      CHECK(tileContent.isRenderContent());
      const RasterOverlayDetails& rasterOverlayDetails =
          tileContent.getRenderContent()->getRasterOverlayDetails();

      // ensure the raster overlay details has geographic projection
      GeographicProjection geographicProjection{Ellipsoid::WGS84};
      auto existingProjectionIt = std::find(
          rasterOverlayDetails.rasterOverlayProjections.begin(),
          rasterOverlayDetails.rasterOverlayProjections.end(),
          Projection{geographicProjection});
      CHECK(
          existingProjectionIt !=
          rasterOverlayDetails.rasterOverlayProjections.end());

      // check the rectangle
      const auto& projectionRectangle =
          rasterOverlayDetails.rasterOverlayRectangles.front();
      auto globeRectangle = geographicProjection.unproject(projectionRectangle);
      CHECK(globeRectangle.getWest() == Approx(-CesiumUtility::Math::OnePi));
      CHECK(
          globeRectangle.getSouth() == Approx(-CesiumUtility::Math::PiOverTwo));
      CHECK(globeRectangle.getEast() == Approx(CesiumUtility::Math::OnePi));
      CHECK(
          globeRectangle.getNorth() == Approx(CesiumUtility::Math::PiOverTwo));

      // check the tile whole region which will be more fitted
      const BoundingRegion& tileRegion =
          std::get<BoundingRegion>(tile.getBoundingVolume());
      CHECK(
          tileRegion.getRectangle().getWest() == Approx(beginCarto.longitude));
      CHECK(
          tileRegion.getRectangle().getSouth() == Approx(beginCarto.latitude));
      CHECK(
          tileRegion.getRectangle().getEast() ==
          Approx(beginCarto.longitude + 9 * 0.01));
      CHECK(
          tileRegion.getRectangle().getNorth() ==
          Approx(beginCarto.latitude + 9 * 0.01));

      // check the UVs
      const auto& renderContent = tileContent.getRenderContent();
      const auto& mesh = renderContent->getModel().meshes.front();
      const auto& meshPrimitive = mesh.primitives.front();
      CesiumGltf::AccessorView<glm::vec2> uv{
          renderContent->getModel(),
          meshPrimitive.attributes.at("_CESIUMOVERLAY_0")};
      CHECK(uv.status() == CesiumGltf::AccessorViewStatus::Valid);

      const auto& looseRectangle =
          originalLooseRegion.getBoundingRegion().getRectangle();
      int64_t uvIdx = 0;
      for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
          double expectedX = (beginCarto.longitude + x * 0.01 -
                              (-CesiumUtility::Math::OnePi)) /
                             looseRectangle.computeWidth();

          double expectedY = (beginCarto.latitude + y * 0.01 -
                              (-CesiumUtility::Math::PiOverTwo)) /
                             looseRectangle.computeHeight();

          CHECK(CesiumUtility::Math::equalsEpsilon(
              uv[uvIdx].x,
              expectedX,
              CesiumUtility::Math::Epsilon7));
          CHECK(CesiumUtility::Math::equalsEpsilon(
              uv[uvIdx].y,
              expectedY,
              CesiumUtility::Math::Epsilon7));
          ++uvIdx;
        }
      }
    }

    SUBCASE("Automatically calculate fit bounding region when tile has loose "
            "region") {
      auto pRemovedOverlay =
          pManager->getRasterOverlayCollection().begin()->get();
      pManager->getRasterOverlayCollection().remove(pRemovedOverlay);
      CHECK(pManager->getRasterOverlayCollection().size() == 0);

      Tile& tile = *pManager->getRootTile();
      auto originalLooseRegion =
          BoundingRegionWithLooseFittingHeights{BoundingRegion{
              GeographicProjection::MAXIMUM_GLOBE_RECTANGLE,
              -1000.0,
              9000.0,
              Ellipsoid::WGS84}};
      tile.setBoundingVolume(originalLooseRegion);

      pManager->loadTileContent(tile, {});
      pManager->waitUntilIdle();

      CHECK(tile.getState() == TileLoadState::ContentLoaded);

      // check the tile whole region which will be more fitted
      const BoundingRegion& tileRegion =
          std::get<BoundingRegion>(tile.getBoundingVolume());
      CHECK(
          tileRegion.getRectangle().getWest() == Approx(beginCarto.longitude));
      CHECK(
          tileRegion.getRectangle().getSouth() == Approx(beginCarto.latitude));
      CHECK(
          tileRegion.getRectangle().getEast() ==
          Approx(beginCarto.longitude + 9 * 0.01));
      CHECK(
          tileRegion.getRectangle().getNorth() ==
          Approx(beginCarto.latitude + 9 * 0.01));
    }
  }

  SUBCASE("Upsamples sparse tile for raster overlays") {
    // add raster overlay
    Tile::LoadedLinkedList loadedTiles;
    RasterOverlayCollection rasterOverlayCollection{loadedTiles, externals};

    class AlwaysMoreDetailProvider : public RasterOverlayTileProvider {
    public:
      AlwaysMoreDetailProvider(
          const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner,
          const CesiumAsync::AsyncSystem& asyncSystem,
          const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
          std::optional<CesiumUtility::Credit> credit,
          const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
              pPrepareRendererResources,
          const std::shared_ptr<spdlog::logger>& pLogger,
          const CesiumGeospatial::Projection& projection,
          const CesiumGeometry::Rectangle& coverageRectangle)
          : RasterOverlayTileProvider(
                pOwner,
                asyncSystem,
                pAssetAccessor,
                credit,
                pPrepareRendererResources,
                pLogger,
                projection,
                coverageRectangle) {}

      CesiumAsync::Future<LoadedRasterOverlayImage>
      loadTileImage(RasterOverlayTile& overlayTile) override {
        CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> pImage;
        CesiumGltf::ImageAsset& image = pImage.emplace();
        image.width = 1;
        image.height = 1;
        image.channels = 1;
        image.bytesPerChannel = 1;
        image.pixelData.resize(1, std::byte(255));

        return this->getAsyncSystem().createResolvedFuture(
            LoadedRasterOverlayImage{
                std::move(pImage),
                overlayTile.getRectangle(),
                {},
                {},
                true});
      }
    };

    class AlwaysMoreDetailRasterOverlay : public RasterOverlay {
    public:
      AlwaysMoreDetailRasterOverlay() : RasterOverlay("AlwaysMoreDetail") {}

      CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
          const CesiumAsync::AsyncSystem& asyncSystem,
          const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
          const std::shared_ptr<
              CesiumUtility::CreditSystem>& /* pCreditSystem */,
          const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
              pPrepareRendererResources,
          const std::shared_ptr<spdlog::logger>& pLogger,
          CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner)
          const override {
        return asyncSystem.createResolvedFuture(CreateTileProviderResult(
            CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>(
                new AlwaysMoreDetailProvider(
                    pOwner ? pOwner : this,
                    asyncSystem,
                    pAssetAccessor,
                    std::nullopt,
                    pPrepareRendererResources,
                    pLogger,
                    CesiumGeospatial::GeographicProjection(),
                    projectRectangleSimple(
                        CesiumGeospatial::GeographicProjection(),
                        GlobeRectangle::MAXIMUM)))));
      }
    };

    rasterOverlayCollection.add(new AlwaysMoreDetailRasterOverlay());
    asyncSystem.dispatchMainThreadTasks();

    // create mock loader
    auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
    GlobeRectangle tileRectangle =
        GlobeRectangle::fromDegrees(0.0010, 0.0011, 0.0012, 0.0013);
    pMockedLoader->mockLoadTileContent = {
        createSparseMesh(tileRectangle),
        CesiumGeometry::Axis::Z,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        nullptr,
        {},
        TileLoadResultState::Success,
        Ellipsoid::WGS84};
    pMockedLoader->mockCreateTileChildren = {{}, TileLoadResultState::Success};

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());

    // create manager
    IntrusivePointer<TilesetContentManager> pManager =
        new TilesetContentManager{
            externals,
            {},
            std::move(rasterOverlayCollection),
            std::move(pMockedLoader),
            std::move(pRootTile)};

    SUBCASE(
        "Generate raster overlay details when tile doesn't have loose region") {
      // test the gltf model
      Tile& tile = *pManager->getRootTile();

      auto loadUntilChildrenExist = [pManager, &asyncSystem](Tile& tile) {
        while (tile.getChildren().empty()) {
          pManager->loadTileContent(tile, {});
          asyncSystem.dispatchMainThreadTasks();
          pManager->updateTileContent(tile, {});
          asyncSystem.dispatchMainThreadTasks();
        }
      };

      loadUntilChildrenExist(tile);

      CHECK(tile.getState() == TileLoadState::Done);
      const TileContent& tileContent = tile.getContent();
      CHECK(tileContent.isRenderContent());
      const RasterOverlayDetails& rasterOverlayDetails =
          tileContent.getRenderContent()->getRasterOverlayDetails();

      // ensure the raster overlay details has geographic projection
      GeographicProjection geographicProjection{Ellipsoid::WGS84};
      auto existingProjectionIt = std::find(
          rasterOverlayDetails.rasterOverlayProjections.begin(),
          rasterOverlayDetails.rasterOverlayProjections.end(),
          Projection{geographicProjection});
      CHECK(
          existingProjectionIt !=
          rasterOverlayDetails.rasterOverlayProjections.end());

      // Both the raster overlay boundingRegion and rectangle should match the
      // tile rectangle.
      REQUIRE(!rasterOverlayDetails.rasterOverlayRectangles.empty());
      const Rectangle& projectionRectangle =
          rasterOverlayDetails.rasterOverlayRectangles.front();
      GlobeRectangle globeRectangle =
          geographicProjection.unproject(projectionRectangle);
      CHECK(GlobeRectangle::equalsEpsilon(
          globeRectangle,
          tileRectangle,
          Math::Epsilon13));
      CHECK(GlobeRectangle::equalsEpsilon(
          rasterOverlayDetails.boundingRegion.getRectangle(),
          tileRectangle,
          Math::Epsilon13));

      // Load the southeast child
      REQUIRE(tile.getChildren().size() == 4);
      Tile& se = tile.getChildren()[1];
      REQUIRE(std::get_if<UpsampledQuadtreeNode>(&se.getTileID()) != nullptr);
      REQUIRE(
          std::get<UpsampledQuadtreeNode>(se.getTileID()).tileID ==
          QuadtreeTileID(1, 1, 0));

      loadUntilChildrenExist(se);

      // Verify the bounding volume is sensible
      const BoundingRegion* pRegion =
          std::get_if<BoundingRegion>(&se.getBoundingVolume());
      REQUIRE(pRegion != nullptr);
      CHECK(
          pRegion->getRectangle().getEast() >
          pRegion->getRectangle().getWest());
      CHECK(
          pRegion->getRectangle().getNorth() >
          pRegion->getRectangle().getSouth());

      // The tight-fitting bounding region from the raster overlay process
      // should be sensible and smaller than the _original_ tile rectangle.
      TileRenderContent* pRenderContent = se.getContent().getRenderContent();
      REQUIRE(pRenderContent != nullptr);
      const GlobeRectangle& tightRectangle =
          pRenderContent->getRasterOverlayDetails()
              .boundingRegion.getRectangle();
      CHECK(tightRectangle.getEast() > tightRectangle.getWest());
      CHECK(tightRectangle.getNorth() > tightRectangle.getSouth());
      CHECK(
          tightRectangle.computeWidth() * 2.0 <
          tileRectangle.computeWidth() * 0.5);
      CHECK(
          tightRectangle.computeHeight() * 2.0 <
          tileRectangle.computeHeight() * 0.5);

      // The rectangle used for texture coordinates should also be sensible and
      // match the southeast quadrant of the original parent tile rectangle.
      REQUIRE(!pRenderContent->getRasterOverlayDetails()
                   .rasterOverlayRectangles.empty());
      const Rectangle& overlayRectangle =
          pRenderContent->getRasterOverlayDetails()
              .rasterOverlayRectangles.front();
      GlobeRectangle overlayGlobeRectangle =
          unprojectRectangleSimple(GeographicProjection(), overlayRectangle);

      GlobeRectangle seQuadrant(
          tileRectangle.computeCenter().longitude,
          tileRectangle.getSouth(),
          tileRectangle.getEast(),
          tileRectangle.computeCenter().latitude);
      CHECK(GlobeRectangle::equalsEpsilon(
          overlayGlobeRectangle,
          seQuadrant,
          Math::Epsilon13));

      // The tile should have a raster overlay mapped to it.
      REQUIRE(se.getMappedRasterTiles().size() == 1);

      // Load the southeast child's southwest child
      REQUIRE(se.getChildren().size() == 4);
      Tile& sw = se.getChildren()[0];
      REQUIRE(std::get_if<UpsampledQuadtreeNode>(&sw.getTileID()) != nullptr);
      REQUIRE(
          std::get<UpsampledQuadtreeNode>(sw.getTileID()).tileID ==
          QuadtreeTileID(2, 2, 0));

      loadUntilChildrenExist(sw);

      // Verify the bounding volume is sensible
      pRegion = std::get_if<BoundingRegion>(&sw.getBoundingVolume());
      REQUIRE(pRegion != nullptr);
      CHECK(
          pRegion->getRectangle().getEast() >
          pRegion->getRectangle().getWest());
      CHECK(
          pRegion->getRectangle().getNorth() >
          pRegion->getRectangle().getSouth());

      // The tight-fitting bounding region from the raster overlay process
      // should be sensible and smaller than the _original_ tile rectangle.
      pRenderContent = sw.getContent().getRenderContent();
      REQUIRE(pRenderContent != nullptr);
      const GlobeRectangle& swTightRectangle =
          pRenderContent->getRasterOverlayDetails()
              .boundingRegion.getRectangle();
      CHECK(swTightRectangle.getEast() > swTightRectangle.getWest());
      CHECK(swTightRectangle.getNorth() > swTightRectangle.getSouth());
      CHECK(
          swTightRectangle.computeWidth() * 2.0 <
          tileRectangle.computeWidth() * 0.25);
      CHECK(
          swTightRectangle.computeHeight() * 2.0 <
          tileRectangle.computeHeight() * 0.25);

      // The rectangle used for texture coordinates should also be sensible and
      // match the southwest quadrant of the southeast quadrant of the original
      // parent tile rectangle.
      REQUIRE(!pRenderContent->getRasterOverlayDetails()
                   .rasterOverlayRectangles.empty());
      const Rectangle& swOverlayRectangle =
          pRenderContent->getRasterOverlayDetails()
              .rasterOverlayRectangles.front();
      GlobeRectangle swOverlayGlobeRectangle =
          unprojectRectangleSimple(GeographicProjection(), swOverlayRectangle);
      CHECK(GlobeRectangle::equalsEpsilon(
          swOverlayGlobeRectangle,
          GlobeRectangle(
              seQuadrant.getWest(),
              seQuadrant.getSouth(),
              seQuadrant.computeCenter().longitude,
              seQuadrant.computeCenter().latitude),
          Math::Epsilon13));

      // The tile should have a raster overlay mapped to it.
      REQUIRE(sw.getMappedRasterTiles().size() == 1);
    }
  }

  SUBCASE("Don't generate raster overlay for existing projection") {
    // create gltf grid
    Cartographic beginCarto{glm::radians(32.0), glm::radians(48.0), 100.0};
    CesiumGltf::Model model = createGlobeGrid(beginCarto, 10, 10, 0.01);
    model.extras["gltfUpAxis"] =
        static_cast<std::underlying_type_t<CesiumGeometry::Axis>>(
            CesiumGeometry::Axis::Z);

    // mock raster overlay detail
    GeographicProjection projection(Ellipsoid::WGS84);
    RasterOverlayDetails rasterOverlayDetails;
    rasterOverlayDetails.rasterOverlayProjections.emplace_back(projection);
    rasterOverlayDetails.rasterOverlayRectangles.emplace_back(
        projection.project(GeographicProjection::MAXIMUM_GLOBE_RECTANGLE));
    rasterOverlayDetails.boundingRegion = BoundingRegion{
        GeographicProjection::MAXIMUM_GLOBE_RECTANGLE,
        -1000.0,
        9000.0,
        Ellipsoid::WGS84};

    // add raster overlay
    Tile::LoadedLinkedList loadedTiles;
    RasterOverlayCollection rasterOverlayCollection{loadedTiles, externals};
    rasterOverlayCollection.add(
        new DebugColorizeTilesRasterOverlay("DebugOverlay"));
    asyncSystem.dispatchMainThreadTasks();

    // create mock loader
    auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
    pMockedLoader->mockLoadTileContent = {
        std::move(model),
        CesiumGeometry::Axis::Z,
        std::nullopt,
        std::nullopt,
        std::move(rasterOverlayDetails),
        nullptr,
        nullptr,
        {},
        TileLoadResultState::Success,
        Ellipsoid::WGS84};
    pMockedLoader->mockCreateTileChildren = {{}, TileLoadResultState::Failed};

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());

    // create manager
    IntrusivePointer<TilesetContentManager> pManager =
        new TilesetContentManager{
            externals,
            {},
            std::move(rasterOverlayCollection),
            std::move(pMockedLoader),
            std::move(pRootTile)};

    Tile& tile = *pManager->getRootTile();
    pManager->loadTileContent(tile, {});
    pManager->waitUntilIdle();

    const auto& renderContent = tile.getContent().getRenderContent();
    CHECK(renderContent);

    // Check that manager doesn't generate raster overlay for duplicate
    // projection. Because the mock rasterOverlayDetails uses the same
    // projection as rasterOverlayCollection, the manager should not generate
    // any extra UVs attribute in the post process. Because we never generate
    // _CESIUMOVERLAY_0 to begin with, so this test only successes when no
    // _CESIUMOVERLAY_0 is found. Otherwise, the manager did generates UV using
    // the duplicated projection
    const CesiumGltf::Model& tileModel = renderContent->getModel();
    CHECK(!tileModel.meshes.empty());
    for (const CesiumGltf::Mesh& tileMesh : tileModel.meshes) {
      CHECK(!tileMesh.primitives.empty());
      for (const CesiumGltf::MeshPrimitive& tilePrimitive :
           tileMesh.primitives) {
        CHECK(
            tilePrimitive.attributes.find("_CESIUMOVERLAY_0") ==
            tilePrimitive.attributes.end());
      }
    }

    pManager->unloadTileContent(tile);
  }

  SUBCASE("Resolve external images, with deduplication") {
    std::filesystem::path dirPath(testDataPath / "SharedImages");

    // mock the requests for all files
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
      pMockedAssetAccessor->mockCompletedRequests.insert(
          {entry.path().filename().string(), createMockRequest(entry.path())});
    }

    std::filesystem::path tilesetPath(dirPath / "tileset.json");
    auto pExternals = createMockJsonTilesetExternals(
        tilesetPath.string(),
        pMockedAssetAccessor);

    auto pJsonLoaderFuture =
        TilesetJsonLoader::createLoader(pExternals, tilesetPath.string(), {});

    externals.asyncSystem.dispatchMainThreadTasks();

    auto loaderResult = pJsonLoaderFuture.wait();

    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);

    auto& rootTile = *loaderResult.pRootTile;
    auto& containerTile = rootTile.getChildren()[0];

    REQUIRE(containerTile.getChildren().size() == 100);

    // create manager
    Tile::LoadedLinkedList loadedTiles;
    IntrusivePointer<TilesetContentManager> pManager =
        new TilesetContentManager{
            externals,
            {},
            RasterOverlayCollection{loadedTiles, externals},
            std::move(loaderResult.pLoader),
            std::move(loaderResult.pRootTile)};

    for (auto& child : containerTile.getChildren()) {
      pManager->loadTileContent(child, {});
      externals.asyncSystem.dispatchMainThreadTasks();
      pManager->waitUntilIdle();

      CHECK(child.getState() == TileLoadState::ContentLoaded);
      CHECK(child.isRenderContent());

      const auto& renderContent = child.getContent().getRenderContent();
      const auto& images = renderContent->getModel().images;
      CHECK(images.size() == 1);
    }

    CHECK(
        pManager->getSharedAssetSystem()
            ->pImage->getInactiveAssetTotalSizeBytes() == 0);
    CHECK(pManager->getSharedAssetSystem()->pImage->getAssetCount() == 2);
    CHECK(pManager->getSharedAssetSystem()->pImage->getActiveAssetCount() == 2);
    CHECK(
        pManager->getSharedAssetSystem()->pImage->getInactiveAssetCount() == 0);

    // unload the tile content
    for (auto& child : containerTile.getChildren()) {
      pManager->unloadTileContent(child);
    }

    // Both of the assets will become inactive, and one of them will be
    // destroyed, in order to bring the total under the limit.
    CHECK(
        pManager->getSharedAssetSystem()
            ->pImage->getInactiveAssetTotalSizeBytes() <=
        pManager->getSharedAssetSystem()->pImage->inactiveAssetSizeLimitBytes);
    CHECK(pManager->getSharedAssetSystem()->pImage->getAssetCount() == 1);
    CHECK(pManager->getSharedAssetSystem()->pImage->getActiveAssetCount() == 0);
    CHECK(
        pManager->getSharedAssetSystem()->pImage->getInactiveAssetCount() == 1);
  }
}
