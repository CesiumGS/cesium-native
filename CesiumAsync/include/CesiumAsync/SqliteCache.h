#pragma once

#define sqlite3 cqlite3

#include "CesiumAsync/ICacheDatabase.h"
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <spdlog/fwd.h>
#include <string>

struct sqlite3;
struct sqlite3_stmt;

namespace CesiumAsync {

/**
 * @brief Cache storage using SQLITE to store completed response.
 */
class CESIUMASYNC_API SqliteCache : public ICacheDatabase {
public:
  /**
   * @brief Constructs a new instance with a given `databaseName` pointing to a
   * database.
   *
   * The instance will connect to the existing database or create a new one if
   * it doesn't exist
   *
   * @param pLogger The logger that receives error messages.
   * @param databaseName the database path.
   * @param maxItems the maximum number of items should be kept in the database
   * after prunning.
   */
  SqliteCache(
      const std::shared_ptr<spdlog::logger>& pLogger,
      const std::string& databaseName,
      uint64_t maxItems = 4096);

  /** @copydoc ICacheDatabase::getEntry*/
  virtual std::optional<CacheItem>
  getEntry(const std::string& key) const override;

  /** @copydoc ICacheDatabase::storeEntry*/
  virtual bool storeEntry(
      const std::string& key,
      std::time_t expiryTime,
      const std::string& url,
      const std::string& requestMethod,
      const HttpHeaders& requestHeaders,
      uint16_t statusCode,
      const HttpHeaders& responseHeaders,
      const gsl::span<const std::byte>& responseData) override;

  /** @copydoc ICacheDatabase::prune*/
  virtual bool prune() override;

  /** @copydoc ICacheDatabase::clearAll*/
  virtual bool clearAll() override;

private:
  struct DeleteSqliteConnection {
    void operator()(sqlite3* pConnection) noexcept;
  };

  struct DeleteSqliteStatement {
    void operator()(sqlite3_stmt* pStmt) noexcept;
  };

  using SqliteConnectionPtr = std::unique_ptr<sqlite3, DeleteSqliteConnection>;
  using SqliteStatementPtr =
      std::unique_ptr<sqlite3_stmt, DeleteSqliteStatement>;

  static SqliteStatementPtr prepareStatement(
      const SqliteConnectionPtr& pConnection,
      const std::string& sql);

  std::shared_ptr<spdlog::logger> _pLogger;
  SqliteConnectionPtr _pConnection;
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
} // namespace CesiumAsync