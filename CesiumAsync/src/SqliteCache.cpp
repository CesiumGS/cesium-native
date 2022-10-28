#include "CesiumAsync/SqliteCache.h"

#include "CesiumAsync/IAssetResponse.h"

#include <CesiumUtility/Tracing.h>
#include <cesium-sqlite3.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <spdlog/spdlog.h>
#include <sqlite3.h>

#include <cstddef>
#include <shared_mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <utility>

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
const std::string CACHE_TABLE_CLIENT_DATA_COLUMN = "clientData";
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
    CACHE_TABLE_CLIENT_DATA_COLUMN + " BLOB," +
    CACHE_TABLE_REQUEST_HEADER_COLUMN + " TEXT NOT NULL," +
    CACHE_TABLE_REQUEST_METHOD_COLUMN + " TEXT NOT NULL," +
    CACHE_TABLE_REQUEST_URL_COLUMN + " TEXT NOT NULL)";

const std::string PRAGMA_WAL_SQL = "PRAGMA journal_mode=WAL";

const std::string PRAGMA_SYNC_SQL = "PRAGMA synchronous=OFF";

const std::string PRAGMA_PAGE_SIZE_SQL = "PRAGMA page_size=8192";

const std::string PRAGMA_CACHE_SIZE = "PRAGMA default_cache_size=-64000";
/*
const std::string PRAGMA_TEMP_STORE = "PRAGMA temp_store=MEMORY";

const std::string PRAGMA_MMAP_SIZE = "PRAGMA mmap_size=30000000000";*/

// Sql commands for getting entry from database
const std::string GET_ENTRY_SQL =
    "SELECT rowid, " + CACHE_TABLE_EXPIRY_TIME_COLUMN + ", " +
    CACHE_TABLE_RESPONSE_HEADER_COLUMN + ", " +
    CACHE_TABLE_RESPONSE_STATUS_CODE_COLUMN + ", " +
    CACHE_TABLE_RESPONSE_DATA_COLUMN + ", " + CACHE_TABLE_CLIENT_DATA_COLUMN +
    ", " + CACHE_TABLE_REQUEST_HEADER_COLUMN + ", " +
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
    CACHE_TABLE_RESPONSE_DATA_COLUMN + ", " + CACHE_TABLE_CLIENT_DATA_COLUMN +
    ", " + CACHE_TABLE_REQUEST_HEADER_COLUMN + ", " +
    CACHE_TABLE_REQUEST_METHOD_COLUMN + ", " + CACHE_TABLE_REQUEST_URL_COLUMN +
    ", " + CACHE_TABLE_KEY_COLUMN + ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

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

struct DeleteSqliteConnection {
  void operator()(CESIUM_SQLITE(sqlite3*) pConnection) noexcept {
    CESIUM_SQLITE(sqlite3_close_v2)(pConnection);
  }
};

struct DeleteSqliteStatement {
  void operator()(CESIUM_SQLITE(sqlite3_stmt*) pStatement) noexcept {
    CESIUM_SQLITE(sqlite3_finalize)(pStatement);
  }
};

using SqliteConnectionPtr =
    std::unique_ptr<CESIUM_SQLITE(sqlite3), DeleteSqliteConnection>;
using SqliteReaderConnectionPtr = std::shared_ptr<CESIUM_SQLITE(sqlite3)>;
using SqliteStatementPtr =
    std::unique_ptr<CESIUM_SQLITE(sqlite3_stmt), DeleteSqliteStatement>;

struct SqliteReader {
  SqliteReaderConnectionPtr pConnection;
  SqliteStatementPtr pGetEntryStmtWrapper;
};

SqliteStatementPtr
prepareStatement(CESIUM_SQLITE(sqlite3*) pConnection, const std::string& sql) {
  CESIUM_SQLITE(sqlite3_stmt*) pStmt;
  const int status = CESIUM_SQLITE(sqlite3_prepare_v2)(
      pConnection,
      sql.c_str(),
      int(sql.size()),
      &pStmt,
      nullptr);
  if (status != SQLITE_OK) {
    throw std::runtime_error(
        std::string(CESIUM_SQLITE(sqlite3_errstr)(status)));
  }
  return SqliteStatementPtr(pStmt);
}

bool resetStatement(
    const SqliteStatementPtr& pStatement,
    const std::shared_ptr<spdlog::logger>& pLogger) {
  int resetStatus = CESIUM_SQLITE(sqlite3_reset)(pStatement.get());
  if (resetStatus != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(pLogger, CESIUM_SQLITE(sqlite3_errstr)(resetStatus));
    return false;
  }

  return true;
}

bool clearBindings(
    const SqliteStatementPtr& pStatement,
    const std::shared_ptr<spdlog::logger>& pLogger) {
  int status = CESIUM_SQLITE(sqlite3_clear_bindings)(pStatement.get());
  if (status != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(pLogger, CESIUM_SQLITE(sqlite3_errstr)(status));
    return false;
  }

  return true;
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
        _updateLastAccessedTimeStmtWrapper(),
        _storeResponseStmtWrapper(),
        _totalItemsQueryStmtWrapper(),
        _deleteExpiredStmtWrapper(),
        _deleteLRUStmtWrapper(),
        _clearAllStmtWrapper() {}

  const SqliteReader& getReader() {
    auto threadId = std::this_thread::get_id();

    // Each unique reader thread registers a separate connection.
    // Only need a shared lock when reading from the thread-connection map.
    std::shared_lock sharedLock(this->_readerThreadMapMutex);
    auto pConnectionIt = this->_readerThreadMap.find(threadId);
    if (pConnectionIt != this->_readerThreadMap.end()) {
      return pConnectionIt->second;
    }

    CESIUM_TRACE("CreateReaderConnection");

    // Need to register a new read connection for this thread.
    sharedLock.unlock();

    // Need an exclusive lock to actually modify the thread-connection map.
    // This blocks all other readers, but it should only happen once per thread
    // on the first read.
    std::unique_lock exclusiveLock(this->_readerThreadMapMutex);

    CESIUM_SQLITE(sqlite3*) pConnection;
    int status = CESIUM_SQLITE(sqlite3_open_v2)(
        this->_databaseName.c_str(),
        &pConnection,
        SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_READONLY,
        nullptr);
    if (status != SQLITE_OK) {
      throw std::runtime_error(CESIUM_SQLITE(sqlite3_errstr)(status));
    }

    SqliteReader reader;
    reader.pConnection = std::shared_ptr<CESIUM_SQLITE(sqlite3)>(
        pConnection,
        DeleteSqliteConnection());
    reader.pGetEntryStmtWrapper =
        prepareStatement(reader.pConnection.get(), GET_ENTRY_SQL);

    auto result = this->_readerThreadMap.emplace(threadId, std::move(reader));

    return result.first->second;
  }

  std::shared_ptr<spdlog::logger> _pLogger;
  SqliteConnectionPtr _pConnection;
  std::unordered_map<std::thread::id, SqliteReader> _readerThreadMap;
  std::string _databaseName;
  uint64_t _maxItems;
  mutable std::mutex _writerMutex;
  mutable std::shared_mutex _readerThreadMapMutex;
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
  assert(CESIUM_SQLITE(sqlite3_threadsafe)() == 2);
  CESIUM_SQLITE(sqlite3_config)(SQLITE_CONFIG_MULTITHREAD);
  createConnection();
}

void SqliteCache::createConnection() const {
  CESIUM_SQLITE(sqlite3*) pConnection;
  int status = CESIUM_SQLITE(sqlite3_open_v2)(
      this->_pImpl->_databaseName.c_str(),
      &pConnection,
      SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_READWRITE,
      nullptr);
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

  // Set cache size
  char* cacheSizeError = nullptr;
  status = CESIUM_SQLITE(sqlite3_exec)(
      this->_pImpl->_pConnection.get(),
      PRAGMA_CACHE_SIZE.c_str(),
      nullptr,
      nullptr,
      &cacheSizeError);
  if (status != SQLITE_OK) {
    std::string errorStr(cacheSizeError);
    CESIUM_SQLITE(sqlite3_free)(cacheSizeError);
    throw std::runtime_error(errorStr);
  }
  /*
    // Set temp store
    char* tempStoreError = nullptr;
    status = CESIUM_SQLITE(sqlite3_exec)(
        this->_pImpl->_pConnection.get(),
        PRAGMA_TEMP_STORE.c_str(),
        nullptr,
        nullptr,
        &tempStoreError);
    if (status != SQLITE_OK) {
      std::string errorStr(tempStoreError);
      CESIUM_SQLITE(sqlite3_free)(tempStoreError);
      throw std::runtime_error(errorStr);
    }

    // Reserve 30gb virtual memory
    char* mmapSizeError = nullptr;
    status = CESIUM_SQLITE(sqlite3_exec)(
        this->_pImpl->_pConnection.get(),
        PRAGMA_MMAP_SIZE.c_str(),
        nullptr,
        nullptr,
        &mmapSizeError);
    if (status != SQLITE_OK) {
      std::string errorStr(mmapSizeError);
      CESIUM_SQLITE(sqlite3_free)(mmapSizeError);
      throw std::runtime_error(errorStr);
    }
  */
  // update last accessed for entry
  this->_pImpl->_updateLastAccessedTimeStmtWrapper = prepareStatement(
      this->_pImpl->_pConnection.get(),
      UPDATE_LAST_ACCESSED_TIME_SQL);

  // store response
  this->_pImpl->_storeResponseStmtWrapper =
      prepareStatement(this->_pImpl->_pConnection.get(), STORE_RESPONSE_SQL);

  // query total items
  this->_pImpl->_totalItemsQueryStmtWrapper =
      prepareStatement(this->_pImpl->_pConnection.get(), TOTAL_ITEMS_QUERY_SQL);

  // delete expired items
  this->_pImpl->_deleteExpiredStmtWrapper = prepareStatement(
      this->_pImpl->_pConnection.get(),
      DELETE_EXPIRED_ITEMS_SQL);

  // delete expired items
  this->_pImpl->_deleteLRUStmtWrapper =
      prepareStatement(this->_pImpl->_pConnection.get(), DELETE_LRU_ITEMS_SQL);

  // clear all items
  this->_pImpl->_clearAllStmtWrapper =
      prepareStatement(this->_pImpl->_pConnection.get(), CLEAR_ALL_SQL);
}

SqliteCache::~SqliteCache() = default;

std::optional<CacheItem>
SqliteCache::getEntryAnyThread(const std::string& key) const {
  CESIUM_TRACE("SqliteCache::getEntry");
  // std::lock_guard<std::mutex> guard(this->_pImpl->_mutex);

  const SqliteReader& reader = this->_pImpl->getReader();

  int status = CESIUM_SQLITE(sqlite3_bind_text)(
      reader.pGetEntryStmtWrapper.get(),
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

  status = CESIUM_SQLITE(sqlite3_step)(reader.pGetEntryStmtWrapper.get());
  if (status == SQLITE_DONE) {
    // Cache miss
    clearBindings(reader.pGetEntryStmtWrapper, this->_pImpl->_pLogger);
    resetStatement(reader.pGetEntryStmtWrapper, this->_pImpl->_pLogger);
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
  const int64_t itemIndex =
      CESIUM_SQLITE(sqlite3_column_int64)(reader.pGetEntryStmtWrapper.get(), 0);

  // parse cache item metadata
  const std::time_t expiryTime =
      CESIUM_SQLITE(sqlite3_column_int64)(reader.pGetEntryStmtWrapper.get(), 1);

  // parse response cache
  std::string serializedResponseHeaders = reinterpret_cast<const char*>(
      CESIUM_SQLITE(sqlite3_column_text)(reader.pGetEntryStmtWrapper.get(), 2));
  std::optional<HttpHeaders> responseHeaders =
      convertStringToHeaders(serializedResponseHeaders, this->_pImpl->_pLogger);
  if (!responseHeaders) {
    return std::nullopt;
  }
  const uint16_t statusCode = static_cast<uint16_t>(
      CESIUM_SQLITE(sqlite3_column_int)(reader.pGetEntryStmtWrapper.get(), 3));

  const std::byte* rawResponseData = reinterpret_cast<const std::byte*>(
      CESIUM_SQLITE(sqlite3_column_blob)(reader.pGetEntryStmtWrapper.get(), 4));
  const int responseDataSize =
      CESIUM_SQLITE(sqlite3_column_bytes)(reader.pGetEntryStmtWrapper.get(), 4);
  std::vector<std::byte> responseData(
      rawResponseData,
      rawResponseData + responseDataSize);

  const std::byte* rawClientData = reinterpret_cast<const std::byte*>(
      CESIUM_SQLITE(sqlite3_column_blob)(reader.pGetEntryStmtWrapper.get(), 5));
  const int clientDataSize =
      CESIUM_SQLITE(sqlite3_column_bytes)(reader.pGetEntryStmtWrapper.get(), 5);
  std::vector<std::byte> clientData(
      rawClientData,
      rawClientData + clientDataSize);

  // parse request
  std::string serializedRequestHeaders = reinterpret_cast<const char*>(
      CESIUM_SQLITE(sqlite3_column_text)(reader.pGetEntryStmtWrapper.get(), 6));
  std::optional<HttpHeaders> requestHeaders =
      convertStringToHeaders(serializedRequestHeaders, this->_pImpl->_pLogger);
  if (!requestHeaders) {
    return std::nullopt;
  }

  std::string requestMethod = reinterpret_cast<const char*>(
      CESIUM_SQLITE(sqlite3_column_text)(reader.pGetEntryStmtWrapper.get(), 7));

  std::string requestUrl = reinterpret_cast<const char*>(
      CESIUM_SQLITE(sqlite3_column_text)(reader.pGetEntryStmtWrapper.get(), 8));

  if (!clearBindings(reader.pGetEntryStmtWrapper, this->_pImpl->_pLogger)) {
    return std::nullopt;
  }

  // reset statement so it is not locking, preventing wal checkpoints
  if (!resetStatement(reader.pGetEntryStmtWrapper, this->_pImpl->_pLogger)) {
    return std::nullopt;
  }

  return CacheItem{
      itemIndex,
      expiryTime,
      CacheRequest{
          std::move(*requestHeaders),
          std::move(requestMethod),
          std::move(requestUrl)},
      CacheResponse{
          statusCode,
          std::move(*responseHeaders),
          std::move(responseData),
          std::move(clientData)}};
}

void SqliteCache::updateLastAccessTimeWriterThread(int64_t rowId) {
  CESIUM_TRACE("Update last access time");

  // update the last accessed time
  int updateStatus = CESIUM_SQLITE(sqlite3_bind_int64)(
      this->_pImpl->_updateLastAccessedTimeStmtWrapper.get(),
      1,
      rowId);
  if (updateStatus != SQLITE_OK) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(updateStatus));
    return;
  }

  updateStatus = CESIUM_SQLITE(sqlite3_step)(
      this->_pImpl->_updateLastAccessedTimeStmtWrapper.get());

  if (!clearBindings(
          this->_pImpl->_updateLastAccessedTimeStmtWrapper,
          this->_pImpl->_pLogger)) {
    return;
  }

  if (!resetStatement(
          this->_pImpl->_updateLastAccessedTimeStmtWrapper,
          this->_pImpl->_pLogger)) {
    return;
  }

  if (updateStatus != SQLITE_DONE) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        CESIUM_SQLITE(sqlite3_errstr)(updateStatus));
    return;
  }
}

bool SqliteCache::storeEntryWriterThread(
    const std::string& key,
    std::time_t expiryTime,
    const std::string& url,
    const std::string& requestMethod,
    const HttpHeaders& requestHeaders,
    uint16_t statusCode,
    const HttpHeaders& responseHeaders,
    const gsl::span<const std::byte>& responseData,
    const gsl::span<const std::byte>& clientData) {
  CESIUM_TRACE("SqliteCache::storeEntry");
  std::lock_guard<std::mutex> guard(this->_pImpl->_writerMutex);

  int status = CESIUM_SQLITE(sqlite3_bind_int64)(
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

  status = CESIUM_SQLITE(sqlite3_bind_blob)(
      this->_pImpl->_storeResponseStmtWrapper.get(),
      6,
      clientData.data(),
      static_cast<int>(clientData.size()),
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
      7,
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
      8,
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
      9,
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
      10,
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

  if (!clearBindings(
          this->_pImpl->_storeResponseStmtWrapper,
          this->_pImpl->_pLogger)) {
    return false;
  }

  if (!resetStatement(
          this->_pImpl->_storeResponseStmtWrapper,
          this->_pImpl->_pLogger)) {
    return false;
  }

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

bool SqliteCache::pruneWriterThread() {
  CESIUM_TRACE("SqliteCache::prune");
  std::lock_guard<std::mutex> guard(this->_pImpl->_writerMutex);

  int64_t totalItems = 0;

  // query total size of response's data
  {
    int totalItemsQueryStatus = CESIUM_SQLITE(sqlite3_step)(
        this->_pImpl->_totalItemsQueryStmtWrapper.get());

    if (totalItemsQueryStatus == SQLITE_DONE) {
      clearBindings(
          this->_pImpl->_totalItemsQueryStmtWrapper,
          this->_pImpl->_pLogger);
      resetStatement(
          this->_pImpl->_totalItemsQueryStmtWrapper,
          this->_pImpl->_pLogger);
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

    if (!clearBindings(
            this->_pImpl->_totalItemsQueryStmtWrapper,
            this->_pImpl->_pLogger)) {
      return false;
    }

    if (!resetStatement(
            this->_pImpl->_totalItemsQueryStmtWrapper,
            this->_pImpl->_pLogger)) {
      return false;
    }
  }

  // delete expired rows first
  {
    int deleteExpiredStatus = CESIUM_SQLITE(sqlite3_step)(
        this->_pImpl->_deleteExpiredStmtWrapper.get());

    if (!clearBindings(
            this->_pImpl->_deleteExpiredStmtWrapper,
            this->_pImpl->_pLogger)) {
      return false;
    }

    if (!resetStatement(
            this->_pImpl->_deleteExpiredStmtWrapper,
            this->_pImpl->_pLogger)) {
      return false;
    }

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
    int deleteLLRUStatus = CESIUM_SQLITE(sqlite3_bind_int64)(
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

    if (!clearBindings(
            this->_pImpl->_deleteLRUStmtWrapper,
            this->_pImpl->_pLogger)) {
      return false;
    }

    if (!resetStatement(
            this->_pImpl->_deleteLRUStmtWrapper,
            this->_pImpl->_pLogger)) {
      return false;
    }

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

bool SqliteCache::clearAllWriterThread() {
  std::lock_guard<std::mutex> guard(this->_pImpl->_writerMutex);

  if (!resetStatement(
          this->_pImpl->_clearAllStmtWrapper,
          this->_pImpl->_pLogger)) {
    return false;
  }

  int status =
      CESIUM_SQLITE(sqlite3_step)(this->_pImpl->_clearAllStmtWrapper.get());

  if (!resetStatement(
          this->_pImpl->_clearAllStmtWrapper,
          this->_pImpl->_pLogger)) {
    return false;
  }

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
  if (remove(_pImpl->_databaseName.c_str()) != 0) {
    SPDLOG_LOGGER_ERROR(
        this->_pImpl->_pLogger,
        "Unable to delete database file.");
  }
  createConnection();
}

} // namespace CesiumAsync
