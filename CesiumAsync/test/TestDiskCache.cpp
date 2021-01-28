#include "catch2/catch.hpp"
#include "MockAssetRequest.h"
#include "MockAssetResponse.h"
#include "CesiumAsync/DiskCache.h"

using namespace CesiumAsync;

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
		REQUIRE(diskCache.storeResponse("TestKey", std::time(0), *request, error));

		std::optional<CacheItem> cacheItem;
		REQUIRE(diskCache.getEntry("TestKey", cacheItem, error));
		REQUIRE(cacheItem->expiryTime == currentTime);
		REQUIRE(cacheItem->lastAccessedTime == currentTime);

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

	SECTION("Test remove cache") {
		std::time_t currentTime = std::time(0);

		// store the first item
		{
			HttpHeaders responseHeaders{ { "Response-Header-1", "Response-Value-1" } };
			ResponseCacheControl responseCacheControl(true, false, true, false, true, false, true, 200, 10);
			std::vector<uint8_t> responseData = {0, 1, 2, 3, 4};
			std::unique_ptr<MockAssetResponse> response = std::make_unique<MockAssetResponse>(
				static_cast<uint16_t>(200), "text/html", responseHeaders, responseCacheControl, responseData);

			HttpHeaders requestHeaders{ { "Request-Header-1", "Request-Value-1" } };
			std::unique_ptr<MockAssetRequest> request = std::make_unique<MockAssetRequest>("GET", 
				"test.com", requestHeaders, std::move(response));

			REQUIRE(diskCache.storeResponse("TestKey-1", currentTime + 100, *request, error));
		}

		// store the second item
		{
			HttpHeaders responseHeaders{ { "Response-Header-2", "Response-Value-2" } };
			ResponseCacheControl responseCacheControl(true, false, false, false, false, true, false, 200, 10);
			std::vector<uint8_t> responseData = {0, 1, 2, 3, 4, 7, 10};
			std::unique_ptr<MockAssetResponse> response = std::make_unique<MockAssetResponse>(
				static_cast<uint16_t>(200), "text/html", responseHeaders, responseCacheControl, responseData);

			HttpHeaders requestHeaders{ { "Request-Header", "Request-Value" } };
			std::unique_ptr<MockAssetRequest> request = std::make_unique<MockAssetRequest>("GET", 
				"test.com", requestHeaders, std::move(response));

			REQUIRE(diskCache.storeResponse("TestKey-2", currentTime, *request, error));
		}

		// remove second item
		REQUIRE(diskCache.removeEntry("TestKey-2", error));

		std::optional<CacheItem> testKey2;
		REQUIRE(diskCache.getEntry("TestKey-2", testKey2, error));
		REQUIRE(testKey2 == std::nullopt);

		// make sure the first item is still in there
		std::optional<CacheItem> testKey1;
		REQUIRE(diskCache.getEntry("TestKey-1", testKey1, error));
		REQUIRE(testKey1->expiryTime == currentTime + 100);
		REQUIRE(testKey1->lastAccessedTime == currentTime);

		const CacheRequest& cacheRequest = testKey1->cacheRequest;
		REQUIRE(cacheRequest.headers == HttpHeaders{ {"Request-Header-1", "Request-Value-1"} });
		REQUIRE(cacheRequest.method == "GET");
		REQUIRE(cacheRequest.url == "test.com");

		const CacheResponse& cacheResponse = testKey1->cacheResponse;
		REQUIRE(cacheResponse.contentType == "text/html");
		REQUIRE(cacheResponse.statusCode == 200);
		REQUIRE(cacheResponse.headers == HttpHeaders{ {"Response-Header-1", "Response-Value-1"} });
		REQUIRE(cacheResponse.data == std::vector<uint8_t>{ 0, 1, 2, 3, 4 });
		REQUIRE(cacheResponse.cacheControl->mustRevalidate() == true);
		REQUIRE(cacheResponse.cacheControl->noCache() == false);
		REQUIRE(cacheResponse.cacheControl->noStore() == true);
		REQUIRE(cacheResponse.cacheControl->noTransform() == false);
		REQUIRE(cacheResponse.cacheControl->accessControlPublic() == true);
		REQUIRE(cacheResponse.cacheControl->accessControlPrivate() == false);
		REQUIRE(cacheResponse.cacheControl->proxyRevalidate() == true);
		REQUIRE(cacheResponse.cacheControl->maxAge() == 200);
		REQUIRE(cacheResponse.cacheControl->sharedMaxAge() == 10);
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

			REQUIRE(diskCache.storeResponse("TestKey" + std::to_string(i), currentTime + interval + i, *request, error));
		}

		REQUIRE(diskCache.prune(error));
		for (int i = 0; i <= 16; ++i) {
			std::optional<CacheItem> cacheItem;
			REQUIRE(diskCache.getEntry("TestKey" + std::to_string(i), cacheItem, error));
			REQUIRE(cacheItem == std::nullopt);
		}

		for (int i = 17; i < 20; ++i) {
			std::optional<CacheItem> cacheItem;
			REQUIRE(diskCache.getEntry("TestKey" + std::to_string(i), cacheItem, error));
			REQUIRE(cacheItem != std::nullopt);

			// make sure the item is still in there
			REQUIRE(cacheItem->expiryTime == currentTime + interval + i);
			REQUIRE(cacheItem->lastAccessedTime == currentTime);

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
			std::optional<CacheItem> item;
			REQUIRE(diskCache.getEntry("TestKey" + std::to_string(i), item, error));
			REQUIRE(item == std::nullopt);
		}
	}
}

