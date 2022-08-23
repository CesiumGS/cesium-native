#include "SimpleAssetAccessor.h"
#include "SimpleAssetRequest.h"
#include "SimpleAssetResponse.h"
#include "SimplePrepareRendererResource.h"
#include "SimpleTaskProcessor.h"
#include "TilesetContentManager.h"
#include "readFile.h"

#include <Cesium3DTilesSelection/DebugColorizeTilesRasterOverlay.h>
#include <Cesium3DTilesSelection/GltfUtilities.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/registerAllTileContentTypes.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/Math.h>

#include <catch2/catch.hpp>
#include <glm/glm.hpp>

#include <filesystem>
#include <vector>

using namespace Cesium3DTilesSelection;
using namespace CesiumGeospatial;
using namespace CesiumGeometry;

namespace {
std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;

class SimpleTilesetContentLoader : public TilesetContentLoader {
public:
  CesiumAsync::Future<TileLoadResult>
  loadTileContent(const TileLoadInput& input) override {
    return input.asyncSystem.createResolvedFuture(
        std::move(mockLoadTileContent));
  }

  TileChildrenResult
  createTileChildren([[maybe_unused]] const Tile& tile) override {
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
  indices.reserve(6 * (width - 1) * (height - 1));
  positions.reserve(width * height);
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
} // namespace

TEST_CASE("Test tile state machine") {
  Cesium3DTilesSelection::registerAllTileContentTypes();

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

  SECTION("Load content successfully") {
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
        [&](Tile&) { initializerCall = true; },
        TileLoadResultState::Success};
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
    TilesetContentManager manager{
        externals,
        options,
        RasterOverlayCollection{loadedTiles, externals},
        {},
        std::move(pMockedLoader),
        std::move(pRootTile)};

    // test manager loading
    Tile& tile = *manager.getRootTile();
    manager.loadTileContent(tile, options);

    SECTION("Load tile from ContentLoading -> Done") {
      // Unloaded -> ContentLoading
      // check the state of the tile before main thread get called
      CHECK(manager.getNumberOfTilesLoading() == 1);
      CHECK(tile.getState() == TileLoadState::ContentLoading);
      CHECK(tile.getContent().isUnknownContent());
      CHECK(!tile.getContent().isRenderContent());
      CHECK(!tile.getContent().isExternalContent());
      CHECK(!tile.getContent().isEmptyContent());
      CHECK(!tile.getContent().getRenderContent());
      CHECK(!initializerCall);

      // ContentLoading -> ContentLoaded
      // check the state of the tile after main thread get called
      manager.waitUntilIdle();
      CHECK(manager.getNumberOfTilesLoading() == 0);
      CHECK(tile.getState() == TileLoadState::ContentLoaded);
      CHECK(tile.getContent().isRenderContent());
      CHECK(tile.getContent().getRenderContent()->getRenderResources());
      CHECK(initializerCall);

      // ContentLoaded -> Done
      // update tile content to move from ContentLoaded -> Done
      manager.updateTileContent(tile, options);
      CHECK(tile.getState() == TileLoadState::Done);
      CHECK(tile.getChildren().size() == 1);
      CHECK(tile.getChildren().front().getContent().isEmptyContent());
      CHECK(tile.getContent().isRenderContent());
      CHECK(tile.getContent().getRenderContent()->getRenderResources());
      CHECK(initializerCall);

      // Done -> Unloaded
      manager.unloadTileContent(tile);
      CHECK(tile.getState() == TileLoadState::Unloaded);
      CHECK(tile.getContent().isUnknownContent());
      CHECK(!tile.getContent().isRenderContent());
      CHECK(!tile.getContent().getRenderContent());
    }

    SECTION("Try to unload tile when it's still loading") {
      // unload tile to move from Done -> Unload
      manager.unloadTileContent(tile);
      CHECK(manager.getNumberOfTilesLoading() == 1);
      CHECK(tile.getState() == TileLoadState::ContentLoading);
      CHECK(tile.getContent().isUnknownContent());
      CHECK(!tile.getContent().isRenderContent());
      CHECK(!tile.getContent().isExternalContent());
      CHECK(!tile.getContent().isEmptyContent());
      CHECK(!tile.getContent().getRenderContent());

      manager.waitUntilIdle();
      CHECK(manager.getNumberOfTilesLoading() == 0);
      CHECK(tile.getState() == TileLoadState::ContentLoaded);
      CHECK(tile.getContent().isRenderContent());
      CHECK(tile.getContent().getRenderContent()->getRenderResources());

      manager.unloadTileContent(tile);
      CHECK(manager.getNumberOfTilesLoading() == 0);
      CHECK(tile.getState() == TileLoadState::Unloaded);
      CHECK(tile.getContent().isUnknownContent());
      CHECK(!tile.getContent().isRenderContent());
      CHECK(!tile.getContent().getRenderContent());
    }
  }

  SECTION("Loader requests retry later") {
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
        [&](Tile&) { initializerCall = true; },
        TileLoadResultState::RetryLater};
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
    TilesetContentManager manager{
        externals,
        options,
        RasterOverlayCollection{loadedTiles, externals},
        {},
        std::move(pMockedLoader),
        std::move(pRootTile)};

    // test manager loading
    Tile& tile = *manager.getRootTile();
    manager.loadTileContent(tile, options);

    // Unloaded -> ContentLoading
    CHECK(manager.getNumberOfTilesLoading() == 1);
    CHECK(tile.getState() == TileLoadState::ContentLoading);
    CHECK(tile.getChildren().empty());
    CHECK(tile.getContent().isUnknownContent());
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().getRenderContent());

    // ContentLoading -> FailedTemporarily
    manager.waitUntilIdle();
    CHECK(manager.getNumberOfTilesLoading() == 0);
    CHECK(tile.getChildren().empty());
    CHECK(tile.getState() == TileLoadState::FailedTemporarily);
    CHECK(tile.getContent().isUnknownContent());
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().getRenderContent());
    CHECK(!initializerCall);

    // FailedTemporarily -> FailedTemporarily
    // tile is failed temporarily but the loader can still add children to it
    manager.updateTileContent(tile, options);
    CHECK(manager.getNumberOfTilesLoading() == 0);
    CHECK(tile.getChildren().size() == 1);
    CHECK(tile.getChildren().front().isEmptyContent());
    CHECK(tile.getState() == TileLoadState::FailedTemporarily);
    CHECK(tile.getContent().isUnknownContent());
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().getRenderContent());
    CHECK(!initializerCall);

    // FailedTemporarily -> ContentLoading
    manager.loadTileContent(tile, options);
    CHECK(manager.getNumberOfTilesLoading() == 1);
    CHECK(tile.getState() == TileLoadState::ContentLoading);
  }

  SECTION("Loader requests failed") {
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
        [&](Tile&) { initializerCall = true; },
        TileLoadResultState::Failed};
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
    TilesetContentManager manager{
        externals,
        options,
        RasterOverlayCollection{loadedTiles, externals},
        {},
        std::move(pMockedLoader),
        std::move(pRootTile)};

    // test manager loading
    Tile& tile = *manager.getRootTile();
    manager.loadTileContent(tile, options);

    // Unloaded -> ContentLoading
    CHECK(manager.getNumberOfTilesLoading() == 1);
    CHECK(tile.getState() == TileLoadState::ContentLoading);
    CHECK(tile.getChildren().empty());
    CHECK(tile.getContent().isUnknownContent());
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().getRenderContent());

    // ContentLoading -> Failed
    manager.waitUntilIdle();
    CHECK(manager.getNumberOfTilesLoading() == 0);
    CHECK(tile.getChildren().empty());
    CHECK(tile.getState() == TileLoadState::Failed);
    CHECK(tile.getContent().isUnknownContent());
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().getRenderContent());
    CHECK(!initializerCall);

    // Failed -> Failed
    // tile is failed but the loader can still add children to it
    manager.updateTileContent(tile, options);
    CHECK(manager.getNumberOfTilesLoading() == 0);
    CHECK(tile.getChildren().size() == 1);
    CHECK(tile.getChildren().front().isEmptyContent());
    CHECK(tile.getState() == TileLoadState::Failed);
    CHECK(tile.getContent().isUnknownContent());
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().getRenderContent());
    CHECK(!initializerCall);

    // cannot transition from Failed -> ContentLoading
    manager.loadTileContent(tile, options);
    CHECK(manager.getNumberOfTilesLoading() == 0);
    CHECK(tile.getState() == TileLoadState::Failed);
    CHECK(tile.getContent().isUnknownContent());
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().isExternalContent());
    CHECK(!tile.getContent().isEmptyContent());
    CHECK(!tile.getContent().getRenderContent());

    // Failed -> Unloaded
    manager.unloadTileContent(tile);
    CHECK(tile.getState() == TileLoadState::Unloaded);
    CHECK(tile.getContent().isUnknownContent());
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().isExternalContent());
    CHECK(!tile.getContent().isEmptyContent());
    CHECK(!tile.getContent().getRenderContent());
  }

  SECTION("Make sure the manager loads parent first before loading upsampled "
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
        [&](Tile&) { initializerCall = true; },
        TileLoadResultState::Success};
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
    TilesetContentManager manager{
        externals,
        options,
        RasterOverlayCollection{loadedTiles, externals},
        {},
        std::move(pMockedLoader),
        std::move(pRootTile)};

    Tile& tile = *manager.getRootTile();
    Tile& upsampledTile = tile.getChildren().back();

    // test manager loading upsample tile
    manager.loadTileContent(upsampledTile, options);

    // since parent is not yet loaded, it will load the parent first.
    // The upsampled tile will not be loaded at the moment
    CHECK(upsampledTile.getState() == TileLoadState::Unloaded);
    CHECK(tile.getState() == TileLoadState::ContentLoading);

    // parent moves from ContentLoading -> ContentLoaded
    manager.waitUntilIdle();
    CHECK(tile.getState() == TileLoadState::ContentLoaded);
    CHECK(tile.isRenderContent());
    CHECK(initializerCall);

    // try again with upsample tile, but still not able to load it
    // because parent is not done yet
    manager.loadTileContent(upsampledTile, options);
    CHECK(upsampledTile.getState() == TileLoadState::Unloaded);

    // parent moves from ContentLoaded -> Done
    manager.updateTileContent(tile, options);
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
        [&](Tile&) { initializerCall = true; },
        TileLoadResultState::Success};
    pMockedLoaderRaw->mockCreateTileChildren = {
        {},
        TileLoadResultState::Failed};
    manager.loadTileContent(upsampledTile, options);
    CHECK(upsampledTile.getState() == TileLoadState::ContentLoading);

    // trying to unload parent while upsampled children is loading won't work
    CHECK(!manager.unloadTileContent(tile));
    CHECK(tile.getState() == TileLoadState::Done);
    CHECK(tile.isRenderContent());

    // upsampled tile: ContentLoading -> ContentLoaded
    manager.waitUntilIdle();
    CHECK(upsampledTile.getState() == TileLoadState::ContentLoaded);
    CHECK(upsampledTile.isRenderContent());

    // trying to unload parent will work now since the upsampled tile is already
    // in the main thread
    CHECK(manager.unloadTileContent(tile));
    CHECK(tile.getState() == TileLoadState::Unloaded);
    CHECK(!tile.isRenderContent());
    CHECK(!tile.getContent().getRenderContent());

    // unload upsampled tile: ContentLoaded -> Done
    CHECK(manager.unloadTileContent(upsampledTile));
    CHECK(upsampledTile.getState() == TileLoadState::Unloaded);
    CHECK(!upsampledTile.isRenderContent());
    CHECK(!upsampledTile.getContent().getRenderContent());
  }
}

TEST_CASE("Test the tileset content manager's post processing for gltf") {
  Cesium3DTilesSelection::registerAllTileContentTypes();

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

  SECTION("Resolve external buffers") {
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
        nullptr,
        {},
        TileLoadResultState::Success};
    pMockedLoader->mockCreateTileChildren = {{}, TileLoadResultState::Failed};

    // add external buffer to the completed request
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"Box0.bin",
         createMockRequest(testDataPath / "gltf" / "box" / "Box0.bin")});

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());

    // create manager
    Tile::LoadedLinkedList loadedTiles;
    TilesetContentManager manager{
        externals,
        {},
        RasterOverlayCollection{loadedTiles, externals},
        {},
        std::move(pMockedLoader),
        std::move(pRootTile)};

    // test the gltf model
    Tile& tile = *manager.getRootTile();
    manager.loadTileContent(tile, {});
    manager.waitUntilIdle();

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
    manager.unloadTileContent(tile);
  }

  SECTION("Ensure the loader generate smooth normal when the mesh doesn't have "
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
        {},
        TileLoadResultState::Success};
    pMockedLoader->mockCreateTileChildren = {{}, TileLoadResultState::Failed};

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());

    // create manager
    TilesetOptions options;
    options.contentOptions.generateMissingNormalsSmooth = true;

    Tile::LoadedLinkedList loadedTiles;
    TilesetContentManager manager{
        externals,
        options,
        RasterOverlayCollection{loadedTiles, externals},
        {},
        std::move(pMockedLoader),
        std::move(pRootTile)};

    // test the gltf model
    Tile& tile = *manager.getRootTile();
    manager.loadTileContent(tile, options);
    manager.waitUntilIdle();

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
    manager.unloadTileContent(tile);
  }

  SECTION("Embed gltf up axis to extra") {
    // create mock loader
    auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
    pMockedLoader->mockLoadTileContent = {
        CesiumGltf::Model{},
        CesiumGeometry::Axis::Z,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        {},
        TileLoadResultState::Success};
    pMockedLoader->mockCreateTileChildren = {{}, TileLoadResultState::Failed};

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());

    // create manager
    Tile::LoadedLinkedList loadedTiles;
    TilesetContentManager manager{
        externals,
        {},
        RasterOverlayCollection{loadedTiles, externals},
        {},
        std::move(pMockedLoader),
        std::move(pRootTile)};

    Tile& tile = *manager.getRootTile();
    manager.loadTileContent(tile, {});
    manager.waitUntilIdle();

    const auto& renderContent = tile.getContent().getRenderContent();
    CHECK(renderContent);

    auto gltfUpAxisIt = renderContent->getModel().extras.find("gltfUpAxis");
    CHECK(gltfUpAxisIt != renderContent->getModel().extras.end());
    CHECK(gltfUpAxisIt->second.getInt64() == 2);

    manager.unloadTileContent(tile);
  }

  SECTION("Generate raster overlay projections") {
    // add raster overlay
    Tile::LoadedLinkedList loadedTiles;
    RasterOverlayCollection rasterOverlayCollection{loadedTiles, externals};
    rasterOverlayCollection.add(
        std::make_unique<DebugColorizeTilesRasterOverlay>("DebugOverlay"));
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
        {},
        TileLoadResultState::Success};
    pMockedLoader->mockCreateTileChildren = {{}, TileLoadResultState::Failed};

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());

    // create manager
    TilesetContentManager manager{
        externals,
        {},
        std::move(rasterOverlayCollection),
        {},
        std::move(pMockedLoader),
        std::move(pRootTile)};

    SECTION(
        "Generate raster overlay details when tile don't have loose region") {
      // test the gltf model
      Tile& tile = *manager.getRootTile();
      manager.loadTileContent(tile, {});
      manager.waitUntilIdle();

      CHECK(tile.getState() == TileLoadState::ContentLoaded);
      const TileContent& tileContent = tile.getContent();
      CHECK(tileContent.isRenderContent());
      const RasterOverlayDetails& rasterOverlayDetails =
          tileContent.getRenderContent()->getRasterOverlayDetails();

      // ensure the raster overlay details has geographic projection
      GeographicProjection geographicProjection{};
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

    SECTION("Generate raster overlay details when tile has loose region") {
      Tile& tile = *manager.getRootTile();
      auto originalLooseRegion =
          BoundingRegionWithLooseFittingHeights{BoundingRegion{
              GeographicProjection::MAXIMUM_GLOBE_RECTANGLE,
              -1000.0,
              9000.0}};
      tile.setBoundingVolume(originalLooseRegion);

      manager.loadTileContent(tile, {});
      manager.waitUntilIdle();

      CHECK(tile.getState() == TileLoadState::ContentLoaded);
      const TileContent& tileContent = tile.getContent();
      CHECK(tileContent.isRenderContent());
      const RasterOverlayDetails& rasterOverlayDetails =
          tileContent.getRenderContent()->getRasterOverlayDetails();

      // ensure the raster overlay details has geographic projection
      GeographicProjection geographicProjection{};
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

    SECTION("Automatically calculate fit bounding region when tile has loose "
            "region") {
      auto pRemovedOverlay =
          manager.getRasterOverlayCollection().begin()->get();
      manager.getRasterOverlayCollection().remove(pRemovedOverlay);
      CHECK(manager.getRasterOverlayCollection().size() == 0);

      Tile& tile = *manager.getRootTile();
      auto originalLooseRegion =
          BoundingRegionWithLooseFittingHeights{BoundingRegion{
              GeographicProjection::MAXIMUM_GLOBE_RECTANGLE,
              -1000.0,
              9000.0}};
      tile.setBoundingVolume(originalLooseRegion);

      manager.loadTileContent(tile, {});
      manager.waitUntilIdle();

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

  SECTION("Don't generate raster overlay for existing projection") {
    // create gltf grid
    Cartographic beginCarto{glm::radians(32.0), glm::radians(48.0), 100.0};
    CesiumGltf::Model model = createGlobeGrid(beginCarto, 10, 10, 0.01);
    model.extras["gltfUpAxis"] =
        static_cast<std::underlying_type_t<CesiumGeometry::Axis>>(
            CesiumGeometry::Axis::Z);

    // mock raster overlay detail
    GeographicProjection projection;
    RasterOverlayDetails rasterOverlayDetails;
    rasterOverlayDetails.rasterOverlayProjections.emplace_back(projection);
    rasterOverlayDetails.rasterOverlayRectangles.emplace_back(
        projection.project(GeographicProjection::MAXIMUM_GLOBE_RECTANGLE));
    rasterOverlayDetails.boundingRegion = BoundingRegion{
        GeographicProjection::MAXIMUM_GLOBE_RECTANGLE,
        -1000.0,
        9000.0};

    // add raster overlay
    Tile::LoadedLinkedList loadedTiles;
    RasterOverlayCollection rasterOverlayCollection{loadedTiles, externals};
    rasterOverlayCollection.add(
        std::make_unique<DebugColorizeTilesRasterOverlay>("DebugOverlay"));
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
        {},
        TileLoadResultState::Success};
    pMockedLoader->mockCreateTileChildren = {{}, TileLoadResultState::Failed};

    // create tile
    auto pRootTile = std::make_unique<Tile>(pMockedLoader.get());

    // create manager
    TilesetContentManager manager{
        externals,
        {},
        std::move(rasterOverlayCollection),
        {},
        std::move(pMockedLoader),
        std::move(pRootTile)};

    Tile& tile = *manager.getRootTile();
    manager.loadTileContent(tile, {});
    manager.waitUntilIdle();

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

    manager.unloadTileContent(tile);
  }
}
