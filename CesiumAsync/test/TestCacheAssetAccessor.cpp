#include "catch2/catch.hpp"
#include "MockAssetRequest.h"
#include "MockAssetResponse.h"
#include "CesiumAsync/CacheAssetAccessor.h"
#include "CesiumAsync/ICacheDatabase.h"
#include "CesiumAsync/ITaskProcessor.h"
#include "CesiumAsync/AsyncSystem.h"

using namespace CesiumAsync;

TEST_CASE("Test the condition of not caching the request") {
	class MockNoStoreCacheDatabase : public ICacheDatabase {
	public:
		virtual bool getEntry(const std::string& /*key*/,
			std::optional<CacheItem>& /*item*/,
			std::string& /*error*/) const override 
		{
			return true;
		}

		virtual bool storeResponse(const std::string& /*key*/,
			std::time_t /*expiryTime*/,
			const IAssetRequest& /*request*/,
			std::string& /*error*/) override
		{
			REQUIRE(false);
			return true;
		}

		virtual bool removeEntry(const std::string& /*key*/, std::string& /*error*/) override
		{
			return true;
		}

		virtual bool prune(std::string& /*error*/) override
		{
			return true;
		}

		virtual bool clearAll(std::string& /*error*/) override
		{
			return true;
		}
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

	SECTION("No store for requests that don't have GET method") {
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

		std::shared_ptr<CacheAssetAccessor> cacheAssetAccessor = std::make_shared<CacheAssetAccessor>(
			std::make_unique<MockAssetAccessor>(mockRequest),
			std::make_unique<MockNoStoreCacheDatabase>()
		);
		std::shared_ptr<MockTaskProcessor> mockTaskProcessor = std::make_shared<MockTaskProcessor>();

		AsyncSystem asyncSystem(cacheAssetAccessor, mockTaskProcessor);
		asyncSystem.requestAsset("test.com", std::vector<IAssetAccessor::THeader>{});
	}

	SECTION("No store for requests that have Authorization header") {

	}

	SECTION("No store for response that have non-cacheable status code") {

	}

	SECTION("No store for response that have No-Store in the cache-control header") {

	}

	SECTION("No store for response that have no Cache-Control and Expires header") {

	}
}
