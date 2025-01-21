#include "MockAssetAccessor.h"
#include "MockAssetRequest.h"
#include "MockAssetResponse.h"
#include "MockTaskProcessor.h"
#include "ResponseCacheControl.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/CacheItem.h>
#include <CesiumAsync/CachingAssetAccessor.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumAsync/ICacheDatabase.h>

#include <doctest/doctest.h>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

using namespace CesiumAsync;

namespace {

class MockStoreCacheDatabase : public ICacheDatabase {
public:
  struct StoreRequestParameters {
    std::string key;
    std::time_t expiryTime;
    std::string url;
    std::string requestMethod;
    HttpHeaders requestHeaders;
    uint16_t statusCode;
    HttpHeaders responseHeaders;
    std::vector<std::byte> responseData;
  };

  MockStoreCacheDatabase()
      : getEntryCall{false},
        storeResponseCall{false},
        pruneCall{false},
        clearAllCall{false} {}

  virtual std::optional<CacheItem>
  getEntry(const std::string& /*key*/) const override {
    this->getEntryCall = true;
    return this->cacheItem;
  }

  virtual bool storeEntry(
      const std::string& key,
      std::time_t expiryTime,
      const std::string& url,
      const std::string& requestMethod,
      const HttpHeaders& requestHeaders,
      uint16_t statusCode,
      const HttpHeaders& responseHeaders,
      const std::span<const std::byte>& responseData) override {
    this->storeRequestParam = StoreRequestParameters{
        key,
        expiryTime,
        url,
        requestMethod,
        requestHeaders,
        statusCode,
        responseHeaders,
        std::vector<std::byte>(responseData.begin(), responseData.end())};
    this->storeResponseCall = true;
    return true;
  }

  virtual bool prune() override {
    this->pruneCall = true;
    return true;
  }

  virtual bool clearAll() override {
    this->clearAllCall = true;
    return true;
  }

  mutable bool getEntryCall;
  bool storeResponseCall;
  bool pruneCall;
  bool clearAllCall;

  std::optional<StoreRequestParameters> storeRequestParam;
  std::optional<CacheItem> cacheItem;
};

} // namespace

bool runResponseCacheTest(
    int statusCode,
    const std::string& methodStr,
    const HttpHeaders& httpHeaders) {
  std::unique_ptr<IAssetResponse> mockResponse =
      std::make_unique<MockAssetResponse>(
          static_cast<uint16_t>(statusCode),
          "app/json",
          httpHeaders,
          std::vector<std::byte>());

  std::shared_ptr<IAssetRequest> mockRequest =
      std::make_shared<MockAssetRequest>(
          methodStr,
          "test.com",
          HttpHeaders{},
          std::move(mockResponse));

  std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase =
      std::make_unique<MockStoreCacheDatabase>();
  MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();

  std::shared_ptr<CachingAssetAccessor> cacheAssetAccessor =
      std::make_shared<CachingAssetAccessor>(
          spdlog::default_logger(),
          std::make_unique<MockAssetAccessor>(mockRequest),
          std::move(ownedMockCacheDatabase));

  std::shared_ptr<MockTaskProcessor> mockTaskProcessor =
      std::make_shared<MockTaskProcessor>();

  AsyncSystem asyncSystem(mockTaskProcessor);
  cacheAssetAccessor
      ->get(asyncSystem, "test.com", std::vector<IAssetAccessor::THeader>{})
      .wait();

  return mockCacheDatabase->storeResponseCall;
}

TEST_CASE("Test the condition of caching the request") {
  SUBCASE("Cache request") {
    SUBCASE("GET request, has max-age, cacheable status code") {
      std::vector<int> statusCodes{200, 202, 203, 204, 205, 304};

      for (auto& statusCode : statusCodes) {
        HttpHeaders headers = {
            {"Content-Type", "app/json"},
            {"Cache-Control", "must-revalidate, max-age=100"}};

        bool responseCached = runResponseCacheTest(statusCode, "GET", headers);
        CHECK(responseCached == true);
      }
    }

    SUBCASE("GET Request, has Expires header, cacheable status code") {
      std::vector<int> statusCodes{200, 202, 203, 204, 205, 304};

      for (auto& statusCode : statusCodes) {

        HttpHeaders headers = {
            {"Content-Type", "app/json"},
            {"Expires", "Wed, 21 Oct 5020 07:28:00 GMT"}};

        bool responseCached = runResponseCacheTest(statusCode, "GET", headers);
        CHECK(responseCached == true);
      }
    }

    SUBCASE("GET Request, max-age 0, old Expires header") {
      std::vector<int> statusCodes{200, 202, 203, 204, 205, 304};

      for (auto& statusCode : statusCodes) {

        // Similar to Google Photorealistic 3D Tiles, root request
        HttpHeaders headers = {
            {"Content-Type", "application/json"},
            {"Cache-Control", "private, max-age=0, must-revalidate"},
            {"ETag", "deadbeef"},
            {"Expires", "Mon, 01 Jan 1990 00:00:00 GMT"}};

        bool responseCached = runResponseCacheTest(statusCode, "GET", headers);
        CHECK(responseCached == true);
      }
    }

    SUBCASE("GET Request, max-age 0, stale-while-revalidate") {
      std::vector<int> statusCodes{200, 202, 203, 204, 205, 304};

      for (auto& statusCode : statusCodes) {

        // Similar to Google Photorealistic 3D Tiles, tile request
        HttpHeaders headers = {
            {"Content-Type", "application/json"},
            {"Cache-Control",
             "private, max-age=0, stale-while-revalidate=86400"},
            {"ETag", "deadbeef"}};

        bool responseCached = runResponseCacheTest(statusCode, "GET", headers);
        CHECK(responseCached == true);
      }
    }

    SUBCASE("GET Request, no-cache with Etag") {
      std::vector<int> statusCodes{200, 202, 203, 204, 205, 304};

      for (auto& statusCode : statusCodes) {

        HttpHeaders headers = {
            {"Content-Type", "application/json"},
            {"Cache-Control", "no-cache"},
            {"ETag", "deadbeef"}};

        bool responseCached = runResponseCacheTest(statusCode, "GET", headers);
        CHECK(responseCached == true);
      }
    }

    SUBCASE("GET Request, no-cache with Last-Modified") {
      std::vector<int> statusCodes{200, 202, 203, 204, 205, 304};

      for (auto& statusCode : statusCodes) {

        HttpHeaders headers = {
            {"Content-Type", "application/json"},
            {"Cache-Control", "no-cache"},
            {"Last-Modified", "Mon, 01 Jan 1990 00:00:00 GMT"}};

        bool responseCached = runResponseCacheTest(statusCode, "GET", headers);
        CHECK(responseCached == true);
      }
    }

    SUBCASE("GET Request, just Last-Modified") {
      std::vector<int> statusCodes{200, 202, 203, 204, 205, 304};

      for (auto& statusCode : statusCodes) {

        HttpHeaders headers = {
            {"Content-Type", "application/json"},
            {"Last-Modified", "Mon, 01 Jan 1990 00:00:00 GMT"}};

        bool responseCached = runResponseCacheTest(statusCode, "GET", headers);
        CHECK(responseCached == true);
      }
    }

    SUBCASE("GET Request, just Etag") {
      std::vector<int> statusCodes{200, 202, 203, 204, 205, 304};

      for (auto& statusCode : statusCodes) {

        HttpHeaders headers = {
            {"Content-Type", "application/json"},
            {"ETag", "deadbeef"}};

        bool responseCached = runResponseCacheTest(statusCode, "GET", headers);
        CHECK(responseCached == true);
      }
    }

    SUBCASE(
        "GET Request, Expires header is less than current, but has an ETag") {
      std::vector<int> statusCodes{200, 202, 203, 204, 205, 304};

      for (auto& statusCode : statusCodes) {

        HttpHeaders headers = {
            {"Content-Type", "application/json"},
            {"ETag", "deadbeef"},
            {"Expires", "Wed, 21 Oct 2010 07:28:00 GMT"}};

        bool responseCached = runResponseCacheTest(statusCode, "GET", headers);
        CHECK(responseCached == true);
      }
    }

    SUBCASE("GET Request, Expires header is less than current, but has a "
            "Last-Modified") {
      std::vector<int> statusCodes{200, 202, 203, 204, 205, 304};

      for (auto& statusCode : statusCodes) {

        HttpHeaders headers = {
            {"Content-Type", "application/json"},
            {"Last-Modified", "Mon, 01 Jan 1990 00:00:00 GMT"},
            {"Expires", "Wed, 21 Oct 2010 07:28:00 GMT"}};

        bool responseCached = runResponseCacheTest(statusCode, "GET", headers);
        CHECK(responseCached == true);
      }
    }

    SUBCASE("GET Request, max-age is zero, but has an ETag") {
      std::vector<int> statusCodes{200, 202, 203, 204, 205, 304};

      for (auto& statusCode : statusCodes) {

        HttpHeaders headers = {
            {"Content-Type", "application/json"},
            {"ETag", "deadbeef"},
            {"Cache-Control", "max-age=0"}};

        bool responseCached = runResponseCacheTest(statusCode, "GET", headers);
        CHECK(responseCached == true);
      }
    }

    SUBCASE("GET Request, max-age is zero, but has a Last-Modified") {
      std::vector<int> statusCodes{200, 202, 203, 204, 205, 304};

      for (auto& statusCode : statusCodes) {

        HttpHeaders headers = {
            {"Content-Type", "application/json"},
            {"Last-Modified", "Mon, 01 Jan 1990 00:00:00 GMT"},
            {"Cache-Control", "max-age=0"}};

        bool responseCached = runResponseCacheTest(statusCode, "GET", headers);
        CHECK(responseCached == true);
      }
    }
  }

  SUBCASE("No cache condition") {
    SUBCASE("No store for response that doesn't have GET method") {
      std::unique_ptr<IAssetResponse> mockResponse =
          std::make_unique<MockAssetResponse>(
              static_cast<uint16_t>(200),
              "app/json",
              HttpHeaders{
                  {"Content-Type", "app/json"},
                  {"Cache-Control",
                   "must-revalidate, max-age=100, public, private"}},
              std::vector<std::byte>());

      std::shared_ptr<IAssetRequest> mockRequest =
          std::make_shared<MockAssetRequest>(
              "POST",
              "test.com",
              HttpHeaders{},
              std::move(mockResponse));

      std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase =
          std::make_unique<MockStoreCacheDatabase>();
      MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();
      std::shared_ptr<CachingAssetAccessor> cacheAssetAccessor =
          std::make_shared<CachingAssetAccessor>(
              spdlog::default_logger(),
              std::make_unique<MockAssetAccessor>(mockRequest),
              std::move(ownedMockCacheDatabase));
      std::shared_ptr<MockTaskProcessor> mockTaskProcessor =
          std::make_shared<MockTaskProcessor>();

      AsyncSystem asyncSystem(mockTaskProcessor);
      cacheAssetAccessor
          ->get(asyncSystem, "test.com", std::vector<IAssetAccessor::THeader>{})
          .wait();
      REQUIRE(mockCacheDatabase->storeResponseCall == false);
    }

    SUBCASE("No store for response that has no cacheable status code") {
      std::unique_ptr<IAssetResponse> mockResponse =
          std::make_unique<MockAssetResponse>(
              static_cast<uint16_t>(404),
              "app/json",
              HttpHeaders{
                  {"Content-Type", "app/json"},
                  {"Cache-Control",
                   "must-revalidate, public, private, max-age=100"}},
              std::vector<std::byte>());

      std::shared_ptr<IAssetRequest> mockRequest =
          std::make_shared<MockAssetRequest>(
              "GET",
              "test.com",
              HttpHeaders{},
              std::move(mockResponse));

      std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase =
          std::make_unique<MockStoreCacheDatabase>();
      MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();
      std::shared_ptr<CachingAssetAccessor> cacheAssetAccessor =
          std::make_shared<CachingAssetAccessor>(
              spdlog::default_logger(),
              std::make_unique<MockAssetAccessor>(mockRequest),
              std::move(ownedMockCacheDatabase));
      std::shared_ptr<MockTaskProcessor> mockTaskProcessor =
          std::make_shared<MockTaskProcessor>();

      AsyncSystem asyncSystem(mockTaskProcessor);
      cacheAssetAccessor
          ->get(asyncSystem, "test.com", std::vector<IAssetAccessor::THeader>{})
          .wait();
      REQUIRE(mockCacheDatabase->storeResponseCall == false);
    }

    SUBCASE(
        "No store for response that has No-Store in the cache-control header") {
      std::unique_ptr<IAssetResponse> mockResponse =
          std::make_unique<MockAssetResponse>(
              static_cast<uint16_t>(200),
              "app/json",
              HttpHeaders{
                  {"Content-Type", "app/json"},
                  {"Cache-Control", "no-store"}},
              std::vector<std::byte>());

      std::shared_ptr<IAssetRequest> mockRequest =
          std::make_shared<MockAssetRequest>(
              "GET",
              "test.com",
              HttpHeaders{},
              std::move(mockResponse));

      std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase =
          std::make_unique<MockStoreCacheDatabase>();
      MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();
      std::shared_ptr<CachingAssetAccessor> cacheAssetAccessor =
          std::make_shared<CachingAssetAccessor>(
              spdlog::default_logger(),
              std::make_unique<MockAssetAccessor>(mockRequest),
              std::move(ownedMockCacheDatabase));
      std::shared_ptr<MockTaskProcessor> mockTaskProcessor =
          std::make_shared<MockTaskProcessor>();

      AsyncSystem asyncSystem(mockTaskProcessor);
      cacheAssetAccessor
          ->get(asyncSystem, "test.com", std::vector<IAssetAccessor::THeader>{})
          .wait();
      REQUIRE(mockCacheDatabase->storeResponseCall == false);
    }

    SUBCASE(
        "No store for response that has No-Cache in the cache-control header") {
      std::unique_ptr<IAssetResponse> mockResponse =
          std::make_unique<MockAssetResponse>(
              static_cast<uint16_t>(200),
              "app/json",
              HttpHeaders{
                  {"Content-Type", "app/json"},
                  {"Cache-Control", "must-revalidate, no-cache"}},
              std::vector<std::byte>());

      std::shared_ptr<IAssetRequest> mockRequest =
          std::make_shared<MockAssetRequest>(
              "GET",
              "test.com",
              HttpHeaders{},
              std::move(mockResponse));

      std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase =
          std::make_unique<MockStoreCacheDatabase>();
      MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();
      std::shared_ptr<CachingAssetAccessor> cacheAssetAccessor =
          std::make_shared<CachingAssetAccessor>(
              spdlog::default_logger(),
              std::make_unique<MockAssetAccessor>(mockRequest),
              std::move(ownedMockCacheDatabase));
      std::shared_ptr<MockTaskProcessor> mockTaskProcessor =
          std::make_shared<MockTaskProcessor>();

      AsyncSystem asyncSystem(mockTaskProcessor);
      cacheAssetAccessor
          ->get(asyncSystem, "test.com", std::vector<IAssetAccessor::THeader>{})
          .wait();

      //
      // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Cache-Control
      //
      // The no-cache response directive indicates that the response can be
      // stored in caches, but the response must be validated with the origin
      // server before each reuse, even when the cache is disconnected from the
      // origin server.

      REQUIRE(mockCacheDatabase->storeResponseCall == false);
    }

    SUBCASE(
        "No store for response that has no Cache-Control and Expires header") {
      std::unique_ptr<IAssetResponse> mockResponse =
          std::make_unique<MockAssetResponse>(
              static_cast<uint16_t>(200),
              "app/json",
              HttpHeaders{{"Content-Type", "app/json"}},
              std::vector<std::byte>());

      std::shared_ptr<IAssetRequest> mockRequest =
          std::make_shared<MockAssetRequest>(
              "GET",
              "test.com",
              HttpHeaders{},
              std::move(mockResponse));

      std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase =
          std::make_unique<MockStoreCacheDatabase>();
      MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();
      std::shared_ptr<CachingAssetAccessor> cacheAssetAccessor =
          std::make_shared<CachingAssetAccessor>(
              spdlog::default_logger(),
              std::make_unique<MockAssetAccessor>(mockRequest),
              std::move(ownedMockCacheDatabase));
      std::shared_ptr<MockTaskProcessor> mockTaskProcessor =
          std::make_shared<MockTaskProcessor>();

      AsyncSystem asyncSystem(mockTaskProcessor);
      cacheAssetAccessor
          ->get(asyncSystem, "test.com", std::vector<IAssetAccessor::THeader>{})
          .wait();
      REQUIRE(mockCacheDatabase->storeResponseCall == false);
    }

    SUBCASE("No store if Expires header is less than the current") {
      std::unique_ptr<IAssetResponse> mockResponse =
          std::make_unique<MockAssetResponse>(
              static_cast<uint16_t>(200),
              "app/json",
              HttpHeaders{
                  {"Content-Type", "app/json"},
                  {"Expires", "Wed, 21 Oct 2010 07:28:00 GMT"}},
              std::vector<std::byte>());

      std::shared_ptr<IAssetRequest> mockRequest =
          std::make_shared<MockAssetRequest>(
              "GET",
              "test.com",
              HttpHeaders{},
              std::move(mockResponse));

      std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase =
          std::make_unique<MockStoreCacheDatabase>();
      MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();
      std::shared_ptr<CachingAssetAccessor> cacheAssetAccessor =
          std::make_shared<CachingAssetAccessor>(
              spdlog::default_logger(),
              std::make_unique<MockAssetAccessor>(mockRequest),
              std::move(ownedMockCacheDatabase));
      std::shared_ptr<MockTaskProcessor> mockTaskProcessor =
          std::make_shared<MockTaskProcessor>();

      AsyncSystem asyncSystem(mockTaskProcessor);
      cacheAssetAccessor
          ->get(asyncSystem, "test.com", std::vector<IAssetAccessor::THeader>{})
          .wait();
      REQUIRE(mockCacheDatabase->storeResponseCall == false);
    }

    SUBCASE("No store if max-age=0 and response has no ETag or Last-Modified") {
      std::vector<int> statusCodes{200, 202, 203, 204, 205, 304};

      for (auto& statusCode : statusCodes) {
        HttpHeaders headers = {
            {"Content-Type", "application/json"},
            {"Cache-Control", "max-age=0"}};

        bool responseCached = runResponseCacheTest(statusCode, "GET", headers);
        CHECK(responseCached == false);
      }
    }
  }
}

TEST_CASE("Test calculation of expiry time for the cached response") {
  SUBCASE("Response has max-age cache control") {
    std::unique_ptr<IAssetResponse> mockResponse =
        std::make_unique<MockAssetResponse>(
            static_cast<uint16_t>(200),
            "app/json",
            HttpHeaders{
                {"Content-Type", "app/json"},
                {"Cache-Control", "must-revalidate, private, max-age=400"}},
            std::vector<std::byte>());

    std::shared_ptr<IAssetRequest> mockRequest =
        std::make_shared<MockAssetRequest>(
            "GET",
            "test.com",
            HttpHeaders{},
            std::move(mockResponse));

    std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase =
        std::make_unique<MockStoreCacheDatabase>();
    MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();
    std::shared_ptr<CachingAssetAccessor> cacheAssetAccessor =
        std::make_shared<CachingAssetAccessor>(
            spdlog::default_logger(),
            std::make_unique<MockAssetAccessor>(mockRequest),
            std::move(ownedMockCacheDatabase));
    std::shared_ptr<MockTaskProcessor> mockTaskProcessor =
        std::make_shared<MockTaskProcessor>();

    AsyncSystem asyncSystem(mockTaskProcessor);
    cacheAssetAccessor
        ->get(asyncSystem, "test.com", std::vector<IAssetAccessor::THeader>{})
        .wait();
    REQUIRE(mockCacheDatabase->storeResponseCall == true);
    REQUIRE(
        mockCacheDatabase->storeRequestParam->expiryTime - std::time(nullptr) ==
        400);
  }

  SUBCASE("Response has Expires header") {
    std::unique_ptr<IAssetResponse> mockResponse =
        std::make_unique<MockAssetResponse>(
            static_cast<uint16_t>(200),
            "app/json",
            HttpHeaders{
                {"Content-Type", "app/json"},
                {"Expires", "Wed, 21 Oct 2037 07:28:00 GMT"}},
            std::vector<std::byte>());

    std::shared_ptr<IAssetRequest> mockRequest =
        std::make_shared<MockAssetRequest>(
            "GET",
            "test.com",
            HttpHeaders{},
            std::move(mockResponse));

    std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase =
        std::make_unique<MockStoreCacheDatabase>();
    MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();
    std::shared_ptr<CachingAssetAccessor> cacheAssetAccessor =
        std::make_shared<CachingAssetAccessor>(
            spdlog::default_logger(),
            std::make_unique<MockAssetAccessor>(mockRequest),
            std::move(ownedMockCacheDatabase));
    std::shared_ptr<MockTaskProcessor> mockTaskProcessor =
        std::make_shared<MockTaskProcessor>();

    AsyncSystem asyncSystem(mockTaskProcessor);
    cacheAssetAccessor
        ->get(asyncSystem, "test.com", std::vector<IAssetAccessor::THeader>{})
        .wait();
    REQUIRE(mockCacheDatabase->storeResponseCall == true);
    REQUIRE(mockCacheDatabase->storeRequestParam->expiryTime == 2139722880);
  }
}

TEST_CASE("Test serving cache item") {
  SUBCASE("Cache item doesn't exist") {
    std::unique_ptr<IAssetResponse> mockResponse =
        std::make_unique<MockAssetResponse>(
            static_cast<uint16_t>(200),
            "app/json",
            HttpHeaders{
                {"Content-Type", "app/json"},
                {"Response-Header", "Response-Value"}},
            std::vector<std::byte>());

    std::shared_ptr<IAssetRequest> mockRequest =
        std::make_shared<MockAssetRequest>(
            "GET",
            "test.com",
            HttpHeaders{{"Request-Header", "Request-Value"}},
            std::move(mockResponse));

    std::shared_ptr<CachingAssetAccessor> cacheAssetAccessor =
        std::make_shared<CachingAssetAccessor>(
            spdlog::default_logger(),
            std::make_unique<MockAssetAccessor>(mockRequest),
            std::make_unique<MockStoreCacheDatabase>());
    std::shared_ptr<MockTaskProcessor> mockTaskProcessor =
        std::make_shared<MockTaskProcessor>();

    // test that the response is from the server
    AsyncSystem asyncSystem(mockTaskProcessor);
    cacheAssetAccessor
        ->get(asyncSystem, "test.com", std::vector<IAssetAccessor::THeader>{})
        .thenImmediately(
            [](const std::shared_ptr<IAssetRequest>& completedRequest) {
              REQUIRE(completedRequest != nullptr);
              REQUIRE(completedRequest->url() == "test.com");
              REQUIRE(
                  completedRequest->headers() ==
                  HttpHeaders{{"Request-Header", "Request-Value"}});
              REQUIRE(completedRequest->method() == "GET");

              const IAssetResponse* response = completedRequest->response();
              REQUIRE(response != nullptr);
              REQUIRE(
                  response->headers().at("Response-Header") ==
                  "Response-Value");
              REQUIRE(response->statusCode() == 200);
              REQUIRE(response->contentType() == "app/json");
              REQUIRE(response->data().empty());
              REQUIRE(!ResponseCacheControl::parseFromResponseHeaders(
                           response->headers())
                           .has_value());
            })
        .wait();
  }

  SUBCASE("Successfully retrieve cache item") {
    // create mock request and mock response. They are intended to be different
    // from the cache content so that we can verify the response in the callback
    // comes from the cache
    std::unique_ptr<IAssetResponse> mockResponse =
        std::make_unique<MockAssetResponse>(
            static_cast<uint16_t>(200),
            "app/json",
            HttpHeaders{
                {"Content-Type", "app/json"},
                {"Response-Header", "Response-Value"}},
            std::vector<std::byte>());

    std::shared_ptr<IAssetRequest> mockRequest =
        std::make_shared<MockAssetRequest>(
            "GET",
            "test.com",
            HttpHeaders{},
            std::move(mockResponse));

    // mock fresh cache item
    std::unique_ptr<MockStoreCacheDatabase> mockCacheDatabase =
        std::make_unique<MockStoreCacheDatabase>();
    std::time_t currentTime = std::time(nullptr);
    CacheRequest cacheRequest(
        HttpHeaders{{"Cache-Request-Header", "Cache-Request-Value"}},
        "GET",
        "cache.com");
    CacheResponse cacheResponse(
        static_cast<uint16_t>(200),
        HttpHeaders{
            {"Content-Type", "app/json"},
            {"Cache-Response-Header", "Cache-Response-Value"},
            {"Cache-Control", "max-age=100, private"}},
        std::vector<std::byte>());
    CacheItem cacheItem(
        currentTime + 100,
        std::move(cacheRequest),
        std::move(cacheResponse));
    mockCacheDatabase->cacheItem = cacheItem;

    std::shared_ptr<CachingAssetAccessor> cacheAssetAccessor =
        std::make_shared<CachingAssetAccessor>(
            spdlog::default_logger(),
            std::make_unique<MockAssetAccessor>(mockRequest),
            std::move(mockCacheDatabase));
    std::shared_ptr<MockTaskProcessor> mockTaskProcessor =
        std::make_shared<MockTaskProcessor>();

    // test that the response is from the cache
    AsyncSystem asyncSystem(mockTaskProcessor);
    cacheAssetAccessor
        ->get(
            asyncSystem,
            "test.com",
            std::vector<IAssetAccessor::THeader>{
                {"Some-Request-Header", "The Value"}})
        .thenImmediately(
            [](const std::shared_ptr<IAssetRequest>& completedRequest) {
              REQUIRE(completedRequest != nullptr);
              REQUIRE(completedRequest->method() == "GET");

              // URL and Headers should match the original request, even if
              // that's different from what's in the cache.
              REQUIRE(completedRequest->url() == "test.com");
              REQUIRE(
                  completedRequest->headers() ==
                  HttpHeaders{{"Some-Request-Header", "The Value"}});

              const IAssetResponse* response = completedRequest->response();
              REQUIRE(response != nullptr);
              REQUIRE(
                  response->headers().at("Cache-Response-Header") ==
                  "Cache-Response-Value");
              REQUIRE(response->statusCode() == 200);
              REQUIRE(response->contentType() == "app/json");
              REQUIRE(response->data().empty());

              std::optional<ResponseCacheControl> cacheControl =
                  ResponseCacheControl::parseFromResponseHeaders(
                      response->headers());

              REQUIRE(cacheControl.has_value());
              REQUIRE(cacheControl->mustRevalidate() == false);
              REQUIRE(cacheControl->noCache() == false);
              REQUIRE(cacheControl->noStore() == false);
              REQUIRE(cacheControl->noTransform() == false);
              REQUIRE(cacheControl->accessControlPublic() == false);
              REQUIRE(cacheControl->accessControlPrivate() == true);
              REQUIRE(cacheControl->proxyRevalidate() == false);
              REQUIRE(cacheControl->maxAgeExists() == true);
              REQUIRE(cacheControl->maxAgeValue() == 100);
              REQUIRE(cacheControl->sharedMaxAgeExists() == false);
            })
        .wait();
  }

  SUBCASE("Retrieve outdated cache item and cache control mandates "
          "revalidation before using it") {
    // Mock 304 response
    std::unique_ptr<IAssetResponse> mockResponse =
        std::make_unique<MockAssetResponse>(
            static_cast<uint16_t>(304),
            "app/json",
            HttpHeaders{
                {"Content-Type", "app/json"},
                {"Revalidation-Response-Header", "Revalidation-Response-Value"},
                {"Cache-Control", "max-age=300, must-revalidate, private"}},
            std::vector<std::byte>());

    std::shared_ptr<IAssetRequest> mockRequest =
        std::make_shared<MockAssetRequest>(
            "GET",
            "test.com",
            HttpHeaders{},
            std::move(mockResponse));

    // mock cache item
    std::unique_ptr<MockStoreCacheDatabase> mockCacheDatabase =
        std::make_unique<MockStoreCacheDatabase>();
    std::time_t currentTime = std::time(nullptr);
    CacheRequest cacheRequest(
        HttpHeaders{{"Cache-Request-Header", "Cache-Request-Value"}},
        "GET",
        "cache.com");
    CacheResponse cacheResponse(
        static_cast<uint16_t>(200),
        HttpHeaders{
            {"Content-Type", "app/json"},
            {"Cache-Response-Header", "Cache-Response-Value"},
            {"Cache-Control", "max-age=100, private"}},
        std::vector<std::byte>());
    CacheItem cacheItem(
        currentTime - 100,
        std::move(cacheRequest),
        std::move(cacheResponse));
    mockCacheDatabase->cacheItem = cacheItem;

    std::shared_ptr<CachingAssetAccessor> cacheAssetAccessor =
        std::make_shared<CachingAssetAccessor>(
            spdlog::default_logger(),
            std::make_unique<MockAssetAccessor>(mockRequest),
            std::move(mockCacheDatabase));
    std::shared_ptr<MockTaskProcessor> mockTaskProcessor =
        std::make_shared<MockTaskProcessor>();

    // test that the response is from the cache and it should update the header
    // and cache control coming from the validation response
    AsyncSystem asyncSystem(mockTaskProcessor);
    cacheAssetAccessor
        ->get(
            asyncSystem,
            "test.com",
            std::vector<IAssetAccessor::THeader>{
                {"Some-Request-Header", "The Value"}})
        .thenImmediately(
            [](const std::shared_ptr<IAssetRequest>& completedRequest) {
              REQUIRE(completedRequest != nullptr);
              REQUIRE(completedRequest->method() == "GET");

              // URL and Headers should match the original request, even if
              // that's different from what's in the cache.
              REQUIRE(completedRequest->url() == "test.com");
              REQUIRE(
                  completedRequest->headers() ==
                  HttpHeaders{{"Some-Request-Header", "The Value"}});

              // check response header is updated
              const IAssetResponse* response = completedRequest->response();
              REQUIRE(response != nullptr);
              REQUIRE(
                  response->headers().at("Revalidation-Response-Header") ==
                  "Revalidation-Response-Value");
              REQUIRE(
                  response->headers().at("Cache-Response-Header") ==
                  "Cache-Response-Value");
              REQUIRE(response->statusCode() == 200);
              REQUIRE(response->contentType() == "app/json");
              REQUIRE(response->data().empty());

              // check cache control is updated
              std::optional<ResponseCacheControl> cacheControl =
                  ResponseCacheControl::parseFromResponseHeaders(
                      response->headers());
              REQUIRE(cacheControl.has_value());
              REQUIRE(cacheControl->mustRevalidate() == true);
              REQUIRE(cacheControl->noCache() == false);
              REQUIRE(cacheControl->noStore() == false);
              REQUIRE(cacheControl->noTransform() == false);
              REQUIRE(cacheControl->accessControlPublic() == false);
              REQUIRE(cacheControl->accessControlPrivate() == true);
              REQUIRE(cacheControl->proxyRevalidate() == false);
              REQUIRE(cacheControl->maxAgeExists() == true);
              REQUIRE(cacheControl->maxAgeValue() == 300);
              REQUIRE(cacheControl->sharedMaxAgeExists() == false);
            })
        .wait();
  }

  SUBCASE("Cache should serve validation response from the server directly if "
          "it is not 304") {
    // Mock 200 response
    std::unique_ptr<IAssetResponse> mockResponse =
        std::make_unique<MockAssetResponse>(
            static_cast<uint16_t>(200),
            "app/json",
            HttpHeaders{
                {"Content-Type", "app/json"},
                {"Revalidation-Response-Header",
                 "Revalidation-Response-Value"}},
            std::vector<std::byte>());

    std::shared_ptr<IAssetRequest> mockRequest =
        std::make_shared<MockAssetRequest>(
            "GET",
            "test.com",
            HttpHeaders{},
            std::move(mockResponse));

    // mock cache item
    std::unique_ptr<MockStoreCacheDatabase> mockCacheDatabase =
        std::make_unique<MockStoreCacheDatabase>();
    std::time_t currentTime = std::time(nullptr);
    CacheRequest cacheRequest(
        HttpHeaders{{"Cache-Request-Header", "Cache-Request-Value"}},
        "GET",
        "cache.com");
    CacheResponse cacheResponse(
        static_cast<uint16_t>(200),
        HttpHeaders{
            {"Content-Type", "app/json"},
            {"Cache-Response-Header", "Cache-Response-Value"},
            {"Cache-Control", "max-age=100, private"}},
        std::vector<std::byte>());
    CacheItem cacheItem(
        currentTime - 100,
        std::move(cacheRequest),
        std::move(cacheResponse));
    mockCacheDatabase->cacheItem = cacheItem;

    std::shared_ptr<CachingAssetAccessor> cacheAssetAccessor =
        std::make_shared<CachingAssetAccessor>(
            spdlog::default_logger(),
            std::make_unique<MockAssetAccessor>(mockRequest),
            std::move(mockCacheDatabase));
    std::shared_ptr<MockTaskProcessor> mockTaskProcessor =
        std::make_shared<MockTaskProcessor>();

    // test that the response is from the server directly
    AsyncSystem asyncSystem(mockTaskProcessor);
    cacheAssetAccessor
        ->get(asyncSystem, "test.com", std::vector<IAssetAccessor::THeader>{})
        .thenImmediately(
            [](const std::shared_ptr<IAssetRequest>& completedRequest) {
              REQUIRE(completedRequest != nullptr);
              REQUIRE(completedRequest->url() == "test.com");
              REQUIRE(completedRequest->headers().empty());
              REQUIRE(completedRequest->method() == "GET");

              const IAssetResponse* response = completedRequest->response();
              REQUIRE(response != nullptr);

              REQUIRE(
                  response->headers().at("Revalidation-Response-Header") ==
                  "Revalidation-Response-Value");
              REQUIRE(response->statusCode() == 200);
              REQUIRE(response->contentType() == "app/json");
              REQUIRE(response->data().empty());
              REQUIRE(!ResponseCacheControl::parseFromResponseHeaders(
                           response->headers())
                           .has_value());
            })
        .wait();
  }
}
