#include "DiskCache.h"
#include "rapidjson/rapidjson.h"
#include <utility>
#include <stdexcept>

namespace CesiumAsync {
	static void checkSQLResult(int expectResult, int currentResult) {
		if (currentResult != expectResult) {
			throw std::runtime_error(sqlite3_errstr(currentResult));
		}
	}

	DiskCache::Sqlite3StmtWrapper::Sqlite3StmtWrapper(const std::string& statement, sqlite3 *connection) 
		: pStmt{nullptr}
	{
		checkSQLResult(SQLITE_OK, 
			sqlite3_prepare_v2(connection, 
				statement.c_str(),
				-1, 
				&pStmt, 
				nullptr));
	}

	DiskCache::Sqlite3StmtWrapper::Sqlite3StmtWrapper(Sqlite3StmtWrapper&& other) noexcept
	{
		this->pStmt = other.pStmt;
		other.pStmt = nullptr;
	}

	DiskCache::Sqlite3StmtWrapper& DiskCache::Sqlite3StmtWrapper::operator=(Sqlite3StmtWrapper&& other) noexcept {
		if (this != &other) {
			std::swap(this->pStmt, other.pStmt);
		}

		return *this;
	}

	DiskCache::Sqlite3StmtWrapper::~Sqlite3StmtWrapper() noexcept {
		sqlite3_finalize(pStmt);
	}

	DiskCache::DiskCache(const std::string &databaseName)
	{
		int status = sqlite3_open(databaseName.c_str(), &this->_pConnection);
		if (status != SQLITE_OK) {
			if (this->_pConnection) {
				throw std::runtime_error(sqlite3_errmsg(this->_pConnection));
			}
			else {
				throw std::runtime_error(std::string("Cannot open database connection for ") + databaseName);
			}
		}

		// create tables if not exist
		Sqlite3StmtWrapper stmt(
			"CREATE TABLE IF NOT EXISTS Cache (url TEXT PRIMARY KEY, responseHeader TEXT, responseStatusCode INTEGER, responseContentType TEXT, responseData BLOB);", 
			this->_pConnection);
		checkSQLResult(SQLITE_DONE, sqlite3_step(stmt.pStmt));

		// initialize statements to manipulate cached entries
		this->_insertStmt = std::make_optional<Sqlite3StmtWrapper>(
			"INSERT OR REPLACE INTO Cache (url, responseHeader, responseStatusCode, responseContentType, responseData) values (?, ?, ?, ?, ?);", 
			this->_pConnection);
	}

	DiskCache::~DiskCache() noexcept
	{
		if (this->_pConnection) {
			this->_insertStmt.reset();
			sqlite3_close(this->_pConnection);
		}
	}

	DiskCache::DiskCache(DiskCache&& other) noexcept
	{
		this->_pConnection = other._pConnection;
		this->_insertStmt = std::move(other._insertStmt);
		other._pConnection = nullptr;
	}

	DiskCache& DiskCache::operator=(DiskCache&& other) noexcept
	{
		if (this != &other) {
			std::swap(this->_pConnection, other._pConnection);
			std::swap(this->_insertStmt, other._insertStmt);
		}

		return *this;
	}

	std::unique_ptr<IAssetRequest> DiskCache::getEntry(const std::string& ) const {
		return nullptr;
	}

	void DiskCache::insertEntry(const IAssetRequest &entry) {
		sqlite3_stmt* stmt = this->_insertStmt->pStmt;
		const IAssetResponse* response = entry.response();
		if (!response) {
			return;
		}

		std::string requestUrl = entry.url();
		const std::string& responseContentType = response->contentType();
		gsl::span<const uint8_t> responseData = response->data();
		checkSQLResult(SQLITE_OK, sqlite3_reset(stmt));
		checkSQLResult(SQLITE_OK, sqlite3_bind_text(stmt, 1, requestUrl.c_str(), static_cast<int>(requestUrl.size()), SQLITE_STATIC));
		checkSQLResult(SQLITE_OK, sqlite3_bind_text(stmt, 2, "", static_cast<int>(1), SQLITE_STATIC)); // header
		checkSQLResult(SQLITE_OK, sqlite3_bind_int(stmt, 3, response->statusCode()));
		checkSQLResult(SQLITE_OK, sqlite3_bind_text(stmt, 4, responseContentType.c_str(), static_cast<int>(responseContentType.size()), SQLITE_STATIC)); 
		checkSQLResult(SQLITE_OK, sqlite3_bind_blob(stmt, 4, responseData.data(), static_cast<int>(responseData.size()), SQLITE_STATIC)); 
		checkSQLResult(SQLITE_DONE, sqlite3_step(stmt));
	}

	void DiskCache::removeEntry(const std::string&)
	{
	}

	void DiskCache::clearAll()
	{
	}
}