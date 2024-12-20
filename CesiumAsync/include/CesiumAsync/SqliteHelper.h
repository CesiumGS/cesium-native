#pragma once

#include <CesiumAsync/cesium-sqlite3.h>

#include <memory>
#include <string>

struct CESIUM_SQLITE(sqlite3);
struct CESIUM_SQLITE(sqlite3_stmt);

namespace CesiumAsync {

/**
 * @brief A deleter that can be used with `std::unique_ptr` to properly destroy
 * a SQLite connection when it is no longer needed.
 */
struct DeleteSqliteConnection {
  /** @brief Closes the provided sqlite connection when called. */
  void operator()(CESIUM_SQLITE(sqlite3*) pConnection) noexcept;
};

/**
 * @brief A deleter that can be used with `std::unique_ptr` to properly destroy
 * a SQLite prepared statement when it is no longer needed.
 */
struct DeleteSqliteStatement {
  /** @brief Finalizes the provided sqlite statement when called. */
  void operator()(CESIUM_SQLITE(sqlite3_stmt*) pStatement) noexcept;
};

/**
 * @brief A `std::unique_ptr<sqlite3>` that will properly delete the connection
 * using the SQLite API.
 */
using SqliteConnectionPtr =
    std::unique_ptr<CESIUM_SQLITE(sqlite3), DeleteSqliteConnection>;

/**
 * @brief A `std::unique_ptr<sqlite3_stmt>` that will properly delete the
 * statement using the SQLite API.
 */
using SqliteStatementPtr =
    std::unique_ptr<CESIUM_SQLITE(sqlite3_stmt), DeleteSqliteStatement>;

/**
 * @brief Helper functions for working with SQLite.
 */
struct SqliteHelper {
  /**
   * @brief Create a prepared statement.
   *
   * @param pConnection The SQLite connection in which to create the prepared
   * statement.
   * @param sql The SQL text for the statement.
   * @return The created prepared statement.
   */
  static SqliteStatementPtr prepareStatement(
      const SqliteConnectionPtr& pConnection,
      const std::string& sql);
};

} // namespace CesiumAsync
