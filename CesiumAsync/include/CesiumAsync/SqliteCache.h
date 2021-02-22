#pragma once

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
    class CESIUMASYNC_API SqliteCache : public ICacheDatabase {
    public:
        /**
         * @brief Constructs a new instance with a given `databaseName` pointing to a database.
         * The instance will connect to the existing database or create a new one if it doesn't exist 
         * @param databaseName the database path.
         * @param maxItems the maximum number of items should be kept in the database after prunning.
         */
        SqliteCache(const std::string& databaseName, uint64_t maxItems = 4096);

        /**
         * @brief Destroys this instance. 
         * This will close the database connection
         */
        ~SqliteCache() noexcept;

        /**
         * @brief Deleted copy constructor. 
         */
        SqliteCache(const SqliteCache&) = delete;

        /**
         * @brief Move constructor. 
         */
        SqliteCache(SqliteCache&&) noexcept;

        /**
         * @brief Deleted copy assignment. 
         */
        SqliteCache& operator=(const SqliteCache&) = delete;

        /**
         * @brief Move assignment. 
         */
        SqliteCache& operator=(SqliteCache&&) noexcept;

        /** @copydoc ICacheDatabase::getEntry(const std::string&, std::function<bool(CacheItem)>, std::string&)*/
        virtual CacheLookupResult getEntry(const std::string& key) const override;

        /** @copydoc ICacheDatabase::storeEntry*/
        virtual CacheStoreResult storeEntry(
            const std::string& key,
            std::time_t expiryTime,
            const std::string& url,
            const std::string& requestMethod,
            const HttpHeaders& requestHeaders,
            uint16_t statusCode,
            const HttpHeaders& responseHeaders,
            const gsl::span<const uint8_t>& responseData
        ) override;

        /** @copydoc ICacheDatabase::prune(std::string&)*/
        virtual bool prune(std::string& error) override;

        /** @copydoc ICacheDatabase::clearAll(std::string&)*/
        virtual bool clearAll(std::string& error) override;

    private:
        struct Sqlite3StmtWrapper {
            Sqlite3StmtWrapper() 
                : stmt{nullptr}
            {}

            ~Sqlite3StmtWrapper() {
                if (stmt) {
                    sqlite3_finalize(stmt);
                }
            }

            sqlite3_stmt *stmt;
        };

        sqlite3* _pConnection;
        uint64_t _maxItems;
        mutable std::mutex _mutex;
        Sqlite3StmtWrapper _getEntryStmtWrapper;
        Sqlite3StmtWrapper _updateLastAccessedTimeStmtWrapper;
        Sqlite3StmtWrapper _storeResponseStmtWrapper;
        Sqlite3StmtWrapper _totalItemsQueryStmtWrapper;
        Sqlite3StmtWrapper _deleteExpiredStmtWrapper;
        Sqlite3StmtWrapper _deleteLRUStmtWrapper;
        Sqlite3StmtWrapper _clearAllStmtWrapper;
    };
}