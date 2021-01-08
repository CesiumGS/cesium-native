#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/ICacheDatabase.h"
#include "sqlite3.h"
#include <memory>
#include <optional>
#include <string>
#include <map>

namespace CesiumAsync {
	class DiskCache : public ICacheDatabase {
	public:
		DiskCache(const std::string &databaseName);

		DiskCache(const DiskCache&) = delete;

		DiskCache(DiskCache&& other) noexcept;

		DiskCache& operator=(const DiskCache&) = delete;

		DiskCache& operator=(DiskCache&&) noexcept;

		~DiskCache() noexcept override;

		virtual std::optional<CacheItem> getEntry(const std::string& url) const override;

		virtual void storeResponse(const std::string& key, 
			std::time_t expiryTime,
			const IAssetRequest& request,
			std::function<void(std::string)> onError) override;

		virtual void removeEntry(const std::string& url) override;

		virtual void prune() override;

		virtual void clearAll() override;

	private:
		sqlite3* _pConnection;
	};
}