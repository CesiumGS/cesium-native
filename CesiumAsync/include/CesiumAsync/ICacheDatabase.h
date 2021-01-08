#pragma once

#include "CesiumAsync/CacheItem.h"
#include "CesiumAsync/IAssetRequest.h"
#include <optional>

namespace CesiumAsync {
	class ICacheDatabase {
	public:
		virtual ~ICacheDatabase() noexcept = default;

		virtual std::optional<CacheItem> getEntry(const std::string& key) const = 0;

		virtual void storeResponse(const std::string& key, 
			std::time_t expiryTime,
			const IAssetRequest& request,
			std::function<void(std::string)> onError) = 0;

		virtual void removeEntry(const std::string& key) = 0;

		virtual void prune() = 0;

		virtual void clearAll() = 0;
	};
}
