#include <Cesium3DTiles/Subtree.h>
#include <Cesium3DTilesContent/SubtreeAvailability.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/ThreadTaskProcessor.h>
#include <CesiumNativeTests/waitForFuture.h>

#include <catch2/catch.hpp>
#include <libmorton/morton.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cstddef>
#include <vector>

using namespace Cesium3DTiles;
using namespace Cesium3DTilesContent;
using namespace CesiumGeometry;
using namespace CesiumNativeTests;

namespace {
struct SubtreeHeader {
  char magic[4];
  uint32_t version;
  uint64_t jsonByteLength;
  uint64_t binaryByteLength;
};

struct SubtreeBuffers {
  std::vector<std::byte> buffers;
  SubtreeAvailability::SubtreeBufferViewAvailability tileAvailability;
  SubtreeAvailability::SubtreeBufferViewAvailability subtreeAvailability;
  SubtreeAvailability::SubtreeBufferViewAvailability contentAvailability;
};

uint64_t calculateTotalNumberOfTilesForQuadtree(uint64_t subtreeLevels) {
  return static_cast<uint64_t>(
      (std::pow(4.0, static_cast<double>(subtreeLevels)) - 1) /
      (static_cast<double>(subtreeLevels) - 1));
}

void markTileAvailableForQuadtree(
    const CesiumGeometry::QuadtreeTileID& tileID,
    gsl::span<std::byte> available) {
  // This function assumes that subtree tile ID is (0, 0, 0).
  // TileID must be within the subtree
  uint64_t numOfTilesFromRootToParentLevel =
      static_cast<uint64_t>(((1 << (2 * tileID.level)) - 1) / 3);
  uint64_t availabilityBitIndex =
      numOfTilesFromRootToParentLevel +
      libmorton::morton2D_64_encode(tileID.x, tileID.y);
  const uint64_t byteIndex = availabilityBitIndex / 8;
  const uint64_t bitIndex = availabilityBitIndex % 8;
  available[byteIndex] |= std::byte(1 << bitIndex);
}

void markSubtreeAvailableForQuadtree(
    const CesiumGeometry::QuadtreeTileID& tileID,
    gsl::span<std::byte> available) {
  uint64_t availabilityBitIndex =
      libmorton::morton2D_64_encode(tileID.x, tileID.y);
  const uint64_t byteIndex = availabilityBitIndex / 8;
  const uint64_t bitIndex = availabilityBitIndex % 8;
  available[byteIndex] |= std::byte(1 << bitIndex);
}

SubtreeBuffers createSubtreeBuffers(
    uint32_t maxSubtreeLevels,
    const std::vector<CesiumGeometry::QuadtreeTileID>& tileAvailabilities,
    const std::vector<CesiumGeometry::QuadtreeTileID>& subtreeAvailabilities) {
  uint64_t numTiles = calculateTotalNumberOfTilesForQuadtree(maxSubtreeLevels);
  uint64_t maxSubtreeTiles = uint64_t(1) << (2 * (maxSubtreeLevels));
  uint64_t bufferSize =
      static_cast<uint64_t>(std::ceil(static_cast<double>(numTiles) / 8.0));
  uint64_t subtreeBufferSize = static_cast<uint64_t>(
      std::ceil(static_cast<double>(maxSubtreeTiles) / 8.0));
  std::vector<std::byte> availabilityBuffer(
      bufferSize + bufferSize + subtreeBufferSize);

  gsl::span<std::byte> contentAvailabilityBuffer(
      availabilityBuffer.data(),
      bufferSize);
  gsl::span<std::byte> tileAvailabilityBuffer(
      availabilityBuffer.data() + bufferSize,
      bufferSize);
  gsl::span<std::byte> subtreeAvailabilityBuffer(
      availabilityBuffer.data() + bufferSize + bufferSize,
      subtreeBufferSize);
  for (const auto& tileID : tileAvailabilities) {
    markTileAvailableForQuadtree(tileID, tileAvailabilityBuffer);
    markTileAvailableForQuadtree(tileID, contentAvailabilityBuffer);
  }

  for (const auto& subtreeID : subtreeAvailabilities) {
    markSubtreeAvailableForQuadtree(subtreeID, subtreeAvailabilityBuffer);
  }

  SubtreeAvailability::SubtreeBufferViewAvailability tileAvailability{
      tileAvailabilityBuffer};
  SubtreeAvailability::SubtreeBufferViewAvailability subtreeAvailability{
      subtreeAvailabilityBuffer};
  SubtreeAvailability::SubtreeBufferViewAvailability contentAvailability{
      contentAvailabilityBuffer};

  return {
      std::move(availabilityBuffer),
      tileAvailability,
      subtreeAvailability,
      contentAvailability};
}

rapidjson::Document createSubtreeJson(
    const SubtreeBuffers& subtreeBuffers,
    const std::string& bufferUrl) {
  // create subtree json
  rapidjson::Document subtreeJson;
  subtreeJson.SetObject();

  // create buffers
  rapidjson::Value bufferObj(rapidjson::kObjectType);
  bufferObj.AddMember(
      "byteLength",
      uint64_t(subtreeBuffers.buffers.size()),
      subtreeJson.GetAllocator());

  if (!bufferUrl.empty()) {
    rapidjson::Value uriStr(rapidjson::kStringType);
    uriStr.SetString(
        bufferUrl.c_str(),
        static_cast<rapidjson::SizeType>(bufferUrl.size()),
        subtreeJson.GetAllocator());
    bufferObj.AddMember("uri", std::move(uriStr), subtreeJson.GetAllocator());
  }

  rapidjson::Value buffersArray(rapidjson::kArrayType);
  buffersArray.GetArray().PushBack(
      std::move(bufferObj),
      subtreeJson.GetAllocator());

  subtreeJson.AddMember(
      "buffers",
      std::move(buffersArray),
      subtreeJson.GetAllocator());

  // create buffer views
  rapidjson::Value tileAvailabilityBufferView(rapidjson::kObjectType);
  tileAvailabilityBufferView.AddMember(
      "buffer",
      uint64_t(0),
      subtreeJson.GetAllocator());
  tileAvailabilityBufferView.AddMember(
      "byteOffset",
      uint64_t(
          subtreeBuffers.tileAvailability.view.data() -
          subtreeBuffers.buffers.data()),
      subtreeJson.GetAllocator());
  tileAvailabilityBufferView.AddMember(
      "byteLength",
      uint64_t(subtreeBuffers.tileAvailability.view.size()),
      subtreeJson.GetAllocator());

  rapidjson::Value contentAvailabilityBufferView(rapidjson::kObjectType);
  contentAvailabilityBufferView.AddMember(
      "buffer",
      uint64_t(0),
      subtreeJson.GetAllocator());
  contentAvailabilityBufferView.AddMember(
      "byteOffset",
      uint64_t(
          subtreeBuffers.contentAvailability.view.data() -
          subtreeBuffers.buffers.data()),
      subtreeJson.GetAllocator());
  contentAvailabilityBufferView.AddMember(
      "byteLength",
      uint64_t(subtreeBuffers.contentAvailability.view.size()),
      subtreeJson.GetAllocator());

  rapidjson::Value subtreeAvailabilityBufferView(rapidjson::kObjectType);
  subtreeAvailabilityBufferView.AddMember(
      "buffer",
      uint64_t(0),
      subtreeJson.GetAllocator());
  subtreeAvailabilityBufferView.AddMember(
      "byteOffset",
      uint64_t(
          subtreeBuffers.subtreeAvailability.view.data() -
          subtreeBuffers.buffers.data()),
      subtreeJson.GetAllocator());
  subtreeAvailabilityBufferView.AddMember(
      "byteLength",
      uint64_t(subtreeBuffers.subtreeAvailability.view.size()),
      subtreeJson.GetAllocator());

  rapidjson::Value bufferViewsArray(rapidjson::kArrayType);
  bufferViewsArray.GetArray().PushBack(
      std::move(tileAvailabilityBufferView),
      subtreeJson.GetAllocator());
  bufferViewsArray.GetArray().PushBack(
      std::move(contentAvailabilityBufferView),
      subtreeJson.GetAllocator());
  bufferViewsArray.GetArray().PushBack(
      std::move(subtreeAvailabilityBufferView),
      subtreeJson.GetAllocator());

  subtreeJson.AddMember(
      "bufferViews",
      std::move(bufferViewsArray),
      subtreeJson.GetAllocator());

  // create tileAvailability field
  rapidjson::Value tileAvailabilityObj(rapidjson::kObjectType);
  tileAvailabilityObj.AddMember("bitstream", 0, subtreeJson.GetAllocator());
  subtreeJson.AddMember(
      "tileAvailability",
      std::move(tileAvailabilityObj),
      subtreeJson.GetAllocator());

  // create contentAvailability field
  rapidjson::Value contentAvailabilityObj(rapidjson::kObjectType);
  contentAvailabilityObj.AddMember("bitstream", 1, subtreeJson.GetAllocator());

  rapidjson::Value contentAvailabilityArray(rapidjson::kArrayType);
  contentAvailabilityArray.GetArray().PushBack(
      std::move(contentAvailabilityObj),
      subtreeJson.GetAllocator());

  subtreeJson.AddMember(
      "contentAvailability",
      std::move(contentAvailabilityArray),
      subtreeJson.GetAllocator());

  // create childSubtreeAvailability
  rapidjson::Value subtreeAvailabilityObj(rapidjson::kObjectType);
  subtreeAvailabilityObj.AddMember("bitstream", 2, subtreeJson.GetAllocator());
  subtreeJson.AddMember(
      "childSubtreeAvailability",
      std::move(subtreeAvailabilityObj),
      subtreeJson.GetAllocator());

  return subtreeJson;
}

std::optional<SubtreeAvailability> mockLoadSubtreeJson(
    uint32_t levelsInSubtree,
    SubtreeBuffers&& subtreeBuffers,
    rapidjson::Document&& subtreeJson) {
  rapidjson::StringBuffer subtreeJsonBuffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(subtreeJsonBuffer);
  subtreeJson.Accept(writer);
  std::vector<std::byte> buffer(subtreeJsonBuffer.GetSize());
  std::memcpy(
      buffer.data(),
      subtreeJsonBuffer.GetString(),
      subtreeJsonBuffer.GetSize());

  // mock the request
  auto pMockSubtreeResponse = std::make_unique<SimpleAssetResponse>(
      uint16_t(200),
      "test",
      CesiumAsync::HttpHeaders{},
      std::move(buffer));
  auto pMockSubtreeRequest = std::make_unique<SimpleAssetRequest>(
      "GET",
      "test",
      CesiumAsync::HttpHeaders{},
      std::move(pMockSubtreeResponse));

  auto pMockBufferResponse = std::make_unique<SimpleAssetResponse>(
      uint16_t(200),
      "buffer",
      CesiumAsync::HttpHeaders{},
      std::move(subtreeBuffers.buffers));
  auto pMockBufferRequest = std::make_unique<SimpleAssetRequest>(
      "GET",
      "buffer",
      CesiumAsync::HttpHeaders{},
      std::move(pMockBufferResponse));

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>> mapUrlToRequest{
      {"test", std::move(pMockSubtreeRequest)},
      {"buffer", std::move(pMockBufferRequest)}};
  auto pMockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mapUrlToRequest));

  // mock async system
  auto pMockTaskProcessor = std::make_shared<ThreadTaskProcessor>();
  CesiumAsync::AsyncSystem asyncSystem{pMockTaskProcessor};

  auto subtreeFuture = SubtreeAvailability::loadSubtree(
      ImplicitTileSubdivisionScheme::Quadtree,
      levelsInSubtree,
      asyncSystem,
      pMockAssetAccessor,
      spdlog::default_logger(),
      "test",
      {});

  return waitForFuture(asyncSystem, std::move(subtreeFuture));
}
} // namespace

TEST_CASE("Test SubtreeAvailability methods") {
  SECTION("Availability stored in constant") {
    SubtreeAvailability subtreeAvailability{
        ImplicitTileSubdivisionScheme::Quadtree,
        5,
        SubtreeAvailability::SubtreeConstantAvailability{true},
        SubtreeAvailability::SubtreeConstantAvailability{false},
        {SubtreeAvailability::SubtreeConstantAvailability{false}},
        {}};

    SECTION("isTileAvailable()") {
      CesiumGeometry::QuadtreeTileID tileID{4, 3, 1};
      CHECK(subtreeAvailability.isTileAvailable(
          tileID.level,
          libmorton::morton2D_64_encode(tileID.x, tileID.y)));
    }

    SECTION("isContentAvailable()") {
      CesiumGeometry::QuadtreeTileID tileID{5, 3, 1};
      CHECK(!subtreeAvailability.isContentAvailable(
          tileID.level,
          libmorton::morton2D_64_encode(tileID.x, tileID.y),
          0));
    }

    SECTION("isSubtreeAvailable()") {
      CesiumGeometry::QuadtreeTileID tileID{6, 3, 1};
      CHECK(!subtreeAvailability.isSubtreeAvailable(
          libmorton::morton2D_64_encode(tileID.x, tileID.y)));
    }
  }

  SECTION("Availability stored in buffer view") {
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
    uint64_t numTiles =
        calculateTotalNumberOfTilesForQuadtree(maxSubtreeLevels);

    uint64_t maxSubtreeTiles = uint64_t(1) << (2 * (maxSubtreeLevels));
    uint64_t bufferSize =
        static_cast<uint64_t>(std::ceil(static_cast<double>(numTiles) / 8.0));
    uint64_t subtreeBufferSize = static_cast<uint64_t>(
        std::ceil(static_cast<double>(maxSubtreeTiles) / 8.0));

    Subtree subtree;
    subtree.buffers.resize(3);
    subtree.bufferViews.resize(3);

    std::vector<std::byte>& contentAvailabilityBuffer =
        subtree.buffers[0].cesium.data;
    std::vector<std::byte>& tileAvailabilityBuffer =
        subtree.buffers[1].cesium.data;
    std::vector<std::byte>& subtreeAvailabilityBuffer =
        subtree.buffers[2].cesium.data;

    subtree.bufferViews[0].buffer = 0;
    subtree.bufferViews[1].buffer = 1;
    subtree.bufferViews[2].buffer = 2;

    contentAvailabilityBuffer.resize(bufferSize);
    tileAvailabilityBuffer.resize(bufferSize);
    subtreeAvailabilityBuffer.resize(subtreeBufferSize);

    subtree.buffers[0].byteLength = subtree.bufferViews[0].byteLength =
        int64_t(bufferSize);
    subtree.buffers[1].byteLength = subtree.bufferViews[1].byteLength =
        int64_t(bufferSize);
    subtree.buffers[2].byteLength = subtree.bufferViews[2].byteLength =
        int64_t(subtreeBufferSize);

    for (const auto& tileID : availableTileIDs) {
      markTileAvailableForQuadtree(tileID, tileAvailabilityBuffer);
      markTileAvailableForQuadtree(tileID, contentAvailabilityBuffer);
    }

    for (const auto& subtreeID : availableSubtreeIDs) {
      markSubtreeAvailableForQuadtree(subtreeID, subtreeAvailabilityBuffer);
    }

    SubtreeAvailability::SubtreeBufferViewAvailability tileAvailability{
        tileAvailabilityBuffer};
    SubtreeAvailability::SubtreeBufferViewAvailability subtreeAvailability{
        subtreeAvailabilityBuffer};
    std::vector<SubtreeAvailability::AvailabilityView> contentAvailability{
        SubtreeAvailability::SubtreeBufferViewAvailability{
            contentAvailabilityBuffer}};

    SubtreeAvailability quadtreeAvailability(
        ImplicitTileSubdivisionScheme::Quadtree,
        static_cast<uint32_t>(maxSubtreeLevels),
        tileAvailability,
        subtreeAvailability,
        std::move(contentAvailability),
        std::move(subtree));

    SECTION("isTileAvailable()") {
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

    SECTION("isContentAvailable()") {
      for (const auto& tileID : availableTileIDs) {
        CHECK(quadtreeAvailability.isContentAvailable(
            tileID.level,
            libmorton::morton2D_64_encode(tileID.x, tileID.y),
            0));
      }

      for (const auto& tileID : unavailableTileIDs) {
        CHECK(!quadtreeAvailability.isContentAvailable(
            tileID.level,
            libmorton::morton2D_64_encode(tileID.x, tileID.y),
            0));
      }
    }

    SECTION("isSubtreeAvailable()") {
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

TEST_CASE("Test parsing subtree format") {
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

  auto subtreeBuffers = createSubtreeBuffers(
      maxSubtreeLevels,
      availableTileIDs,
      availableSubtreeIDs);

  SECTION("Parse binary subtree") {
    // create subtree json
    auto subtreeJson = createSubtreeJson(subtreeBuffers, "");

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
    subtreeHeader.binaryByteLength = subtreeBuffers.buffers.size();

    std::vector<std::byte> buffer(
        sizeof(subtreeHeader) + subtreeHeader.jsonByteLength +
        subtreeHeader.binaryByteLength);
    std::memcpy(buffer.data(), &subtreeHeader, sizeof(subtreeHeader));
    std::memcpy(
        buffer.data() + sizeof(subtreeHeader),
        subtreeJsonBuffer.GetString(),
        subtreeHeader.jsonByteLength);
    std::memcpy(
        buffer.data() + sizeof(subtreeHeader) + subtreeHeader.jsonByteLength,
        subtreeBuffers.buffers.data(),
        subtreeHeader.binaryByteLength);

    // mock the request
    auto pMockResponse = std::make_unique<SimpleAssetResponse>(
        uint16_t(200),
        "test",
        CesiumAsync::HttpHeaders{},
        std::move(buffer));
    auto pMockRequest = std::make_unique<SimpleAssetRequest>(
        "GET",
        "test",
        CesiumAsync::HttpHeaders{},
        std::move(pMockResponse));
    std::map<std::string, std::shared_ptr<SimpleAssetRequest>> mapUrlToRequest{
        {"test", std::move(pMockRequest)}};
    auto pMockAssetAccessor =
        std::make_shared<SimpleAssetAccessor>(std::move(mapUrlToRequest));

    // mock async system
    auto pMockTaskProcessor = std::make_shared<SimpleTaskProcessor>();
    CesiumAsync::AsyncSystem asyncSystem{pMockTaskProcessor};

    auto subtreeFuture = SubtreeAvailability::loadSubtree(
        ImplicitTileSubdivisionScheme::Quadtree,
        maxSubtreeLevels,
        asyncSystem,
        pMockAssetAccessor,
        spdlog::default_logger(),
        "test",
        {});

    asyncSystem.dispatchMainThreadTasks();
    auto parsedSubtree = subtreeFuture.wait();
    REQUIRE(parsedSubtree != std::nullopt);

    for (const auto& tileID : availableTileIDs) {
      uint64_t mortonID = libmorton::morton2D_64_encode(tileID.x, tileID.y);
      CHECK(parsedSubtree->isTileAvailable(tileID.level, mortonID));
      CHECK(parsedSubtree->isContentAvailable(tileID.level, mortonID, 0));
    }

    for (const auto& tileID : unavailableTileIDs) {
      uint64_t mortonID = libmorton::morton2D_64_encode(tileID.x, tileID.y);
      CHECK(!parsedSubtree->isTileAvailable(tileID.level, mortonID));
      CHECK(!parsedSubtree->isContentAvailable(tileID.level, mortonID, 0));
    }

    for (const auto& subtreeID : availableSubtreeIDs) {
      CHECK(parsedSubtree->isSubtreeAvailable(
          libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
    }

    for (const auto& subtreeID : unavailableSubtreeIDs) {
      CHECK(!parsedSubtree->isSubtreeAvailable(
          libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
    }
  }

  SECTION("Parse json subtree") {
    auto subtreeJson = createSubtreeJson(subtreeBuffers, "buffer");

    auto parsedSubtree = mockLoadSubtreeJson(
        maxSubtreeLevels,
        std::move(subtreeBuffers),
        std::move(subtreeJson));

    REQUIRE(parsedSubtree != std::nullopt);

    for (const auto& tileID : availableTileIDs) {
      uint64_t mortonID = libmorton::morton2D_64_encode(tileID.x, tileID.y);
      CHECK(parsedSubtree->isTileAvailable(tileID.level, mortonID));
      CHECK(parsedSubtree->isContentAvailable(tileID.level, mortonID, 0));
    }

    for (const auto& tileID : unavailableTileIDs) {
      uint64_t mortonID = libmorton::morton2D_64_encode(tileID.x, tileID.y);
      CHECK(!parsedSubtree->isTileAvailable(tileID.level, mortonID));
      CHECK(!parsedSubtree->isContentAvailable(tileID.level, mortonID, 0));
    }

    for (const auto& subtreeID : availableSubtreeIDs) {
      CHECK(parsedSubtree->isSubtreeAvailable(
          libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
    }

    for (const auto& subtreeID : unavailableSubtreeIDs) {
      CHECK(!parsedSubtree->isSubtreeAvailable(
          libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
    }
  }

  SECTION("Subtree json has ill form format") {
    auto subtreeJson = createSubtreeJson(subtreeBuffers, "buffer");

    SECTION("Subtree json has no tileAvailability field") {
      subtreeJson.RemoveMember("tileAvailability");
      CHECK(
          mockLoadSubtreeJson(
              maxSubtreeLevels,
              std::move(subtreeBuffers),
              std::move(subtreeJson)) == std::nullopt);
    }

    SECTION("Subtree json has no contentAvailability field") {
      subtreeJson.RemoveMember("contentAvailability");
      CHECK(
          mockLoadSubtreeJson(
              maxSubtreeLevels,
              std::move(subtreeBuffers),
              std::move(subtreeJson)) == std::nullopt);
    }

    SECTION("Subtree json has no childSubtreeAvailability field") {
      subtreeJson.RemoveMember("childSubtreeAvailability");
      CHECK(
          mockLoadSubtreeJson(
              maxSubtreeLevels,
              std::move(subtreeBuffers),
              std::move(subtreeJson)) == std::nullopt);
    }

    SECTION("Subtree json has no buffers though availability points to buffer "
            "view") {
      subtreeJson.RemoveMember("buffers");
      CHECK(
          mockLoadSubtreeJson(
              maxSubtreeLevels,
              std::move(subtreeBuffers),
              std::move(subtreeJson)) == std::nullopt);
    }

    SECTION("Buffer does not have byteLength field") {
      auto bufferIt = subtreeJson.FindMember("buffers");
      auto bufferObj = bufferIt->value.GetArray().Begin();
      bufferObj->RemoveMember("byteLength");
      CHECK(
          mockLoadSubtreeJson(
              maxSubtreeLevels,
              std::move(subtreeBuffers),
              std::move(subtreeJson)) == std::nullopt);
    }

    SECTION("Buffer does not have string uri field") {
      auto bufferIt = subtreeJson.FindMember("buffers");
      auto bufferObj = bufferIt->value.GetArray().Begin();
      bufferObj->RemoveMember("uri");
      bufferObj->AddMember("uri", 12, subtreeJson.GetAllocator());
      CHECK(
          mockLoadSubtreeJson(
              maxSubtreeLevels,
              std::move(subtreeBuffers),
              std::move(subtreeJson)) == std::nullopt);
    }

    SECTION("Subtree json has no buffer views though availability points to "
            "buffer view") {
      subtreeJson.RemoveMember("bufferViews");
      CHECK(
          mockLoadSubtreeJson(
              maxSubtreeLevels,
              std::move(subtreeBuffers),
              std::move(subtreeJson)) == std::nullopt);
    }
  }
}

TEST_CASE("SubtreeAvailability modifications") {
  std::optional<SubtreeAvailability> maybeAvailability =
      SubtreeAvailability::createEmpty(
          ImplicitTileSubdivisionScheme::Quadtree,
          5);
  REQUIRE(maybeAvailability);

  SubtreeAvailability& availability = *maybeAvailability;

  SECTION("initially has all tiles available, and no content or subtrees "
          "available") {
    CHECK(availability.isTileAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(0, 0, 0)));
    CHECK(availability.isTileAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(4, 15, 15)));

    CHECK(!availability.isContentAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(0, 0, 0),
        0));
    CHECK(!availability.isContentAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(4, 15, 15),
        0));

    CHECK(!availability.isSubtreeAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(5, 0, 0)));
    CHECK(!availability.isSubtreeAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(5, 31, 31)));
  }

  SECTION("can set a single tile's state") {
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

    availability.setContentAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(4, 15, 15),
        0,
        true);

    CHECK(!availability.isContentAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(0, 0, 0),
        0));
    CHECK(availability.isContentAvailable(
        QuadtreeTileID(0, 0, 0),
        QuadtreeTileID(4, 15, 15),
        0));

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
  }
}
