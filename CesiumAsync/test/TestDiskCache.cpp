#include "catch2/catch.hpp"
#include "MockAssetRequest.h"
#include "MockAssetResponse.h"
#include "CesiumAsync/DiskCache.h"

using namespace CesiumAsync;

TEST_CASE("Test disk cache with Sqlite") {
	SECTION("Test store cache") {
		DiskCache diskCache("test.db", 3);

		HttpHeaders responseHeaders;
		ResponseCacheControl responseCacheControl(false, false, false, false, false, false, false, 0, 0);
		std::vector<uint8_t> responseData = {0, 1, 2, 3, 4};
		std::unique_ptr<MockAssetResponse> response = std::make_unique<MockAssetResponse>(
			static_cast<uint16_t>(200), "text/html", responseHeaders, responseCacheControl, responseData);

		HttpHeaders requestHeaders;
		std::unique_ptr<MockAssetRequest> request = std::make_unique<MockAssetRequest>("GET", 
			"test.com", requestHeaders, std::move(response));

		std::string error;
		REQUIRE(diskCache.storeResponse("TestKey", std::time(0), *request, error));

		// TODO make sure the cache is in the database
	}

	SECTION("Test retrieve cache") {
		DiskCache diskCache("test.db", 3);

		// store data in the cache first
		HttpHeaders responseHeaders;
		responseHeaders["Response-Test-Header-0"] = "Response-Header-Entry-Value-0";
		responseHeaders["Response-Test-Header-1"] = "Response-Header-Entry-Value-1";
		ResponseCacheControl responseCacheControl(false, false, false, false, false, false, false, 0, 0);
		std::vector<uint8_t> responseData = {0, 1, 2, 3, 4};
		std::unique_ptr<MockAssetResponse> response = std::make_unique<MockAssetResponse>(
			static_cast<uint16_t>(200), "text/html", responseHeaders, responseCacheControl, responseData);

		HttpHeaders requestHeaders;
		requestHeaders["Request-Test-Header-0"] = "Request-Header-Entry-Value-0";
		requestHeaders["Request-Test-Header-1"] = "Request-Header-Entry-Value-1";
		std::unique_ptr<MockAssetRequest> request = std::make_unique<MockAssetRequest>("GET", 
			"test.com", requestHeaders, std::move(response));

		std::string error;
		REQUIRE(diskCache.storeResponse("TestKey", std::time(0), *request, error));

		// retrieve it
		error.clear();
		std::optional<CacheItem> cacheItem; 
		REQUIRE(diskCache.getEntry("TestKey", cacheItem, error));
		REQUIRE(cacheItem != std::nullopt);

		// TODO: check the content of item
	}

	SECTION("Test remove cache") {
		DiskCache diskCache("test.db", 3);

		// store data in the cache first
		HttpHeaders responseHeaders;
		responseHeaders["Response-Test-Header-0"] = "Response-Header-Entry-Value-0";
		responseHeaders["Response-Test-Header-1"] = "Response-Header-Entry-Value-1";
		ResponseCacheControl responseCacheControl(false, false, false, false, false, false, false, 0, 0);
		std::vector<uint8_t> responseData = {0, 1, 2, 3, 4};
		std::unique_ptr<MockAssetResponse> response = std::make_unique<MockAssetResponse>(
			static_cast<uint16_t>(200), "text/html", responseHeaders, responseCacheControl, responseData);

		HttpHeaders requestHeaders;
		requestHeaders["Request-Test-Header-0"] = "Request-Header-Entry-Value-0";
		requestHeaders["Request-Test-Header-1"] = "Request-Header-Entry-Value-1";
		std::unique_ptr<MockAssetRequest> request = std::make_unique<MockAssetRequest>("GET", 
			"test.com", requestHeaders, std::move(response));

		std::string error;
		REQUIRE(diskCache.storeResponse("TestKey", std::time(0), *request, error));

		// remove it
		error.clear();
		REQUIRE(diskCache.removeEntry("TestKey", error));

		// TODO: check make sure item is deleted
	}

	SECTION("Test prune") {
		DiskCache diskCache("test.db", 3);

		// store data in the cache first
		HttpHeaders responseHeaders;
		responseHeaders["Response-Test-Header-0"] = "Response-Header-Entry-Value-0";
		responseHeaders["Response-Test-Header-1"] = "Response-Header-Entry-Value-1";
		ResponseCacheControl responseCacheControl(false, false, false, false, false, false, false, 0, 0);
		std::vector<uint8_t> responseData = {0, 1, 2, 3, 4};
		std::unique_ptr<MockAssetResponse> response = std::make_unique<MockAssetResponse>(
			static_cast<uint16_t>(200), "text/html", responseHeaders, responseCacheControl, responseData);

		HttpHeaders requestHeaders;
		requestHeaders["Request-Test-Header-0"] = "Request-Header-Entry-Value-0";
		requestHeaders["Request-Test-Header-1"] = "Request-Header-Entry-Value-1";
		std::unique_ptr<MockAssetRequest> request = std::make_unique<MockAssetRequest>("GET", 
			"test.com", requestHeaders, std::move(response));

		std::string error;
		REQUIRE(diskCache.storeResponse("TestKey", std::time(0), *request, error));
		REQUIRE(diskCache.storeResponse("TestKey1", std::time(0), *request, error));
		REQUIRE(diskCache.storeResponse("TestKey2", std::time(0), *request, error));
		REQUIRE(diskCache.storeResponse("TestKey3", std::time(0), *request, error));
		REQUIRE(diskCache.storeResponse("TestKey4", std::time(0), *request, error));

		// clear all
		error.clear();
		REQUIRE(diskCache.prune(error));
	}

	SECTION("Test clear all") {
		DiskCache diskCache("test.db", 3);

		// store data in the cache first
		HttpHeaders responseHeaders;
		responseHeaders["Response-Test-Header-0"] = "Response-Header-Entry-Value-0";
		responseHeaders["Response-Test-Header-1"] = "Response-Header-Entry-Value-1";
		ResponseCacheControl responseCacheControl(false, false, false, false, false, false, false, 0, 0);
		std::vector<uint8_t> responseData = {0, 1, 2, 3, 4};
		std::unique_ptr<MockAssetResponse> response = std::make_unique<MockAssetResponse>(
			static_cast<uint16_t>(200), "text/html", responseHeaders, responseCacheControl, responseData);

		HttpHeaders requestHeaders;
		requestHeaders["Request-Test-Header-0"] = "Request-Header-Entry-Value-0";
		requestHeaders["Request-Test-Header-1"] = "Request-Header-Entry-Value-1";
		std::unique_ptr<MockAssetRequest> request = std::make_unique<MockAssetRequest>("GET", 
			"test.com", requestHeaders, std::move(response));

		std::string error;
		REQUIRE(diskCache.storeResponse("TestKey", std::time(0), *request, error));

		// clear all
		error.clear();
		REQUIRE(diskCache.clearAll(error));
	}

}

