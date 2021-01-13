#pragma once

#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/ICacheDatabase.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CacheAssetResponse.h"

namespace CesiumAsync {
	class CacheAssetRequest : public IAssetRequest 
	{
	public:
        CacheAssetRequest(const std::string& url, 
            const std::vector<IAssetAccessor::THeader>& headers,
            const AsyncSystem* pAsyncSystem,
            IAssetAccessor* pAssetAccessor, 
            ICacheDatabase* pCacheDatabase);

        virtual const std::string& method() const override;

        virtual const std::string& url() const override;

        virtual const std::map<std::string, std::string> &headers() const override;

        virtual const IAssetResponse* response() const override;

        virtual void bind(std::function<void(IAssetRequest*)> callback) override;

        virtual void cancel() noexcept override;

	private:
        static bool isCacheValid(const CacheItem& cacheItem);

        static bool shouldCacheRequest(const IAssetRequest& request);

        static std::time_t calculateExpiryTime(const IAssetRequest& request);

        static std::string hashRequest(const IAssetRequest& request);

        IAssetAccessor* _pAssetAccessor;
        ICacheDatabase* _pCacheDatabase;
        const AsyncSystem* _pAsyncSystem;
        std::shared_ptr<IAssetRequest> _pRequest;
        std::optional<CacheItem> _cacheItem;
        std::optional<CacheAssetResponse> _cacheAssetResponse;
        std::string _url;
        std::vector<IAssetAccessor::THeader> _headers;
	};
}