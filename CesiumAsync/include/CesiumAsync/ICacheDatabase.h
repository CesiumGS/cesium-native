#pragma once

#include "CesiumAsync/CacheItem.h"
#include "CesiumAsync/IAssetRequest.h"
#include <optional>

namespace CesiumAsync {
	class ICacheDatabase {
	public:
		virtual ~ICacheDatabase() noexcept = default;

		virtual bool getEntry(const std::string& url, 
			std::optional<CacheItem> &item, 
			std::string& error) const = 0;

		virtual bool storeResponse(const std::string& key, 
			std::time_t expiryTime,
			const IAssetRequest& request,
			std::string& error) = 0;

		virtual bool removeEntry(const std::string& key, std::string& error) = 0;

		virtual bool prune(std::string& error) = 0;

		virtual bool clearAll(std::string& error) = 0;
	};
}
