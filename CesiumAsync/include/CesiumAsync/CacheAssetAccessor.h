#pragma once

#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/ICacheDatabase.h"
#include <string>
#include <memory>

namespace CesiumAsync {
    class AsyncSystem;

	class CacheAssetAccessor : public IAssetAccessor {
	public:
        CacheAssetAccessor(
            std::unique_ptr<IAssetAccessor> assetAccessor,
            std::unique_ptr<ICacheDatabase> cacheDatabase);

        ~CacheAssetAccessor() noexcept override;

        void requestAsset(const AsyncSystem* pAsyncSystem, 
			const std::string& url, 
			const std::vector<THeader>& headers,
			std::function<void(std::unique_ptr<IAssetRequest>)> callback) override;

        void tick() noexcept override;

    private:
        static bool isCacheValid(const CacheItem& cacheItem);

        static bool shouldCacheRequest(const IAssetRequest& request);

        static std::time_t calculateExpiryTime(const IAssetRequest& request);

		std::unique_ptr<IAssetAccessor> _pAssetAccessor;
		std::unique_ptr<ICacheDatabase> _pCacheDatabase;
	};
}

