#pragma once

#include "CesiumAsync/Library.h"
#include "CesiumAsync/CacheItem.h"
#include "CesiumAsync/IAssetRequest.h"
#include <optional>

namespace CesiumAsync {
    /**
     * @brief The result of a cache lookup with {@link ICacheDatabase::getEntry}.
     */
    struct CacheLookupResult {
        /**
         * @brief The found cache item, or `std::nullopt`.
         * 
         * This property will be `std::nullopt` if the cache key does not exist in the
         * database, or if an error occurs.
         */
        std::optional<CacheItem> item;

        /**
         * @brief Details of an error that occurred during cache lookup, if any.
         * 
         * If the string is empty, no error occurred.
         */
        std::string error;
    };

    /**
     * @brief The result of a cache store with {@link ICacheDatabase::storeEntry}.
     */
    struct CacheStoreResult {
        /**
         * @brief Details of an error that occuring during cache store, if any.
         * 
         * If the string is empty, no error occurred.
         */
        std::string error;
    };

    /**
     * @brief Provides database storage interface to cache completed request.
     */
    class CESIUMASYNC_API ICacheDatabase {
    public:
        virtual ~ICacheDatabase() noexcept = default;

        /**
         * @brief Gets a cache entry from the database.
         * 
         * @param key The unique key associated with the cache entry.
         * @return The result of the cache lookup.
         */
        virtual CacheLookupResult getEntry(const std::string& key) const = 0;

        /**
         * @brief Store response into the database. 
         * @param key the unique key associated with the response
         * @param expiryTime the time point that this response should be expired. An expired response will be removed when prunning the database
         * @param request the completed request and response that will be stored in the database
         * @param error the error message when there are problems happening when storing the response
         * @return A boolean true if there are no errors when calling this function
         */
        virtual CacheStoreResult storeEntry(
            const std::string& key,
            std::time_t expiryTime,
            const std::string& url,
            const std::string& requestMethod,
            const HttpHeaders& requestHeaders,
            uint16_t statusCode,
            const HttpHeaders& responseHeaders,
            const gsl::span<const uint8_t>& responseData
        ) = 0;

        /**
         * @brief Remove cache entries from the database to satisfy the database invariant condition (.e.g exired response or LRU). 
         * @param error the error message when there are problems happening when deleting entries
         * @return A boolean true if there are no errors when calling this function
         */
        virtual bool prune(std::string& error) = 0;

        /**
         * @brief Remove all cache entries from the database. 
         * @param error the error message when there are problems happening when deleting entries
         * @return A boolean true if there are no errors when calling this function
         */
        virtual bool clearAll(std::string& error) = 0;
    };
}
