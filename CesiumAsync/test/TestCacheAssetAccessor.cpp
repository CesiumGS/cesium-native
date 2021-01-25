#include "catch2/catch.hpp"
#include "MockAssetRequest.h"
#include "MockAssetResponse.h"
#include "CesiumAsync/CacheAssetAccessor.h"
#include "CesiumAsync/ICacheDatabase.h"
#include "CesiumAsync/ITaskProcessor.h"
#include "CesiumAsync/AsyncSystem.h"

using namespace CesiumAsync;

class MockStoreCacheDatabase : public ICacheDatabase {
public:
	MockStoreCacheDatabase() 
		: getEntryCall{false}
		, storeResponseCall{false}
		, removeEntryCall{false}
		, pruneCall{false}
		, clearAllCall{false}
	{}

	virtual bool getEntry(const std::string& /*key*/,
		std::optional<CacheItem>& /*item*/,
		std::string& /*error*/) const override 
	{
		this->getEntryCall = true;
		return true;
	}

	virtual bool storeResponse(const std::string& /*key*/,
		std::time_t /*expiryTime*/,
		const IAssetRequest& /*request*/,
		std::string& /*error*/) override
	{
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
				HttpHeaders{ {"Expires", "Wed, 21 Oct 2015 07:28:00 GMT"} }, 
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
		SECTION("No store for requests that don't have GET method") {
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

		SECTION("No store for requests that have Authorization header") {
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

		SECTION("No store for response that have non-cacheable status code") {
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

		SECTION("No store for response that have No-Store in the cache-control header") {
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

		SECTION("No store for response that have no Cache-Control and Expires header") {
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
	}
}

TEST_CASE("Test expiry time of the cached response") {
	SECTION("Response has max-age cache control") {
		
	}

	SECTION("Response has Expires header") {

	}
}

