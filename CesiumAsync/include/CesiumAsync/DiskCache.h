#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/ICacheDatabase.h"
#include <sqlite3.h>
#include <mutex>
#include <memory>
#include <optional>
#include <string>
#include <map>

namespace CesiumAsync {

    /**
     * @brief Cache storage using SQLITE to store completed response.
     */
	class DiskCache : public ICacheDatabase {
	public:
        /**
         * @brief Constructs a new instance with a given `databaseName` pointing to a database.
		 * The instance will connect to the existing database or create a new one if it doesn't exist 
         * @param databaseName the database path.
         * @param maxItems the maximum number of items should be kept in the database after prunning.
         */
		DiskCache(const std::string &databaseName, uint64_t maxItems = 512);

        /**
         * @brief Destroys this instance. 
         * This will close the database connection
         */
		~DiskCache() noexcept;

        /**
         * @brief Deleted copy constructor. 
         */
		DiskCache(const DiskCache&) = delete;

        /**
         * @brief Move constructor. 
         */
		DiskCache(DiskCache&&) noexcept;

        /**
         * @brief Deleted copy assignment. 
         */
		DiskCache& operator=(const DiskCache&) = delete;

        /**
         * @brief Move assignment. 
         */
		DiskCache& operator=(DiskCache&&) noexcept;

        /** @copydoc ICacheDatabase::getEntry(const std::string&, std::function<bool(CacheItem)>, std::string&)*/
		virtual bool getEntry(const std::string& key, 
			std::function<bool(CacheItem)> predicate, 
			std::string& error) const override;

        /** @copydoc ICacheDatabase::storeResponse(const std::string&, std::time_t, const IAssetRequest&, std::string&)*/
		virtual bool storeResponse(const std::string& key, 
			std::time_t expiryTime,
			const IAssetRequest& request,
			std::string& error) override;

        /** @copydoc ICacheDatabase::prune(std::string&)*/
		virtual bool prune(std::string& error) override;

        /** @copydoc ICacheDatabase::clearAll(std::string&)*/
		virtual bool clearAll(std::string& error) override;

	private:
		sqlite3* _pConnection;
		uint64_t _maxItems;
	};
}