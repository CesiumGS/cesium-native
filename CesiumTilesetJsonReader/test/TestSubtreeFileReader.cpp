#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumJsonReader/JsonReader.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/SubtreeTestUtility.h>
#include <CesiumNativeTests/TestContext.h>
#include <CesiumNativeTests/waitForFuture.h>
#include <CesiumTilesetJson/Availability.h>
#include <CesiumTilesetJson/Buffer.h>
#include <CesiumTilesetJson/Subtree.h>
#include <CesiumTilesetJsonReader/SubtreeFileReader.h>

#include <doctest/doctest.h>
#include <rapidjson/document.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumJsonReader;
using namespace CesiumNativeTests;
using namespace CesiumTilesetJson;
using namespace CesiumTilesetJsonReader;

TEST_CASE("SubtreeFileReader") {
  SubtreeFileReader reader;

  SUBCASE("loads subtree from URL") {
    std::string subtreeJson = R"({
      "tileAvailability": {
        "constant": 1
      },
      "childSubtreeAvailability": {
        "constant": 0
      }
    })";
    std::vector<std::byte> data(subtreeJson.size());
    std::memcpy(data.data(), subtreeJson.data(), subtreeJson.size());

    std::map<std::string, std::shared_ptr<SimpleAssetRequest>> requestMap{
        {"subtree.json",
         createSuccessfulRequest(
             "GET",
             "test",
             CesiumAsync::HttpHeaders{},
             std::move(data))}};

    TestContext<SimpleTaskProcessor> context(std::move(requestMap));

    ReadJsonResult<Subtree> result = waitForFuture(
        context.asyncSystem,
        reader.load(
            context.asyncSystem,
            context.pAssetAccessor,
            "subtree.json",
            {}));
    REQUIRE_UNARY(result.value);
    CHECK(result.errors.empty());
    CHECK_EQ(result.value->tileAvailability.constant, 1);
    CHECK_EQ(result.value->childSubtreeAvailability.constant, 0);
  }

  SUBCASE("errors for URL with 404 response") {
    std::map<std::string, std::shared_ptr<SimpleAssetRequest>> requestMap{
        {"subtree.json",
         createFailedRequest("GET", "test", CesiumAsync::HttpHeaders{})}};
    TestContext<SimpleTaskProcessor> context(std::move(requestMap));

    ReadJsonResult<Subtree> result = waitForFuture(
        context.asyncSystem,
        reader.load(
            context.asyncSystem,
            context.pAssetAccessor,
            "subtree.json",
            {}));
    CHECK_UNARY_FALSE(result.value);
    CHECK_FALSE(result.errors.empty());
  }

  SUBCASE("loads subtree from a completed request") {
    std::string subtreeJson = R"({
      "tileAvailability": {
        "constant": 1
      },
      "childSubtreeAvailability": {
        "constant": 0
      }
    })";
    std::vector<std::byte> data(subtreeJson.size());
    std::memcpy(data.data(), subtreeJson.data(), subtreeJson.size());

    TestContext<SimpleTaskProcessor> context;
    std::shared_ptr<IAssetRequest> pRequest = createSuccessfulRequest(
        "GET",
        "subtree.json",
        CesiumAsync::HttpHeaders{},
        std::move(data));

    ReadJsonResult<Subtree> result = waitForFuture(
        context.asyncSystem,
        reader.load(context.asyncSystem, context.pAssetAccessor, pRequest));
    REQUIRE_UNARY(result.value);
    CHECK(result.errors.empty());
    CHECK_EQ(result.value->tileAvailability.constant, 1);
    CHECK_EQ(result.value->childSubtreeAvailability.constant, 0);
  }

  SUBCASE("errors for request without a response") {
    TestContext<SimpleTaskProcessor> context;
    std::shared_ptr<IAssetRequest> pRequest =
        createFailedRequest("GET", "subtree.json", CesiumAsync::HttpHeaders{});

    ReadJsonResult<Subtree> result = waitForFuture(
        context.asyncSystem,
        reader.load(context.asyncSystem, context.pAssetAccessor, pRequest));
    CHECK_UNARY_FALSE(result.value);
    CHECK_FALSE(result.errors.empty());
  }

  SUBCASE("loads subtree from data buffer with JSON payload") {
    std::string subtreeJson = R"({
      "tileAvailability": {
        "constant": 1
      },
      "childSubtreeAvailability": {
        "constant": 0
      }
    })";
    TestContext<SimpleTaskProcessor> context;
    ReadJsonResult<Subtree> result = waitForFuture(
        context.asyncSystem,
        reader.load(
            context.asyncSystem,
            context.pAssetAccessor,
            "subtree.json",
            {},
            std::span<const std::byte>(
                reinterpret_cast<std::byte*>(subtreeJson.data()),
                subtreeJson.size())));
    REQUIRE_UNARY(result.value);
    CHECK(result.errors.empty());
    CHECK_EQ(result.value->tileAvailability.constant, 1);
    CHECK_EQ(result.value->childSubtreeAvailability.constant, 0);
  }

  SUBCASE("loads external buffers") {
    std::string subtreeJson = R"({
      "tileAvailability": {
        "constant": 1
      },
      "childSubtreeAvailability": {
        "constant": 0
      },
      "buffers": [
        {
          "uri": "buffer.bin",
          "byteLength": 8
        },
        {
          "uri": "buffer2.bin",
          "byteLength": 4
        } 
      ]
    })";

    std::vector<std::byte> bufferData(8, std::byte(0));
    std::vector<std::byte> buffer2Data(4, std::byte(1));
    std::map<std::string, std::shared_ptr<SimpleAssetRequest>> requestMap{
        {"http://example.com/buffer.bin",
         createSuccessfulRequest(
             "GET",
             "http://example.com/buffer.bin",
             CesiumAsync::HttpHeaders{},
             std::vector<std::byte>(bufferData))},
        {"http://example.com/buffer2.bin",
         createSuccessfulRequest(
             "GET",
             "http://example.com/buffer2.bin",
             CesiumAsync::HttpHeaders{},
             std::vector<std::byte>(buffer2Data))}};
    TestContext<SimpleTaskProcessor> context(std::move(requestMap));

    ReadJsonResult<Subtree> result = waitForFuture(
        context.asyncSystem,
        reader.load(
            context.asyncSystem,
            context.pAssetAccessor,
            "http://example.com/subtree.json",
            {},
            std::span<const std::byte>(
                reinterpret_cast<std::byte*>(subtreeJson.data()),
                subtreeJson.size())));

    REQUIRE_UNARY(result.value);
    REQUIRE_EQ(result.value->buffers.size(), 2);
    CHECK_EQ(result.value->buffers[0].cesium.data, bufferData);
    CHECK_EQ(result.value->buffers[0].byteLength, 8);
    CHECK_EQ(result.value->buffers[1].cesium.data, buffer2Data);
    CHECK_EQ(result.value->buffers[1].byteLength, 4);
    CHECK(result.warnings.empty());
  }

  SUBCASE("shrinks byteLength when the external buffer is smaller") {
    std::string subtreeJson = R"({
      "tileAvailability": {
        "constant": 1
      },
      "childSubtreeAvailability": {
        "constant": 0
      },
      "buffers": [
        {
          "uri": "buffer.bin",
          "byteLength": 8
        }
      ]
    })";

    std::map<std::string, std::shared_ptr<SimpleAssetRequest>> requestMap{
        {"buffer.bin",
         createSuccessfulRequest(
             "GET",
             "buffer.bin",
             CesiumAsync::HttpHeaders{},
             std::vector<std::byte>(4, std::byte(1)))}};
    TestContext<SimpleTaskProcessor> context(std::move(requestMap));

    ReadJsonResult<Subtree> result = waitForFuture(
        context.asyncSystem,
        reader.load(
            context.asyncSystem,
            context.pAssetAccessor,
            "subtree.json",
            {},
            std::span<const std::byte>(
                reinterpret_cast<std::byte*>(subtreeJson.data()),
                subtreeJson.size())));
    REQUIRE_UNARY(result.value);
    REQUIRE_EQ(result.value->buffers.size(), 1);
    CHECK_EQ(result.value->buffers[0].byteLength, 4);
    CHECK_EQ(result.value->buffers[0].cesium.data.size(), 4);
    CHECK_FALSE(result.warnings.empty());
  }

  SUBCASE("errors for empty data buffer") {
    TestContext<SimpleTaskProcessor> context;
    ReadJsonResult<Subtree> result = waitForFuture(
        context.asyncSystem,
        reader.load(
            context.asyncSystem,
            context.pAssetAccessor,
            "subtree.json",
            {},
            std::span<const std::byte>()));
    CHECK_UNARY_FALSE(result.value);
    CHECK_FALSE(result.errors.empty());
  }

  SUBCASE("errors for data buffer with invalid JSON") {
    std::string badInput = "not JSON";
    TestContext<SimpleTaskProcessor> context;
    ReadJsonResult<Subtree> result = waitForFuture(
        context.asyncSystem,
        reader.load(
            context.asyncSystem,
            context.pAssetAccessor,
            "subtree.json",
            {},
            std::span<const std::byte>(
                reinterpret_cast<std::byte*>(badInput.data()),
                badInput.size())));
    CHECK_UNARY_FALSE(result.value);
    CHECK_FALSE(result.errors.empty());
  }

  SUBCASE("loads subtree from data buffer with binary payload") {
    // Create a subtree JSON, then serialize it into binary
    uint32_t maxSubtreeLevel = 1;
    std::vector<CesiumGeometry::QuadtreeTileID> availableTileIDs{
        CesiumGeometry::QuadtreeTileID{0, 0, 0},
        CesiumGeometry::QuadtreeTileID{1, 0, 0},
        CesiumGeometry::QuadtreeTileID{1, 1, 0}};

    auto subtreeContent =
        SubtreeContent(maxSubtreeLevel, availableTileIDs, false);

    TestContext<SimpleTaskProcessor> context;
    ReadJsonResult<Subtree> result = waitForFuture(
        context.asyncSystem,
        reader.load(
            context.asyncSystem,
            context.pAssetAccessor,
            "test",
            {},
            subtreeContent.toBinary()));
    REQUIRE_UNARY(result.value);
    CHECK(result.errors.empty());
    CHECK_EQ(result.value->bufferViews.size(), 2);

    // tileAvailability and contentAvailability are written with the specified
    // availableTileIDs, and both are output to the same buffer.
    std::vector<std::byte> expected{std::byte(7), std::byte(7)};
    REQUIRE_EQ(result.value->buffers.size(), 1);
    CHECK_EQ(result.value->buffers[0].cesium.data, expected);
  }

  SUBCASE("loads a binary subtree without a binary chunk") {
    auto subtreeContent = SubtreeContent(2, true, false);

    TestContext<SimpleTaskProcessor> context;
    ReadJsonResult<Subtree> result = waitForFuture(
        context.asyncSystem,
        reader.load(
            context.asyncSystem,
            context.pAssetAccessor,
            "test",
            {},
            subtreeContent.toBinary()));
    REQUIRE(result.value);
    CHECK(result.errors.empty());
  }

  SUBCASE("rejects data smaller than the header") {
    std::string badInput = "subt0123456";
    std::vector<std::byte> data(badInput.size());
    std::memcpy(data.data(), badInput.data(), badInput.size());

    TestContext<SimpleTaskProcessor> context;
    ReadJsonResult<Subtree> result = waitForFuture(
        context.asyncSystem,
        reader.load(
            context.asyncSystem,
            context.pAssetAccessor,
            "test",
            {},
            data));
    CHECK_UNARY_FALSE(result.value);
    CHECK_FALSE(result.errors.empty());
  }

  SUBCASE("rejects a jsonByteLength that exceeds the data") {
    auto subtreeContent = SubtreeContent(2, true, false);
    std::vector<std::byte> data = subtreeContent.toBinary();

    uint64_t* pJsonByteLength = reinterpret_cast<uint64_t*>(
        data.data() + sizeof(char[4]) + sizeof(uint32_t));
    *pJsonByteLength = 1000;

    TestContext<SimpleTaskProcessor> context;
    ReadJsonResult<Subtree> result = waitForFuture(
        context.asyncSystem,
        reader.load(
            context.asyncSystem,
            context.pAssetAccessor,
            "test",
            {},
            data));
    CHECK_UNARY_FALSE(result.value);
    CHECK_FALSE(result.errors.empty());
  }

  SUBCASE("rejects a binaryByteLength that exceeds the data") {
    auto subtreeContent = SubtreeContent(2, true, false);
    std::vector<std::byte> data = subtreeContent.toBinary();

    uint64_t* pBinaryByteLength = reinterpret_cast<uint64_t*>(
        data.data() + sizeof(char[4]) + sizeof(uint32_t) + sizeof(uint64_t));
    *pBinaryByteLength = 1000;

    TestContext<SimpleTaskProcessor> context;
    ReadJsonResult<Subtree> result = waitForFuture(
        context.asyncSystem,
        reader.load(
            context.asyncSystem,
            context.pAssetAccessor,
            "test",
            {},
            data));
    CHECK_UNARY_FALSE(result.value);
    CHECK_FALSE(result.errors.empty());
  }

  SUBCASE("load subtree from a rapidjson document") {
    std::string subtreeJson = R"({
      "tileAvailability": {
        "constant": 1
      },
      "childSubtreeAvailability": {
        "constant": 0
      }
    })";
    rapidjson::Document document;
    document.Parse(subtreeJson.c_str());
    REQUIRE_FALSE(document.HasParseError());

    TestContext<SimpleTaskProcessor> context;
    ReadJsonResult<Subtree> result = waitForFuture(
        context.asyncSystem,
        reader.load(
            context.asyncSystem,
            context.pAssetAccessor,
            "subtree.json",
            {},
            document));
    REQUIRE_UNARY(result.value);
    CHECK(result.errors.empty());
    CHECK_EQ(result.value->tileAvailability.constant, 1);
    CHECK_EQ(result.value->childSubtreeAvailability.constant, 0);
  }

  SUBCASE("loads external buffers for a document") {
    std::string subtreeJson = R"({
      "tileAvailability": {
        "constant": 1
      },
      "childSubtreeAvailability": {
        "constant": 0
      },
      "buffers": [
        {
          "uri": "buffer.bin",
          "byteLength": 8
        },
        {
          "uri": "buffer2.bin",
          "byteLength": 4
        } 
      ]
    })";

    rapidjson::Document document;
    document.Parse(subtreeJson.c_str());
    REQUIRE(!document.HasParseError());

    std::vector<std::byte> bufferData(8, std::byte(0));
    std::vector<std::byte> buffer2Data(4, std::byte(1));
    std::map<std::string, std::shared_ptr<SimpleAssetRequest>> requestMap{
        {"http://example.com/buffer.bin",
         createSuccessfulRequest(
             "GET",
             "http://example.com/buffer.bin",
             CesiumAsync::HttpHeaders{},
             std::vector<std::byte>(bufferData))},
        {"http://example.com/buffer2.bin",
         createSuccessfulRequest(
             "GET",
             "http://example.com/buffer2.bin",
             CesiumAsync::HttpHeaders{},
             std::vector<std::byte>(buffer2Data))}};
    TestContext<SimpleTaskProcessor> context(std::move(requestMap));

    ReadJsonResult<Subtree> result = waitForFuture(
        context.asyncSystem,
        reader.load(
            context.asyncSystem,
            context.pAssetAccessor,
            "http://example.com/subtree.json",
            {},
            document));
    REQUIRE_UNARY(result.value);
    REQUIRE_EQ(result.value->buffers.size(), 2);
    CHECK_EQ(result.value->buffers[0].cesium.data.size(), 8);
    CHECK_EQ(result.value->buffers[1].cesium.data.size(), 4);
  }

  SUBCASE("rejects a null document") {
    TestContext<SimpleTaskProcessor> context;
    rapidjson::Document document;
    ReadJsonResult<Subtree> result = waitForFuture(
        context.asyncSystem,
        reader.load(
            context.asyncSystem,
            context.pAssetAccessor,
            "subtree.json",
            {},
            document));
    CHECK_UNARY_FALSE(result.value);
    CHECK_FALSE(result.errors.empty());
  }
}
