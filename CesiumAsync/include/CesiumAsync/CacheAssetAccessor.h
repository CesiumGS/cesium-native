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

        std::unique_ptr<IAssetRequest> requestAsset(const AsyncSystem* asyncSystem, 
            const std::string& url, 
            const std::vector<THeader>& headers = std::vector<THeader>()) override;

        void tick() noexcept override;

    private:
		std::unique_ptr<IAssetAccessor> _pAssetAccessor;
		std::unique_ptr<ICacheDatabase> _pCacheDatabase;
	};
}

