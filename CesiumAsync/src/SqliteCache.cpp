#include "CesiumAsync/IAssetResponse.h"
#include "CesiumAsync/SqliteCache.h"
#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/writer.h"
#include <sqlite3.h>
#include <stdexcept>
#include <utility>
#include <spdlog/spdlog.h>

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

    void SqliteCache::DeleteSqliteConnection::operator()(sqlite3* pConnection) noexcept {
        sqlite3_close_v2(pConnection);
    }

    void SqliteCache::DeleteSqliteStatement::operator()(sqlite3_stmt* pStatement) noexcept {
        sqlite3_finalize(pStatement);
    }

    SqliteCache::SqliteCache(
        const std::shared_ptr<spdlog::logger>& pLogger,
        const std::string& databaseName,
        uint64_t maxItems
    ) :
        _pLogger(pLogger),
        _pConnection(nullptr),
        _maxItems(maxItems),
        _mutex(),
        _getEntryStmtWrapper(),
        _updateLastAccessedTimeStmtWrapper(),
        _storeResponseStmtWrapper(),
        _totalItemsQueryStmtWrapper(),
        _deleteExpiredStmtWrapper(),
        _deleteLRUStmtWrapper(),
        _clearAllStmtWrapper()
    {
        sqlite3* pConnection;
        int status = sqlite3_open(databaseName.c_str(), &pConnection);
        if (status != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errstr(status));
        }

        this->_pConnection = std::unique_ptr<sqlite3, DeleteSqliteConnection>(pConnection);

        // create cache tables if not exist. Key -> Cache table: one-to-many relationship
        char* createTableError = nullptr;
        status = sqlite3_exec(this->_pConnection.get(), CREATE_CACHE_TABLE_SQL.c_str(), nullptr, nullptr, &createTableError);
        if (status != SQLITE_OK) {
            std::string errorStr(createTableError);
            sqlite3_free(createTableError);
            throw std::runtime_error(errorStr);
        }

        // turn on WAL mode
        char* walError = nullptr;
        status = sqlite3_exec(this->_pConnection.get(), PRAGMA_WAL_SQL.c_str(), nullptr, nullptr, &walError);
        if (status != SQLITE_OK) {
            std::string errorStr(walError);
            sqlite3_free(walError);
            throw std::runtime_error(errorStr);
        }

        // turn off synchronous mode
        char* syncError = nullptr;
        status = sqlite3_exec(this->_pConnection.get(), PRAGMA_SYNC_SQL.c_str(), nullptr, nullptr, &syncError);
        if (status != SQLITE_OK) {
            std::string errorStr(syncError);
            sqlite3_free(syncError);
            throw std::runtime_error(errorStr);
        }

        // increase page size
        char* pageSizeError = nullptr;
        status = sqlite3_exec(this->_pConnection.get(), PRAGMA_PAGE_SIZE_SQL.c_str(), nullptr, nullptr, &pageSizeError);
        if (status != SQLITE_OK) {
            std::string errorStr(pageSizeError);
            sqlite3_free(pageSizeError);
            throw std::runtime_error(errorStr);
        }

        // get entry based on key
        this->_getEntryStmtWrapper = prepareStatement(this->_pConnection, GET_ENTRY_SQL);

        // update last accessed for entry
        this->_updateLastAccessedTimeStmtWrapper = prepareStatement(this->_pConnection, UPDATE_LAST_ACCESSED_TIME_SQL);

        // store response 
        this->_storeResponseStmtWrapper = prepareStatement(this->_pConnection, STORE_RESPONSE_SQL);

        // query total items
        this->_totalItemsQueryStmtWrapper = prepareStatement(this->_pConnection, TOTAL_ITEMS_QUERY_SQL);

        // delete expired items
        this->_deleteExpiredStmtWrapper = prepareStatement(this->_pConnection, DELETE_EXPIRED_ITEMS_SQL);

        // delete expired items
        this->_deleteLRUStmtWrapper = prepareStatement(this->_pConnection, DELETE_LRU_ITEMS_SQL);

        // clear all items
        this->_clearAllStmtWrapper = prepareStatement(this->_pConnection, CLEAR_ALL_SQL);
    }

    std::optional<CacheItem> SqliteCache::getEntry(const std::string& key) const {
        std::lock_guard<std::mutex> guard(this->_mutex);

        // get entry based on key
        int status = sqlite3_reset(this->_getEntryStmtWrapper.get());
        if (status != SQLITE_OK) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(status));
            return std::nullopt;
        }

        status = sqlite3_clear_bindings(this->_getEntryStmtWrapper.get());
        if (status != SQLITE_OK) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(status));
            return std::nullopt;
        }

        status = sqlite3_bind_text(this->_getEntryStmtWrapper.get(), 1, key.c_str(), -1, SQLITE_STATIC);
        if (status != SQLITE_OK) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(status));
            return std::nullopt;
        }

        status = sqlite3_step(this->_getEntryStmtWrapper.get());
        if (status == SQLITE_DONE) {
            // Cache miss
            return std::nullopt;
        }

        if (status != SQLITE_ROW) {
            // Something went wrong.
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(status));
            return std::nullopt;
        }

        // Cache hit - unpack and return it.
        int64_t itemIndex = sqlite3_column_int64(this->_getEntryStmtWrapper.get(), 0);

        // parse cache item metadata
        std::time_t expiryTime = sqlite3_column_int64(this->_getEntryStmtWrapper.get(), 1);

        // parse response cache 
        std::string serializedResponseHeaders = reinterpret_cast<const char*>(sqlite3_column_text(this->_getEntryStmtWrapper.get(), 2));
        HttpHeaders responseHeaders = convertStringToHeaders(serializedResponseHeaders);

        uint16_t statusCode = static_cast<uint16_t>(sqlite3_column_int(this->_getEntryStmtWrapper.get(), 3));

        const uint8_t* rawResponseData = reinterpret_cast<const uint8_t*>(sqlite3_column_blob(this->_getEntryStmtWrapper.get(), 4));
        int responseDataSize = sqlite3_column_bytes(this->_getEntryStmtWrapper.get(), 4);
        std::vector<uint8_t> responseData(rawResponseData, rawResponseData + responseDataSize);

        // parse request
        std::string serializedRequestHeaders = reinterpret_cast<const char*>(sqlite3_column_text(this->_getEntryStmtWrapper.get(), 5));
        HttpHeaders requestHeaders = convertStringToHeaders(serializedRequestHeaders);

        std::string requestMethod = reinterpret_cast<const char*>(sqlite3_column_text(this->_getEntryStmtWrapper.get(), 6));

        std::string requestUrl = reinterpret_cast<const char*>(sqlite3_column_text(this->_getEntryStmtWrapper.get(), 7));

        // update the last accessed time
        int updateStatus = sqlite3_reset(this->_updateLastAccessedTimeStmtWrapper.get());
        if (updateStatus != SQLITE_OK) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(updateStatus));
            return std::nullopt;
        }

        updateStatus = sqlite3_clear_bindings(this->_updateLastAccessedTimeStmtWrapper.get());
        if (updateStatus != SQLITE_OK) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(updateStatus));
            return std::nullopt;
        }

        updateStatus = sqlite3_bind_int64(this->_updateLastAccessedTimeStmtWrapper.get(), 1, itemIndex);
        if (updateStatus != SQLITE_OK) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(updateStatus));
            return std::nullopt;
        }

        updateStatus = sqlite3_step(this->_updateLastAccessedTimeStmtWrapper.get());
        if (updateStatus != SQLITE_DONE) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(updateStatus));
            return std::nullopt;
        }

        return CacheItem {
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
        };
    }

    bool SqliteCache::storeEntry(
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
        int status = sqlite3_reset(this->_storeResponseStmtWrapper.get());
        if (status != SQLITE_OK) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(status));
            return false;
        }

        status = sqlite3_clear_bindings(this->_storeResponseStmtWrapper.get());
        if (status != SQLITE_OK) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(status));
            return false;
        }

        status = sqlite3_bind_int64(this->_storeResponseStmtWrapper.get(), 1, static_cast<int64_t>(expiryTime));
        if (status != SQLITE_OK) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(status));
            return false;
        }

        status = sqlite3_bind_int64(this->_storeResponseStmtWrapper.get(), 2, static_cast<int64_t>(std::time(nullptr)));
        if (status != SQLITE_OK) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(status));
            return false;
        }

        std::string responseHeaderString = convertHeadersToString(responseHeaders);
        status = sqlite3_bind_text(this->_storeResponseStmtWrapper.get(), 3, responseHeaderString.c_str(), -1, SQLITE_STATIC);
        if (status != SQLITE_OK) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(status));
            return false;
        }
        
        status = sqlite3_bind_int(this->_storeResponseStmtWrapper.get(), 4, static_cast<int>(statusCode));
        if (status != SQLITE_OK) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(status));
            return false;
        }

        status = sqlite3_bind_blob(this->_storeResponseStmtWrapper.get(), 5, responseData.data(), static_cast<int>(responseData.size()), SQLITE_STATIC);
        if (status != SQLITE_OK) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(status));
            return false;
        }

        std::string requestHeaderString = convertHeadersToString(requestHeaders);
        status = sqlite3_bind_text(this->_storeResponseStmtWrapper.get(), 6, requestHeaderString.c_str(), -1, SQLITE_STATIC);
        if (status != SQLITE_OK) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(status));
            return false;
        }

        status = sqlite3_bind_text(this->_storeResponseStmtWrapper.get(), 7, requestMethod.c_str(), -1, SQLITE_STATIC);
        if (status != SQLITE_OK) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(status));
            return false;
        }

        status = sqlite3_bind_text(this->_storeResponseStmtWrapper.get(), 8, url.c_str(), -1, SQLITE_STATIC);
        if (status != SQLITE_OK) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(status));
            return false;
        }

        status = sqlite3_bind_text(this->_storeResponseStmtWrapper.get(), 9, key.c_str(), -1, SQLITE_STATIC);
        if (status != SQLITE_OK) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(status));
            return false;
        }

        status = sqlite3_step(this->_storeResponseStmtWrapper.get());
        if (status != SQLITE_DONE) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(status));
            return false;
        }

        return true;
    }

    bool SqliteCache::prune()
    {
        std::lock_guard<std::mutex> guard(this->_mutex);

        int64_t totalItems = 0;

        // query total size of response's data
        {
            int totalItemsQueryStatus = sqlite3_reset(this->_totalItemsQueryStmtWrapper.get());
            if (totalItemsQueryStatus != SQLITE_OK) {
                SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(totalItemsQueryStatus));
                return false;
            }

            totalItemsQueryStatus = sqlite3_clear_bindings(this->_totalItemsQueryStmtWrapper.get());
            if (totalItemsQueryStatus != SQLITE_OK) {
                SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(totalItemsQueryStatus));
                return false;
            }

            totalItemsQueryStatus = sqlite3_step(this->_totalItemsQueryStmtWrapper.get());
            if (totalItemsQueryStatus == SQLITE_DONE) {
                return true;
            } else if (totalItemsQueryStatus != SQLITE_ROW) {
                SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(totalItemsQueryStatus));
                return false;
            }

            // prune the rows if over maximum
            totalItems = sqlite3_column_int64(this->_totalItemsQueryStmtWrapper.get(), 0);
            if (totalItems > 0 && totalItems <= static_cast<int64_t>(_maxItems)) {
                return true;
            }
        }

        // delete expired rows first
        {
            int deleteExpiredStatus = sqlite3_reset(this->_deleteExpiredStmtWrapper.get());
            if (deleteExpiredStatus != SQLITE_OK) {
                SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(deleteExpiredStatus));
                return false;
            }

            deleteExpiredStatus = sqlite3_clear_bindings(this->_deleteExpiredStmtWrapper.get());
            if (deleteExpiredStatus != SQLITE_OK) {
                SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(deleteExpiredStatus));
                return false;
            }

            deleteExpiredStatus = sqlite3_step(this->_deleteExpiredStmtWrapper.get());
            if (deleteExpiredStatus != SQLITE_DONE) {
                SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(deleteExpiredStatus));
                return false;
            }
        }

        // check if we should delete more
        int deletedRows = sqlite3_changes(this->_pConnection.get());
        if (totalItems - deletedRows < static_cast<int>(this->_maxItems)) {
            return true;
        }

        totalItems -= deletedRows;

        // delete rows LRU if we are still over maximum
        {
            int deleteLLRUStatus = sqlite3_reset(this->_deleteLRUStmtWrapper.get());
            if (deleteLLRUStatus != SQLITE_OK) {
                SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(deleteLLRUStatus));
                return false;
            }

            deleteLLRUStatus = sqlite3_clear_bindings(this->_deleteLRUStmtWrapper.get());
            if (deleteLLRUStatus != SQLITE_OK) {
                SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(deleteLLRUStatus));
                return false;
            }

            deleteLLRUStatus = sqlite3_bind_int64(this->_deleteLRUStmtWrapper.get(), 1, totalItems - static_cast<int64_t>(this->_maxItems));
            if (deleteLLRUStatus != SQLITE_OK) {
                SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(deleteLLRUStatus));
                return false;
            }

            deleteLLRUStatus = sqlite3_step(this->_deleteLRUStmtWrapper.get());
            if (deleteLLRUStatus != SQLITE_DONE) {
                SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(deleteLLRUStatus));
                return false;
            }
        }

        return true;
    }

    bool SqliteCache::clearAll()
    {
        std::lock_guard<std::mutex> guard(this->_mutex);

        int status = sqlite3_reset(this->_clearAllStmtWrapper.get());
        if (status != SQLITE_OK) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(status));
            return false;
        }

        status = sqlite3_step(this->_clearAllStmtWrapper.get());
        if (status != SQLITE_DONE) {
            SPDLOG_LOGGER_ERROR(this->_pLogger, sqlite3_errstr(status));
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

    /*static*/ SqliteCache::SqliteStatementPtr SqliteCache::prepareStatement(
        const SqliteCache::SqliteConnectionPtr& pConnection,
        const std::string& sql
    ) {
        sqlite3_stmt* pStmt;
        int status = sqlite3_prepare_v2(pConnection.get(), sql.c_str(), int(sql.size()), &pStmt, nullptr);
        if (status != SQLITE_OK) {
            throw std::runtime_error(std::string(sqlite3_errstr(status)));
        }
        return SqliteStatementPtr(pStmt);
    }
}
