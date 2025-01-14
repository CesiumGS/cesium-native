#include <Cesium3DTilesSelection/DebugTileStateDatabase.h>
#include <Cesium3DTilesSelection/TileID.h>
#include <Cesium3DTilesSelection/TileSelectionState.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <CesiumAsync/SqliteHelper.h>
#include <CesiumAsync/cesium-sqlite3.h>

#include <fmt/format.h>
#include <sqlite3.h>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace CesiumAsync;

namespace {

const std::string DROP_STATE_TABLE_SQL = "DROP TABLE IF EXISTS TileStates";
const std::string CREATE_STATE_TABLE_SQL =
    "CREATE TABLE IF NOT EXISTS TileSelectionStates ("
    " Value INTEGER PRIMARY KEY,"
    " Name TEXT"
    ");"
    "CREATE TABLE IF NOT EXISTS IsRenderableStates ("
    " Value INTEGER PRIMARY KEY, "
    " Name TEXT"
    ");"
    "CREATE TABLE IF NOT EXISTS TileSelectionFrames ("
    " Pointer INTEGER NOT NULL,"
    " FrameNumber INTEGER NOT NULL,"
    " TileID TEXT,"
    " SelectionStateFrameNumber INTEGER,"
    " SelectionState INTEGER,"
    " IsRenderable BOOLEAN,"
    " PRIMARY KEY (Pointer, FrameNumber),"
    " FOREIGN KEY (SelectionState) REFERENCES TileSelectionStates(Value),"
    " FOREIGN KEY (IsRenderable) REFERENCES IsRenderableStates(Value)"
    ")";
const std::string PRAGMA_WAL_SQL = "PRAGMA journal_mode=WAL";
const std::string PRAGMA_SYNC_SQL = "PRAGMA synchronous=OFF";
const std::string WRITE_TILE_SELECTION_SQL =
    "REPLACE INTO TileSelectionFrames ("
    " Pointer,"
    " FrameNumber,"
    " TileID,"
    " SelectionStateFrameNumber,"
    " SelectionState,"
    " IsRenderable"
    ") VALUES ("
    " ?, ?, ?, ?, ?, ?"
    ")";

} // namespace

namespace Cesium3DTilesSelection {

struct DebugTileStateDatabase::Impl {
  CesiumAsync::SqliteConnectionPtr pConnection;
  CesiumAsync::SqliteStatementPtr writeTileState;
};

DebugTileStateDatabase::DebugTileStateDatabase(
    const std::string& databaseFilename)
    : _pImpl(std::make_unique<Impl>()) {
  CESIUM_SQLITE(sqlite3*) pConnection;
  int status =
      CESIUM_SQLITE(sqlite3_open)(databaseFilename.c_str(), &pConnection);
  if (status != SQLITE_OK) {
    throw std::runtime_error(CESIUM_SQLITE(sqlite3_errstr)(status));
  }

  this->_pImpl->pConnection =
      std::unique_ptr<CESIUM_SQLITE(sqlite3), DeleteSqliteConnection>(
          pConnection);

  // Recreate state database
  char* createTableError = nullptr;

  status = CESIUM_SQLITE(sqlite3_exec)(
      this->_pImpl->pConnection.get(),
      DROP_STATE_TABLE_SQL.c_str(),
      nullptr,
      nullptr,
      &createTableError);
  if (status != SQLITE_OK) {
    std::string errorStr(createTableError);
    CESIUM_SQLITE(sqlite3_free)(createTableError);
    throw std::runtime_error(errorStr);
  }

  status = CESIUM_SQLITE(sqlite3_exec)(
      this->_pImpl->pConnection.get(),
      CREATE_STATE_TABLE_SQL.c_str(),
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
      this->_pImpl->pConnection.get(),
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
      this->_pImpl->pConnection.get(),
      PRAGMA_SYNC_SQL.c_str(),
      nullptr,
      nullptr,
      &syncError);
  if (status != SQLITE_OK) {
    std::string errorStr(syncError);
    CESIUM_SQLITE(sqlite3_free)(syncError);
    throw std::runtime_error(errorStr);
  }

  this->_pImpl->writeTileState = SqliteHelper::prepareStatement(
      this->_pImpl->pConnection,
      WRITE_TILE_SELECTION_SQL);

  std::vector<std::pair<TileSelectionState::Result, std::string>>
      tileSelectionStates = {
          {TileSelectionState::Result::None, "None"},
          {TileSelectionState::Result::Culled, "Culled"},
          {TileSelectionState::Result::Rendered, "Rendered"},
          {TileSelectionState::Result::Refined, "Refined"},
          {TileSelectionState::Result::RenderedAndKicked, "RenderedAndKicked"},
          {TileSelectionState::Result::RefinedAndKicked, "RefinedAndKicked"}};

  for (const auto& state : tileSelectionStates) {
    std::string sql = fmt::format(
        "REPLACE INTO TileSelectionStates (Value, Name) VALUES ({}, '{}');",
        static_cast<int32_t>(state.first),
        state.second.c_str());
    status = CESIUM_SQLITE(sqlite3_exec)(
        this->_pImpl->pConnection.get(),
        sql.c_str(),
        nullptr,
        nullptr,
        nullptr);
  }

  std::vector<std::pair<int32_t, std::string>> isRenderableStates = {
      {0, "Not Renderable"},
      {1, "Renderable"}};

  for (const auto& state : isRenderableStates) {
    std::string sql = fmt::format(
        "REPLACE INTO IsRenderableStates (Value, Name) VALUES ({}, '{}');",
        state.first,
        state.second.c_str());
    status = CESIUM_SQLITE(sqlite3_exec)(
        this->_pImpl->pConnection.get(),
        sql.c_str(),
        nullptr,
        nullptr,
        nullptr);
  }
}

DebugTileStateDatabase::~DebugTileStateDatabase() noexcept = default;

void DebugTileStateDatabase::recordAllTileStates(
    int32_t frameNumber,
    const Tileset& tileset) {
  int status = CESIUM_SQLITE(sqlite3_exec)(
      this->_pImpl->pConnection.get(),
      "BEGIN TRANSACTION",
      nullptr,
      nullptr,
      nullptr);
  if (status != SQLITE_OK) {
    return;
  }

  tileset.forEachLoadedTile([frameNumber, this](const Tile& tile) {
    this->recordTileState(frameNumber, tile);
  });

  status = CESIUM_SQLITE(sqlite3_exec)(
      this->_pImpl->pConnection.get(),
      "COMMIT TRANSACTION",
      nullptr,
      nullptr,
      nullptr);
}

void DebugTileStateDatabase::recordTileState(
    int32_t frameNumber,
    const Tile& tile) {
  int status = CESIUM_SQLITE(sqlite3_reset)(this->_pImpl->writeTileState.get());
  if (status != SQLITE_OK) {
    return;
  }

  status =
      CESIUM_SQLITE(sqlite3_clear_bindings)(this->_pImpl->writeTileState.get());
  if (status != SQLITE_OK) {
    return;
  }

  status = CESIUM_SQLITE(sqlite3_bind_int64)(
      this->_pImpl->writeTileState.get(),
      1, // Pointer
      reinterpret_cast<int64_t>(&tile));
  if (status != SQLITE_OK) {
    return;
  }

  status = CESIUM_SQLITE(sqlite3_bind_int)(
      this->_pImpl->writeTileState.get(),
      2, // FrameNumber
      frameNumber);
  if (status != SQLITE_OK) {
    return;
  }

  std::string id = TileIdUtilities::createTileIdString(tile.getTileID());
  status = CESIUM_SQLITE(sqlite3_bind_text)(
      this->_pImpl->writeTileState.get(),
      3, // TileID
      id.c_str(),
      static_cast<int>(id.size()),
      SQLITE_STATIC);
  if (status != SQLITE_OK) {
    return;
  }

  status = CESIUM_SQLITE(sqlite3_bind_int)(
      this->_pImpl->writeTileState.get(),
      4, // SelectionStateFrameNumber
      tile.getLastSelectionState().getFrameNumber());
  if (status != SQLITE_OK) {
    return;
  }

  status = CESIUM_SQLITE(sqlite3_bind_int)(
      this->_pImpl->writeTileState.get(),
      5, // SelectionState
      static_cast<int>(tile.getLastSelectionState().getResult(
          tile.getLastSelectionState().getFrameNumber())));
  if (status != SQLITE_OK) {
    return;
  }

  status = CESIUM_SQLITE(sqlite3_bind_int)(
      this->_pImpl->writeTileState.get(),
      6, // IsRenderable
      static_cast<int>(tile.isRenderable()));
  if (status != SQLITE_OK) {
    return;
  }

  status = CESIUM_SQLITE(sqlite3_step)(this->_pImpl->writeTileState.get());
  if (status != SQLITE_DONE) {
    return;
  }
}

} // namespace Cesium3DTilesSelection
