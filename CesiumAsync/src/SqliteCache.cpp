#include "CesiumAsync/SqliteCache.h"
#include "CesiumAsync/IAssetResponse.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include <utility>
#include <stdexcept>

namespace CesiumAsync {
    // Cache table column names
    static const std::string CACHE_TABLE = "CacheItemTable";
    static const std::string CACHE_TABLE_KEY_COLUMN = "key";
    static const std::string CACHE_TABLE_EXPIRY_TIME_COLUMN = "expiryTime";
    static const std::string CACHE_TABLE_LAST_ACCESSED_TIME_COLUMN = "lastAccessedTime";
    static const std::string CACHE_TABLE_RESPONSE_HEADER_COLUMN = "responseHeaders";
    static const std::string CACHE_TABLE_RESPONSE_STATUS_CODE_COLUMN = "responseStatusCode";
    static const std::string CACHE_TABLE_RESPONSE_DATA_COLUMN = "responseData";
    static const std::string CACHE_TABLE_REQUEST_HEADER_COLUMN = "requestHeader";
    static const std::string CACHE_TABLE_REQUEST_METHOD_COLUMN = "requestMethod";
    static const std::string CACHE_TABLE_REQUEST_URL_COLUMN = "requestUrl";
    static const std::string CACHE_TABLE_VIRTUAL_TOTAL_ITEMS_COLUMN = "totalItems";

    // Sql commands for setting up database
    static const std::string CREATE_CACHE_TABLE_SQL = "CREATE TABLE IF NOT EXISTS " + CACHE_TABLE + "(" +
            CACHE_TABLE_KEY_COLUMN + " TEXT PRIMARY KEY NOT NULL," +
            CACHE_TABLE_EXPIRY_TIME_COLUMN + " DATETIME NOT NULL," +
            CACHE_TABLE_LAST_ACCESSED_TIME_COLUMN + " DATETIME NOT NULL," +
            CACHE_TABLE_RESPONSE_HEADER_COLUMN + " TEXT NOT NULL," +
            CACHE_TABLE_RESPONSE_STATUS_CODE_COLUMN + " INTEGER NOT NULL," +
            CACHE_TABLE_RESPONSE_DATA_COLUMN + " BLOB," +
            CACHE_TABLE_REQUEST_HEADER_COLUMN + " TEXT NOT NULL," +
            CACHE_TABLE_REQUEST_METHOD_COLUMN + " TEXT NOT NULL," +
            CACHE_TABLE_REQUEST_URL_COLUMN + " TEXT NOT NULL)";

    static const std::string PRAGMA_WAL_SQL = "PRAGMA journal_mode=WAL";

    static const std::string PRAGMA_SYNC_SQL = "PRAGMA synchronous=OFF";

    static const std::string PRAGMA_PAGE_SIZE_SQL = "PRAGMA page_size=4096";

    // Sql commands for getting entry from database
    static const std::string GET_ENTRY_SQL = "SELECT rowid, " +
        CACHE_TABLE_EXPIRY_TIME_COLUMN + ", " +
        CACHE_TABLE_RESPONSE_HEADER_COLUMN + ", " +
        CACHE_TABLE_RESPONSE_STATUS_CODE_COLUMN + ", " +
        CACHE_TABLE_RESPONSE_DATA_COLUMN + ", " +
        CACHE_TABLE_REQUEST_HEADER_COLUMN + ", " +
        CACHE_TABLE_REQUEST_METHOD_COLUMN + ", " +
        CACHE_TABLE_REQUEST_URL_COLUMN +
        " FROM " + CACHE_TABLE + " WHERE " + CACHE_TABLE_KEY_COLUMN + "=?";

    static const std::string UPDATE_LAST_ACCESSED_TIME_SQL = "UPDATE " + CACHE_TABLE + 
        " SET " + CACHE_TABLE_LAST_ACCESSED_TIME_COLUMN + " = strftime('%s','now') WHERE " + CACHE_TABLE_KEY_COLUMN + " =?";

    // Sql commands for storing response
    static const std::string STORE_RESPONSE_SQL = "REPLACE INTO " + CACHE_TABLE + " (" +
        CACHE_TABLE_EXPIRY_TIME_COLUMN + ", " +
        CACHE_TABLE_LAST_ACCESSED_TIME_COLUMN + ", " +
        CACHE_TABLE_RESPONSE_HEADER_COLUMN + ", " +
        CACHE_TABLE_RESPONSE_STATUS_CODE_COLUMN + ", " +
        CACHE_TABLE_RESPONSE_DATA_COLUMN + ", " +
        CACHE_TABLE_REQUEST_HEADER_COLUMN + ", " +
        CACHE_TABLE_REQUEST_METHOD_COLUMN + ", " +
        CACHE_TABLE_REQUEST_URL_COLUMN + ", " +
        CACHE_TABLE_KEY_COLUMN +
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

    // Sql commands for prunning the database
    static const std::string TOTAL_ITEMS_QUERY_SQL = "SELECT COUNT(*) " + CACHE_TABLE_VIRTUAL_TOTAL_ITEMS_COLUMN + " FROM " + CACHE_TABLE;

    static const std::string DELETE_EXPIRED_ITEMS_SQL = "DELETE FROM " + CACHE_TABLE + " WHERE " + CACHE_TABLE_EXPIRY_TIME_COLUMN + " < strftime('%s','now')";

    static const std::string DELETE_LRU_ITEMS_SQL = "DELETE FROM " + CACHE_TABLE + " WHERE rowid " + 
        " IN (SELECT rowid FROM " + CACHE_TABLE + " ORDER BY " + CACHE_TABLE_LAST_ACCESSED_TIME_COLUMN + " ASC " + " LIMIT ?)";

    // Sql commands for clean all items
    static const std::string CLEAR_ALL_SQL = "DELETE FROM " + CACHE_TABLE;

    static std::string convertHeadersToString(const HttpHeaders& headers);

    static std::string convertCacheControlToString(const ResponseCacheControl* cacheControl);

    static HttpHeaders convertStringToHeaders(const std::string& serializedHeaders);

    static std::optional<ResponseCacheControl> convertStringToResponseCacheControl(const char* serializedResponseCacheControl);

    DiskCache::DiskCache(const std::string &databaseName, uint64_t maxItems)
        : _pConnection{nullptr}
        , _maxItems{maxItems}
    {
        int status = sqlite3_open(databaseName.c_str(), &this->_pConnection);
        if (status != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errstr(status));
        }

        // create cache tables if not exist. Key -> Cache table: one-to-many relationship
        char* createTableError = nullptr;
        status = sqlite3_exec(this->_pConnection, CREATE_CACHE_TABLE_SQL.c_str(), nullptr, nullptr, &createTableError);
        if (status != SQLITE_OK) {
            std::string errorStr(createTableError);
            sqlite3_free(createTableError);
            throw std::runtime_error(errorStr);
        }

        // turn on WAL mode
        char* walError = nullptr;
        status = sqlite3_exec(this->_pConnection, PRAGMA_WAL_SQL.c_str(), nullptr, nullptr, &walError);
        if (status != SQLITE_OK) {
            std::string errorStr(walError);
            sqlite3_free(walError);
            throw std::runtime_error(errorStr);
        }

        // turn off synchronous mode
        char* syncError = nullptr;
        status = sqlite3_exec(this->_pConnection, PRAGMA_SYNC_SQL.c_str(), nullptr, nullptr, &syncError);
        if (status != SQLITE_OK) {
            std::string errorStr(syncError);
            sqlite3_free(syncError);
            throw std::runtime_error(errorStr);
        }

        // increase page size
        char* pageSizeError = nullptr;
        status = sqlite3_exec(this->_pConnection, PRAGMA_PAGE_SIZE_SQL.c_str(), nullptr, nullptr, &pageSizeError);
        if (status != SQLITE_OK) {
            std::string errorStr(pageSizeError);
            sqlite3_free(pageSizeError);
            throw std::runtime_error(errorStr);
        }

        // get entry based on key
        status = sqlite3_prepare_v2(_pConnection, GET_ENTRY_SQL.c_str(), -1, &this->_getEntryStmtWrapper.stmt, nullptr);
        if (status != SQLITE_OK) {
            throw std::runtime_error(std::string(sqlite3_errstr(status)));
        }

        // update last accessed for entry
        status = sqlite3_prepare_v2(
            this->_pConnection, UPDATE_LAST_ACCESSED_TIME_SQL.c_str(), -1, &this->_updateLastAccessedTimeStmtWrapper.stmt, nullptr);
        if (status != SQLITE_OK) {
            throw std::runtime_error(std::string(sqlite3_errstr(status)));
        }

        // store response 
        status = sqlite3_prepare_v2(this->_pConnection, STORE_RESPONSE_SQL.c_str(), -1, &this->_storeResponseStmtWrapper.stmt, nullptr);
        if (status != SQLITE_OK) {
            throw std::runtime_error(std::string(sqlite3_errstr(status)));
        }

        // query total items
        status = sqlite3_prepare_v2(this->_pConnection, TOTAL_ITEMS_QUERY_SQL.c_str(), -1, &this->_totalItemsQueryStmtWrapper.stmt, nullptr);
        if (status != SQLITE_OK) {
            throw std::runtime_error(std::string(sqlite3_errstr(status)));
        }

        // delete expired items
        status = sqlite3_prepare_v2(this->_pConnection, DELETE_EXPIRED_ITEMS_SQL.c_str(), -1, &this->_deleteExpiredStmtWrapper.stmt, nullptr);
        if (status != SQLITE_OK) {
            throw std::runtime_error(std::string(sqlite3_errstr(status)));
        }

        // delete expired items
        status = sqlite3_prepare_v2(this->_pConnection, DELETE_LRU_ITEMS_SQL.c_str(), -1, &this->_deleteLRUStmtWrapper.stmt, nullptr);
        if (status != SQLITE_OK) {
            throw std::runtime_error(std::string(sqlite3_errstr(status)));
        }

        // clear all items
        status = sqlite3_prepare_v2(this->_pConnection, CLEAR_ALL_SQL.c_str(), -1, &this->_clearAllStmtWrapper.stmt, nullptr);
        if (status != SQLITE_OK) {
            throw std::runtime_error(std::string(sqlite3_errstr(status)));
        }
    }

    DiskCache::DiskCache(DiskCache&& other) noexcept {
        this->_pConnection = other._pConnection;
        this->_maxItems = other._maxItems;

        other._pConnection = nullptr;
        other._maxItems = 0;
    }

    DiskCache& DiskCache::operator=(DiskCache&& other) noexcept {
        if (&other != this) {
            this->_pConnection = other._pConnection;
            this->_maxItems = other._maxItems;

            other._pConnection = nullptr;
            other._maxItems = 0;
        }

        return *this;
    }

    DiskCache::~DiskCache() noexcept {
        if (this->_pConnection) {
            sqlite3_close(this->_pConnection);
        }
    }

    CacheLookupResult DiskCache::getEntry(const std::string& key) const {
        std::lock_guard<std::mutex> guard(this->_mutex);

        // get entry based on key
        int status = sqlite3_reset(this->_getEntryStmtWrapper.stmt);
        if (status != SQLITE_OK) {
            return CacheLookupResult {
                std::nullopt,
                sqlite3_errstr(status)
            };
        }

        status = sqlite3_clear_bindings(this->_getEntryStmtWrapper.stmt);
        if (status != SQLITE_OK) {
            return CacheLookupResult {
                std::nullopt,
                sqlite3_errstr(status)
            };
        }

        status = sqlite3_bind_text(this->_getEntryStmtWrapper.stmt, 1, key.c_str(), -1, SQLITE_STATIC);
        if (status != SQLITE_OK) {
            return CacheLookupResult {
                std::nullopt,
                sqlite3_errstr(status)
            };
        }

        status = sqlite3_step(this->_getEntryStmtWrapper.stmt);
        if (status == SQLITE_DONE) {
            // Cache miss
            return CacheLookupResult {
                std::nullopt,
                std::string()
            };
        }

        if (status != SQLITE_ROW) {
            // Something went wrong.
            return CacheLookupResult {
                std::nullopt,
                sqlite3_errstr(status)
            };
        }

        // Cache hit - unpack and return it.
        int64_t itemIndex = sqlite3_column_int64(this->_getEntryStmtWrapper.stmt, 0);

        // parse cache item metadata
        std::time_t expiryTime = sqlite3_column_int64(this->_getEntryStmtWrapper.stmt, 1);

        // parse response cache 
        std::string serializedResponseHeaders = reinterpret_cast<const char*>(sqlite3_column_text(this->_getEntryStmtWrapper.stmt, 2));
        HttpHeaders responseHeaders = convertStringToHeaders(serializedResponseHeaders);

        uint16_t statusCode = static_cast<uint16_t>(sqlite3_column_int(this->_getEntryStmtWrapper.stmt, 3));

        const uint8_t* rawResponseData = reinterpret_cast<const uint8_t*>(sqlite3_column_blob(this->_getEntryStmtWrapper.stmt, 4));
        int responseDataSize = sqlite3_column_bytes(this->_getEntryStmtWrapper.stmt, 4);
        std::vector<uint8_t> responseData(rawResponseData, rawResponseData + responseDataSize);

        // parse request
        std::string serializedRequestHeaders = reinterpret_cast<const char*>(sqlite3_column_text(this->_getEntryStmtWrapper.stmt, 5));
        HttpHeaders requestHeaders = convertStringToHeaders(serializedRequestHeaders);

        std::string requestMethod = reinterpret_cast<const char*>(sqlite3_column_text(this->_getEntryStmtWrapper.stmt, 6));

        std::string requestUrl = reinterpret_cast<const char*>(sqlite3_column_text(this->_getEntryStmtWrapper.stmt, 7));

        // update the last accessed time
        int updateStatus = sqlite3_reset(this->_updateLastAccessedTimeStmtWrapper.stmt);
        if (updateStatus != SQLITE_OK) {
            return CacheLookupResult {
                std::nullopt,
                sqlite3_errstr(updateStatus)
            };
        }

        updateStatus = sqlite3_clear_bindings(this->_updateLastAccessedTimeStmtWrapper.stmt);
        if (updateStatus != SQLITE_OK) {
            return CacheLookupResult {
                std::nullopt,
                sqlite3_errstr(updateStatus)
            };
        }

        updateStatus = sqlite3_bind_int64(this->_updateLastAccessedTimeStmtWrapper.stmt, 1, itemIndex);
        if (updateStatus != SQLITE_OK) {
            return CacheLookupResult {
                std::nullopt,
                sqlite3_errstr(updateStatus)
            };
        }

        updateStatus = sqlite3_step(this->_updateLastAccessedTimeStmtWrapper.stmt);
        if (updateStatus != SQLITE_DONE) {
            return CacheLookupResult {
                std::nullopt,
                sqlite3_errstr(updateStatus)
            };
        }

        return CacheLookupResult {
            CacheItem {
                expiryTime,
                CacheRequest {
                    std::move(requestHeaders),
                    std::move(requestMethod),
                    std::move(requestUrl)
                },
                CacheResponse {
                    statusCode,
                    std::move(responseHeaders),
                    std::move(responseData)
                }
            },
            std::string()
        };
    }

    CacheStoreResult DiskCache::storeEntry(
        const std::string& key,
        std::time_t expiryTime,
        const std::string& url,
        const std::string& requestMethod,
        const HttpHeaders& requestHeaders,
        uint16_t statusCode,
        const HttpHeaders& responseHeaders,
        const gsl::span<const uint8_t>& responseData
    ) {
        std::lock_guard<std::mutex> guard(this->_mutex);

        // cache the request with the key
        int status = sqlite3_reset(this->_storeResponseStmtWrapper.stmt);
        if (status != SQLITE_OK) {
            return CacheStoreResult {
                std::string(sqlite3_errstr(status))
            };
        }

        status = sqlite3_clear_bindings(this->_storeResponseStmtWrapper.stmt);
        if (status != SQLITE_OK) {
            return CacheStoreResult {
                std::string(sqlite3_errstr(status))
            };
        }

        status = sqlite3_bind_int64(this->_storeResponseStmtWrapper.stmt, 1, static_cast<int64_t>(expiryTime));
        if (status != SQLITE_OK) {
            return CacheStoreResult {
                std::string(sqlite3_errstr(status))
            };
        }

        status = sqlite3_bind_int64(this->_storeResponseStmtWrapper.stmt, 2, static_cast<int64_t>(std::time(nullptr)));
        if (status != SQLITE_OK) {
            return CacheStoreResult {
                std::string(sqlite3_errstr(status))
            };
        }

        std::string responseHeaderString = convertHeadersToString(responseHeaders);
        status = sqlite3_bind_text(this->_storeResponseStmtWrapper.stmt, 3, responseHeaderString.c_str(), -1, SQLITE_STATIC);
        if (status != SQLITE_OK) {
            return CacheStoreResult {
                std::string(sqlite3_errstr(status))
            };
        }
        
        status = sqlite3_bind_int(this->_storeResponseStmtWrapper.stmt, 4, static_cast<int>(statusCode));
        if (status != SQLITE_OK) {
            return CacheStoreResult {
                std::string(sqlite3_errstr(status))
            };
        }

        status = sqlite3_bind_blob(this->_storeResponseStmtWrapper.stmt, 5, responseData.data(), static_cast<int>(responseData.size()), SQLITE_STATIC);
        if (status != SQLITE_OK) {
            return CacheStoreResult {
                std::string(sqlite3_errstr(status))
            };
        }

        std::string requestHeaderString = convertHeadersToString(requestHeaders);
        status = sqlite3_bind_text(this->_storeResponseStmtWrapper.stmt, 6, requestHeaderString.c_str(), -1, SQLITE_STATIC);
        if (status != SQLITE_OK) {
            return CacheStoreResult {
                std::string(sqlite3_errstr(status))
            };
        }

        status = sqlite3_bind_text(this->_storeResponseStmtWrapper.stmt, 7, requestMethod.c_str(), -1, SQLITE_STATIC);
        if (status != SQLITE_OK) {
            return CacheStoreResult {
                std::string(sqlite3_errstr(status))
            };
        }

        status = sqlite3_bind_text(this->_storeResponseStmtWrapper.stmt, 8, url.c_str(), -1, SQLITE_STATIC);
        if (status != SQLITE_OK) {
            return CacheStoreResult {
                std::string(sqlite3_errstr(status))
            };
        }

        status = sqlite3_bind_text(this->_storeResponseStmtWrapper.stmt, 9, key.c_str(), -1, SQLITE_STATIC);
        if (status != SQLITE_OK) {
            return CacheStoreResult {
                std::string(sqlite3_errstr(status))
            };
        }

        status = sqlite3_step(this->_storeResponseStmtWrapper.stmt);
        if (status != SQLITE_DONE) {
            return CacheStoreResult {
                std::string(sqlite3_errstr(status))
            };
        }

        return CacheStoreResult {
            std::string()
        };
    }

    bool DiskCache::prune(std::string& error)
    {
        std::lock_guard<std::mutex> guard(this->_mutex);

        int64_t totalItems = 0;

        // query total size of response's data
        {
            int totalItemsQueryStatus = sqlite3_reset(this->_totalItemsQueryStmtWrapper.stmt);
            if (totalItemsQueryStatus != SQLITE_OK) {
                error = std::string(sqlite3_errstr(totalItemsQueryStatus));
                return false;
            }

            totalItemsQueryStatus = sqlite3_clear_bindings(this->_totalItemsQueryStmtWrapper.stmt);
            if (totalItemsQueryStatus != SQLITE_OK) {
                error = std::string(sqlite3_errstr(totalItemsQueryStatus));
                return false;
            }

            totalItemsQueryStatus = sqlite3_step(this->_totalItemsQueryStmtWrapper.stmt);
            if (totalItemsQueryStatus == SQLITE_DONE) {
                return true;
            }
            else if (totalItemsQueryStatus != SQLITE_ROW) {
                error = std::string(sqlite3_errstr(totalItemsQueryStatus));
                return false;
            }

            // prune the rows if over maximum
            totalItems = sqlite3_column_int64(this->_totalItemsQueryStmtWrapper.stmt, 0);
            if (totalItems > 0 && totalItems <= static_cast<int64_t>(_maxItems)) {
                return true;
            }
        }

        // delete expired rows first
        {
            int deleteExpiredStatus = sqlite3_reset(this->_deleteExpiredStmtWrapper.stmt);
            if (deleteExpiredStatus != SQLITE_OK) {
                error = std::string(sqlite3_errstr(deleteExpiredStatus));
                return false;
            }

            deleteExpiredStatus = sqlite3_clear_bindings(this->_deleteExpiredStmtWrapper.stmt);
            if (deleteExpiredStatus != SQLITE_OK) {
                error = std::string(sqlite3_errstr(deleteExpiredStatus));
                return false;
            }

            deleteExpiredStatus = sqlite3_step(this->_deleteExpiredStmtWrapper.stmt);
            if (deleteExpiredStatus != SQLITE_DONE) {
                error = std::string(sqlite3_errstr(deleteExpiredStatus));
                return false;
            }
        }

        // check if we should delete more
        int deletedRows = sqlite3_changes(this->_pConnection);
        if (totalItems - deletedRows < static_cast<int>(this->_maxItems)) {
            return true;
        }

        totalItems -= deletedRows;

        // delete rows LRU if we are still over maximum
        {
            int deleteLLRUStatus = sqlite3_reset(this->_deleteLRUStmtWrapper.stmt);
            if (deleteLLRUStatus != SQLITE_OK) {
                error = std::string(sqlite3_errstr(deleteLLRUStatus));
                return false;
            }

            deleteLLRUStatus = sqlite3_clear_bindings(this->_deleteLRUStmtWrapper.stmt);
            if (deleteLLRUStatus != SQLITE_OK) {
                error = std::string(sqlite3_errstr(deleteLLRUStatus));
                return false;
            }

            deleteLLRUStatus = sqlite3_bind_int64(this->_deleteLRUStmtWrapper.stmt, 1, totalItems - static_cast<int64_t>(this->_maxItems));
            if (deleteLLRUStatus != SQLITE_OK) {
                error = std::string(sqlite3_errstr(deleteLLRUStatus));
                return false;
            }

            deleteLLRUStatus = sqlite3_step(this->_deleteLRUStmtWrapper.stmt);
            if (deleteLLRUStatus != SQLITE_DONE) {
                error = std::string(sqlite3_errstr(deleteLLRUStatus));
                return false;
            }
        }

        return true;
    }

    bool DiskCache::clearAll(std::string& error)
    {
        std::lock_guard<std::mutex> guard(this->_mutex);

        int status = sqlite3_reset(this->_clearAllStmtWrapper.stmt);
        if (status != SQLITE_OK) {
            error = std::string(sqlite3_errstr(status));
            return false;
        }

        status = sqlite3_step(this->_clearAllStmtWrapper.stmt);
        if (status != SQLITE_DONE) {
            error = std::string(sqlite3_errstr(status));
            return false;
        }

        return true;
    }

    std::string convertHeadersToString(const HttpHeaders& headers) {
        rapidjson::Document document;
        rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
        rapidjson::Value root(rapidjson::kObjectType);
        rapidjson::Value key(rapidjson::kStringType);
        rapidjson::Value value(rapidjson::kStringType);
        for (const std::pair<const std::string, std::string>& header : headers) 
        {
            key.SetString(header.first.c_str(), allocator);
            value.SetString(header.second.c_str(), allocator);
            root.AddMember(key, value, allocator);
        }

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        root.Accept(writer);
        return buffer.GetString();
    }

    HttpHeaders convertStringToHeaders(const std::string& serializedHeaders) {
        rapidjson::Document document;
        document.Parse(serializedHeaders.c_str());

        HttpHeaders headers;
        for (rapidjson::Document::ConstMemberIterator it = document.MemberBegin(); it != document.MemberEnd(); ++it) {
            headers.insert({ it->name.GetString(), it->value.GetString() });
        }

        return headers;
    }
}