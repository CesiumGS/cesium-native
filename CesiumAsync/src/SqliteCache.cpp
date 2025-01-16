#include <CesiumAsync/CacheItem.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/SqliteCache.h>
#include <CesiumAsync/SqliteHelper.h>
#include <CesiumAsync/cesium-sqlite3.h>
#include <CesiumUtility/Tracing.h>

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <sqlite3.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace CesiumAsync;

namespace {
// Cache table column names
const std::string CACHE_TABLE = "CacheItemTable";
const std::string CACHE_TABLE_KEY_COLUMN = "key";
const std::string CACHE_TABLE_EXPIRY_TIME_COLUMN = "expiryTime";
const std::string CACHE_TABLE_LAST_ACCESSED_TIME_COLUMN = "lastAccessedTime";
const std::string CACHE_TABLE_RESPONSE_HEADER_COLUMN = "responseHeaders";
const std::string CACHE_TABLE_RESPONSE_STATUS_CODE_COLUMN =
    "responseStatusCode";
const std::string CACHE_TABLE_RESPONSE_DATA_COLUMN = "responseData";
const std::string CACHE_TABLE_REQUEST_HEADER_COLUMN = "requestHeader";
const std::string CACHE_TABLE_REQUEST_METHOD_COLUMN = "requestMethod";
const std::string CACHE_TABLE_REQUEST_URL_COLUMN = "requestUrl";
const std::string CACHE_TABLE_VIRTUAL_TOTAL_ITEMS_COLUMN = "totalItems";

// Sql commands for setting up database
const std::string CREATE_CACHE_TABLE_SQL =
    "CREATE TABLE IF NOT EXISTS " + CACHE_TABLE + "(" + CACHE_TABLE_KEY_COLUMN +
    " TEXT PRIMARY KEY NOT NULL," + CACHE_TABLE_EXPIRY_TIME_COLUMN +
    " DATETIME NOT NULL," + CACHE_TABLE_LAST_ACCESSED_TIME_COLUMN +
    " DATETIME NOT NULL," + CACHE_TABLE_RESPONSE_HEADER_COLUMN +
    " TEXT NOT NULL," + CACHE_TABLE_RESPONSE_STATUS_CODE_COLUMN +
    " INTEGER NOT NULL," + CACHE_TABLE_RESPONSE_DATA_COLUMN + " BLOB," +
    CACHE_TABLE_REQUEST_HEADER_COLUMN + " TEXT NOT NULL," +
    CACHE_TABLE_REQUEST_METHOD_COLUMN + " TEXT NOT NULL," +
    CACHE_TABLE_REQUEST_URL_COLUMN + " TEXT NOT NULL)";

const std::string PRAGMA_WAL_SQL = "PRAGMA journal_mode=WAL";

const std::string PRAGMA_SYNC_SQL = "PRAGMA synchronous=OFF";

const std::string PRAGMA_PAGE_SIZE_SQL = "PRAGMA page_size=4096";

// Sql commands for getting entry from database
const std::string GET_ENTRY_SQL =
    "SELECT rowid, " + CACHE_TABLE_EXPIRY_TIME_COLUMN + ", " +
    CACHE_TABLE_RESPONSE_HEADER_COLUMN + ", " +
    CACHE_TABLE_RESPONSE_STATUS_CODE_COLUMN + ", " +
    CACHE_TABLE_RESPONSE_DATA_COLUMN + ", " +
    CACHE_TABLE_REQUEST_HEADER_COLUMN + ", " +
    CACHE_TABLE_REQUEST_METHOD_COLUMN + ", " + CACHE_TABLE_REQUEST_URL_COLUMN +
    " FROM " + CACHE_TABLE + " WHERE " + CACHE_TABLE_KEY_COLUMN + "=?";

const std::string UPDATE_LAST_ACCESSED_TIME_SQL =
    "UPDATE " + CACHE_TABLE + " SET " + CACHE_TABLE_LAST_ACCESSED_TIME_COLUMN +
    " = strftime('%s','now') WHERE rowid =?";

// Sql commands for storing response
const std::string STORE_RESPONSE_SQL =
    "REPLACE INTO " + CACHE_TABLE + " (" + CACHE_TABLE_EXPIRY_TIME_COLUMN +
    ", " + CACHE_TABLE_LAST_ACCESSED_TIME_COLUMN + ", " +
    CACHE_TABLE_RESPONSE_HEADER_COLUMN + ", " +
    CACHE_TABLE_RESPONSE_STATUS_CODE_COLUMN + ", " +
    CACHE_TABLE_RESPONSE_DATA_COLUMN + ", " +
    CACHE_TABLE_REQUEST_HEADER_COLUMN + ", " +
    CACHE_TABLE_REQUEST_METHOD_COLUMN + ", " + CACHE_TABLE_REQUEST_URL_COLUMN +
    ", " + CACHE_TABLE_KEY_COLUMN + ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

// Sql commands for prunning the database
const std::string TOTAL_ITEMS_QUERY_SQL =
    "SELECT COUNT(*) " + CACHE_TABLE_VIRTUAL_TOTAL_ITEMS_COLUMN + " FROM " +
    CACHE_TABLE;

const std::string DELETE_EXPIRED_ITEMS_SQL =
    "DELETE FROM " + CACHE_TABLE + " WHERE " + CACHE_TABLE_EXPIRY_TIME_COLUMN +
    " < strftime('%s','now')";

const std::string DELETE_LRU_ITEMS_SQL =
    "DELETE FROM " + CACHE_TABLE + " WHERE rowid " + " IN (SELECT rowid FROM " +
    CACHE_TABLE + " ORDER BY " + CACHE_TABLE_LAST_ACCESSED_TIME_COLUMN +
    " ASC " + " LIMIT ?)";

// Sql commands for clean all items
const std::string CLEAR_ALL_SQL = "DELETE FROM " + CACHE_TABLE;

std::string convertHeadersToString(const HttpHeaders& headers) {
  rapidjson::Document document;
  rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
  rapidjson::Value root(rapidjson::kObjectType);
  rapidjson::Value key(rapidjson::kStringType);
  rapidjson::Value value(rapidjson::kStringType);
  for (const std::pair<const std::string, std::string>& header : headers) {
    key.SetString(header.first.c_str(), allocator);
    value.SetString(header.second.c_str(), allocator);
    root.AddMember(key, value, allocator);
  }

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  root.Accept(writer);
  return buffer.GetString();
}

std::optional<HttpHeaders> convertStringToHeaders(
    const std::string& serializedHeaders,
    const std::shared_ptr<spdlog::logger>& pLogger) {
  rapidjson::Document document;
  document.Parse(serializedHeaders.c_str());
  if (document.HasParseError()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Unable to parse http header string from cache.");
    return std::nullopt;
  }
  std::optional<HttpHeaders> headers = std::make_optional<HttpHeaders>();
  for (rapidjson::Document::ConstMemberIterator it = document.MemberBegin();
       it != document.MemberEnd();
       ++it) {
    headers->insert({it->name.GetString(), it->value.GetString()});
  }
  return headers;
}

} // namespace

namespace CesiumAsync {

struct SqliteCache::Impl {
  Impl(
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& databaseName,
      uint64_t maxItems)
      : _pLogger(pLogger),
        _pConnection(nullptr),
        _databaseName(databaseName),
        _maxItems(maxItems),
        _getEntryStmtWrapper(),
        _updateLastAccessedTimeStmtWrapper(),
        _storeResponseStmtWrapper(),
        _totalItemsQueryStmtWrapper(),
        _deleteExpiredStmtWrapper(),
        _deleteLRUStmtWrapper(),
        _clearAllStmtWrapper() {}

  std::shared_ptr<spdlog::logger> _pLogger;
  SqliteConnectionPtr _pConnection;
  std::string _databaseName;
  uint64_t _maxItems;
  mutable std::mutex _mutex;
  SqliteStatementPtr _getEntryStmtWrapper;
  SqliteStatementPtr _updateLastAccessedTimeStmtWrapper;
  SqliteStatementPtr _storeResponseStmtWrapper;
  SqliteStatementPtr _totalItemsQueryStmtWrapper;
  SqliteStatementPtr _deleteExpiredStmtWrapper;
  SqliteStatementPtr _deleteLRUStmtWrapper;
  SqliteStatementPtr _clearAllStmtWrapper;
};

SqliteCache::SqliteCache(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::string& databaseName,
    uint64_t maxItems)
    : _pImpl(std::make_unique<Impl>(pLogger, databaseName, maxItems)) {
  createConnection();
}

void SqliteCache::createConnection() const {
  CESIUM_SQLITE(sqlite3*) pConnection;
  int status = CESIUM_SQLITE(
      sqlite3_open)(this->_pImpl->_databaseName.c_str(), &pConnection);
  if (status != SQLITE_OK) {
    throw std::runtime_error(CESIUM_SQLITE(sqlite3_errstr)(status));
  }

  this->_pImpl->_pConnection =
      std::unique_ptr<CESIUM_SQLITE(sqlite3), DeleteSqliteConnection>(
          pConnection);

  // create cache tables if not exist. Key -> Cache table: one-to-many
  // relationship
  char* createTableError = nullptr;
  status = CESIUM_SQLITE(sqlite3_exec)(
      this->_pImpl->_pConnection.get(),
      CREATE_CACHE_TABLE_SQL.c_str(),
      nullptr,
      nullptr,
      &createTableError);
  if (status != SQLITE_OK) {
    std::string errorStr(createTableError);
    CESIUM_SQLITE(sqlite3_free)(createTableError);
    throw std::runtime_error(errorStr);
  }

  // turn on WAL mode
  char* walError = nullptr;
  status = CESIUM_SQLITE(sqlite3_exec)(
      this->_pImpl->_pConnection.get(),
      PRAGMA_WAL_SQL.c_str(),
      nullptr,
      nullptr,
      &walError);
  if (status != SQLITE_OK) {
    std::string errorStr(walError);
    CESIUM_SQLITE(sqlite3_free)(walError);
    throw std::runtime_error(errorStr);
  }

  // turn off synchronous mode
  char* syncError = nullptr;
  status = CESIUM_SQLITE(sqlite3_exec)(
      this->_pImpl->_pConnection.get(),
      PRAGMA_SYNC_SQL.c_str(),
      nullptr,
      nullptr,
      &syncError);
  if (status != SQLITE_OK) {
    std::string errorStr(syncError);
    CESIUM_SQLITE(sqlite3_free)(syncError);
    throw std::runtime_error(errorStr);
  }

  // increase page size
  char* pageSizeError = nullptr;
  status = CESIUM_SQLITE(sqlite3_exec)(
      this->_pImpl->_pConnection.get(),
      PRAGMA_PAGE_SIZE_SQL.c_str(),
      nullptr,
      nullptr,
      &pageSizeError);
  if (status != SQLITE_OK) {
    std::string errorStr(pageSizeError);
    CESIUM_SQLITE(sqlite3_free)(pageSizeError);
    throw std::runtime_error(errorStr);
  }

  // get entry based on key
  this->_pImpl->_getEntryStmtWrapper =
      SqliteHelper::prepareStatement(this->_pImpl->_pConnection, GET_ENTRY_SQL);

  // update last accessed for entry
  this->_pImpl->_updateLastAccessedTimeStmtWrapper =
      SqliteHelper::prepareStatement(
          this->_pImpl->_pConnection,
          UPDATE_LAST_ACCESSED_TIME_SQL);

  // store response
  this->_pImpl->_storeResponseStmtWrapper = SqliteHelper::prepareStatement(
      this->_pImpl->_pConnection,
      STORE_RESPONSE_SQL);

  // query total items
  this->_pImpl->_totalItemsQueryStmtWrapper = SqliteHelper::prepareStatement(
      this->_pImpl->_pConnection,
      TOTAL_ITEMS_QUERY_SQL);

  // delete expired items
  this->_pImpl->_deleteExpiredStmtWrapper = SqliteHelper::prepareStatement(
      this->_pImpl->_pConnection,
      DELETE_EXPIRED_ITEMS_SQL);

  // delete expired items
  this->_pImpl->_deleteLRUStmtWrapper = SqliteHelper::prepareStatement(
      this->_pImpl->_pConnection,
      DELETE_LRU_ITEMS_SQL);

  // clear all items
  this->_pImpl->_clearAllStmtWrapper =
      SqliteHelper::prepareStatement(this->_pImpl->_pConnection, CLEAR_ALL_SQL);
}

SqliteCache::~SqliteCache() = default;

std::optional<CacheItem> SqliteCache::getEntry(const std::string& key) const {
  CESIUM_TRACE("SqliteCache::getEntry");
  std::lock_guard<std::mutex> guard(this->_pImpl->_mutex);

  // get entry based on key
  int status =
      CESIUM_SQLITE(sqlite3_reset)(this->_pImpl->_getEntryStmtWrapper.get());
  if (status != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(status));
    return std::nullopt;
  }

  status = CESIUM_SQLITE(sqlite3_clear_bindings)(
      this->_pImpl->_getEntryStmtWrapper.get());
  if (status != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(status));
    return std::nullopt;
  }

  status = CESIUM_SQLITE(sqlite3_bind_text)(
      this->_pImpl->_getEntryStmtWrapper.get(),
      1,
      key.c_str(),
      -1,
      SQLITE_STATIC);
  if (status != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(status));
    return std::nullopt;
  }

  status =
      CESIUM_SQLITE(sqlite3_step)(this->_pImpl->_getEntryStmtWrapper.get());
  if (status == SQLITE_DONE) {
    // Cache miss
    return std::nullopt;
  }

  if (status != SQLITE_ROW) {
    // Something went wrong.
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(status));
    return std::nullopt;
  }

  // Cache hit - unpack and return it.
  const int64_t itemIndex = CESIUM_SQLITE(
      sqlite3_column_int64)(this->_pImpl->_getEntryStmtWrapper.get(), 0);

  // parse cache item metadata
  const std::time_t expiryTime = CESIUM_SQLITE(
      sqlite3_column_int64)(this->_pImpl->_getEntryStmtWrapper.get(), 1);

  // parse response cache
  std::string serializedResponseHeaders =
      reinterpret_cast<const char*>(CESIUM_SQLITE(
          sqlite3_column_text)(this->_pImpl->_getEntryStmtWrapper.get(), 2));
  std::optional<HttpHeaders> responseHeaders =
      convertStringToHeaders(serializedResponseHeaders, this->_pImpl->_pLogger);
  if (!responseHeaders) {
    return std::nullopt;
  }
  const uint16_t statusCode = static_cast<uint16_t>(CESIUM_SQLITE(
      sqlite3_column_int)(this->_pImpl->_getEntryStmtWrapper.get(), 3));

  const std::byte* rawResponseData =
      reinterpret_cast<const std::byte*>(CESIUM_SQLITE(
          sqlite3_column_blob)(this->_pImpl->_getEntryStmtWrapper.get(), 4));
  const int responseDataSize = CESIUM_SQLITE(
      sqlite3_column_bytes)(this->_pImpl->_getEntryStmtWrapper.get(), 4);
  std::vector<std::byte> responseData(
      rawResponseData,
      rawResponseData + responseDataSize);

  // parse request
  std::string serializedRequestHeaders =
      reinterpret_cast<const char*>(CESIUM_SQLITE(
          sqlite3_column_text)(this->_pImpl->_getEntryStmtWrapper.get(), 5));
  std::optional<HttpHeaders> requestHeaders =
      convertStringToHeaders(serializedRequestHeaders, this->_pImpl->_pLogger);
  if (!requestHeaders) {
    return std::nullopt;
  }

  std::string requestMethod = reinterpret_cast<const char*>(CESIUM_SQLITE(
      sqlite3_column_text)(this->_pImpl->_getEntryStmtWrapper.get(), 6));

  std::string requestUrl = reinterpret_cast<const char*>(CESIUM_SQLITE(
      sqlite3_column_text)(this->_pImpl->_getEntryStmtWrapper.get(), 7));

  // update the last accessed time
  int updateStatus = CESIUM_SQLITE(sqlite3_reset)(
      this->_pImpl->_updateLastAccessedTimeStmtWrapper.get());
  if (updateStatus != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(updateStatus));
    return std::nullopt;
  }

  updateStatus = CESIUM_SQLITE(sqlite3_clear_bindings)(
      this->_pImpl->_updateLastAccessedTimeStmtWrapper.get());
  if (updateStatus != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(updateStatus));
    return std::nullopt;
  }

  updateStatus = CESIUM_SQLITE(sqlite3_bind_int64)(
      this->_pImpl->_updateLastAccessedTimeStmtWrapper.get(),
      1,
      itemIndex);
  if (updateStatus != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(updateStatus));
    return std::nullopt;
  }

  updateStatus = CESIUM_SQLITE(sqlite3_step)(
      this->_pImpl->_updateLastAccessedTimeStmtWrapper.get());
  if (updateStatus != SQLITE_DONE) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(updateStatus));
    return std::nullopt;
  }

  return CacheItem{
      expiryTime,
      CacheRequest{
          std::move(*requestHeaders),
          std::move(requestMethod),
          std::move(requestUrl)},
      CacheResponse{
          statusCode,
          std::move(*responseHeaders),
          std::move(responseData)}};
}

bool SqliteCache::storeEntry(
    const std::string& key,
    std::time_t expiryTime,
    const std::string& url,
    const std::string& requestMethod,
    const HttpHeaders& requestHeaders,
    uint16_t statusCode,
    const HttpHeaders& responseHeaders,
    const std::span<const std::byte>& responseData) {
  CESIUM_TRACE("SqliteCache::storeEntry");
  std::lock_guard<std::mutex> guard(this->_pImpl->_mutex);

  // cache the request with the key
  int status = CESIUM_SQLITE(sqlite3_reset)(
      this->_pImpl->_storeResponseStmtWrapper.get());
  if (status != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(status));
    return false;
  }

  status = CESIUM_SQLITE(sqlite3_clear_bindings)(
      this->_pImpl->_storeResponseStmtWrapper.get());
  if (status != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(status));
    return false;
  }

  status = CESIUM_SQLITE(sqlite3_bind_int64)(
      this->_pImpl->_storeResponseStmtWrapper.get(),
      1,
      static_cast<int64_t>(expiryTime));
  if (status != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(status));
    return false;
  }

  status = CESIUM_SQLITE(sqlite3_bind_int64)(
      this->_pImpl->_storeResponseStmtWrapper.get(),
      2,
      static_cast<int64_t>(std::time(nullptr)));
  if (status != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(status));
    return false;
  }

  std::string responseHeaderString = convertHeadersToString(responseHeaders);
  status = CESIUM_SQLITE(sqlite3_bind_text)(
      this->_pImpl->_storeResponseStmtWrapper.get(),
      3,
      responseHeaderString.c_str(),
      -1,
      SQLITE_STATIC);
  if (status != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(status));
    return false;
  }

  status = CESIUM_SQLITE(sqlite3_bind_int)(
      this->_pImpl->_storeResponseStmtWrapper.get(),
      4,
      static_cast<int>(statusCode));
  if (status != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(status));
    return false;
  }

  status = CESIUM_SQLITE(sqlite3_bind_blob)(
      this->_pImpl->_storeResponseStmtWrapper.get(),
      5,
      responseData.data(),
      static_cast<int>(responseData.size()),
      SQLITE_STATIC);
  if (status != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(status));
    return false;
  }

  std::string requestHeaderString = convertHeadersToString(requestHeaders);
  status = CESIUM_SQLITE(sqlite3_bind_text)(
      this->_pImpl->_storeResponseStmtWrapper.get(),
      6,
      requestHeaderString.c_str(),
      -1,
      SQLITE_STATIC);
  if (status != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(status));
    return false;
  }

  status = CESIUM_SQLITE(sqlite3_bind_text)(
      this->_pImpl->_storeResponseStmtWrapper.get(),
      7,
      requestMethod.c_str(),
      -1,
      SQLITE_STATIC);
  if (status != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(status));
    return false;
  }

  status = CESIUM_SQLITE(sqlite3_bind_text)(
      this->_pImpl->_storeResponseStmtWrapper.get(),
      8,
      url.c_str(),
      -1,
      SQLITE_STATIC);
  if (status != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(status));
    return false;
  }

  status = CESIUM_SQLITE(sqlite3_bind_text)(
      this->_pImpl->_storeResponseStmtWrapper.get(),
      9,
      key.c_str(),
      -1,
      SQLITE_STATIC);
  if (status != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(status));
    return false;
  }

  status = CESIUM_SQLITE(sqlite3_step)(
      this->_pImpl->_storeResponseStmtWrapper.get());
  if (status != SQLITE_DONE) {
    if (status == SQLITE_CORRUPT) {
      destroyDatabase();
    }
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(status));
    return false;
  }

  return true;
}

bool SqliteCache::prune() {
  CESIUM_TRACE("SqliteCache::prune");
  std::lock_guard<std::mutex> guard(this->_pImpl->_mutex);

  int64_t totalItems = 0;

  // query total size of response's data
  {
    int totalItemsQueryStatus = CESIUM_SQLITE(sqlite3_reset)(
        this->_pImpl->_totalItemsQueryStmtWrapper.get());
    if (totalItemsQueryStatus != SQLITE_OK) {
      SPDLOG_LOGGER_ERROR(
          this->_pImpl->_pLogger,
          CESIUM_SQLITE(sqlite3_errstr)(totalItemsQueryStatus));
      return false;
    }

    totalItemsQueryStatus = CESIUM_SQLITE(sqlite3_clear_bindings)(
        this->_pImpl->_totalItemsQueryStmtWrapper.get());
    if (totalItemsQueryStatus != SQLITE_OK) {
      SPDLOG_LOGGER_ERROR(
          this->_pImpl->_pLogger,
          CESIUM_SQLITE(sqlite3_errstr)(totalItemsQueryStatus));
      return false;
    }

    totalItemsQueryStatus = CESIUM_SQLITE(sqlite3_step)(
        this->_pImpl->_totalItemsQueryStmtWrapper.get());

    if (totalItemsQueryStatus == SQLITE_DONE) {
      return true;
    }

    if (totalItemsQueryStatus != SQLITE_ROW) {
      if (totalItemsQueryStatus == SQLITE_CORRUPT) {
        destroyDatabase();
      }
      SPDLOG_LOGGER_ERROR(
          this->_pImpl->_pLogger,
          CESIUM_SQLITE(sqlite3_errstr)(totalItemsQueryStatus));
      return false;
    }

    // prune the rows if over maximum
    totalItems = CESIUM_SQLITE(sqlite3_column_int64)(
        this->_pImpl->_totalItemsQueryStmtWrapper.get(),
        0);
    if (totalItems > 0 &&
        totalItems <= static_cast<int64_t>(this->_pImpl->_maxItems)) {
      return true;
    }
  }

  // delete expired rows first
  {
    int deleteExpiredStatus = CESIUM_SQLITE(sqlite3_reset)(
        this->_pImpl->_deleteExpiredStmtWrapper.get());
    if (deleteExpiredStatus != SQLITE_OK) {
      SPDLOG_LOGGER_ERROR(
          this->_pImpl->_pLogger,
          CESIUM_SQLITE(sqlite3_errstr)(deleteExpiredStatus));
      return false;
    }

    deleteExpiredStatus = CESIUM_SQLITE(sqlite3_clear_bindings)(
        this->_pImpl->_deleteExpiredStmtWrapper.get());
    if (deleteExpiredStatus != SQLITE_OK) {
      SPDLOG_LOGGER_ERROR(
          this->_pImpl->_pLogger,
          CESIUM_SQLITE(sqlite3_errstr)(deleteExpiredStatus));
      return false;
    }

    deleteExpiredStatus = CESIUM_SQLITE(sqlite3_step)(
        this->_pImpl->_deleteExpiredStmtWrapper.get());
    if (deleteExpiredStatus != SQLITE_DONE) {
      if (deleteExpiredStatus == SQLITE_CORRUPT) {
        destroyDatabase();
      }
      SPDLOG_LOGGER_ERROR(
          this->_pImpl->_pLogger,
          CESIUM_SQLITE(sqlite3_errstr)(deleteExpiredStatus));
      return false;
    }
  }

  // check if we should delete more
  const int deletedRows =
      CESIUM_SQLITE(sqlite3_changes)(this->_pImpl->_pConnection.get());
  if (totalItems - deletedRows < static_cast<int>(this->_pImpl->_maxItems)) {
    return true;
  }

  totalItems -= deletedRows;

  // delete rows LRU if we are still over maximum
  {
    int deleteLLRUStatus =
        CESIUM_SQLITE(sqlite3_reset)(this->_pImpl->_deleteLRUStmtWrapper.get());
    if (deleteLLRUStatus != SQLITE_OK) {
      SPDLOG_LOGGER_ERROR(
          this->_pImpl->_pLogger,
          CESIUM_SQLITE(sqlite3_errstr)(deleteLLRUStatus));
      return false;
    }

    deleteLLRUStatus = CESIUM_SQLITE(sqlite3_clear_bindings)(
        this->_pImpl->_deleteLRUStmtWrapper.get());
    if (deleteLLRUStatus != SQLITE_OK) {
      SPDLOG_LOGGER_ERROR(
          this->_pImpl->_pLogger,
          CESIUM_SQLITE(sqlite3_errstr)(deleteLLRUStatus));
      return false;
    }

    deleteLLRUStatus = CESIUM_SQLITE(sqlite3_bind_int64)(
        this->_pImpl->_deleteLRUStmtWrapper.get(),
        1,
        totalItems - static_cast<int64_t>(this->_pImpl->_maxItems));
    if (deleteLLRUStatus != SQLITE_OK) {
      SPDLOG_LOGGER_ERROR(
          this->_pImpl->_pLogger,
          CESIUM_SQLITE(sqlite3_errstr)(deleteLLRUStatus));
      return false;
    }

    deleteLLRUStatus =
        CESIUM_SQLITE(sqlite3_step)(this->_pImpl->_deleteLRUStmtWrapper.get());
    if (deleteLLRUStatus != SQLITE_DONE) {
      if (deleteLLRUStatus == SQLITE_CORRUPT) {
        destroyDatabase();
      }
      SPDLOG_LOGGER_ERROR(
          this->_pImpl->_pLogger,
          CESIUM_SQLITE(sqlite3_errstr)(deleteLLRUStatus));
      return false;
    }
  }

  return true;
}

bool SqliteCache::clearAll() {
  std::lock_guard<std::mutex> guard(this->_pImpl->_mutex);

  int status =
      CESIUM_SQLITE(sqlite3_reset)(this->_pImpl->_clearAllStmtWrapper.get());
  if (status != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(status));
    return false;
  }

  status =
      CESIUM_SQLITE(sqlite3_step)(this->_pImpl->_clearAllStmtWrapper.get());
  if (status != SQLITE_DONE) {
    if (status == SQLITE_CORRUPT) {
      destroyDatabase();
    }
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(status));
    return false;
  }

  return true;
}

void SqliteCache::destroyDatabase() {
  std::shared_ptr<spdlog::logger> pLogger = _pImpl->_pLogger;
  std::string databaseName = _pImpl->_databaseName;
  uint64_t maxItems = _pImpl->_maxItems;
  _pImpl.reset();
  _pImpl = std::make_unique<Impl>(pLogger, databaseName, maxItems);
  if (std::remove(_pImpl->_databaseName.c_str()) != 0) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        "Unable to delete database file.");
  }
  createConnection();
}

} // namespace CesiumAsync
