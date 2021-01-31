#pragma once

#include "CesiumAsync/CacheItem.h"
#include "CesiumAsync/IAssetRequest.h"
#include <optional>

namespace CesiumAsync {

    /**
     * @brief Provides database storage interface to cache completed request.
     */
	class ICacheDatabase {
	public:
		virtual ~ICacheDatabase() noexcept = default;

        /**
         * @brief Get cache entry from the database. 
		 * predicate callback will be invoked if there are any existing cache 
         * entries in the database. The predicate should return true to stop the database
         * from searching more entries associated with the key.
         * @param key the unique key associated with the cache entries 
         * @param predicate the function that is invoked when there are any existing cache item.
         * @param error the error message when there are problems happening when retrieving cache entry
         * @return A boolean true if there are no errors when calling this function
         */
		virtual bool getEntry(const std::string& key, 
			std::function<bool(CacheItem)> predicate, 
			std::string& error) const = 0;

        /**
         * @brief Store response into the database. 
         * @param key the unique key associated with the response
         * @param expiryTime the time point that this response should be expired. An expired response will be removed when prunning the database
         * @param request the completed request and response that will be stored in the database
         * @param error the error message when there are problems happening when storing the response
         * @return A boolean true if there are no errors when calling this function
         */
		virtual bool storeResponse(const std::string& key, 
			std::time_t expiryTime,
			const IAssetRequest& request,
			std::string& error) = 0;

        /**
         * @brief Remove cache entry from the database. 
         * @param key the unique key associated with the entries that will be removed
         * @param error the error message when there are problems happening when deleting entries
         * @return A boolean true if there are no errors when calling this function
         */
		virtual bool removeEntry(const std::string& key, std::string& error) = 0;

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
