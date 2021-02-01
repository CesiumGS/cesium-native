#include "catch2/catch.hpp"
#include "MockAssetRequest.h"
#include "MockAssetResponse.h"
#include "CesiumAsync/DiskCache.h"

using namespace CesiumAsync;

static void getFirstCacheItem(const ICacheDatabase& database, const std::string& key, std::optional<CacheItem>& cacheItem) {
    std::string error;
    auto getItemCallback = [&cacheItem](CacheItem item) mutable { cacheItem = std::move(item); return true; };
    REQUIRE(database.getEntry(key, getItemCallback, error));
}

TEST_CASE("Test disk cache with Sqlite") {
    DiskCache diskCache("test.db", 3);

    std::string error;
    REQUIRE(diskCache.clearAll(error));

    SECTION("Test store and retrive cache") {
        HttpHeaders responseHeaders = { { "Response-Header", "Response-Value" } };
        ResponseCacheControl responseCacheControl(false, false, false, false, false, false, false, 0, 0);
        std::vector<uint8_t> responseData = {0, 1, 2, 3, 4};
        std::unique_ptr<MockAssetResponse> response = std::make_unique<MockAssetResponse>(
            static_cast<uint16_t>(200), "text/html", responseHeaders, responseCacheControl, responseData);

        HttpHeaders requestHeaders{ {"Request-Header", "Request-Value" } };
        std::unique_ptr<MockAssetRequest> request = std::make_unique<MockAssetRequest>("GET", 
            "test.com", requestHeaders, std::move(response));

        std::time_t currentTime = std::time(0);
        REQUIRE(diskCache.storeResponse("TestKey", currentTime, *request, error));

        std::optional<CacheItem> cacheItem;
        getFirstCacheItem(diskCache, "TestKey", cacheItem);
        REQUIRE(cacheItem->expiryTime == currentTime);

        const CacheRequest& cacheRequest = cacheItem->cacheRequest;
        REQUIRE(cacheRequest.headers == HttpHeaders{ {"Request-Header", "Request-Value"} });
        REQUIRE(cacheRequest.method == "GET");
        REQUIRE(cacheRequest.url == "test.com");

        const CacheResponse& cacheResponse = cacheItem->cacheResponse;
        REQUIRE(cacheResponse.contentType == "text/html");
        REQUIRE(cacheResponse.statusCode == 200);
        REQUIRE(cacheResponse.headers == HttpHeaders{ {"Response-Header", "Response-Value"} });
        REQUIRE(cacheResponse.data == std::vector<uint8_t>{ 0, 1, 2, 3, 4 });
        REQUIRE(cacheResponse.cacheControl->mustRevalidate() == false);
        REQUIRE(cacheResponse.cacheControl->noCache() == false);
        REQUIRE(cacheResponse.cacheControl->noStore() == false);
        REQUIRE(cacheResponse.cacheControl->noTransform() == false);
        REQUIRE(cacheResponse.cacheControl->accessControlPublic() == false);
        REQUIRE(cacheResponse.cacheControl->accessControlPrivate() == false);
        REQUIRE(cacheResponse.cacheControl->proxyRevalidate() == false);
        REQUIRE(cacheResponse.cacheControl->maxAge() == 0);
        REQUIRE(cacheResponse.cacheControl->sharedMaxAge() == 0);
    }

    SECTION("Test prune") {
        // store data in the cache first
        std::time_t currentTime = std::time(0);
        std::time_t interval = -10;
        for (size_t i = 0; i < 20; ++i) {
            HttpHeaders responseHeaders{ {"Response-Header-" + std::to_string(i), "Response-Value-" + std::to_string(i)} };
            ResponseCacheControl responseCacheControl(true, false, true, false, true, false, true, 0, 0);
            std::vector<uint8_t> responseData = {0, 1, 2, 3, 4};
            std::unique_ptr<MockAssetResponse> response = std::make_unique<MockAssetResponse>(
                static_cast<uint16_t>(200), "text/html", responseHeaders, responseCacheControl, responseData);

            HttpHeaders requestHeaders{ {"Request-Header-" + std::to_string(i), "Request-Value-" + std::to_string(i)} };
            std::unique_ptr<MockAssetRequest> request = std::make_unique<MockAssetRequest>(
                "GET", "test.com", requestHeaders, std::move(response));

            REQUIRE(diskCache.storeResponse("TestKey" + std::to_string(i), currentTime + interval + static_cast<std::time_t>(i), *request, error));
        }

        REQUIRE(diskCache.prune(error));
        for (int i = 0; i <= 16; ++i) {
            std::optional<CacheItem> cacheItem;
            getFirstCacheItem(diskCache, "TestKey" + std::to_string(i), cacheItem);
            REQUIRE(cacheItem == std::nullopt);
        }

        for (int i = 17; i < 20; ++i) {
            std::optional<CacheItem> cacheItem;
            getFirstCacheItem(diskCache, "TestKey" + std::to_string(i), cacheItem);
            REQUIRE(cacheItem != std::nullopt);

            // make sure the item is still in there
            REQUIRE(cacheItem->expiryTime == currentTime + interval + i);

            const CacheRequest& cacheRequest = cacheItem->cacheRequest;
            REQUIRE(cacheRequest.headers == HttpHeaders{ {"Request-Header-" + std::to_string(i), "Request-Value-" + std::to_string(i)} });
            REQUIRE(cacheRequest.method == "GET");
            REQUIRE(cacheRequest.url == "test.com");

            const CacheResponse& cacheResponse = cacheItem->cacheResponse;
            REQUIRE(cacheResponse.contentType == "text/html");
            REQUIRE(cacheResponse.statusCode == 200);
            REQUIRE(cacheResponse.headers == HttpHeaders{ {"Response-Header-" + std::to_string(i), "Response-Value-" + std::to_string(i)} });
            REQUIRE(cacheResponse.data == std::vector<uint8_t>{ 0, 1, 2, 3, 4 });
            REQUIRE(cacheResponse.cacheControl->mustRevalidate() == true);
            REQUIRE(cacheResponse.cacheControl->noCache() == false);
            REQUIRE(cacheResponse.cacheControl->noStore() == true);
            REQUIRE(cacheResponse.cacheControl->noTransform() == false);
            REQUIRE(cacheResponse.cacheControl->accessControlPublic() == true);
            REQUIRE(cacheResponse.cacheControl->accessControlPrivate() == false);
            REQUIRE(cacheResponse.cacheControl->proxyRevalidate() == true);
            REQUIRE(cacheResponse.cacheControl->maxAge() == 0);
            REQUIRE(cacheResponse.cacheControl->sharedMaxAge() == 0);
        }
    }

    SECTION("Test clear all") {
        // store data in the cache first
        HttpHeaders responseHeaders{ {"Response-Header", "Response-Value"} };
        ResponseCacheControl responseCacheControl(false, false, false, false, false, false, false, 0, 0);
        std::vector<uint8_t> responseData = {0, 1, 2, 3, 4};
        std::unique_ptr<MockAssetResponse> response = std::make_unique<MockAssetResponse>(
            static_cast<uint16_t>(200), "text/html", responseHeaders, responseCacheControl, responseData);

        HttpHeaders requestHeaders{ {"Request-Header", "Request-Value"} };
        std::unique_ptr<MockAssetRequest> request = std::make_unique<MockAssetRequest>("GET", 
            "test.com", requestHeaders, std::move(response));

        for (size_t i = 0; i < 10; ++i) {
            REQUIRE(diskCache.storeResponse("TestKey" + std::to_string(i), std::time(0), *request, error));
        }

        // clear all
        REQUIRE(diskCache.clearAll(error));
        for (size_t i = 0; i < 10; ++i) {
            std::optional<CacheItem> cacheItem;
            getFirstCacheItem(diskCache, "TestKey" + std::to_string(i), cacheItem);
            REQUIRE(cacheItem == std::nullopt);
        }
    }
}

