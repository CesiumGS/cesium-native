#include "MockAssetRequest.h"
#include "MockAssetResponse.h"
#include "ResponseCacheControl.h"

#include <CesiumAsync/CacheItem.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/SqliteCache.h>

#include <doctest/doctest.h>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace CesiumAsync;

TEST_CASE("Test disk cache with Sqlite") {
  SqliteCache diskCache(spdlog::default_logger(), "test.db", 3);

  REQUIRE(diskCache.clearAll());

  SUBCASE("Test store and retrive cache") {
    HttpHeaders responseHeaders = {
        {"Response-Header", "Response-Value"},
        {"Content-Type", "text/html"}};
    std::vector<std::byte> responseData =
        {std::byte(0), std::byte(1), std::byte(2), std::byte(3), std::byte(4)};
    std::unique_ptr<MockAssetResponse> response =
        std::make_unique<MockAssetResponse>(
            static_cast<uint16_t>(200),
            "text/html",
            responseHeaders,
            responseData);

    HttpHeaders requestHeaders{{"Request-Header", "Request-Value"}};
    std::unique_ptr<MockAssetRequest> request =
        std::make_unique<MockAssetRequest>(
            "GET",
            "test.com",
            requestHeaders,
            std::move(response));

    std::time_t currentTime = std::time(nullptr);
    REQUIRE(diskCache.storeEntry(
        "TestKey",
        currentTime,
        request->url(),
        request->method(),
        request->headers(),
        request->response()->statusCode(),
        request->response()->headers(),
        request->response()->data()));

    std::optional<CacheItem> cacheItem = diskCache.getEntry("TestKey");
    REQUIRE(cacheItem->expiryTime == currentTime);

    const CacheRequest& cacheRequest = cacheItem->cacheRequest;
    REQUIRE(
        cacheRequest.headers ==
        HttpHeaders{{"Request-Header", "Request-Value"}});
    REQUIRE(cacheRequest.method == "GET");
    REQUIRE(cacheRequest.url == "test.com");

    const CacheResponse& cacheResponse = cacheItem->cacheResponse;
    REQUIRE(cacheResponse.headers.at("Content-Type") == "text/html");
    REQUIRE(cacheResponse.statusCode == 200);
    REQUIRE(cacheResponse.headers.at("Response-Header") == "Response-Value");
    REQUIRE(
        cacheResponse.data == std::vector<std::byte>{
                                  std::byte(0),
                                  std::byte(1),
                                  std::byte(2),
                                  std::byte(3),
                                  std::byte(4)});

    std::optional<ResponseCacheControl> cacheControl =
        ResponseCacheControl::parseFromResponseHeaders(cacheResponse.headers);
    REQUIRE(!cacheControl.has_value());
  }

  SUBCASE("Test prune") {
    // store data in the cache first
    std::time_t currentTime = std::time(nullptr);
    std::time_t interval = -10;
    for (size_t i = 0; i < 20; ++i) {
      HttpHeaders responseHeaders{
          {"Response-Header-" + std::to_string(i),
           "Response-Value-" + std::to_string(i)},
          {"Content-Type", "text/html"},
          {"Cache-Control",
           "must-revalidate, no-store, public, proxy-revalidate"}};
      std::vector<std::byte> responseData = {
          std::byte(0),
          std::byte(1),
          std::byte(2),
          std::byte(3),
          std::byte(4)};
      std::unique_ptr<MockAssetResponse> response =
          std::make_unique<MockAssetResponse>(
              static_cast<uint16_t>(200),
              "text/html",
              responseHeaders,
              responseData);

      HttpHeaders requestHeaders{
          {"Request-Header-" + std::to_string(i),
           "Request-Value-" + std::to_string(i)}};
      std::unique_ptr<MockAssetRequest> request =
          std::make_unique<MockAssetRequest>(
              "GET",
              "test.com",
              requestHeaders,
              std::move(response));

      REQUIRE(diskCache.storeEntry(
          "TestKey" + std::to_string(i),
          currentTime + interval + static_cast<std::time_t>(i),
          request->url(),
          request->method(),
          request->headers(),
          request->response()->statusCode(),
          request->response()->headers(),
          request->response()->data()));
    }

    REQUIRE(diskCache.prune());
    for (int i = 0; i <= 16; ++i) {
      std::optional<CacheItem> cacheItem =
          diskCache.getEntry("TestKey" + std::to_string(i));
      REQUIRE(cacheItem == std::nullopt);
    }

    for (int i = 17; i < 20; ++i) {
      std::optional<CacheItem> cacheItem =
          diskCache.getEntry("TestKey" + std::to_string(i));
      REQUIRE(cacheItem != std::nullopt);

      // make sure the item is still in there
      REQUIRE(cacheItem->expiryTime == currentTime + interval + i);

      const CacheRequest& cacheRequest = cacheItem->cacheRequest;
      REQUIRE(
          cacheRequest.headers == HttpHeaders{
                                      {"Request-Header-" + std::to_string(i),
                                       "Request-Value-" + std::to_string(i)}});
      REQUIRE(cacheRequest.method == "GET");
      REQUIRE(cacheRequest.url == "test.com");

      const CacheResponse& cacheResponse = cacheItem->cacheResponse;
      REQUIRE(cacheResponse.headers.at("Content-Type") == "text/html");
      REQUIRE(cacheResponse.statusCode == 200);
      REQUIRE(
          cacheResponse.headers.at("Response-Header-" + std::to_string(i)) ==
          "Response-Value-" + std::to_string(i));
      REQUIRE(
          cacheResponse.data == std::vector<std::byte>{
                                    std::byte(0),
                                    std::byte(1),
                                    std::byte(2),
                                    std::byte(3),
                                    std::byte(4)});

      std::optional<ResponseCacheControl> cacheControl =
          ResponseCacheControl::parseFromResponseHeaders(cacheResponse.headers);
      REQUIRE(cacheControl.has_value());
      REQUIRE(cacheControl->mustRevalidate() == true);
      REQUIRE(cacheControl->noCache() == false);
      REQUIRE(cacheControl->noStore() == true);
      REQUIRE(cacheControl->noTransform() == false);
      REQUIRE(cacheControl->accessControlPublic() == true);
      REQUIRE(cacheControl->accessControlPrivate() == false);
      REQUIRE(cacheControl->proxyRevalidate() == true);
      REQUIRE(cacheControl->maxAgeExists() == false);
      REQUIRE(cacheControl->sharedMaxAgeExists() == false);
    }
  }

  SUBCASE("Test clear all") {
    // store data in the cache first
    HttpHeaders responseHeaders{
        {"Content-Type", "text/html"},
        {"Response-Header", "Response-Value"}};
    std::vector<std::byte> responseData =
        {std::byte(0), std::byte(1), std::byte(2), std::byte(3), std::byte(4)};
    std::unique_ptr<MockAssetResponse> response =
        std::make_unique<MockAssetResponse>(
            static_cast<uint16_t>(200),
            "text/html",
            responseHeaders,
            responseData);

    HttpHeaders requestHeaders{{"Request-Header", "Request-Value"}};
    std::unique_ptr<MockAssetRequest> request =
        std::make_unique<MockAssetRequest>(
            "GET",
            "test.com",
            requestHeaders,
            std::move(response));

    for (size_t i = 0; i < 10; ++i) {
      REQUIRE(diskCache.storeEntry(
          "TestKey" + std::to_string(i),
          std::time(nullptr),
          request->url(),
          request->method(),
          request->headers(),
          request->response()->statusCode(),
          request->response()->headers(),
          request->response()->data()));
    }

    // clear all
    REQUIRE(diskCache.clearAll());
    for (size_t i = 0; i < 10; ++i) {
      std::optional<CacheItem> cacheItem =
          diskCache.getEntry("TestKey" + std::to_string(i));
      REQUIRE(cacheItem == std::nullopt);
    }
  }
}
