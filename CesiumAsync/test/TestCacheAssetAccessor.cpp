#include "catch2/catch.hpp"
#include "MockAssetRequest.h"
#include "MockAssetResponse.h"
#include "CesiumAsync/CacheAssetAccessor.h"
#include "CesiumAsync/ICacheDatabase.h"
#include "CesiumAsync/ITaskProcessor.h"
#include "CesiumAsync/AsyncSystem.h"
#include <optional>

using namespace CesiumAsync;

class MockStoreCacheDatabase : public ICacheDatabase {
public:
	struct StoreRequestParameters {
		std::string key;
		std::time_t expiryTime;
		const IAssetRequest* assetRequest;
	};

	MockStoreCacheDatabase() 
		: getEntryCall{false}
		, storeResponseCall{false}
		, removeEntryCall{false}
		, pruneCall{false}
		, clearAllCall{false}
	{}

	virtual bool getEntry(const std::string& /*key*/, 
		std::function<bool(CacheItem)> predicate, 
		std::string& /*error*/) const override
	{
		if (this->cacheItem) {
			predicate(*this->cacheItem);
		}

		this->getEntryCall = true;
		return true;
	}

	virtual bool storeResponse(const std::string& key,
		std::time_t expiryTime,
		const IAssetRequest& request,
		std::string& /*error*/) override
	{
		this->storeRequestParam = StoreRequestParameters{ key, expiryTime, &request };
		this->storeResponseCall = true;
		return true;
	}

	virtual bool removeEntry(const std::string& /*key*/, std::string& /*error*/) override
	{
		this->removeEntryCall = true;
		return true;
	}

	virtual bool prune(std::string& /*error*/) override
	{
		this->pruneCall = true;
		return true;
	}

	virtual bool clearAll(std::string& /*error*/) override
	{
		this->clearAllCall = true;
		return true;
	}

	mutable bool getEntryCall;
	bool storeResponseCall;
	bool removeEntryCall;
	bool pruneCall;
	bool clearAllCall;

	std::optional<StoreRequestParameters> storeRequestParam;
	std::optional<CacheItem> cacheItem;
};

class MockAssetAccessor : public IAssetAccessor {
public:
	MockAssetAccessor(std::shared_ptr<IAssetRequest> request) 
		: testRequest{request}
	{}

	virtual void requestAsset(
		const AsyncSystem* /*asyncSystem*/,
		const std::string& /*url*/,
		const std::vector<THeader>& /*headers*/,
		std::function<void(std::shared_ptr<IAssetRequest>)> callback
	) override {
		callback(testRequest);
	}

	virtual void tick() noexcept override {}

	std::shared_ptr<IAssetRequest> testRequest;
};

class MockTaskProcessor : public ITaskProcessor {
public:
	virtual void startTask(std::function<void()> f) override {
		f();
	}
};

TEST_CASE("Test the condition of caching the request") {
	SECTION("Cache request") {
		SECTION("Request with GET method, has max-age, cacheable status code, no Authorization header") {
			int statusCode = GENERATE(200, 202, 203, 204, 205, 304);

			std::unique_ptr<IAssetResponse> mockResponse = std::make_unique<MockAssetResponse>(
				static_cast<uint16_t>(statusCode), 
				"app/json",
				HttpHeaders{}, 
				ResponseCacheControl{ true, false, false, false, false, false, false, 100, 0 },
				std::vector<uint8_t>());

			std::shared_ptr<IAssetRequest> mockRequest = std::make_shared<MockAssetRequest>(
				"GET",
				"test.com",
				HttpHeaders{},
				std::move(mockResponse)
			);

			std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase = std::make_unique<MockStoreCacheDatabase>();
			MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();
			std::shared_ptr<CacheAssetAccessor> cacheAssetAccessor = std::make_shared<CacheAssetAccessor>(
				std::make_unique<MockAssetAccessor>(mockRequest),
				std::move(ownedMockCacheDatabase)
			);
			std::shared_ptr<MockTaskProcessor> mockTaskProcessor = std::make_shared<MockTaskProcessor>();

			AsyncSystem asyncSystem(cacheAssetAccessor, mockTaskProcessor);
			asyncSystem.requestAsset("test.com", std::vector<IAssetAccessor::THeader>{});
			REQUIRE(mockCacheDatabase->storeResponseCall == true);
		}

		SECTION("Request with GET method, has Expires header, cacheable status code, no Authorization header") {
			int statusCode = GENERATE(200, 202, 203, 204, 205, 304);

			std::unique_ptr<IAssetResponse> mockResponse = std::make_unique<MockAssetResponse>(
				static_cast<uint16_t>(statusCode), 
				"app/json",
				HttpHeaders{ {"Expires", "Wed, 21 Oct 5020 07:28:00 GMT"} }, 
				std::nullopt,
				std::vector<uint8_t>());

			std::shared_ptr<IAssetRequest> mockRequest = std::make_shared<MockAssetRequest>(
				"GET",
				"test.com",
				HttpHeaders{},
				std::move(mockResponse)
			);

			std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase = std::make_unique<MockStoreCacheDatabase>();
			MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();
			std::shared_ptr<CacheAssetAccessor> cacheAssetAccessor = std::make_shared<CacheAssetAccessor>(
				std::make_unique<MockAssetAccessor>(mockRequest),
				std::move(ownedMockCacheDatabase)
			);
			std::shared_ptr<MockTaskProcessor> mockTaskProcessor = std::make_shared<MockTaskProcessor>();

			AsyncSystem asyncSystem(cacheAssetAccessor, mockTaskProcessor);
			asyncSystem.requestAsset("test.com", std::vector<IAssetAccessor::THeader>{});
			REQUIRE(mockCacheDatabase->storeResponseCall == true);
		}
	}

	SECTION("No cache condition") {
		SECTION("No store for response that doesn't have GET method") {
			std::unique_ptr<IAssetResponse> mockResponse = std::make_unique<MockAssetResponse>(
				static_cast<uint16_t>(200), 
				"app/json",
				HttpHeaders{}, 
				ResponseCacheControl{ true, false, false, false, true, true, false, 100, 0 },
				std::vector<uint8_t>());

			std::shared_ptr<IAssetRequest> mockRequest = std::make_shared<MockAssetRequest>(
				"POST",
				"test.com",
				HttpHeaders{},
				std::move(mockResponse)
			);

			std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase = std::make_unique<MockStoreCacheDatabase>();
			MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();
			std::shared_ptr<CacheAssetAccessor> cacheAssetAccessor = std::make_shared<CacheAssetAccessor>(
				std::make_unique<MockAssetAccessor>(mockRequest),
				std::move(ownedMockCacheDatabase)
			);
			std::shared_ptr<MockTaskProcessor> mockTaskProcessor = std::make_shared<MockTaskProcessor>();

			AsyncSystem asyncSystem(cacheAssetAccessor, mockTaskProcessor);
			asyncSystem.requestAsset("test.com", std::vector<IAssetAccessor::THeader>{});
			REQUIRE(mockCacheDatabase->storeResponseCall == false);
		}

		SECTION("No store for requests that has Authorization header") {
			std::unique_ptr<IAssetResponse> mockResponse = std::make_unique<MockAssetResponse>(
				static_cast<uint16_t>(200), 
				"app/json",
				HttpHeaders{}, 
				ResponseCacheControl{ true, false, false, false, true, true, false, 100, 0 },
				std::vector<uint8_t>());

			std::shared_ptr<IAssetRequest> mockRequest = std::make_shared<MockAssetRequest>(
				"GET",
				"test.com",
				HttpHeaders{ {"Authorization", "Basic YWxhZGRpbjpvcGVuc2VzYW1l"} },
				std::move(mockResponse)
			);

			std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase = std::make_unique<MockStoreCacheDatabase>();
			MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();
			std::shared_ptr<CacheAssetAccessor> cacheAssetAccessor = std::make_shared<CacheAssetAccessor>(
				std::make_unique<MockAssetAccessor>(mockRequest),
				std::move(ownedMockCacheDatabase)
			);
			std::shared_ptr<MockTaskProcessor> mockTaskProcessor = std::make_shared<MockTaskProcessor>();

			AsyncSystem asyncSystem(cacheAssetAccessor, mockTaskProcessor);
			asyncSystem.requestAsset("test.com", std::vector<IAssetAccessor::THeader>{});
			REQUIRE(mockCacheDatabase->storeResponseCall == false);
		}

		SECTION("No store for response that has no cacheable status code") {
			std::unique_ptr<IAssetResponse> mockResponse = std::make_unique<MockAssetResponse>(
				static_cast<uint16_t>(404), 
				"app/json",
				HttpHeaders{}, 
				ResponseCacheControl{ true, false, false, false, true, true, false, 100, 0 },
				std::vector<uint8_t>());

			std::shared_ptr<IAssetRequest> mockRequest = std::make_shared<MockAssetRequest>(
				"GET",
				"test.com",
				HttpHeaders{},
				std::move(mockResponse)
			);

			std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase = std::make_unique<MockStoreCacheDatabase>();
			MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();
			std::shared_ptr<CacheAssetAccessor> cacheAssetAccessor = std::make_shared<CacheAssetAccessor>(
				std::make_unique<MockAssetAccessor>(mockRequest),
				std::move(ownedMockCacheDatabase)
			);
			std::shared_ptr<MockTaskProcessor> mockTaskProcessor = std::make_shared<MockTaskProcessor>();

			AsyncSystem asyncSystem(cacheAssetAccessor, mockTaskProcessor);
			asyncSystem.requestAsset("test.com", std::vector<IAssetAccessor::THeader>{});
			REQUIRE(mockCacheDatabase->storeResponseCall == false);
		}

		SECTION("No store for response that has No-Store in the cache-control header") {
			std::unique_ptr<IAssetResponse> mockResponse = std::make_unique<MockAssetResponse>(
				static_cast<uint16_t>(200), 
				"app/json",
				HttpHeaders{}, 
				ResponseCacheControl{ false, false, true, false, false, false, false, 0, 0 },
				std::vector<uint8_t>());

			std::shared_ptr<IAssetRequest> mockRequest = std::make_shared<MockAssetRequest>(
				"GET",
				"test.com",
				HttpHeaders{},
				std::move(mockResponse)
			);

			std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase = std::make_unique<MockStoreCacheDatabase>();
			MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();
			std::shared_ptr<CacheAssetAccessor> cacheAssetAccessor = std::make_shared<CacheAssetAccessor>(
				std::make_unique<MockAssetAccessor>(mockRequest),
				std::move(ownedMockCacheDatabase)
			);
			std::shared_ptr<MockTaskProcessor> mockTaskProcessor = std::make_shared<MockTaskProcessor>();

			AsyncSystem asyncSystem(cacheAssetAccessor, mockTaskProcessor);
			asyncSystem.requestAsset("test.com", std::vector<IAssetAccessor::THeader>{});
			REQUIRE(mockCacheDatabase->storeResponseCall == false);
		}

		SECTION("No store for response that has No-Cache in the cache-control header") {
			std::unique_ptr<IAssetResponse> mockResponse = std::make_unique<MockAssetResponse>(
				static_cast<uint16_t>(200), 
				"app/json",
				HttpHeaders{}, 
				ResponseCacheControl{ true, false, true, false, false, false, false, 0, 0 },
				std::vector<uint8_t>());

			std::shared_ptr<IAssetRequest> mockRequest = std::make_shared<MockAssetRequest>(
				"GET",
				"test.com",
				HttpHeaders{},
				std::move(mockResponse)
			);

			std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase = std::make_unique<MockStoreCacheDatabase>();
			MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();
			std::shared_ptr<CacheAssetAccessor> cacheAssetAccessor = std::make_shared<CacheAssetAccessor>(
				std::make_unique<MockAssetAccessor>(mockRequest),
				std::move(ownedMockCacheDatabase)
			);
			std::shared_ptr<MockTaskProcessor> mockTaskProcessor = std::make_shared<MockTaskProcessor>();

			AsyncSystem asyncSystem(cacheAssetAccessor, mockTaskProcessor);
			asyncSystem.requestAsset("test.com", std::vector<IAssetAccessor::THeader>{});
			REQUIRE(mockCacheDatabase->storeResponseCall == false);
		}

		SECTION("No store for response that has no Cache-Control and Expires header") {
			std::unique_ptr<IAssetResponse> mockResponse = std::make_unique<MockAssetResponse>(
				static_cast<uint16_t>(200),
				"app/json",
				HttpHeaders{},
				std::nullopt,
				std::vector<uint8_t>());

			std::shared_ptr<IAssetRequest> mockRequest = std::make_shared<MockAssetRequest>(
				"GET",
				"test.com",
				HttpHeaders{},
				std::move(mockResponse)
			);

			std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase = std::make_unique<MockStoreCacheDatabase>();
			MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();
			std::shared_ptr<CacheAssetAccessor> cacheAssetAccessor = std::make_shared<CacheAssetAccessor>(
				std::make_unique<MockAssetAccessor>(mockRequest),
				std::move(ownedMockCacheDatabase)
			);
			std::shared_ptr<MockTaskProcessor> mockTaskProcessor = std::make_shared<MockTaskProcessor>();

			AsyncSystem asyncSystem(cacheAssetAccessor, mockTaskProcessor);
			asyncSystem.requestAsset("test.com", std::vector<IAssetAccessor::THeader>{});
			REQUIRE(mockCacheDatabase->storeResponseCall == false);
		}

		SECTION("No store if Expires header is less than the current") {
			std::unique_ptr<IAssetResponse> mockResponse = std::make_unique<MockAssetResponse>(
				static_cast<uint16_t>(200),
				"app/json",
				HttpHeaders{ {"Expires", "Wed, 21 Oct 2010 07:28:00 GMT"} },
				std::nullopt,
				std::vector<uint8_t>());

			std::shared_ptr<IAssetRequest> mockRequest = std::make_shared<MockAssetRequest>(
				"GET",
				"test.com",
				HttpHeaders{},
				std::move(mockResponse)
			);

			std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase = std::make_unique<MockStoreCacheDatabase>();
			MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();
			std::shared_ptr<CacheAssetAccessor> cacheAssetAccessor = std::make_shared<CacheAssetAccessor>(
				std::make_unique<MockAssetAccessor>(mockRequest),
				std::move(ownedMockCacheDatabase)
			);
			std::shared_ptr<MockTaskProcessor> mockTaskProcessor = std::make_shared<MockTaskProcessor>();

			AsyncSystem asyncSystem(cacheAssetAccessor, mockTaskProcessor);
			asyncSystem.requestAsset("test.com", std::vector<IAssetAccessor::THeader>{});
			REQUIRE(mockCacheDatabase->storeResponseCall == false);
		}
	}
}

TEST_CASE("Test calculation of expiry time for the cached response") {
	SECTION("Response has max-age cache control") {
		std::unique_ptr<IAssetResponse> mockResponse = std::make_unique<MockAssetResponse>(
			static_cast<uint16_t>(200),
			"app/json",
			HttpHeaders{},
			ResponseCacheControl{ true, false, false, false, false, true, false, 400, 0 },
			std::vector<uint8_t>());

		std::shared_ptr<IAssetRequest> mockRequest = std::make_shared<MockAssetRequest>(
			"GET",
			"test.com",
			HttpHeaders{},
			std::move(mockResponse)
		);

		std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase = std::make_unique<MockStoreCacheDatabase>();
		MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();
		std::shared_ptr<CacheAssetAccessor> cacheAssetAccessor = std::make_shared<CacheAssetAccessor>(
			std::make_unique<MockAssetAccessor>(mockRequest),
			std::move(ownedMockCacheDatabase)
		);
		std::shared_ptr<MockTaskProcessor> mockTaskProcessor = std::make_shared<MockTaskProcessor>();

		AsyncSystem asyncSystem(cacheAssetAccessor, mockTaskProcessor);
		asyncSystem.requestAsset("test.com", std::vector<IAssetAccessor::THeader>{});
		REQUIRE(mockCacheDatabase->storeResponseCall == true);
		REQUIRE(mockCacheDatabase->storeRequestParam->expiryTime - std::time(0) == 400);
	}

	SECTION("Response has Expires header") {
		std::unique_ptr<IAssetResponse> mockResponse = std::make_unique<MockAssetResponse>(
			static_cast<uint16_t>(200),
			"app/json",
			HttpHeaders{ {"Expires", "Wed, 21 Oct 2021 07:28:00 GMT"} },
			ResponseCacheControl{ true, false, false, false, false, true, false, 0, 0 },
			std::vector<uint8_t>());

		std::shared_ptr<IAssetRequest> mockRequest = std::make_shared<MockAssetRequest>(
			"GET",
			"test.com",
			HttpHeaders{},
			std::move(mockResponse)
		);

		std::unique_ptr<MockStoreCacheDatabase> ownedMockCacheDatabase = std::make_unique<MockStoreCacheDatabase>();
		MockStoreCacheDatabase* mockCacheDatabase = ownedMockCacheDatabase.get();
		std::shared_ptr<CacheAssetAccessor> cacheAssetAccessor = std::make_shared<CacheAssetAccessor>(
			std::make_unique<MockAssetAccessor>(mockRequest),
			std::move(ownedMockCacheDatabase)
		);
		std::shared_ptr<MockTaskProcessor> mockTaskProcessor = std::make_shared<MockTaskProcessor>();

		AsyncSystem asyncSystem(cacheAssetAccessor, mockTaskProcessor);
		asyncSystem.requestAsset("test.com", std::vector<IAssetAccessor::THeader>{});
		REQUIRE(mockCacheDatabase->storeResponseCall == true);
		REQUIRE(mockCacheDatabase->storeRequestParam->expiryTime == 1634801280);
	}
}

TEST_CASE("Test serving cache item") {
	SECTION("Cache item doesn't exist") {
		std::unique_ptr<IAssetResponse> mockResponse = std::make_unique<MockAssetResponse>(
			static_cast<uint16_t>(200),
			"app/json",
			HttpHeaders{ {"Response-Header", "Response-Value"} },
			std::nullopt,
			std::vector<uint8_t>());

		std::shared_ptr<IAssetRequest> mockRequest = std::make_shared<MockAssetRequest>(
			"GET",
			"test.com",
			HttpHeaders{ {"Request-Header", "Request-Value"} },
			std::move(mockResponse)
		);

		std::shared_ptr<CacheAssetAccessor> cacheAssetAccessor = std::make_shared<CacheAssetAccessor>(
			std::make_unique<MockAssetAccessor>(mockRequest),
			std::make_unique<MockStoreCacheDatabase>()
		);
		std::shared_ptr<MockTaskProcessor> mockTaskProcessor = std::make_shared<MockTaskProcessor>();

		// test that the response is from the server
		AsyncSystem asyncSystem(cacheAssetAccessor, mockTaskProcessor);
		asyncSystem.requestAsset("test.com", std::vector<IAssetAccessor::THeader>{})
			.thenInMainThread([](std::shared_ptr<IAssetRequest> completedRequest) {
				REQUIRE(completedRequest != nullptr);
				REQUIRE(completedRequest->url() == "test.com");
				REQUIRE(completedRequest->headers() == HttpHeaders{ {"Request-Header", "Request-Value"} });
				REQUIRE(completedRequest->method() == "GET");

				const IAssetResponse* response = completedRequest->response();
				REQUIRE(response != nullptr);
				REQUIRE(response->headers() == HttpHeaders{ {"Response-Header", "Response-Value"} });
				REQUIRE(response->statusCode() == 200);
				REQUIRE(response->contentType() == "app/json");
				REQUIRE(response->data().empty());
				REQUIRE(response->cacheControl() == nullptr);
			});
		asyncSystem.dispatchMainThreadTasks();
	}

	SECTION("Successfully retrieve cache item") {
		// create mock request and mock response. They are intended to be different from
		// the cache content so that we can verify the response in the callback comes from the cache 
		std::unique_ptr<IAssetResponse> mockResponse = std::make_unique<MockAssetResponse>(
			static_cast<uint16_t>(200),
			"app/json",
			HttpHeaders{},
			std::nullopt,
			std::vector<uint8_t>());

		std::shared_ptr<IAssetRequest> mockRequest = std::make_shared<MockAssetRequest>(
			"GET",
			"test.com",
			HttpHeaders{},
			std::move(mockResponse)
		);

		// mock fresh cache item
		std::unique_ptr<MockStoreCacheDatabase> mockCacheDatabase = std::make_unique<MockStoreCacheDatabase>();
		std::time_t currentTime = std::time(0);
		CacheRequest cacheRequest(
			HttpHeaders{ { "Cache-Request-Header", "Cache-Request-Value" } }, 
			"GET", 
			"cache.com");
		CacheResponse cacheResponse(
			static_cast<uint16_t>(200), 
			"app/json", 
			HttpHeaders{ {"Cache-Response-Header", "Cache-Response-Value"} },
			ResponseCacheControl{ false, false, false, false, false, true, false, 100, 0 },
			std::vector<uint8_t>());
		CacheItem cacheItem(currentTime + 100, currentTime, cacheRequest, cacheResponse);
		mockCacheDatabase->cacheItem = cacheItem;

		std::shared_ptr<CacheAssetAccessor> cacheAssetAccessor = std::make_shared<CacheAssetAccessor>(
			std::make_unique<MockAssetAccessor>(mockRequest),
			std::move(mockCacheDatabase)
		);
		std::shared_ptr<MockTaskProcessor> mockTaskProcessor = std::make_shared<MockTaskProcessor>();

		// test that the response is from the cache
		AsyncSystem asyncSystem(cacheAssetAccessor, mockTaskProcessor);
		asyncSystem.requestAsset("test.com", std::vector<IAssetAccessor::THeader>{})
			.thenInMainThread([](std::shared_ptr<IAssetRequest> completedRequest) {
				REQUIRE(completedRequest != nullptr);
				REQUIRE(completedRequest->url() == "cache.com");
				REQUIRE(completedRequest->headers() == HttpHeaders{ {"Cache-Request-Header", "Cache-Request-Value"} });
				REQUIRE(completedRequest->method() == "GET");

				const IAssetResponse* response = completedRequest->response();
				REQUIRE(response != nullptr);
				REQUIRE(response->headers() == HttpHeaders{ {"Cache-Response-Header", "Cache-Response-Value"} });
				REQUIRE(response->statusCode() == 200);
				REQUIRE(response->contentType() == "app/json");
				REQUIRE(response->data().empty());

				REQUIRE(response->cacheControl()->mustRevalidate() == false);
				REQUIRE(response->cacheControl()->noCache() == false);
				REQUIRE(response->cacheControl()->noStore() == false);
				REQUIRE(response->cacheControl()->noTransform() == false);
				REQUIRE(response->cacheControl()->accessControlPublic() == false);
				REQUIRE(response->cacheControl()->accessControlPrivate() == true);
				REQUIRE(response->cacheControl()->proxyRevalidate() == false);
				REQUIRE(response->cacheControl()->maxAge() == 100);
				REQUIRE(response->cacheControl()->sharedMaxAge() == 0);
			});
		asyncSystem.dispatchMainThreadTasks();
	}

	SECTION("Retrieve outdated cache item and cache control mandates revalidation before using it") {
		// Mock 304 response
		std::unique_ptr<IAssetResponse> mockResponse = std::make_unique<MockAssetResponse>(
			static_cast<uint16_t>(304),
			"app/json",
			HttpHeaders{ {"Revalidation-Response-Header", "Revalidation-Response-Value"} },
			ResponseCacheControl{ true, false, false, false, false, true, false, 300, 0 },
			std::vector<uint8_t>());

		std::shared_ptr<IAssetRequest> mockRequest = std::make_shared<MockAssetRequest>(
			"GET",
			"test.com",
			HttpHeaders{},
			std::move(mockResponse)
		);

		// mock cache item
		std::unique_ptr<MockStoreCacheDatabase> mockCacheDatabase = std::make_unique<MockStoreCacheDatabase>();
		std::time_t currentTime = std::time(0);
		CacheRequest cacheRequest(
			HttpHeaders{ { "Cache-Request-Header", "Cache-Request-Value" } }, 
			"GET", 
			"cache.com");
		CacheResponse cacheResponse(
			static_cast<uint16_t>(200), 
			"app/json", 
			HttpHeaders{ {"Cache-Response-Header", "Cache-Response-Value"} },
			ResponseCacheControl{ false, false, false, false, false, true, false, 100, 0 },
			std::vector<uint8_t>());
		CacheItem cacheItem(currentTime - 100, currentTime, cacheRequest, cacheResponse);
		mockCacheDatabase->cacheItem = cacheItem;

		std::shared_ptr<CacheAssetAccessor> cacheAssetAccessor = std::make_shared<CacheAssetAccessor>(
			std::make_unique<MockAssetAccessor>(mockRequest),
			std::move(mockCacheDatabase)
		);
		std::shared_ptr<MockTaskProcessor> mockTaskProcessor = std::make_shared<MockTaskProcessor>();

		// test that the response is from the cache and it should update the header and cache control coming 
		// from the validation response
		AsyncSystem asyncSystem(cacheAssetAccessor, mockTaskProcessor);
		asyncSystem.requestAsset("test.com", std::vector<IAssetAccessor::THeader>{})
			.thenInMainThread([](std::shared_ptr<IAssetRequest> completedRequest) {
				REQUIRE(completedRequest != nullptr);
				REQUIRE(completedRequest->url() == "cache.com");
				REQUIRE(completedRequest->headers() == HttpHeaders{ {"Cache-Request-Header", "Cache-Request-Value"} });
				REQUIRE(completedRequest->method() == "GET");

				// check response header is updated
				const IAssetResponse* response = completedRequest->response();
				REQUIRE(response != nullptr);
				REQUIRE(response->headers() == HttpHeaders{ 
					{"Revalidation-Response-Header", "Revalidation-Response-Value"},
					{"Cache-Response-Header", "Cache-Response-Value"} });
				REQUIRE(response->statusCode() == 200);
				REQUIRE(response->contentType() == "app/json");
				REQUIRE(response->data().empty());

				// check cache control is updated
				REQUIRE(response->cacheControl()->mustRevalidate() == true);
				REQUIRE(response->cacheControl()->noCache() == false);
				REQUIRE(response->cacheControl()->noStore() == false);
				REQUIRE(response->cacheControl()->noTransform() == false);
				REQUIRE(response->cacheControl()->accessControlPublic() == false);
				REQUIRE(response->cacheControl()->accessControlPrivate() == true);
				REQUIRE(response->cacheControl()->proxyRevalidate() == false);
				REQUIRE(response->cacheControl()->maxAge() == 300);
				REQUIRE(response->cacheControl()->sharedMaxAge() == 0);
			});
		asyncSystem.dispatchMainThreadTasks();
	}

	SECTION("Cache should serve validation response from the server directly if it is not 304") {
		// Mock 200 response
		std::unique_ptr<IAssetResponse> mockResponse = std::make_unique<MockAssetResponse>(
			static_cast<uint16_t>(200),
			"app/json",
			HttpHeaders{ {"Revalidation-Response-Header", "Revalidation-Response-Value"} },
			std::nullopt,
			std::vector<uint8_t>());

		std::shared_ptr<IAssetRequest> mockRequest = std::make_shared<MockAssetRequest>(
			"GET",
			"test.com",
			HttpHeaders{},
			std::move(mockResponse)
		);

		// mock cache item
		std::unique_ptr<MockStoreCacheDatabase> mockCacheDatabase = std::make_unique<MockStoreCacheDatabase>();
		std::time_t currentTime = std::time(0);
		CacheRequest cacheRequest(
			HttpHeaders{ { "Cache-Request-Header", "Cache-Request-Value" } }, 
			"GET", 
			"cache.com");
		CacheResponse cacheResponse(
			static_cast<uint16_t>(200), 
			"app/json", 
			HttpHeaders{ {"Cache-Response-Header", "Cache-Response-Value"} },
			ResponseCacheControl{ false, false, false, false, false, true, false, 100, 0 },
			std::vector<uint8_t>());
		CacheItem cacheItem(currentTime - 100, currentTime, cacheRequest, cacheResponse);
		mockCacheDatabase->cacheItem = cacheItem;

		std::shared_ptr<CacheAssetAccessor> cacheAssetAccessor = std::make_shared<CacheAssetAccessor>(
			std::make_unique<MockAssetAccessor>(mockRequest),
			std::move(mockCacheDatabase)
		);
		std::shared_ptr<MockTaskProcessor> mockTaskProcessor = std::make_shared<MockTaskProcessor>();

		// test that the response is from the server directly
		AsyncSystem asyncSystem(cacheAssetAccessor, mockTaskProcessor);
		asyncSystem.requestAsset("test.com", std::vector<IAssetAccessor::THeader>{})
			.thenInMainThread([](std::shared_ptr<IAssetRequest> completedRequest) {
				REQUIRE(completedRequest != nullptr);
				REQUIRE(completedRequest->url() == "test.com");
				REQUIRE(completedRequest->headers() == HttpHeaders{});
				REQUIRE(completedRequest->method() == "GET");

				const IAssetResponse* response = completedRequest->response();
				REQUIRE(response != nullptr);
				REQUIRE(response->headers() == HttpHeaders{ {"Revalidation-Response-Header", "Revalidation-Response-Value"} });
				REQUIRE(response->statusCode() == 200);
				REQUIRE(response->contentType() == "app/json");
				REQUIRE(response->data().empty());
				REQUIRE(response->cacheControl() == nullptr);
			});
		asyncSystem.dispatchMainThreadTasks();
	}
}

