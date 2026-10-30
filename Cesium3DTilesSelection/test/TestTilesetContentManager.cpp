#include "SimplePrepareRendererResource.h"
#include "TestTilesetJsonLoader.h"
#include "TilesetContentManager.h"
#include "TilesetJsonLoader.h"

#include <Cesium3DTilesContent/registerAllTileContentTypes.h>
#include <Cesium3DTilesSelection/GltfModifier.h>
#include <Cesium3DTilesSelection/GltfModifierVersionExtension.h>
#include <Cesium3DTilesSelection/RasterMappedTo3DTile.h>
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
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/AccessorWriter.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Node.h>
#include <CesiumGltf/Scene.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumImage/ImageAsset.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumRasterOverlays/CreateRasterOverlayTileProviderParameters.h>
#include <CesiumRasterOverlays/DebugColorizeTilesRasterOverlay.h>
#include <CesiumRasterOverlays/IPrepareRasterOverlayRendererResources.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Math.h>
#include <CesiumUtility/Uri.h>

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
    IntrusivePointer<TilesetContentManager> pManager =
        TilesetContentManager::createFromUrl(externals, {}, "tileset.json");
    TilesetContentManager& manager = *pManager;
    CHECK(manager.getNumberOfTilesLoading() == 1);

    manager.waitUntilIdle(5000.0);
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
    IntrusivePointer<TilesetContentManager> pManager =
        TilesetContentManager::createFromUrl(externals, {}, "layer.json");
    TilesetContentManager& manager = *pManager;
    CHECK(manager.getNumberOfTilesLoading() == 1);

    manager.waitUntilIdle(5000.0);
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
    IntrusivePointer<TilesetContentManager> pManager =
        TilesetContentManager::createFromUrl(externals, {}, "layer.json");
    TilesetContentManager& manager = *pManager;
    CHECK(manager.getNumberOfTilesLoading() == 1);

    manager.waitUntilIdle(5000.0);
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
        TileID(),
        TileEmptyContent());

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());

    // Give the tile an ID so it is eligible for unloading.
    pRootTile->setTileID("foo");

    // create manager
    TilesetOptions options{};
    options.contentOptions.generateMissingNormalsSmooth = true;

    IntrusivePointer<TilesetContentManager> pManager =
        TilesetContentManager::createFromLoader(
            externals,
            options,
            std::move(pMockedLoader),
            std::move(pRootTile));

    pManager->waitUntilIdle(5000.0);

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
      pManager->waitUntilIdle(5000.0);
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

      pManager->waitUntilIdle(5000.0);
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
        TileID(),
        TileEmptyContent());

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());

    // Give the tile an ID so it is eligible for unloading.
    pRootTile->setTileID("foo");

    // create manager
    TilesetOptions options{};
    options.contentOptions.generateMissingNormalsSmooth = true;

    IntrusivePointer<TilesetContentManager> pManager =
        TilesetContentManager::createFromLoader(
            externals,
            options,
            std::move(pMockedLoader),
            std::move(pRootTile));

    pManager->waitUntilIdle(5000.0);

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
    pManager->waitUntilIdle(5000.0);
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
        TileID(),
        TileEmptyContent());

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());

    // Give the tile an ID so it is eligible for unloading.
    pRootTile->setTileID("foo");

    // create manager
    TilesetOptions options{};
    options.contentOptions.generateMissingNormalsSmooth = true;

    IntrusivePointer<TilesetContentManager> pManager =
        TilesetContentManager::createFromLoader(
            externals,
            options,
            std::move(pMockedLoader),
            std::move(pRootTile));

    pManager->waitUntilIdle(5000.0);

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
    pManager->waitUntilIdle(5000.0);
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

    IntrusivePointer<TilesetContentManager> pManager =
        TilesetContentManager::createFromLoader(
            externals,
            options,
            std::move(pMockedLoader),
            std::move(pRootTile));

    pManager->waitUntilIdle(5000.0);

    Tile& tile = *pManager->getRootTile();
    Tile& upsampledTile = tile.getChildren().back();

    // test manager loading upsample tile
    pManager->loadTileContent(upsampledTile, options);

    // since parent is not yet loaded, it will load the parent first.
    // The upsampled tile will not be loaded at the moment
    CHECK(upsampledTile.getState() == TileLoadState::Unloaded);
    CHECK(tile.getState() == TileLoadState::ContentLoading);

    // parent moves from ContentLoading -> ContentLoaded
    pManager->waitUntilIdle(5000.0);
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
    CHECK(pManager->unloadTileContent(tile) == UnloadTileContentResult::Keep);
    CHECK(tile.getState() == TileLoadState::Unloading);
    CHECK(tile.isRenderContent());

    // Unloading again will have the same result.
    CHECK(pManager->unloadTileContent(tile) == UnloadTileContentResult::Keep);
    CHECK(tile.getState() == TileLoadState::Unloading);
    CHECK(tile.isRenderContent());

    // Attempting to load won't do anything - unloading must finish first.
    pManager->loadTileContent(tile, options);
    CHECK(tile.getState() == TileLoadState::Unloading);

    // upsampled tile: ContentLoading -> ContentLoaded
    pManager->waitUntilIdle(5000.0);
    CHECK(upsampledTile.getState() == TileLoadState::ContentLoaded);
    CHECK(upsampledTile.isRenderContent());

    // trying to unload parent will work now since the upsampled tile is already
    // in the main thread
    CHECK(pManager->unloadTileContent(tile) == UnloadTileContentResult::Remove);
    CHECK(tile.getState() == TileLoadState::Unloaded);
    CHECK(!tile.isRenderContent());
    CHECK(!tile.getContent().getRenderContent());

    // unload upsampled tile: ContentLoaded -> Done
    CHECK(
        pManager->unloadTileContent(upsampledTile) ==
        UnloadTileContentResult::Remove);
    CHECK(upsampledTile.getState() == TileLoadState::Unloaded);
    CHECK(!upsampledTile.isRenderContent());
    CHECK(!upsampledTile.getContent().getRenderContent());
  }

  SUBCASE("Resolves root tile promise for invalid URL") {
    // create manager
    TilesetOptions options{};
    options.contentOptions.generateMissingNormalsSmooth = true;

    // Create an invalid Cesium ion loader that will never attempt to make a
    // request.
    IntrusivePointer<TilesetContentManager> pManager =
        TilesetContentManager::createFromUrl(externals, options, std::string());

    bool eventResolved = false;
    pManager->getRootTileAvailableEvent().thenInMainThread(
        [&eventResolved]() { eventResolved = true; });

    pManager->waitUntilIdle(5000.0);
    CHECK(eventResolved);
    CHECK(!pManager->getRootTile());
  }

  SUBCASE("Resolves root tile promise for invalid loader") {
    // create manager
    TilesetOptions options{};
    options.contentOptions.generateMissingNormalsSmooth = true;

    // Create an invalid Cesium ion loader that will never attempt to make a
    // request.
    IntrusivePointer<TilesetContentManager> pManager =
        TilesetContentManager::createFromCesiumIon(
            externals,
            options,
            0,
            std::string());

    bool eventResolved = false;
    pManager->getRootTileAvailableEvent().thenInMainThread(
        [&eventResolved]() { eventResolved = true; });

    pManager->waitUntilIdle(5000.0);
    CHECK(eventResolved);
    CHECK(!pManager->getRootTile());
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
    std::filesystem::path boxPath = testDataPath / "gltf" / "box";
    std::string fileName = (boxPath / "Box.gltf").string();
    std::vector<std::byte> gltfBoxFile = readFile(fileName);
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

    // add external buffer to the completed request
    std::filesystem::path binPath = boxPath / "Box0.bin";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {Uri::nativePathToUriPath(binPath.string()),
         createMockRequest(binPath)});

    // Load the model and resolve external content
    CesiumGltf::Model tileModel;
    {
      auto future =
          gltfReader
              .readGltfAndExternalData(
                  gltfBoxFile,
                  asyncSystem,
                  CesiumAsync::HttpHeaders(),
                  pMockedAssetAccessor,
                  Uri::nativePathToUriPath(fileName))
              .thenInMainThread(
                  [&tileModel](
                      CesiumGltfReader::GltfReaderResult&& externalReadResult) {
                    CHECK(externalReadResult.errors.empty());
                    CHECK(externalReadResult.warnings.empty());
                    CHECK(externalReadResult.model);
                    tileModel = std::move(*externalReadResult.model);
                    return externalReadResult;
                  });
      future.waitInMainThread();
    }
    auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
    pMockedLoader->mockLoadTileContent = {
        std::move(tileModel),
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

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());

    // Give the tile an ID so it is eligible for unloading.
    pRootTile->setTileID("foo");

    // create manager
    IntrusivePointer<TilesetContentManager> pManager =
        TilesetContentManager::createFromLoader(
            externals,
            {},
            std::move(pMockedLoader),
            std::move(pRootTile));

    pManager->waitUntilIdle(5000.0);

    // test the gltf model
    Tile& tile = *pManager->getRootTile();
    pManager->loadTileContent(tile, {});
    pManager->waitUntilIdle(5000.0);

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

    // Give the tile an ID so it is eligible for unloading.
    pRootTile->setTileID("foo");

    // create manager
    TilesetOptions options;
    options.contentOptions.generateMissingNormalsSmooth = true;

    IntrusivePointer<TilesetContentManager> pManager =
        TilesetContentManager::createFromLoader(
            externals,
            options,
            std::move(pMockedLoader),
            std::move(pRootTile));

    pManager->waitUntilIdle(5000.0);

    // test the gltf model
    Tile& tile = *pManager->getRootTile();
    pManager->loadTileContent(tile, options);
    pManager->waitUntilIdle(5000.0);

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

    // Give the tile an ID so it is eligible for unloading.
    pRootTile->setTileID("foo");

    // create manager
    IntrusivePointer<TilesetContentManager> pManager =
        TilesetContentManager::createFromLoader(
            externals,
            {},
            std::move(pMockedLoader),
            std::move(pRootTile));

    pManager->waitUntilIdle(5000.0);

    Tile& tile = *pManager->getRootTile();
    pManager->loadTileContent(tile, {});
    pManager->waitUntilIdle(5000.0);

    const auto& renderContent = tile.getContent().getRenderContent();
    CHECK(renderContent);

    auto gltfUpAxisIt = renderContent->getModel().extras.find("gltfUpAxis");
    CHECK(gltfUpAxisIt != renderContent->getModel().extras.end());
    CHECK(gltfUpAxisIt->second.getInt64() == 2);

    pManager->unloadTileContent(tile);
  }

  SUBCASE("Generate raster overlay projections") {
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

    // Give the tile an ID so it is eligible for unloading.
    pRootTile->setTileID("foo");

    // create manager
    IntrusivePointer<TilesetContentManager> pManager =
        TilesetContentManager::createFromLoader(
            externals,
            {},
            std::move(pMockedLoader),
            std::move(pRootTile));

    // add raster overlay
    pManager->getRasterOverlayCollection().add(
        new DebugColorizeTilesRasterOverlay("DebugOverlay"));

    pManager->waitUntilIdle(5000.0);

    SUBCASE(
        "Generate raster overlay details when tile doesn't have loose region") {
      // test the gltf model
      Tile& tile = *pManager->getRootTile();
      pManager->loadTileContent(tile, {});
      pManager->waitUntilIdle(5000.0);

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
      pManager->waitUntilIdle(5000.0);

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
      pManager->waitUntilIdle(5000.0);

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
    class AlwaysMoreDetailProvider : public RasterOverlayTileProvider {
    public:
      AlwaysMoreDetailProvider(
          const CesiumUtility::IntrusivePointer<const RasterOverlay>& pCreator,
          const CreateRasterOverlayTileProviderParameters& parameters,
          const CesiumGeospatial::Projection& projection,
          const CesiumGeometry::Rectangle& coverageRectangle)
          : RasterOverlayTileProvider(
                pCreator,
                parameters,
                projection,
                coverageRectangle) {}

      CesiumAsync::Future<LoadedRasterOverlayImage>
      loadTileImage(const RasterOverlayTile& overlayTile) override {
        CesiumUtility::IntrusivePointer<CesiumImage::ImageAsset> pImage;
        CesiumImage::ImageAsset& image = pImage.emplace();
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
          const CreateRasterOverlayTileProviderParameters& parameters)
          const override {
        return parameters.externals.asyncSystem.createResolvedFuture(
            CreateTileProviderResult(
                CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>(
                    new AlwaysMoreDetailProvider(
                        this,
                        parameters,
                        CesiumGeospatial::GeographicProjection(),
                        projectRectangleSimple(
                            CesiumGeospatial::GeographicProjection(),
                            GlobeRectangle::MAXIMUM)))));
      }
    };

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

    // Give the tile an ID so it is eligible for unloading.
    pRootTile->setTileID("foo");

    // create manager
    IntrusivePointer<TilesetContentManager> pManager =
        TilesetContentManager::createFromLoader(
            externals,
            {},
            std::move(pMockedLoader),
            std::move(pRootTile));

    pManager->getRasterOverlayCollection().add(
        new AlwaysMoreDetailRasterOverlay());

    pManager->waitUntilIdle(5000.0);

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

    // Give the tile an ID so it is eligible for unloading.
    pRootTile->setTileID("foo");

    // create manager
    IntrusivePointer<TilesetContentManager> pManager =
        TilesetContentManager::createFromLoader(
            externals,
            {},
            std::move(pMockedLoader),
            std::move(pRootTile));

    // add raster overlay
    pManager->getRasterOverlayCollection().add(
        new DebugColorizeTilesRasterOverlay("DebugOverlay"));

    pManager->waitUntilIdle(5000.0);

    Tile& tile = *pManager->getRootTile();
    pManager->loadTileContent(tile, {});
    pManager->waitUntilIdle(5000.0);

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
    IntrusivePointer<TilesetContentManager> pManager =
        TilesetContentManager::createFromLoader(
            externals,
            {},
            std::move(loaderResult.pLoader),
            std::move(loaderResult.pRootTile));

    pManager->waitUntilIdle(5000.0);

    for (auto& child : containerTile.getChildren()) {
      pManager->loadTileContent(child, {});
      externals.asyncSystem.dispatchMainThreadTasks();
      pManager->waitUntilIdle(5000.0);

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

TEST_CASE("IPrepareRendererResources::prepareInLoadThread parameters") {
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

  SUBCASE("Passes initial bounding volumes correctly") {
    const BoundingVolume boundingVolume =
        BoundingSphere(glm::dvec3(1, 2, 3), 4);
    const BoundingVolume contentBoundingVolume =
        BoundingSphere(glm::dvec3(5, 6, 7), 8);

    pMockedPrepareRendererResources->prepareInLoadThreadTestCallback =
        [](const TileLoadResult& result) {
          REQUIRE(result.initialBoundingVolume);
          REQUIRE(result.initialContentBoundingVolume);

          const BoundingSphere* boundingSphere =
              std::get_if<BoundingSphere>(&*result.initialBoundingVolume);
          const BoundingSphere* contentBoundingSphere =
              std::get_if<BoundingSphere>(
                  &*result.initialContentBoundingVolume);

          REQUIRE(boundingSphere);
          REQUIRE(contentBoundingSphere);

          CHECK(boundingSphere->getCenter() == glm::dvec3(1, 2, 3));
          CHECK(boundingSphere->getRadius() == 4);
          CHECK(contentBoundingSphere->getCenter() == glm::dvec3(5, 6, 7));
          CHECK(contentBoundingSphere->getRadius() == 8);
        };

    // create mock loader
    auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
    pMockedLoader->mockLoadTileContent = TileLoadResult{
        CesiumGltf::Model(),
        CesiumGeometry::Axis::Y,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        nullptr,
        [&](Tile&) {},
        TileLoadResultState::Success,
        Ellipsoid::WGS84,
        boundingVolume,
        contentBoundingVolume};
    pMockedLoader->mockCreateTileChildren = {{}, TileLoadResultState::Success};
    pMockedLoader->mockCreateTileChildren.children.emplace_back(
        pMockedLoader.get(),
        TileID(),
        TileEmptyContent());

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());
    pRootTile->setBoundingVolume(boundingVolume);
    pRootTile->setContentBoundingVolume(contentBoundingVolume);

    // Give the tile an ID so it is eligible for unloading.
    pRootTile->setTileID("foo");

    // create manager
    TilesetOptions options{};
    options.contentOptions.generateMissingNormalsSmooth = true;

    IntrusivePointer<TilesetContentManager> pManager =
        TilesetContentManager::createFromLoader(
            externals,
            options,
            std::move(pMockedLoader),
            std::move(pRootTile));

    pManager->waitUntilIdle(5000.0);

    Tile& tile = *pManager->getRootTile();
    pManager->loadTileContent(tile, options);
    pManager->waitUntilIdle(5000.0);
    pManager->unloadTileContent(tile);
  }
}

TEST_CASE("Test glTF modifier state machine") {
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

  class SimpleGltfModifier : public GltfModifier {
  public:
    SimpleGltfModifier() {}

    int applyCallCount = 0;
    CesiumAsync::Future<std::optional<GltfModifierOutput>>
    apply(GltfModifierInput&& input) override {
      ++applyCallCount;
      GltfModifierOutput output{.modifiedModel = input.previousModel};
      return input.asyncSystem.createResolvedFuture(
          std::make_optional(std::move(output)));
    }

    int onRegisterCallCount = 0;
    CesiumAsync::Future<void> onRegister(
        const CesiumAsync::AsyncSystem& asyncSystem,
        const std::shared_ptr<CesiumAsync::IAssetAccessor>&,
        const std::shared_ptr<spdlog::logger>&,
        const TilesetMetadata&,
        const Tile&) override {
      ++onRegisterCallCount;
      return asyncSystem.createResolvedFuture();
    }
  };

  auto pGltfModifier = std::make_shared<SimpleGltfModifier>();
  externals.pGltfModifier = pGltfModifier;

  auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
  pMockedLoader->mockLoadTileContent = {
      CesiumGltf::Model(),
      CesiumGeometry::Axis::Y,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      nullptr,
      nullptr,
      {},
      TileLoadResultState::Success};

  // create tile
  auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());

  // Give the tile an ID so it is eligible for unloading.
  pRootTile->setTileID("foo");

  // create manager
  TilesetOptions options{};
  options.contentOptions.generateMissingNormalsSmooth = true;

  IntrusivePointer<TilesetContentManager> pManager =
      TilesetContentManager::createFromLoader(
          externals,
          options,
          std::move(pMockedLoader),
          std::move(pRootTile));

  pManager->waitUntilIdle(5000.0);

  CHECK(pGltfModifier->onRegisterCallCount == 1);

  // test manager loading
  Tile& tile = *pManager->getRootTile();
  pManager->loadTileContent(tile, options);
  pManager->waitUntilIdle(5000.0);
  pManager->updateTileContent(tile, options);
  CHECK(tile.getState() == TileLoadState::Done);
  CHECK(tile.getContent().isRenderContent());
  // Constructed modifier is nilpotent until the first call to trigger(), so:
  CHECK(pGltfModifier->applyCallCount == 0);
  CHECK(pMockedPrepareRendererResources->totalAllocation == 1);

  int expectedCallCount = 1;
  auto const& applyModifier = [&]() {
    pGltfModifier->trigger();
    CHECK(tile.needsWorkerThreadLoading(pGltfModifier.get()));
    // Start worker-thread phase of glTF modifier.
    pManager->loadTileContent(tile, options);
    // Unloading should be refused while worker-thread is running.
    CHECK(pManager->unloadTileContent(tile) == UnloadTileContentResult::Keep);
    // Wait completion of worker-thread phase.
    pManager->waitUntilIdle(5000.0);
    CHECK(!tile.needsWorkerThreadLoading(pGltfModifier.get()));
    CHECK(tile.needsMainThreadLoading(pGltfModifier.get()));
    CHECK(pGltfModifier->applyCallCount == expectedCallCount);
    // The temporary renderer resource should have been created.
    CHECK(pMockedPrepareRendererResources->totalAllocation == 2);

    SUBCASE("Perform main-thread phase of glTF modifier") {
      pManager->finishLoading(tile, options);
      CHECK(!tile.needsWorkerThreadLoading(pGltfModifier.get()));
      CHECK(!tile.needsMainThreadLoading(pGltfModifier.get()));
      // The temporary renderer resource should have been freed.
      CHECK(pGltfModifier->applyCallCount == expectedCallCount);
      CHECK(pMockedPrepareRendererResources->totalAllocation == 1);
    }
  };

  // Increment modifier version, thus requiring a first glTF modifier of the
  // already loaded tile
  applyModifier();

  // Unload tile so that we can now test loading the tile with an _active_
  // modifier
  CHECK(pManager->unloadTileContent(tile));
  CHECK(pMockedPrepareRendererResources->totalAllocation == 0);

  ++expectedCallCount;
  // loaded tile below will be already processed by the glTF modifier
  pManager->loadTileContent(tile, options);
  pManager->waitUntilIdle(5000.0);
  pManager->updateTileContent(tile, options);
  CHECK(tile.getState() == TileLoadState::Done);
  CHECK(tile.getContent().isRenderContent());
  // After the tile is loaded, glTF modifier should not be needed,
  // as it has already been done as part of the loading.
  CHECK(!tile.needsWorkerThreadLoading(pGltfModifier.get()));
  CHECK(!tile.needsMainThreadLoading(pGltfModifier.get()));
  CHECK(pGltfModifier->applyCallCount == expectedCallCount);
  CHECK(pMockedPrepareRendererResources->totalAllocation == 1);

  ++expectedCallCount;
  // Increment modifier version, thus requiring a new glTF modifier.
  applyModifier();

  SUBCASE("Unload tile after main-thread phase of glTF modifier") {
    CHECK(pManager->unloadTileContent(tile));
    CHECK(pMockedPrepareRendererResources->totalAllocation == 0);
  }
}

TEST_CASE("Test glTF modifier reapplies raster overlay attributes") {
  Cesium3DTilesContent::registerAllTileContentTypes();

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

  class RemoveRasterOverlayAttributeModifier : public GltfModifier {
  public:
    CesiumAsync::Future<std::optional<GltfModifierOutput>>
    apply(GltfModifierInput&& input) override {
      GltfModifierOutput output{.modifiedModel = input.previousModel};
      for (CesiumGltf::Mesh& mesh : output.modifiedModel.meshes) {
        for (CesiumGltf::MeshPrimitive& primitive : mesh.primitives) {
          primitive.attributes.erase("_CESIUMOVERLAY_0");
        }
      }

      return input.asyncSystem.createResolvedFuture(
          std::make_optional(std::move(output)));
    }

  protected:
    CesiumAsync::Future<void> onRegister(
        const CesiumAsync::AsyncSystem& asyncSystem,
        const std::shared_ptr<CesiumAsync::IAssetAccessor>&,
        const std::shared_ptr<spdlog::logger>&,
        const TilesetMetadata&,
        const Tile&) override {
      return asyncSystem.createResolvedFuture();
    }
  };

  auto pGltfModifier = std::make_shared<RemoveRasterOverlayAttributeModifier>();
  externals.pGltfModifier = pGltfModifier;

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

  auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());
  pRootTile->setTileID("foo");

  TilesetOptions options{};
  IntrusivePointer<TilesetContentManager> pManager =
      TilesetContentManager::createFromLoader(
          externals,
          options,
          std::move(pMockedLoader),
          std::move(pRootTile));
  pManager->getRasterOverlayCollection().add(
      new DebugColorizeTilesRasterOverlay("DebugOverlay"));
  pManager->waitUntilIdle(5000.0);

  Tile& tile = *pManager->getRootTile();
  pManager->loadTileContent(tile, options);
  pManager->waitUntilIdle(5000.0);
  pManager->updateTileContent(tile, options);
  REQUIRE(tile.getState() == TileLoadState::Done);

  const auto* pRenderContent = tile.getContent().getRenderContent();
  REQUIRE(pRenderContent != nullptr);
  const auto& originalPrimitive =
      pRenderContent->getModel().meshes.front().primitives.front();
  REQUIRE(
      originalPrimitive.attributes.find("_CESIUMOVERLAY_0") !=
      originalPrimitive.attributes.end());

  pGltfModifier->trigger();
  REQUIRE(tile.needsWorkerThreadLoading(pGltfModifier.get()));
  pManager->loadTileContent(tile, options);
  pManager->waitUntilIdle(5000.0);
  REQUIRE(tile.needsMainThreadLoading(pGltfModifier.get()));
  pManager->finishLoading(tile, options);

  REQUIRE(tile.getState() == TileLoadState::Done);
  pRenderContent = tile.getContent().getRenderContent();
  REQUIRE(pRenderContent != nullptr);
  REQUIRE(!pRenderContent->getRasterOverlayDetails()
               .rasterOverlayProjections.empty());
  REQUIRE(!pRenderContent->getRasterOverlayDetails()
               .rasterOverlayRectangles.empty());

  const auto& modifiedPrimitive =
      pRenderContent->getModel().meshes.front().primitives.front();
  auto overlayAttribute = modifiedPrimitive.attributes.find("_CESIUMOVERLAY_0");
  REQUIRE(overlayAttribute != modifiedPrimitive.attributes.end());
  CesiumGltf::AccessorView<glm::vec2> uv{
      pRenderContent->getModel(),
      overlayAttribute->second};
  CHECK(uv.status() == CesiumGltf::AccessorViewStatus::Valid);
}

TEST_CASE("Test glTF modifier preserves raster overlay mappings") {
  class ReplacementModifier : public GltfModifier {
  public:
    std::optional<CesiumGltf::Model> replacement;

    CesiumAsync::Future<std::optional<GltfModifierOutput>>
    apply(GltfModifierInput&& input) override {
      CesiumGltf::Model model =
          replacement ? *replacement : input.previousModel;
      model.extras["gltfUpAxis"] = static_cast<int>(Axis::Z);
      for (auto& mesh : model.meshes) {
        for (auto& primitive : mesh.primitives) {
          std::erase_if(primitive.attributes, [](const auto& attribute) {
            return attribute.first.starts_with("_CESIUMOVERLAY_");
          });
        }
      }
      return input.asyncSystem.createResolvedFuture(
          std::make_optional(GltfModifierOutput{std::move(model)}));
    }

  protected:
    CesiumAsync::Future<void> onRegister(
        const CesiumAsync::AsyncSystem& asyncSystem,
        const std::shared_ptr<CesiumAsync::IAssetAccessor>&,
        const std::shared_ptr<spdlog::logger>&,
        const TilesetMetadata&,
        const Tile&) override {
      return asyncSystem.createResolvedFuture();
    }
  };

  class SolidOverlay : public RasterOverlay {
  public:
    explicit SolidOverlay(const Projection& projection)
        : RasterOverlay("Solid"), _projection(projection) {}

    CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
        const CreateRasterOverlayTileProviderParameters& parameters)
        const override {
      class Provider : public RasterOverlayTileProvider {
      public:
        Provider(
            const IntrusivePointer<const RasterOverlay>& pOwner,
            const CreateRasterOverlayTileProviderParameters& parameters,
            const Projection& projection)
            : RasterOverlayTileProvider(
                  pOwner,
                  parameters,
                  projection,
                  projectRectangleSimple(
                      projection,
                      GlobeRectangle::fromDegrees(-180, -80, 180, 80))) {}

        CesiumAsync::Future<LoadedRasterOverlayImage>
        loadTileImage(const RasterOverlayTile& tile) override {
          LoadedRasterOverlayImage image;
          auto& asset = image.pImage.emplace();
          asset.width = 1;
          asset.height = 1;
          asset.channels = 4;
          asset.bytesPerChannel = 1;
          asset.pixelData.resize(4, std::byte(255));
          image.rectangle = tile.getRectangle();
          image.moreDetailAvailable = false;
          return this->getAsyncSystem().createResolvedFuture(std::move(image));
        }
      };

      return parameters.externals.asyncSystem
          .createResolvedFuture<CreateTileProviderResult>(
              IntrusivePointer<RasterOverlayTileProvider>(
                  new Provider(this, parameters, this->_projection)));
    }

  private:
    Projection _projection;
  };

  class RecordingRenderer : public SimplePrepareRendererResource {
  public:
    struct Attachment {
      size_t generation;
      int32_t coordinateID;
      glm::dvec2 translation;
      glm::dvec2 scale;
    };

    size_t generation = 0;
    std::vector<Attachment> attachments;

    void* prepareInMainThread(Tile& tile, void* pResult) override {
      ++generation;
      return SimplePrepareRendererResource::prepareInMainThread(tile, pResult);
    }

    void attachRasterInMainThread(
        const Tile& tile,
        int32_t coordinateID,
        const RasterOverlayTile& raster,
        void* pResources,
        const glm::dvec2& translation,
        const glm::dvec2& scale) override {
      attachments.push_back({generation, coordinateID, translation, scale});
      SimplePrepareRendererResource::attachRasterInMainThread(
          tile,
          coordinateID,
          raster,
          pResources,
          translation,
          scale);
    }
  };

  Cesium3DTilesContent::registerAllTileContentTypes();
  auto pRenderer = std::make_shared<RecordingRenderer>();
  auto pModifier = std::make_shared<ReplacementModifier>();
  TilesetExternals externals{
      std::make_shared<SimpleAssetAccessor>(
          std::map<std::string, std::shared_ptr<SimpleAssetRequest>>{}),
      pRenderer,
      CesiumAsync::AsyncSystem{std::make_shared<SimpleTaskProcessor>()},
      std::make_shared<CreditSystem>()};
  externals.pGltfModifier = pModifier;

  const Cartographic start{glm::radians(32.0), glm::radians(48.0), 100.0};
  auto pLoader = std::make_unique<SimpleTilesetContentLoader>();
  auto* pLoaderRaw = pLoader.get();
  const auto resetLoadResult = [&]() {
    pLoaderRaw->mockLoadTileContent = {
        createGlobeGrid(start, 3, 3, 0.01),
        Axis::Z,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        nullptr,
        {},
        TileLoadResultState::Success,
        Ellipsoid::WGS84};
    pLoaderRaw->mockCreateTileChildren = {{}, TileLoadResultState::Failed};
  };
  resetLoadResult();
  auto pRoot = std::make_unique<Tile>(pLoader.get());
  pRoot->setTileID("reloadable");
  pRoot->setBoundingVolume(BoundingSphere(
      Ellipsoid::WGS84.cartographicToCartesian(start),
      1000000.0));
  TilesetOptions options;
  auto pManager = TilesetContentManager::createFromLoader(
      externals,
      options,
      std::move(pLoader),
      std::move(pRoot));
  auto& overlays = pManager->getRasterOverlayCollection();
  IntrusivePointer<const RasterOverlay> geographic =
      new SolidOverlay(GeographicProjection());
  IntrusivePointer<const RasterOverlay> mercator =
      new SolidOverlay(WebMercatorProjection());
  overlays.add(geographic);
  overlays.add(mercator);
  REQUIRE(pManager->waitUntilIdle(5000.0));
  Tile& tile = *pManager->getRootTile();
  pManager->loadTileContent(tile, options);
  REQUIRE(pManager->waitUntilIdle(5000.0));

  const auto allAttached = [&]() {
    return !tile.getMappedRasterTiles().empty() &&
           std::all_of(
               tile.getMappedRasterTiles().begin(),
               tile.getMappedRasterTiles().end(),
               [](const auto& mapped) {
                 return mapped.getState() ==
                        RasterMappedTo3DTile::AttachmentState::Attached;
               });
  };
  const auto attachOverlays = [&]() {
    for (int i = 0; i < 10 && !allAttached(); ++i) {
      pManager->updateTileContent(tile, options);
      pManager->loadTileContent(tile, options);
      REQUIRE(pManager->waitUntilIdle(5000.0));
    }
    REQUIRE(tile.getState() == TileLoadState::Done);
    REQUIRE(allAttached());
  };
  size_t attachedDuringFinish = 0;
  const auto reapply = [&]() {
    const RasterOverlayDetails previousDetails =
        tile.getContent().getRenderContent()->getRasterOverlayDetails();
    std::vector<std::pair<const RasterOverlayTile*, int32_t>> previousMappings;
    for (const auto& mapped : tile.getMappedRasterTiles()) {
      previousMappings.emplace_back(
          mapped.getReadyTile(),
          mapped.getTextureCoordinateID());
    }
    const size_t detachCount = pRenderer->rasterDetachCount;
    pModifier->trigger();
    REQUIRE(tile.needsWorkerThreadLoading(pModifier.get()));
    pManager->loadTileContent(tile, options);
    REQUIRE(pManager->waitUntilIdle(5000.0));
    REQUIRE(tile.needsMainThreadLoading(pModifier.get()));
    CHECK(pRenderer->rasterDetachCount == detachCount);
    REQUIRE(tile.getMappedRasterTiles().size() == previousMappings.size());
    for (size_t i = 0; i < previousMappings.size(); ++i) {
      CHECK(
          tile.getMappedRasterTiles()[i].getReadyTile() ==
          previousMappings[i].first);
      CHECK(
          tile.getMappedRasterTiles()[i].getTextureCoordinateID() ==
          previousMappings[i].second);
    }
    CHECK(GlobeRectangle::equalsEpsilon(
        tile.getContent()
            .getRenderContent()
            ->getRasterOverlayDetails()
            .boundingRegion.getRectangle(),
        previousDetails.boundingRegion.getRectangle(),
        1e-8));
    pManager->finishLoading(tile, options);
    attachedDuringFinish = static_cast<size_t>(std::count_if(
        pRenderer->attachments.begin(),
        pRenderer->attachments.end(),
        [&](const auto& attachment) {
          return attachment.generation == pRenderer->generation;
        }));
    pManager->updateTileContent(tile, options);
  };
  const auto checkUVs = [&](int32_t id,
                            const Projection& projection,
                            const Cartographic& origin,
                            double dimension) {
    const auto& model = tile.getContent().getRenderContent()->getModel();
    REQUIRE(!model.meshes.empty());
    const auto& attributes = model.meshes.front().primitives.front().attributes;
    const auto it = attributes.find("_CESIUMOVERLAY_" + std::to_string(id));
    REQUIRE(it != attributes.end());
    CesiumGltf::AccessorView<glm::vec2> uv(model, it->second);
    REQUIRE(uv.status() == CesiumGltf::AccessorViewStatus::Valid);
    REQUIRE(uv.size() == 9);
    const auto southwest = projectPosition(projection, origin);
    const auto northeast = projectPosition(
        projection,
        Cartographic(
            origin.longitude + dimension * 2.0,
            origin.latitude + dimension * 2.0,
            origin.height));
    const auto center = projectPosition(
        projection,
        Cartographic(
            origin.longitude + dimension,
            origin.latitude + dimension,
            origin.height));
    CHECK(uv[0].x == Approx(0.0).epsilon(1e-5));
    CHECK(uv[0].y == Approx(0.0).epsilon(1e-5));
    CHECK(
        uv[4].x ==
        Approx((center.x - southwest.x) / (northeast.x - southwest.x))
            .epsilon(1e-5));
    CHECK(
        uv[4].y ==
        Approx((center.y - southwest.y) / (northeast.y - southwest.y))
            .epsilon(1e-5));
    CHECK(uv[8].x == Approx(1.0).epsilon(1e-5));
    CHECK(uv[8].y == Approx(1.0).epsilon(1e-5));
  };

  SUBCASE(
      "Attached imagery is attached to each replacement renderer resource") {
    attachOverlays();
    for (int cycle = 0; cycle < 2; ++cycle) {
      const auto* pFirstReady = tile.getMappedRasterTiles()[0].getReadyTile();
      const auto* pSecondReady = tile.getMappedRasterTiles()[1].getReadyTile();
      const size_t previousGeneration = pRenderer->generation;
      pRenderer->attachments.clear();
      reapply();
      CHECK(tile.getState() == TileLoadState::Done);
      REQUIRE(tile.getMappedRasterTiles().size() == 2);
      CHECK(tile.getMappedRasterTiles()[0].getReadyTile() == pFirstReady);
      CHECK(tile.getMappedRasterTiles()[1].getReadyTile() == pSecondReady);
      CHECK(tile.getMappedRasterTiles()[0].getTextureCoordinateID() == 0);
      CHECK(tile.getMappedRasterTiles()[1].getTextureCoordinateID() == 1);
      CHECK(pRenderer->generation == previousGeneration + 1);
      CHECK(attachedDuringFinish == 2);
      CHECK(pRenderer->attachments.size() == 2);
      for (const auto& attachment : pRenderer->attachments) {
        CHECK(attachment.generation == pRenderer->generation);
      }
      checkUVs(0, GeographicProjection(), start, 0.01);
      checkUVs(1, WebMercatorProjection(), start, 0.01);
    }
  }

  SUBCASE("Imagery covering a smaller extent is reattached immediately") {
    attachOverlays();
    const auto* pFirstReady = tile.getMappedRasterTiles()[0].getReadyTile();
    const auto* pSecondReady = tile.getMappedRasterTiles()[1].getReadyTile();
    const Cartographic innerStart{
        start.longitude + 0.005,
        start.latitude + 0.005,
        start.height};
    pModifier->replacement = createGlobeGrid(innerStart, 3, 3, 0.004);
    pRenderer->attachments.clear();
    reapply();
    CHECK(attachedDuringFinish == 2);
    REQUIRE(tile.getMappedRasterTiles().size() == 2);
    CHECK(tile.getMappedRasterTiles()[0].getReadyTile() == pFirstReady);
    CHECK(tile.getMappedRasterTiles()[1].getReadyTile() == pSecondReady);
    CHECK(allAttached());
    checkUVs(0, GeographicProjection(), innerStart, 0.004);
    checkUVs(1, WebMercatorProjection(), innerStart, 0.004);

    const auto& details =
        tile.getContent().getRenderContent()->getRasterOverlayDetails();
    REQUIRE(pRenderer->attachments.size() == 2);
    for (const auto& attachment : pRenderer->attachments) {
      CHECK(attachment.generation == pRenderer->generation);
      const size_t i = static_cast<size_t>(attachment.coordinateID);
      REQUIRE(i < details.rasterOverlayRectangles.size());
      const Rectangle& geometry = details.rasterOverlayRectangles[i];
      const Rectangle& image =
          tile.getMappedRasterTiles()[i].getReadyTile()->getRectangle();
      CHECK(
          attachment.translation.x ==
          Approx((geometry.minimumX - image.minimumX) / image.computeWidth()));
      CHECK(
          attachment.translation.y ==
          Approx((geometry.minimumY - image.minimumY) / image.computeHeight()));
      CHECK(
          attachment.scale.x ==
          Approx(geometry.computeWidth() / image.computeWidth()));
      CHECK(
          attachment.scale.y ==
          Approx(geometry.computeHeight() / image.computeHeight()));
      CHECK(attachment.scale.x < 1.0);
      CHECK(attachment.scale.y < 1.0);
    }
  }

  SUBCASE("Removed overlays do not renumber remaining projection coordinates") {
    attachOverlays();
    overlays.remove(geographic);
    REQUIRE(tile.getMappedRasterTiles().size() == 1);
    REQUIRE(tile.getMappedRasterTiles()[0].getTextureCoordinateID() == 1);
    reapply();
    REQUIRE(tile.getMappedRasterTiles().size() == 1);
    CHECK(tile.getMappedRasterTiles()[0].getTextureCoordinateID() == 1);
    checkUVs(1, WebMercatorProjection(), start, 0.01);
  }

  SUBCASE("A newly added projection uses the normal reload mechanism") {
    attachOverlays();
    overlays.remove(mercator);
    REQUIRE(pManager->unloadTileContent(tile));
    resetLoadResult();
    pManager->loadTileContent(tile, options);
    REQUIRE(pManager->waitUntilIdle(5000.0));
    attachOverlays();
    REQUIRE(
        tile.getContent()
            .getRenderContent()
            ->getRasterOverlayDetails()
            .rasterOverlayProjections.size() == 1);
    overlays.add(mercator);
    REQUIRE(pManager->waitUntilIdle(5000.0));
    reapply();
    CHECK(tile.getState() == TileLoadState::Unloaded);
    resetLoadResult();
    pManager->loadTileContent(tile, options);
    REQUIRE(pManager->waitUntilIdle(5000.0));
    attachOverlays();
    REQUIRE(tile.getMappedRasterTiles().size() == 2);
    checkUVs(0, GeographicProjection(), start, 0.01);
    checkUVs(1, WebMercatorProjection(), start, 0.01);
  }

  SUBCASE("Changed and empty models publish their new overlay bounds") {
    bool contentLoaded = false;
    bool empty = false;
    SUBCASE("Changed extent on an already rendered tile") {}
    SUBCASE("Changed extent before main-thread preparation") {
      contentLoaded = true;
    }
    SUBCASE("Empty model on an already rendered tile") { empty = true; }
    SUBCASE("Empty model before main-thread preparation") {
      contentLoaded = true;
      empty = true;
    }
    if (contentLoaded) {
      REQUIRE(tile.getState() == TileLoadState::ContentLoaded);
    } else {
      attachOverlays();
    }
    const Cartographic movedStart{
        start.longitude + 0.03,
        start.latitude + 0.04,
        200.0};
    pModifier->replacement =
        empty ? CesiumGltf::Model() : createGlobeGrid(movedStart, 3, 3, 0.02);

    pRenderer->attachments.clear();
    pModifier->trigger();
    pManager->loadTileContent(tile, options);
    REQUIRE(pManager->waitUntilIdle(5000.0));
    if (contentLoaded) {
      pManager->updateTileContent(tile, options);
    } else {
      pManager->finishLoading(tile, options);
      pManager->updateTileContent(tile, options);
    }
    REQUIRE(tile.getState() == TileLoadState::Done);
    if (!empty) {
      attachOverlays();
    }
    const auto& details =
        tile.getContent().getRenderContent()->getRasterOverlayDetails();
    CHECK(details.boundingRegion.getRectangle().isEmpty() == empty);
    if (!empty) {
      CHECK(GlobeRectangle::equalsEpsilon(
          details.boundingRegion.getRectangle(),
          GlobeRectangle(
              movedStart.longitude,
              movedStart.latitude,
              movedStart.longitude + 0.04,
              movedStart.latitude + 0.04),
          1e-8));
      REQUIRE(details.rasterOverlayRectangles.size() == 2);
      checkUVs(0, GeographicProjection(), movedStart, 0.02);
      checkUVs(1, WebMercatorProjection(), movedStart, 0.02);
      CHECK(pRenderer->attachments.size() == 2);
      for (size_t i = 0; i < details.rasterOverlayProjections.size(); ++i) {
        const auto expected = projectRectangleSimple(
            details.rasterOverlayProjections[i],
            GlobeRectangle(
                movedStart.longitude,
                movedStart.latitude,
                movedStart.longitude + 0.04,
                movedStart.latitude + 0.04));
        const auto& actual = details.rasterOverlayRectangles[i];
        CHECK(actual.minimumX == Approx(expected.minimumX));
        CHECK(actual.minimumY == Approx(expected.minimumY));
        CHECK(actual.maximumX == Approx(expected.maximumX));
        CHECK(actual.maximumY == Approx(expected.maximumY));
        const auto attachment = std::find_if(
            pRenderer->attachments.begin(),
            pRenderer->attachments.end(),
            [&](const auto& candidate) {
              return candidate.generation == pRenderer->generation &&
                     candidate.coordinateID == static_cast<int32_t>(i);
            });
        if (attachment != pRenderer->attachments.end()) {
          const auto* pRaster = tile.getMappedRasterTiles()[i].getReadyTile();
          REQUIRE(pRaster != nullptr);
          const Rectangle& image = pRaster->getRectangle();
          CHECK(
              attachment->translation.x ==
              Approx(
                  (expected.minimumX - image.minimumX) / image.computeWidth()));
          CHECK(
              attachment->translation.y ==
              Approx(
                  (expected.minimumY - image.minimumY) /
                  image.computeHeight()));
          CHECK(
              attachment->scale.x ==
              Approx(expected.computeWidth() / image.computeWidth()));
          CHECK(
              attachment->scale.y ==
              Approx(expected.computeHeight() / image.computeHeight()));
        } else {
          FAIL_CHECK(
              "No imagery attachment to the replacement renderer resource");
        }
      }
    } else {
      CHECK(tile.getContent().getRenderContent()->getModel().meshes.empty());
      CHECK(tile.getMappedRasterTiles().empty());
      CHECK(
          details.boundingRegion.getMinimumHeight() >
          details.boundingRegion.getMaximumHeight());
      for (int i = 0; i < 3; ++i) {
        pManager->updateTileContent(tile, options);
        CHECK(tile.getState() == TileLoadState::Done);
        CHECK(tile.getChildren().empty());
        CHECK(tile.getMappedRasterTiles().empty());
      }
      pModifier->replacement = createGlobeGrid(start, 3, 3, 0.01);
      reapply();
      CHECK(!tile.getContent()
                 .getRenderContent()
                 ->getRasterOverlayDetails()
                 .boundingRegion.getRectangle()
                 .isEmpty());
      checkUVs(0, GeographicProjection(), start, 0.01);
      CHECK(tile.getMappedRasterTiles().size() == 2);
      attachOverlays();
      CHECK(tile.getMappedRasterTiles().size() == 2);
    }
  }
}
