#include <Cesium3DTilesContent/SubtreeAvailability.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGltf/Extension3dTilesSubtree.h>
#include <CesiumGltf/Model.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/SubtreeTestUtility.h>
#include <CesiumNativeTests/TestContext.h>
#include <CesiumNativeTests/ThreadTaskProcessor.h>
#include <CesiumNativeTests/waitForFuture.h>
#include <CesiumTilesetJson/Subtree.h>

#include <doctest/doctest.h>
#include <libmorton/morton.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <spdlog/spdlog.h>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace CesiumTilesetJson;
using namespace Cesium3DTilesContent;
using namespace CesiumGeometry;
using namespace CesiumNativeTests;

namespace {
CesiumUtility::Result<SubtreeAvailability> mockLoadSubtreeJson(
    uint32_t levelsInSubtree,
    SubtreeContent&& subtreeContent,
    rapidjson::Document&& subtreeJson) {
  rapidjson::StringBuffer subtreeJsonBuffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(subtreeJsonBuffer);
  subtreeJson.Accept(writer);
  std::vector<std::byte> buffer(subtreeJsonBuffer.GetSize());
  std::memcpy(
      buffer.data(),
      subtreeJsonBuffer.GetString(),
      subtreeJsonBuffer.GetSize());

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>> requestMap{
      {"test",
       createSuccessfulRequest(
           "GET",
           "test",
           CesiumAsync::HttpHeaders{},
           std::move(buffer))},
      {"buffer",
       createSuccessfulRequest(
           "GET",
           "buffer",
           CesiumAsync::HttpHeaders{},
           std::move(subtreeContent.buffers))}};

  TestContext<ThreadTaskProcessor> context(std::move(requestMap));

  auto subtreeFuture = SubtreeAvailability::loadSubtree(
      ImplicitTileSubdivisionScheme::Quadtree,
      levelsInSubtree,
      context.asyncSystem,
      context.pAssetAccessor,
      "test",
      {});

  return waitForFuture(context.asyncSystem, std::move(subtreeFuture));
}

CesiumGltf::Model createEmptySubtreeGltf() {
  CesiumGltf::Model model;
  model.addExtension<CesiumGltf::Extension3dTilesSubtree>();
  return model;
}
} // namespace

TEST_CASE("Test SubtreeAvailability methods") {
  SUBCASE("Availability stored in constant") {
    SubtreeAvailability subtreeAvailability{
        ImplicitTileSubdivisionScheme::Quadtree,
        5,
        SubtreeAvailability::SubtreeConstantAvailability{true},
        SubtreeAvailability::SubtreeConstantAvailability{false},
        {SubtreeAvailability::SubtreeConstantAvailability{false}},
        createEmptySubtreeGltf()};

    SUBCASE("isTileAvailable()") {
      CesiumGeometry::QuadtreeTileID tileID{4, 3, 1};
      CHECK(subtreeAvailability.isTileAvailable(
          tileID.level,
          libmorton::morton2D_64_encode(tileID.x, tileID.y)));
    }

    SUBCASE("isContentAvailable()") {
      CesiumGeometry::QuadtreeTileID tileID{5, 3, 1};
      CHECK(!subtreeAvailability.isContentAvailable(
          tileID.level,
          libmorton::morton2D_64_encode(tileID.x, tileID.y)));
    }

    SUBCASE("isSubtreeAvailable()") {
      CesiumGeometry::QuadtreeTileID tileID{6, 3, 1};
      CHECK(!subtreeAvailability.isSubtreeAvailable(
          libmorton::morton2D_64_encode(tileID.x, tileID.y)));
    }
  }

  SUBCASE("Availability stored in buffer view") {
    // create expected available tiles
    std::vector<CesiumGeometry::QuadtreeTileID> availableTileIDs{
        CesiumGeometry::QuadtreeTileID{0, 0, 0},
        CesiumGeometry::QuadtreeTileID{1, 1, 0},
        CesiumGeometry::QuadtreeTileID{2, 2, 2},
        CesiumGeometry::QuadtreeTileID{2, 3, 1}};

    // create expected unavailable tiles
    std::vector<CesiumGeometry::QuadtreeTileID> unavailableTileIDs{
        CesiumGeometry::QuadtreeTileID{1, 1, 1},
        CesiumGeometry::QuadtreeTileID{1, 0, 0},
        CesiumGeometry::QuadtreeTileID{2, 0, 2},
        CesiumGeometry::QuadtreeTileID{2, 3, 0},
        CesiumGeometry::QuadtreeTileID{3, 0, 4},

        // illegal ID, so it shouldn't crash
        CesiumGeometry::QuadtreeTileID{0, 1, 1},
        CesiumGeometry::QuadtreeTileID{2, 12, 1},
        CesiumGeometry::QuadtreeTileID{12, 16, 14},
    };

    // create available subtree
    std::vector<CesiumGeometry::QuadtreeTileID> availableSubtreeIDs{
        CesiumGeometry::QuadtreeTileID{5, 31, 31},
        CesiumGeometry::QuadtreeTileID{5, 30, 28},
        CesiumGeometry::QuadtreeTileID{5, 20, 10},
        CesiumGeometry::QuadtreeTileID{5, 11, 1}};

    // create unavailable subtree
    std::vector<CesiumGeometry::QuadtreeTileID> unavailableSubtreeIDs{
        CesiumGeometry::QuadtreeTileID{5, 3, 31},
        CesiumGeometry::QuadtreeTileID{5, 10, 18},
        CesiumGeometry::QuadtreeTileID{5, 20, 12},
        CesiumGeometry::QuadtreeTileID{5, 11, 12}};

    // setup tile availability buffer
    uint64_t maxSubtreeLevels = 5;
    uint64_t numTiles = getTotalNumberOfTilesForQuadtree(maxSubtreeLevels);

    uint64_t maxSubtreeTiles = uint64_t(1) << (2 * (maxSubtreeLevels));
    uint64_t bufferSize =
        static_cast<uint64_t>(std::ceil(static_cast<double>(numTiles) / 8.0));
    uint64_t subtreeBufferSize = static_cast<uint64_t>(
        std::ceil(static_cast<double>(maxSubtreeTiles) / 8.0));

    CesiumGltf::Model model;
    model.addExtension<CesiumGltf::Extension3dTilesSubtree>();
    model.buffers.resize(3);
    model.bufferViews.resize(3);

    std::vector<std::byte>& contentAvailabilityBuffer =
        model.buffers[0].cesium.data;
    std::vector<std::byte>& tileAvailabilityBuffer =
        model.buffers[1].cesium.data;
    std::vector<std::byte>& subtreeAvailabilityBuffer =
        model.buffers[2].cesium.data;

    model.bufferViews[0].buffer = 0;
    model.bufferViews[1].buffer = 1;
    model.bufferViews[2].buffer = 2;

    contentAvailabilityBuffer.resize(size_t(bufferSize));
    tileAvailabilityBuffer.resize(size_t(bufferSize));
    subtreeAvailabilityBuffer.resize(size_t(subtreeBufferSize));

    model.buffers[0].byteLength = model.bufferViews[0].byteLength =
        int64_t(bufferSize);
    model.buffers[1].byteLength = model.bufferViews[1].byteLength =
        int64_t(bufferSize);
    model.buffers[2].byteLength = model.bufferViews[2].byteLength =
        int64_t(subtreeBufferSize);

    for (const auto& tileID : availableTileIDs) {
      markTileAvailable(tileID, tileAvailabilityBuffer);
      markTileAvailable(tileID, contentAvailabilityBuffer);
    }

    for (const auto& subtreeID : availableSubtreeIDs) {
      markSubtreeAvailable(subtreeID, subtreeAvailabilityBuffer);
    }

    SubtreeAvailability::SubtreeBufferViewAvailability tileAvailability{
        tileAvailabilityBuffer};
    SubtreeAvailability::SubtreeBufferViewAvailability subtreeAvailability{
        subtreeAvailabilityBuffer};
    SubtreeAvailability::SubtreeBufferViewAvailability contentAvailability{
        contentAvailabilityBuffer};

    SubtreeAvailability quadtreeAvailability(
        ImplicitTileSubdivisionScheme::Quadtree,
        static_cast<uint32_t>(maxSubtreeLevels),
        tileAvailability,
        subtreeAvailability,
        contentAvailability,
        std::move(model));

    SUBCASE("isTileAvailable()") {
      for (const auto& tileID : availableTileIDs) {
        CHECK(quadtreeAvailability.isTileAvailable(
            tileID.level,
            libmorton::morton2D_64_encode(tileID.x, tileID.y)));
      }

      for (const auto& tileID : unavailableTileIDs) {
        CHECK(!quadtreeAvailability.isTileAvailable(
            tileID.level,
            libmorton::morton2D_64_encode(tileID.x, tileID.y)));
      }
    }

    SUBCASE("isContentAvailable()") {
      for (const auto& tileID : availableTileIDs) {
        CHECK(quadtreeAvailability.isContentAvailable(
            tileID.level,
            libmorton::morton2D_64_encode(tileID.x, tileID.y)));
      }

      for (const auto& tileID : unavailableTileIDs) {
        CHECK(!quadtreeAvailability.isContentAvailable(
            tileID.level,
            libmorton::morton2D_64_encode(tileID.x, tileID.y)));
      }
    }

    SUBCASE("isSubtreeAvailable()") {
      for (const auto& subtreeID : availableSubtreeIDs) {
        CHECK(quadtreeAvailability.isSubtreeAvailable(
            libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
      }

      for (const auto& subtreeID : unavailableSubtreeIDs) {
        CHECK(!quadtreeAvailability.isSubtreeAvailable(
            libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
      }
    }
  }
}

TEST_CASE("Test parsing 3D Tiles 1.1 subtree") {
  uint32_t maxSubtreeLevels = 5;

  std::vector<CesiumGeometry::QuadtreeTileID> availableTileIDs{
      CesiumGeometry::QuadtreeTileID{0, 0, 0},
      CesiumGeometry::QuadtreeTileID{1, 0, 0},
      CesiumGeometry::QuadtreeTileID{1, 1, 0},
      CesiumGeometry::QuadtreeTileID{2, 2, 2},
      CesiumGeometry::QuadtreeTileID{2, 3, 2},
      CesiumGeometry::QuadtreeTileID{2, 0, 0},
      CesiumGeometry::QuadtreeTileID{3, 1, 0},
  };

  std::vector<CesiumGeometry::QuadtreeTileID> unavailableTileIDs{
      CesiumGeometry::QuadtreeTileID{1, 0, 1},
      CesiumGeometry::QuadtreeTileID{1, 1, 1},
      CesiumGeometry::QuadtreeTileID{2, 2, 3},
      CesiumGeometry::QuadtreeTileID{2, 3, 1},
      CesiumGeometry::QuadtreeTileID{2, 1, 0},
      CesiumGeometry::QuadtreeTileID{3, 2, 0},
  };

  std::vector<CesiumGeometry::QuadtreeTileID> availableSubtreeIDs{
      CesiumGeometry::QuadtreeTileID{5, 31, 31},
      CesiumGeometry::QuadtreeTileID{5, 30, 28},
      CesiumGeometry::QuadtreeTileID{5, 20, 10},
      CesiumGeometry::QuadtreeTileID{5, 11, 1}};

  std::vector<CesiumGeometry::QuadtreeTileID> unavailableSubtreeIDs{
      CesiumGeometry::QuadtreeTileID{5, 31, 30},
      CesiumGeometry::QuadtreeTileID{5, 31, 28},
      CesiumGeometry::QuadtreeTileID{5, 21, 11},
      CesiumGeometry::QuadtreeTileID{5, 11, 12}};

  SUBCASE("Parse binary subtree") {
    // create subtree json
    auto subtreeContent =
        SubtreeContent(maxSubtreeLevels, availableTileIDs, availableSubtreeIDs);
    auto subtreeJson = subtreeContent.toJson("");

    // serialize it into binary subtree format
    rapidjson::StringBuffer subtreeJsonBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(subtreeJsonBuffer);
    subtreeJson.Accept(writer);

    SubtreeHeader subtreeHeader;
    subtreeHeader.magic[0] = 's';
    subtreeHeader.magic[1] = 'u';
    subtreeHeader.magic[2] = 'b';
    subtreeHeader.magic[3] = 't';
    subtreeHeader.version = 1U;
    subtreeHeader.jsonByteLength = subtreeJsonBuffer.GetSize();
    subtreeHeader.binaryByteLength = subtreeContent.buffers.size();

    std::vector<std::byte> buffer(size_t(
        sizeof(subtreeHeader) + subtreeHeader.jsonByteLength +
        subtreeHeader.binaryByteLength));
    std::memcpy(buffer.data(), &subtreeHeader, sizeof(subtreeHeader));
    std::memcpy(
        buffer.data() + sizeof(subtreeHeader),
        subtreeJsonBuffer.GetString(),
        size_t(subtreeHeader.jsonByteLength));
    std::memcpy(
        buffer.data() + sizeof(subtreeHeader) + subtreeHeader.jsonByteLength,
        subtreeContent.buffers.data(),
        size_t(subtreeHeader.binaryByteLength));

    std::map<std::string, std::shared_ptr<SimpleAssetRequest>> requestMap{
        {"test",
         createSuccessfulRequest(
             "GET",
             "test",
             CesiumAsync::HttpHeaders{},
             std::move(buffer))}};
    TestContext<SimpleTaskProcessor> context(std::move(requestMap));

    auto subtreeFuture = SubtreeAvailability::loadSubtree(
        ImplicitTileSubdivisionScheme::Quadtree,
        maxSubtreeLevels,
        context.asyncSystem,
        context.pAssetAccessor,
        "test",
        {});

    context.asyncSystem.dispatchMainThreadTasks();
    auto result = subtreeFuture.wait();
    REQUIRE_UNARY(result.value);
    SubtreeAvailability& parsedSubtree = *result.value;

    for (const auto& tileID : availableTileIDs) {
      uint64_t mortonID = libmorton::morton2D_64_encode(tileID.x, tileID.y);
      CHECK(parsedSubtree.isTileAvailable(tileID.level, mortonID));
      CHECK(parsedSubtree.isContentAvailable(tileID.level, mortonID));
    }

    for (const auto& tileID : unavailableTileIDs) {
      uint64_t mortonID = libmorton::morton2D_64_encode(tileID.x, tileID.y);
      CHECK_FALSE(parsedSubtree.isTileAvailable(tileID.level, mortonID));
      CHECK_FALSE(parsedSubtree.isContentAvailable(tileID.level, mortonID));
    }

    for (const auto& subtreeID : availableSubtreeIDs) {
      CHECK(parsedSubtree.isSubtreeAvailable(
          libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
    }

    for (const auto& subtreeID : unavailableSubtreeIDs) {
      CHECK(!parsedSubtree.isSubtreeAvailable(
          libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
    }
  }

  SUBCASE("Parse binary subtree with mixed availability types") {
    auto subtreeContent =
        SubtreeContent(maxSubtreeLevels, true, availableSubtreeIDs);
    auto subtreeJson = subtreeContent.toJson("");

    // serialize it into binary subtree format
    rapidjson::StringBuffer subtreeJsonBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(subtreeJsonBuffer);
    subtreeJson.Accept(writer);

    SubtreeHeader subtreeHeader;
    subtreeHeader.magic[0] = 's';
    subtreeHeader.magic[1] = 'u';
    subtreeHeader.magic[2] = 'b';
    subtreeHeader.magic[3] = 't';
    subtreeHeader.version = 1U;
    subtreeHeader.jsonByteLength = subtreeJsonBuffer.GetSize();
    subtreeHeader.binaryByteLength = subtreeContent.buffers.size();

    std::vector<std::byte> buffer(size_t(
        sizeof(subtreeHeader) + subtreeHeader.jsonByteLength +
        subtreeHeader.binaryByteLength));
    std::memcpy(buffer.data(), &subtreeHeader, sizeof(subtreeHeader));
    std::memcpy(
        buffer.data() + sizeof(subtreeHeader),
        subtreeJsonBuffer.GetString(),
        size_t(subtreeHeader.jsonByteLength));
    std::memcpy(
        buffer.data() + sizeof(subtreeHeader) + subtreeHeader.jsonByteLength,
        subtreeContent.buffers.data(),
        size_t(subtreeHeader.binaryByteLength));

    std::map<std::string, std::shared_ptr<SimpleAssetRequest>> requestMap{
        {"test",
         createSuccessfulRequest(
             "GET",
             "test",
             CesiumAsync::HttpHeaders{},
             std::move(buffer))}};
    TestContext<SimpleTaskProcessor> context(std::move(requestMap));

    auto subtreeFuture = SubtreeAvailability::loadSubtree(
        ImplicitTileSubdivisionScheme::Quadtree,
        maxSubtreeLevels,
        context.asyncSystem,
        context.pAssetAccessor,
        "test",
        {});

    context.asyncSystem.dispatchMainThreadTasks();
    auto result = subtreeFuture.wait();
    REQUIRE_UNARY(result.value);
    SubtreeAvailability& parsedSubtree = *result.value;

    for (const auto& tileID : availableTileIDs) {
      uint64_t mortonID = libmorton::morton2D_64_encode(tileID.x, tileID.y);
      CHECK(parsedSubtree.isTileAvailable(tileID.level, mortonID));
      CHECK(parsedSubtree.isContentAvailable(tileID.level, mortonID));
    }

    for (const auto& tileID : unavailableTileIDs) {
      uint64_t mortonID = libmorton::morton2D_64_encode(tileID.x, tileID.y);
      CHECK(parsedSubtree.isTileAvailable(tileID.level, mortonID));
      CHECK(parsedSubtree.isContentAvailable(tileID.level, mortonID));
    }

    for (const auto& subtreeID : availableSubtreeIDs) {
      CHECK(parsedSubtree.isSubtreeAvailable(
          libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
    }

    for (const auto& subtreeID : unavailableSubtreeIDs) {
      CHECK_FALSE(parsedSubtree.isSubtreeAvailable(
          libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
    }
  }

  SUBCASE("Parse binary subtree with constant availability only") {
    // create subtree json
    auto subtreeContent = SubtreeContent(maxSubtreeLevels, true, false);
    auto subtreeJson = subtreeContent.toJson("");

    // serialize it into binary subtree format
    rapidjson::StringBuffer subtreeJsonBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(subtreeJsonBuffer);
    subtreeJson.Accept(writer);

    SubtreeHeader subtreeHeader;
    subtreeHeader.magic[0] = 's';
    subtreeHeader.magic[1] = 'u';
    subtreeHeader.magic[2] = 'b';
    subtreeHeader.magic[3] = 't';
    subtreeHeader.version = 1U;
    subtreeHeader.jsonByteLength = subtreeJsonBuffer.GetSize();
    subtreeHeader.binaryByteLength = 0;

    std::vector<std::byte> buffer(size_t(
        sizeof(subtreeHeader) + subtreeHeader.jsonByteLength +
        subtreeHeader.binaryByteLength));
    std::memcpy(buffer.data(), &subtreeHeader, sizeof(subtreeHeader));
    std::memcpy(
        buffer.data() + sizeof(subtreeHeader),
        subtreeJsonBuffer.GetString(),
        size_t(subtreeHeader.jsonByteLength));

    std::map<std::string, std::shared_ptr<SimpleAssetRequest>> requestMap{
        {"test",
         createSuccessfulRequest(
             "GET",
             "test",
             CesiumAsync::HttpHeaders{},
             std::move(buffer))}};
    TestContext<SimpleTaskProcessor> context(std::move(requestMap));

    auto subtreeFuture = SubtreeAvailability::loadSubtree(
        ImplicitTileSubdivisionScheme::Quadtree,
        maxSubtreeLevels,
        context.asyncSystem,
        context.pAssetAccessor,
        "test",
        {});

    context.asyncSystem.dispatchMainThreadTasks();
    auto result = subtreeFuture.wait();
    REQUIRE_UNARY(result.value);
    SubtreeAvailability& parsedSubtree = *result.value;

    for (const auto& tileID : availableTileIDs) {
      uint64_t mortonID = libmorton::morton2D_64_encode(tileID.x, tileID.y);
      CHECK(parsedSubtree.isTileAvailable(tileID.level, mortonID));
      CHECK(parsedSubtree.isContentAvailable(tileID.level, mortonID));
    }

    for (const auto& tileID : unavailableTileIDs) {
      uint64_t mortonID = libmorton::morton2D_64_encode(tileID.x, tileID.y);
      CHECK(parsedSubtree.isTileAvailable(tileID.level, mortonID));
      CHECK(parsedSubtree.isContentAvailable(tileID.level, mortonID));
    }

    for (const auto& subtreeID : availableSubtreeIDs) {
      CHECK_FALSE(parsedSubtree.isSubtreeAvailable(
          libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
    }

    for (const auto& subtreeID : unavailableSubtreeIDs) {
      CHECK_FALSE(parsedSubtree.isSubtreeAvailable(
          libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
    }
  }

  SUBCASE("Parse json subtree") {
    auto subtreeContent =
        SubtreeContent(maxSubtreeLevels, availableTileIDs, availableSubtreeIDs);
    auto subtreeJson = subtreeContent.toJson("buffer");

    auto result = mockLoadSubtreeJson(
        maxSubtreeLevels,
        std::move(subtreeContent),
        std::move(subtreeJson));

    REQUIRE_UNARY(result.value);
    SubtreeAvailability& parsedSubtree = *result.value;

    for (const auto& tileID : availableTileIDs) {
      uint64_t mortonID = libmorton::morton2D_64_encode(tileID.x, tileID.y);
      CHECK(parsedSubtree.isTileAvailable(tileID.level, mortonID));
      CHECK(parsedSubtree.isContentAvailable(tileID.level, mortonID));
    }

    for (const auto& tileID : unavailableTileIDs) {
      uint64_t mortonID = libmorton::morton2D_64_encode(tileID.x, tileID.y);
      CHECK_FALSE(parsedSubtree.isTileAvailable(tileID.level, mortonID));
      CHECK_FALSE(parsedSubtree.isContentAvailable(tileID.level, mortonID));
    }

    for (const auto& subtreeID : availableSubtreeIDs) {
      CHECK(parsedSubtree.isSubtreeAvailable(
          libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
    }

    for (const auto& subtreeID : unavailableSubtreeIDs) {
      CHECK_FALSE(parsedSubtree.isSubtreeAvailable(
          libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
    }
  }

  SUBCASE("Parse json subtree with mixed availability types") {
    auto subtreeContent =
        SubtreeContent(maxSubtreeLevels, availableTileIDs, false);
    auto subtreeJson = subtreeContent.toJson("buffer");

    auto result = mockLoadSubtreeJson(
        maxSubtreeLevels,
        std::move(subtreeContent),
        std::move(subtreeJson));

    REQUIRE_UNARY(result.value);
    SubtreeAvailability& parsedSubtree = *result.value;

    for (const auto& tileID : availableTileIDs) {
      uint64_t mortonID = libmorton::morton2D_64_encode(tileID.x, tileID.y);
      CHECK(parsedSubtree.isTileAvailable(tileID.level, mortonID));
      CHECK(parsedSubtree.isContentAvailable(tileID.level, mortonID));
    }

    for (const auto& tileID : unavailableTileIDs) {
      uint64_t mortonID = libmorton::morton2D_64_encode(tileID.x, tileID.y);
      CHECK_FALSE(parsedSubtree.isTileAvailable(tileID.level, mortonID));
      CHECK_FALSE(parsedSubtree.isContentAvailable(tileID.level, mortonID));
    }

    for (const auto& subtreeID : availableSubtreeIDs) {
      CHECK_FALSE(parsedSubtree.isSubtreeAvailable(
          libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
    }

    for (const auto& subtreeID : unavailableSubtreeIDs) {
      CHECK_FALSE(parsedSubtree.isSubtreeAvailable(
          libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
    }
  }

  SUBCASE("Subtree json has ill form format") {
    auto subtreeContent =
        SubtreeContent(maxSubtreeLevels, availableTileIDs, availableSubtreeIDs);
    auto subtreeJson = subtreeContent.toJson("buffer");

    SUBCASE("Subtree json has no tileAvailability field") {
      subtreeJson.RemoveMember("tileAvailability");
      CHECK_UNARY_FALSE(mockLoadSubtreeJson(
                            maxSubtreeLevels,
                            std::move(subtreeContent),
                            std::move(subtreeJson))
                            .value);
    }

    SUBCASE("Subtree json has no childSubtreeAvailability field") {
      subtreeJson.RemoveMember("childSubtreeAvailability");
      CHECK_UNARY_FALSE(mockLoadSubtreeJson(
                            maxSubtreeLevels,
                            std::move(subtreeContent),
                            std::move(subtreeJson))
                            .value);
    }

    SUBCASE("Subtree json has no buffers though availability points to buffer "
            "view") {
      subtreeJson.RemoveMember("buffers");
      CHECK_UNARY_FALSE(mockLoadSubtreeJson(
                            maxSubtreeLevels,
                            std::move(subtreeContent),
                            std::move(subtreeJson))
                            .value);
    }

    SUBCASE("Buffer does not have byteLength field") {
      auto bufferIt = subtreeJson.FindMember("buffers");
      auto bufferObj = bufferIt->value.GetArray().Begin();
      bufferObj->RemoveMember("byteLength");
      CHECK_UNARY_FALSE(mockLoadSubtreeJson(
                            maxSubtreeLevels,
                            std::move(subtreeContent),
                            std::move(subtreeJson))
                            .value);
    }

    SUBCASE("Buffer does not have string uri field") {
      auto bufferIt = subtreeJson.FindMember("buffers");
      auto bufferObj = bufferIt->value.GetArray().Begin();
      bufferObj->RemoveMember("uri");
      bufferObj->AddMember("uri", 12, subtreeJson.GetAllocator());
      CHECK_UNARY_FALSE(mockLoadSubtreeJson(
                            maxSubtreeLevels,
                            std::move(subtreeContent),
                            std::move(subtreeJson))
                            .value);
    }

    SUBCASE("Subtree json has no buffer views though availability points to "
            "buffer view") {
      subtreeJson.RemoveMember("bufferViews");
      CHECK_UNARY_FALSE(mockLoadSubtreeJson(
                            maxSubtreeLevels,
                            std::move(subtreeContent),
                            std::move(subtreeJson))
                            .value);
    }
  }
}

TEST_CASE("Test SubtreeAvailability::loadSubtree for glTF") {
  std::string gltfString = R"(
    {
      "asset": {
        "version": "2.1"
      },
      "extensions": {
        "3DTILES_subtree": {
          "tileAvailability": {
            "constant": 1
          },
          "contentAvailability": {
            "constant": 1
          },
          "childSubtreeAvailability": {
            "constant": 0
          }
        }
      }
    }
  )";

  std::vector<std::byte> buffer(gltfString.size());
  std::memcpy(buffer.data(), gltfString.data(), gltfString.size());

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>> requestMap{
      {"test.subtree.gltf",
       createSuccessfulRequest(
           "GET",
           "test.subtree.gltf",
           CesiumAsync::HttpHeaders{},
           std::move(buffer))}};
  TestContext<SimpleTaskProcessor> context(std::move(requestMap));

  auto subtreeFuture = SubtreeAvailability::loadSubtree(
      ImplicitTileSubdivisionScheme::Quadtree,
      3,
      context.asyncSystem,
      context.pAssetAccessor,
      "test.subtree.gltf",
      {});

  context.asyncSystem.dispatchMainThreadTasks();
  auto parsedSubtree = subtreeFuture.wait();
  REQUIRE_UNARY(parsedSubtree.value);
}

TEST_CASE("Test SubtreeAvailability::createEmpty") {
  SUBCASE("With no tiles available") {
    CesiumUtility::Result<SubtreeAvailability> maybeAvailability =
        SubtreeAvailability::createEmpty(
            ImplicitTileSubdivisionScheme::Quadtree,
            5,
            false);
    REQUIRE_UNARY(maybeAvailability.value);

    SubtreeAvailability& availability = *maybeAvailability.value;

    CHECK_FALSE(availability.isTileAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(0, 0, 0)));
    CHECK_FALSE(availability.isTileAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(4, 15, 15)));
  }

  SUBCASE("With all tiles available") {
    CesiumUtility::Result<SubtreeAvailability> maybeAvailability =
        SubtreeAvailability::createEmpty(
            ImplicitTileSubdivisionScheme::Quadtree,
            5,
            true);
    REQUIRE_UNARY(maybeAvailability.value);

    SubtreeAvailability& availability = *maybeAvailability.value;

    CHECK(availability.isTileAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(0, 0, 0)));
    CHECK(availability.isTileAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(4, 15, 15)));
  }
}

TEST_CASE("SubtreeAvailability modifications") {
  CesiumUtility::Result<SubtreeAvailability> maybeAvailability =
      SubtreeAvailability::createEmpty(
          ImplicitTileSubdivisionScheme::Quadtree,
          5,
          true);
  REQUIRE_UNARY(maybeAvailability.value);

  SubtreeAvailability& availability = *maybeAvailability.value;
  const CesiumGltf::Model& model = availability.getModel();
  const auto* pSubtreeExtension =
      model.getExtension<CesiumGltf::Extension3dTilesSubtree>();
  REQUIRE(pSubtreeExtension);

  SUBCASE("initially has all tiles available, and no content or subtrees "
          "available") {
    CHECK(availability.isTileAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(0, 0, 0)));
    CHECK(availability.isTileAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(4, 15, 15)));

    CHECK(!availability.isContentAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(0, 0, 0)));
    CHECK(!availability.isContentAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(4, 15, 15)));

    CHECK(!availability.isSubtreeAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(5, 0, 0)));
    CHECK(!availability.isSubtreeAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(5, 31, 31)));
  }

  SUBCASE("can set a single tile's state") {
    availability.setTileAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(4, 15, 15),
        false);

    CHECK(availability.isTileAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(0, 0, 0)));
    CHECK(!availability.isTileAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(4, 15, 15)));

    CHECK_GE(pSubtreeExtension->tileAvailability.bitstream, 0);
    CHECK_UNARY_FALSE(pSubtreeExtension->tileAvailability.constant);

    availability.setContentAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(4, 15, 15),
        true);

    CHECK(!availability.isContentAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(0, 0, 0)));
    CHECK(availability.isContentAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(4, 15, 15)));

    CHECK_UNARY(pSubtreeExtension->contentAvailability);
    CHECK_GE(pSubtreeExtension->contentAvailability->bitstream, 0);
    CHECK_UNARY_FALSE(pSubtreeExtension->contentAvailability->constant);

    availability.setSubtreeAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(5, 31, 31),
        true);

    CHECK(!availability.isSubtreeAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(5, 0, 0)));
    CHECK(availability.isSubtreeAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(5, 31, 31)));

    CHECK_GE(pSubtreeExtension->childSubtreeAvailability.bitstream, 0);
    CHECK_UNARY_FALSE(pSubtreeExtension->childSubtreeAvailability.constant);

    // Check that tile availability is still correct - e.g. spans are updated
    // after the buffer grows
    CHECK(availability.isTileAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(0, 0, 0)));
    CHECK(!availability.isTileAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(4, 15, 15)));
  }
}
