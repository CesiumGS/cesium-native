#include <CesiumAsync/SqliteHelper.h>
#include <CesiumAsync/cesium-sqlite3.h>

#include <sqlite3.h>

#include <stdexcept>
#include <string>

namespace CesiumAsync {

void DeleteSqliteConnection::operator()(CESIUM_SQLITE(sqlite3*)
                                            pConnection) noexcept {
  CESIUM_SQLITE(sqlite3_close_v2)(pConnection);
}

void DeleteSqliteStatement::operator()(CESIUM_SQLITE(sqlite3_stmt*)
                                           pStatement) noexcept {
  CESIUM_SQLITE(sqlite3_finalize)(pStatement);
}

SqliteStatementPtr SqliteHelper::prepareStatement(
    const SqliteConnectionPtr& pConnection,
    const std::string& sql) {
  CESIUM_SQLITE(sqlite3_stmt*) pStmt;
  const int status = CESIUM_SQLITE(sqlite3_prepare_v2)(
      pConnection.get(),
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

} // namespace CesiumAsync
