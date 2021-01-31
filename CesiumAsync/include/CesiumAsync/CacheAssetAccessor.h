#pragma once

#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/ICacheDatabase.h"
#include <spdlog/fwd.h>
#include <string>
#include <memory>
#include <mutex>

namespace CesiumAsync {
    class AsyncSystem;

	class CacheAssetAccessor : public IAssetAccessor {
	public:
        CacheAssetAccessor(
			std::shared_ptr<spdlog::logger> logger,
            std::unique_ptr<IAssetAccessor> assetAccessor,
            std::unique_ptr<ICacheDatabase> cacheDatabase,
			uint32_t databaseCleanCheckpoint = 10000);

        ~CacheAssetAccessor() noexcept override;

        void requestAsset(const AsyncSystem* pAsyncSystem, 
			const std::string& url, 
			const std::vector<THeader>& headers,
			std::function<void(std::shared_ptr<IAssetRequest>)> callback) override;

        void tick() noexcept override;

    private:
		std::mutex _requestCountMutex;
		uint32_t _databaseCleanCheckpoint;
		uint32_t _requestCount;
		std::shared_ptr<spdlog::logger> _pLogger;
		std::unique_ptr<IAssetAccessor> _pAssetAccessor;
		std::unique_ptr<ICacheDatabase> _pCacheDatabase;
	};
}

