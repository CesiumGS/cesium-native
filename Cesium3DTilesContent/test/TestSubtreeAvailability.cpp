#include <Cesium3DTiles/Subtree.h>
#include <Cesium3DTilesContent/SubtreeAvailability.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/ThreadTaskProcessor.h>
#include <CesiumNativeTests/waitForFuture.h>

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

struct SubtreeContent {
  std::vector<std::byte> buffers;
  SubtreeAvailability::AvailabilityView tileAvailability;
  SubtreeAvailability::AvailabilityView subtreeAvailability;
  SubtreeAvailability::AvailabilityView contentAvailability;
};

uint64_t calculateTotalNumberOfTilesForQuadtree(uint64_t subtreeLevels) {
  return static_cast<uint64_t>(
      (std::pow(4.0, static_cast<double>(subtreeLevels)) - 1) /
      (static_cast<double>(subtreeLevels) - 1));
}

void markTileAvailableForQuadtree(
    const CesiumGeometry::QuadtreeTileID& tileID,
    std::span<std::byte> available) {
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
    std::span<std::byte> available) {
  uint64_t availabilityBitIndex =
      libmorton::morton2D_64_encode(tileID.x, tileID.y);
  const uint64_t byteIndex = availabilityBitIndex / 8;
  const uint64_t bitIndex = availabilityBitIndex % 8;
  available[byteIndex] |= std::byte(1 << bitIndex);
}

using SubtreeContentInput =
    std::variant<bool, std::vector<CesiumGeometry::QuadtreeTileID>>;

struct NeedsAvailabilityBuffer {
  bool operator()(bool) { return false; }
  bool operator()(const std::vector<CesiumGeometry::QuadtreeTileID>&) {
    return true;
  }
};

struct GetAvailabilityView {
  std::span<std::byte> buffer;
  bool isSubtreeBuffer;

  SubtreeAvailability::AvailabilityView operator()(bool constant) {
    return SubtreeAvailability::SubtreeConstantAvailability{constant};
  }

  SubtreeAvailability::AvailabilityView
  operator()(const std::vector<CesiumGeometry::QuadtreeTileID>& availableIDs) {
    if (isSubtreeBuffer) {
      for (const auto& subtreeID : availableIDs) {
        markSubtreeAvailableForQuadtree(subtreeID, buffer);
      }
    } else {
      for (const auto& tileID : availableIDs) {
        markTileAvailableForQuadtree(tileID, buffer);
      }
    }

    return SubtreeAvailability::SubtreeBufferViewAvailability{buffer};
  }
};

SubtreeContent createSubtreeContent(
    uint32_t maxSubtreeLevels,
    const SubtreeContentInput& tileAvailabilities,
    const SubtreeContentInput& subtreeAvailabilities) {
  bool needsTileAvailabilityBuffer =
      std::visit(NeedsAvailabilityBuffer{}, tileAvailabilities);
  bool needsSubtreeAvailabilityBuffer =
      std::visit(NeedsAvailabilityBuffer{}, subtreeAvailabilities);

  // Create and populate the availability buffers.
  uint64_t numTiles = calculateTotalNumberOfTilesForQuadtree(maxSubtreeLevels);
  uint64_t maxSubtreeTiles = uint64_t(1) << (2 * (maxSubtreeLevels));

  uint64_t bufferSize = needsTileAvailabilityBuffer
                            ? static_cast<uint64_t>(std::ceil(
                                  static_cast<double>(numTiles) / 8.0))
                            : 0;
  uint64_t subtreeBufferSize =
      needsSubtreeAvailabilityBuffer
          ? static_cast<uint64_t>(
                std::ceil(static_cast<double>(maxSubtreeTiles) / 8.0))
          : 0;

  std::vector<std::byte> availabilityBuffer(
      bufferSize + bufferSize + subtreeBufferSize);

  std::span<std::byte> contentAvailabilityBuffer(
      availabilityBuffer.data(),
      bufferSize);
  std::span<std::byte> tileAvailabilityBuffer(
      availabilityBuffer.data() + bufferSize,
      bufferSize);
  std::span<std::byte> subtreeAvailabilityBuffer(
      availabilityBuffer.data() + bufferSize + bufferSize,
      subtreeBufferSize);

  SubtreeAvailability::AvailabilityView tileAvailability = std::visit(
      GetAvailabilityView{tileAvailabilityBuffer, false},
      tileAvailabilities);
  SubtreeAvailability::AvailabilityView contentAvailability = std::visit(
      GetAvailabilityView{contentAvailabilityBuffer, false},
      tileAvailabilities);
  SubtreeAvailability::AvailabilityView subtreeAvailability = std::visit(
      GetAvailabilityView{subtreeAvailabilityBuffer, true},
      subtreeAvailabilities);

  return {
      std::move(availabilityBuffer),
      tileAvailability,
      subtreeAvailability,
      contentAvailability};
}

rapidjson::Document createSubtreeJson(
    const SubtreeContent& subtreeContent,
    const std::string& bufferUrl) {
  // create subtree json
  rapidjson::Document subtreeJson;
  subtreeJson.SetObject();

  bool hasTileAvailabilityBufferView = false;
  bool hasContentAvailabilityBufferView = false;
  bool hasSubtreeAvailabilityBufferView = false;

  // create buffers and buffer views, if necessary
  if (!subtreeContent.buffers.empty()) {
    rapidjson::Value bufferObj(rapidjson::kObjectType);
    bufferObj.AddMember(
        "byteLength",
        uint64_t(subtreeContent.buffers.size()),
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
    rapidjson::Value bufferViewsArray(rapidjson::kArrayType);

    auto addBufferViewIfNeeded =
        [&subtreeJson, &bufferViewsArray](
            const SubtreeAvailability::AvailabilityView& view,
            const std::vector<std::byte>& data,
            bool& result) {
          const auto* pBufferView =
              std::get_if<SubtreeAvailability::SubtreeBufferViewAvailability>(
                  &view);
          if (pBufferView) {
            rapidjson::Value bufferView(rapidjson::kObjectType);
            bufferView.AddMember(
                "buffer",
                uint64_t(0),
                subtreeJson.GetAllocator());
            bufferView.AddMember(
                "byteOffset",
                uint64_t(pBufferView->view.data() - data.data()),
                subtreeJson.GetAllocator());
            bufferView.AddMember(
                "byteLength",
                uint64_t(pBufferView->view.size()),
                subtreeJson.GetAllocator());
            bufferViewsArray.GetArray().PushBack(
                std::move(bufferView),
                subtreeJson.GetAllocator());
          }
          result = (pBufferView != nullptr);
        };

    addBufferViewIfNeeded(
        subtreeContent.tileAvailability,
        subtreeContent.buffers,
        hasTileAvailabilityBufferView);
    addBufferViewIfNeeded(
        subtreeContent.contentAvailability,
        subtreeContent.buffers,
        hasContentAvailabilityBufferView);
    addBufferViewIfNeeded(
        subtreeContent.subtreeAvailability,
        subtreeContent.buffers,
        hasSubtreeAvailabilityBufferView);

    subtreeJson.AddMember(
        "bufferViews",
        std::move(bufferViewsArray),
        subtreeJson.GetAllocator());
  }

  int32_t bufferViewIndex = 0;

  // create tileAvailability field
  rapidjson::Value tileAvailabilityObj(rapidjson::kObjectType);
  if (hasTileAvailabilityBufferView) {
    tileAvailabilityObj.AddMember(
        "bitstream",
        bufferViewIndex++,
        subtreeJson.GetAllocator());
  } else {
    const auto* pTileAvailabilityConstant =
        std::get_if<SubtreeAvailability::SubtreeConstantAvailability>(
            &subtreeContent.tileAvailability);
    assert(pTileAvailabilityConstant);
    tileAvailabilityObj.AddMember(
        "constant",
        pTileAvailabilityConstant->constant ? 1 : 0,
        subtreeJson.GetAllocator());
  }
  subtreeJson.AddMember(
      "tileAvailability",
      std::move(tileAvailabilityObj),
      subtreeJson.GetAllocator());

  // create contentAvailability field
  rapidjson::Value contentAvailabilityObj(rapidjson::kObjectType);
  if (hasContentAvailabilityBufferView) {
    contentAvailabilityObj.AddMember(
        "bitstream",
        bufferViewIndex++,
        subtreeJson.GetAllocator());
  } else {
    const auto* pContentAvailabilityConstant =
        std::get_if<SubtreeAvailability::SubtreeConstantAvailability>(
            &subtreeContent.contentAvailability);
    assert(pContentAvailabilityConstant);
    contentAvailabilityObj.AddMember(
        "constant",
        pContentAvailabilityConstant->constant ? 1 : 0,
        subtreeJson.GetAllocator());
  }

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
  if (hasSubtreeAvailabilityBufferView) {
    subtreeAvailabilityObj.AddMember(
        "bitstream",
        bufferViewIndex++,
        subtreeJson.GetAllocator());
  } else {
    const auto* pSubtreeAvailabilityConstant =
        std::get_if<SubtreeAvailability::SubtreeConstantAvailability>(
            &subtreeContent.subtreeAvailability);
    assert(pSubtreeAvailabilityConstant);
    subtreeAvailabilityObj.AddMember(
        "constant",
        pSubtreeAvailabilityConstant->constant ? 1 : 0,
        subtreeJson.GetAllocator());
  }
  subtreeJson.AddMember(
      "childSubtreeAvailability",
      std::move(subtreeAvailabilityObj),
      subtreeJson.GetAllocator());

  return subtreeJson;
}

std::optional<SubtreeAvailability> mockLoadSubtreeJson(
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
      std::move(subtreeContent.buffers));
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
  SUBCASE("Availability stored in constant") {
    SubtreeAvailability subtreeAvailability{
        ImplicitTileSubdivisionScheme::Quadtree,
        5,
        SubtreeAvailability::SubtreeConstantAvailability{true},
        SubtreeAvailability::SubtreeConstantAvailability{false},
        {SubtreeAvailability::SubtreeConstantAvailability{false}},
        {}};

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
          libmorton::morton2D_64_encode(tileID.x, tileID.y),
          0));
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

  SUBCASE("Parse binary subtree") {
    // create subtree json
    auto subtreeBuffers = createSubtreeContent(
        maxSubtreeLevels,
        availableTileIDs,
        availableSubtreeIDs);
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

  SUBCASE("Parse binary subtree with mixed availability types") {
    // create subtree json
    auto subtreeContent =
        createSubtreeContent(maxSubtreeLevels, true, availableSubtreeIDs);
    auto subtreeJson = createSubtreeJson(subtreeContent, "");

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
        subtreeContent.buffers.data(),
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
      CHECK(parsedSubtree->isTileAvailable(tileID.level, mortonID));
      CHECK(parsedSubtree->isContentAvailable(tileID.level, mortonID, 0));
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

  SUBCASE("Parse binary subtree with constant availability only") {
    // create subtree json
    auto subtreeContent = createSubtreeContent(maxSubtreeLevels, true, false);
    auto subtreeJson = createSubtreeJson(subtreeContent, "");

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

    std::vector<std::byte> buffer(
        sizeof(subtreeHeader) + subtreeHeader.jsonByteLength +
        subtreeHeader.binaryByteLength);
    std::memcpy(buffer.data(), &subtreeHeader, sizeof(subtreeHeader));
    std::memcpy(
        buffer.data() + sizeof(subtreeHeader),
        subtreeJsonBuffer.GetString(),
        subtreeHeader.jsonByteLength);

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
      CHECK(parsedSubtree->isTileAvailable(tileID.level, mortonID));
      CHECK(parsedSubtree->isContentAvailable(tileID.level, mortonID, 0));
    }

    for (const auto& subtreeID : availableSubtreeIDs) {
      CHECK(!parsedSubtree->isSubtreeAvailable(
          libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
    }

    for (const auto& subtreeID : unavailableSubtreeIDs) {
      CHECK(!parsedSubtree->isSubtreeAvailable(
          libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
    }
  }

  SUBCASE("Parse json subtree") {
    auto subtreeBuffers = createSubtreeContent(
        maxSubtreeLevels,
        availableTileIDs,
        availableSubtreeIDs);
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

  SUBCASE("Parse json subtree with mixed availability types") {
    auto subtreeBuffers =
        createSubtreeContent(maxSubtreeLevels, availableTileIDs, false);
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
      CHECK(!parsedSubtree->isSubtreeAvailable(
          libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
    }

    for (const auto& subtreeID : unavailableSubtreeIDs) {
      CHECK(!parsedSubtree->isSubtreeAvailable(
          libmorton::morton2D_64_encode(subtreeID.x, subtreeID.y)));
    }
  }

  SUBCASE("Subtree json has ill form format") {
    auto subtreeBuffers = createSubtreeContent(
        maxSubtreeLevels,
        availableTileIDs,
        availableSubtreeIDs);
    auto subtreeJson = createSubtreeJson(subtreeBuffers, "buffer");

    SUBCASE("Subtree json has no tileAvailability field") {
      subtreeJson.RemoveMember("tileAvailability");
      CHECK(
          mockLoadSubtreeJson(
              maxSubtreeLevels,
              std::move(subtreeBuffers),
              std::move(subtreeJson)) == std::nullopt);
    }

    SUBCASE("Subtree json has no contentAvailability field") {
      subtreeJson.RemoveMember("contentAvailability");
      CHECK(
          mockLoadSubtreeJson(
              maxSubtreeLevels,
              std::move(subtreeBuffers),
              std::move(subtreeJson)) == std::nullopt);
    }

    SUBCASE("Subtree json has no childSubtreeAvailability field") {
      subtreeJson.RemoveMember("childSubtreeAvailability");
      CHECK(
          mockLoadSubtreeJson(
              maxSubtreeLevels,
              std::move(subtreeBuffers),
              std::move(subtreeJson)) == std::nullopt);
    }

    SUBCASE("Subtree json has no buffers though availability points to buffer "
            "view") {
      subtreeJson.RemoveMember("buffers");
      CHECK(
          mockLoadSubtreeJson(
              maxSubtreeLevels,
              std::move(subtreeBuffers),
              std::move(subtreeJson)) == std::nullopt);
    }

    SUBCASE("Buffer does not have byteLength field") {
      auto bufferIt = subtreeJson.FindMember("buffers");
      auto bufferObj = bufferIt->value.GetArray().Begin();
      bufferObj->RemoveMember("byteLength");
      CHECK(
          mockLoadSubtreeJson(
              maxSubtreeLevels,
              std::move(subtreeBuffers),
              std::move(subtreeJson)) == std::nullopt);
    }

    SUBCASE("Buffer does not have string uri field") {
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

    SUBCASE("Subtree json has no buffer views though availability points to "
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
