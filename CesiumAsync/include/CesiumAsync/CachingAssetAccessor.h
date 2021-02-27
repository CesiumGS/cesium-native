// Copyright CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/ICacheDatabase.h"
#include <spdlog/fwd.h>
#include <string>
#include <memory>
#include <atomic>

namespace CesiumAsync {
    class AsyncSystem;

    /**
     * @brief A decorator for an {@link IAssetAccessor} that caches requests and responses in an {@link ICacheDatabase}.
     * 
     * This can be used to improve asset loading performance by caching assets across runs.
     */
    class CachingAssetAccessor : public IAssetAccessor {
    public:
        /**
         * @brief Constructs a new instance.
         * 
         * @param pLogger The logger that receives messages about the status of this instance.
         * @param pAssetAccessor The underlying {@link IAssetAccessor} used to retrieve assets that are not in the cache.
         * @param pCacheDatabase The database in which to cache requests and responses.
         * @param requestsPerCacheClean The number of requests to handle before each {@link ICacheDatabase::prune} of old cached results from the database.
         */
        CachingAssetAccessor(
            const std::shared_ptr<spdlog::logger>& pLogger,
            const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
            const std::shared_ptr<ICacheDatabase>& pCacheDatabase,
            int32_t requestsPerCachePrune = 10000);

        virtual ~CachingAssetAccessor() noexcept override;

        /** @copydoc IAssetAccessor::requestAsset */
        virtual Future<std::shared_ptr<IAssetRequest>> requestAsset(
            const AsyncSystem& asyncSystem,
            const std::string& url, 
            const std::vector<THeader>& headers) override;

        /** @copydoc IAssetAccessor::tick */
        virtual void tick() noexcept override;

    private:
        int32_t _requestsPerCachePrune;
        std::atomic<int32_t> _requestSinceLastPrune;
        std::shared_ptr<spdlog::logger> _pLogger;
        std::shared_ptr<IAssetAccessor> _pAssetAccessor;
        std::shared_ptr<ICacheDatabase> _pCacheDatabase;
    };
}
