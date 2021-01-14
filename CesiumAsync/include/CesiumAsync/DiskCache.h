#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/ICacheDatabase.h"
#include "sqlite3.h"
#include <mutex>
#include <memory>
#include <optional>
#include <string>
#include <map>

namespace CesiumAsync {
	class DiskCache : public ICacheDatabase {
	public:
		DiskCache(const std::string &databaseName, uint64_t maxSize = 512 * 1024 * 1024);

		~DiskCache() noexcept;

		virtual bool getEntry(const std::string& url, 
			std::optional<CacheItem> &item, 
			std::string& error) const override;

		virtual bool storeResponse(const std::string& key, 
			std::time_t expiryTime,
			const IAssetRequest& request,
			std::string& error) override;

		virtual bool removeEntry(const std::string& key, std::string& error) override;

		virtual bool prune(std::string& error) override;

		virtual bool clearAll(std::string& error) override;

	private:
		uint64_t _maxSize;
		std::string _databaseName;
	};
}