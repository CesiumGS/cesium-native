#include "CesiumAsync/DiskCache.h"
#include "CesiumAsync/IAssetResponse.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include <utility>
#include <stdexcept>

namespace CesiumAsync {
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

	static const std::string CACHE_TABLE = "CacheItem";
	static const std::string KEY_COLUMN = "key";
	static const std::string EXPIRY_TIME_COLUMN = "expiryTime";
	static const std::string LAST_ACCESSED_TIME_COLUMN = "lastAccessedTime";
	static const std::string RESPONSE_HEADER_COLUMN = "responseHeaders";
	static const std::string RESPONSE_CONTENT_TYPE_COLUMN = "responseContentType";
	static const std::string RESPONSE_STATUS_CODE_COLUMN = "responseStatusCode";
	static const std::string RESPONSE_CACHE_CONTROL_COLUMN = "responseCacheControl";
	static const std::string RESPONSE_DATA_COLUMN = "responseData";
	static const std::string REQUEST_HEADER_COLUMN = "requestHeader";
	static const std::string REQUEST_METHOD_COLUMN = "requestMethod";
	static const std::string REQUEST_URL_COLUMN = "requestUrl";
	static const std::string VIRTUAL_RESPONSE_DATA_SIZE = "responseDataTotalSize";

	static std::string convertHeadersToString(const HttpHeaders& headers);

	static std::string convertCacheControlToString(const ResponseCacheControl* cacheControl);

	static HttpHeaders convertStringToHeaders(const std::string& serializedHeaders);

	static std::optional<ResponseCacheControl> convertStringToResponseCacheControl(const std::string& serializedResponseCacheControl);

	DiskCache::DiskCache(const std::string &databaseName, uint64_t maxSize)
		: _maxSize{maxSize}
		, _pConnection{nullptr}
	{
		int status = sqlite3_open(databaseName.c_str(), &this->_pConnection);
		if (status != SQLITE_OK) {
			throw std::runtime_error(sqlite3_errstr(status));
		}

		// create tables if not exist
		std::string sqlStr = "CREATE TABLE IF NOT EXISTS " + CACHE_TABLE + "(" +
			KEY_COLUMN + " TEXT PRIMARY KEY NOT NULL," +
			EXPIRY_TIME_COLUMN + " DATETIME NOT NULL," +
			LAST_ACCESSED_TIME_COLUMN + " DATETIME NOT NULL," +
			RESPONSE_HEADER_COLUMN + " TEXT NOT NULL," +
			RESPONSE_CONTENT_TYPE_COLUMN + " TEXT NOT NULL," +
			RESPONSE_STATUS_CODE_COLUMN + " INTEGER NOT NULL," +
			RESPONSE_CACHE_CONTROL_COLUMN + " TEXT NOT NULL," +
			RESPONSE_DATA_COLUMN + " BLOB," +
			REQUEST_HEADER_COLUMN + " TEXT NOT NULL," +
			REQUEST_METHOD_COLUMN + " TEXT NOT NULL," +
			REQUEST_URL_COLUMN + " TEXT NOT NULL" +
			")";
		char* createTableError;
		status = sqlite3_exec(this->_pConnection, sqlStr.c_str(), nullptr, nullptr, &createTableError);
		if (status != SQLITE_OK) {
			std::string errorStr(createTableError);
			sqlite3_free(createTableError);
			throw std::runtime_error(errorStr);
		}
		
		char* walError = nullptr;
		status = sqlite3_exec(this->_pConnection, "PRAGMA journal_mode=WAL", nullptr, nullptr, &walError);
		if (status != SQLITE_OK) {
			std::string errorStr(walError);
			sqlite3_free(walError);
			throw std::runtime_error(errorStr);
		}

		sqlite3_busy_timeout(this->_pConnection, 5000);
	}

	DiskCache::DiskCache(DiskCache&& other) noexcept {
		this->_pConnection = other._pConnection;
		this->_maxSize = other._maxSize;

		other._pConnection = nullptr;
		other._maxSize = 0;
	}

	DiskCache& DiskCache::operator=(DiskCache&& other) noexcept {
		if (&other != this) {
			this->_pConnection = other._pConnection;
			this->_maxSize = other._maxSize;

			other._pConnection = nullptr;
			other._maxSize = 0;
		}

		return *this;
	}

	DiskCache::~DiskCache() noexcept {
		if (this->_pConnection) {
			sqlite3_close(this->_pConnection);
		}
	}

	bool DiskCache::getEntry(const std::string& key, std::optional<CacheItem>& item, std::string& error) const {
		// update the last accessed time
		std::string updateSqlStr = "UPDATE " + CACHE_TABLE + " SET " + LAST_ACCESSED_TIME_COLUMN + " = strftime('%s','now') WHERE " + KEY_COLUMN + " =?";
		Sqlite3StmtWrapper updateStmtWrapper;
		int status = sqlite3_prepare_v2(this->_pConnection, updateSqlStr.c_str(), -1, &updateStmtWrapper.stmt, nullptr);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_bind_text(updateStmtWrapper.stmt, 1, key.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_step(updateStmtWrapper.stmt);
		if (status != SQLITE_DONE) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		// get entry based on key
		std::string selectSqlStr = "SELECT " +
			EXPIRY_TIME_COLUMN + ", " +
			LAST_ACCESSED_TIME_COLUMN + ", " +
			RESPONSE_HEADER_COLUMN + ", " +
			RESPONSE_CONTENT_TYPE_COLUMN + " , " +
			RESPONSE_STATUS_CODE_COLUMN + ", " +
			RESPONSE_CACHE_CONTROL_COLUMN + ", " +
			RESPONSE_DATA_COLUMN + ", " +
			REQUEST_HEADER_COLUMN + ", " +
			REQUEST_METHOD_COLUMN + ", " +
			REQUEST_URL_COLUMN +
			" FROM " + CACHE_TABLE + " WHERE " + KEY_COLUMN + "=?";

		Sqlite3StmtWrapper selectStmtWrapper;
		status = sqlite3_prepare_v2(_pConnection, selectSqlStr.c_str(), -1, &selectStmtWrapper.stmt, nullptr);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_bind_text(selectStmtWrapper.stmt, 1, key.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_step(selectStmtWrapper.stmt);
		if (status == SQLITE_DONE) {
			return true;
		}
		else if (status != SQLITE_ROW) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		// parse cache item metadata
		std::time_t expiryTime = sqlite3_column_int64(selectStmtWrapper.stmt, 0);

		std::time_t lastAccessedTime = sqlite3_column_int64(selectStmtWrapper.stmt, 1);

		// parse response cache 
		std::string serializedResponseHeaders = reinterpret_cast<const char*>(sqlite3_column_text(selectStmtWrapper.stmt, 2));
		HttpHeaders responseHeaders = convertStringToHeaders(serializedResponseHeaders);

		std::string responseContentType = reinterpret_cast<const char*>(sqlite3_column_text(selectStmtWrapper.stmt, 3));

		uint16_t statusCode = static_cast<uint16_t>(sqlite3_column_int(selectStmtWrapper.stmt, 4));

		std::string serializedResponseCacheControl = reinterpret_cast<const char*>(sqlite3_column_text(selectStmtWrapper.stmt, 5));
		std::optional<ResponseCacheControl> responseCacheControl = convertStringToResponseCacheControl(serializedResponseCacheControl);

		const void* rawResponseData = sqlite3_column_blob(selectStmtWrapper.stmt, 6);
		int responseDataSize = sqlite3_column_bytes(selectStmtWrapper.stmt, 6);
		std::vector<uint8_t> responseData(responseDataSize);
		if (responseDataSize != 0) {
			std::memcpy(responseData.data(), rawResponseData, responseDataSize);
		}

		CacheResponse cacheResponse(statusCode, 
			std::move(responseContentType), 
			std::move(responseHeaders), 
			std::move(responseCacheControl), 
			std::move(responseData));
		
		// parse request
		std::string serializedRequestHeaders = reinterpret_cast<const char*>(sqlite3_column_text(selectStmtWrapper.stmt, 7));
		HttpHeaders requestHeaders = convertStringToHeaders(serializedRequestHeaders);

		std::string requestMethod = reinterpret_cast<const char*>(sqlite3_column_text(selectStmtWrapper.stmt, 8));

		std::string requestUrl = reinterpret_cast<const char*>(sqlite3_column_text(selectStmtWrapper.stmt, 9));

		CacheRequest cacheRequest(std::move(requestHeaders), 
			std::move(requestMethod), 
			std::move(requestUrl));

		item = std::make_optional<CacheItem>(expiryTime, 
			lastAccessedTime, 
			std::move(cacheRequest), 
			std::move(cacheResponse));

		return true;
	}

	bool DiskCache::storeResponse(const std::string& key, 
		std::time_t expiryTime,
		const IAssetRequest& request,
		std::string& error) 
	{
		const IAssetResponse* response = request.response();
		if (response == nullptr) {
			error = std::string("Request needs to have a response");
			return false;
		}

		gsl::span<const uint8_t> responseData = response->data();

		Sqlite3StmtWrapper replaceStmtWrapper;
		std::string sqlStr = "REPLACE INTO " + CACHE_TABLE + " (" +
			KEY_COLUMN + ", " +
			EXPIRY_TIME_COLUMN + ", " +
			LAST_ACCESSED_TIME_COLUMN + ", " +
			RESPONSE_HEADER_COLUMN + ", " +
			RESPONSE_CONTENT_TYPE_COLUMN + " , " +
			RESPONSE_STATUS_CODE_COLUMN + ", " +
			RESPONSE_CACHE_CONTROL_COLUMN + ", " +
			RESPONSE_DATA_COLUMN + ", " +
			REQUEST_HEADER_COLUMN + ", " +
			REQUEST_METHOD_COLUMN + ", " +
			REQUEST_URL_COLUMN + ") " +
			"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
		int status = sqlite3_prepare_v2(this->_pConnection, sqlStr.c_str(), -1, &replaceStmtWrapper.stmt, nullptr);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_bind_text(replaceStmtWrapper.stmt, 1, key.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_bind_int64(replaceStmtWrapper.stmt, 2, static_cast<int64_t>(expiryTime));
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_bind_int64(replaceStmtWrapper.stmt, 3, static_cast<int64_t>(std::time(0)));
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		std::string responseHeader = convertHeadersToString(response->headers());
		status = sqlite3_bind_text(replaceStmtWrapper.stmt, 4, responseHeader.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}
		
		const std::string& responseContentType = response->contentType();
		status = sqlite3_bind_text(replaceStmtWrapper.stmt, 5, responseContentType.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_bind_int(replaceStmtWrapper.stmt, 6, static_cast<int>(response->statusCode()));
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		std::string responseCacheControl = convertCacheControlToString(response->cacheControl());
		status = sqlite3_bind_text(replaceStmtWrapper.stmt, 7, responseCacheControl.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_bind_blob(replaceStmtWrapper.stmt, 8, responseData.data(), static_cast<int>(responseData.size()), SQLITE_STATIC);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		std::string requestHeader = convertHeadersToString(request.headers());
		status = sqlite3_bind_text(replaceStmtWrapper.stmt, 9, requestHeader.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		const std::string& requestMethod = request.method();
		status = sqlite3_bind_text(replaceStmtWrapper.stmt, 10, requestMethod.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		const std::string& requestUrl = request.url();
		status = sqlite3_bind_text(replaceStmtWrapper.stmt, 11, requestUrl.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_step(replaceStmtWrapper.stmt);
		if (status != SQLITE_DONE) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		return true;
	}

	bool DiskCache::removeEntry(const std::string& key, std::string& error) 
	{
		std::string sqlStr = "DELETE FROM " + CACHE_TABLE + " WHERE " + KEY_COLUMN + "=?";

		Sqlite3StmtWrapper removeStmtWrapper;
		int status = sqlite3_prepare_v2(this->_pConnection, sqlStr.c_str(), -1, &removeStmtWrapper.stmt, nullptr);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_bind_text(removeStmtWrapper.stmt, 1, key.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_step(removeStmtWrapper.stmt);
		if (status != SQLITE_DONE) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		return true;
	}

	bool DiskCache::prune(std::string& error)
	{
		// query total size of response's data
		std::string totalSizeQuerySqlStr = "SELECT SUM(LENGTH(" + RESPONSE_DATA_COLUMN + ")) AS " + 
			VIRTUAL_RESPONSE_DATA_SIZE + 
			" FROM " + 
			CACHE_TABLE;
		Sqlite3StmtWrapper totalSizeQueryStmtWrapper;
		int status = sqlite3_prepare_v2(this->_pConnection, totalSizeQuerySqlStr.c_str(), -1, &totalSizeQueryStmtWrapper.stmt, nullptr);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_step(totalSizeQueryStmtWrapper.stmt);
		if (status == SQLITE_DONE) {
			return true;
		}
		else if (status != SQLITE_ROW) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		// prune the row if over maximum
		int64_t totalDataSize = sqlite3_column_int64(totalSizeQueryStmtWrapper.stmt, 0);
		if (totalDataSize > 0 && totalDataSize <= static_cast<int64_t>(_maxSize)) {
			return true;
		}

		// sort by expiry time first
		Sqlite3StmtWrapper expiryTimeQueryByOrderStmtWrapper;
		std::string expiryTimeQueryByOrderSqlStr = "SELECT " + KEY_COLUMN + ", LENGTH(" + RESPONSE_DATA_COLUMN + ") FROM " +
			CACHE_TABLE + " ORDER BY " + EXPIRY_TIME_COLUMN + " ASC";
		int expiryTimeQueryByOrderStatus = sqlite3_prepare_v2(this->_pConnection, expiryTimeQueryByOrderSqlStr.c_str(), -1, &expiryTimeQueryByOrderStmtWrapper.stmt, nullptr);
		if (expiryTimeQueryByOrderStatus != SQLITE_OK) {
			error = std::string(sqlite3_errstr(expiryTimeQueryByOrderStatus));
			return false;
		}

		// prepare delete sqlite command
		Sqlite3StmtWrapper deleteStmtWrapper;
		std::string deleteSqlStr = "DELETE FROM " + CACHE_TABLE + " WHERE " + KEY_COLUMN + "=?";
		int deleteStatus = sqlite3_prepare_v2(this->_pConnection, deleteSqlStr.c_str(), -1, &deleteStmtWrapper.stmt, nullptr);
		if (deleteStatus != SQLITE_OK) {
			error = std::string(sqlite3_errstr(deleteStatus));
			return false;
		}

		while (totalDataSize > static_cast<int64_t>(_maxSize) && ((expiryTimeQueryByOrderStatus = sqlite3_step(expiryTimeQueryByOrderStmtWrapper.stmt)) == SQLITE_ROW)) {
			const char *key = reinterpret_cast<const char *>(sqlite3_column_text(expiryTimeQueryByOrderStmtWrapper.stmt, 0));
			int64_t size = sqlite3_column_int64(expiryTimeQueryByOrderStmtWrapper.stmt, 1);
			totalDataSize -= size;

			deleteStatus = sqlite3_bind_text(deleteStmtWrapper.stmt, 1, key, -1, SQLITE_STATIC);
			if (deleteStatus != SQLITE_OK) {
				error = std::string(sqlite3_errstr(deleteStatus));
				return false;
			}

			deleteStatus = sqlite3_step(deleteStmtWrapper.stmt);
			if (deleteStatus != SQLITE_DONE) {
				error = std::string(sqlite3_errstr(deleteStatus));
				return false;
			}

			deleteStatus = sqlite3_reset(deleteStmtWrapper.stmt);
			if (deleteStatus != SQLITE_OK) {
				error = std::string(sqlite3_errstr(deleteStatus));
				return false;
			}
		}

		// check final status
		if (expiryTimeQueryByOrderStatus != SQLITE_DONE && expiryTimeQueryByOrderStatus != SQLITE_ROW) {
			error = std::string(sqlite3_errstr(expiryTimeQueryByOrderStatus));
			return false;
		}

		return true;
	}

	bool DiskCache::clearAll(std::string& error)
	{
		std::string sqlStr = "DELETE FROM " + CACHE_TABLE;

		Sqlite3StmtWrapper deleteStmtWrapper;
		int status = sqlite3_prepare_v2(this->_pConnection, sqlStr.c_str(), -1, &deleteStmtWrapper.stmt, nullptr);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_step(deleteStmtWrapper.stmt);
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
		for (const std::pair<std::string, std::string>& header : headers) 
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

	std::string convertCacheControlToString(const ResponseCacheControl* cacheControl) {
		if (!cacheControl) {
			return "";
		}

		rapidjson::Document document;
		rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
		rapidjson::Value root(rapidjson::kObjectType);
		root.AddMember("mustRevalidate", rapidjson::Value(cacheControl->mustRevalidate()), allocator);
		root.AddMember("noCache", rapidjson::Value(cacheControl->noCache()), allocator);
		root.AddMember("noStore", rapidjson::Value(cacheControl->noStore()), allocator);
		root.AddMember("noTransform", rapidjson::Value(cacheControl->noTransform()), allocator);
		root.AddMember("accessControlPublic", rapidjson::Value(cacheControl->accessControlPublic()), allocator);
		root.AddMember("accessControlPrivate", rapidjson::Value(cacheControl->accessControlPublic()), allocator);
		root.AddMember("proxyRevalidate", rapidjson::Value(cacheControl->proxyRevalidate()), allocator);
		root.AddMember("maxAge", rapidjson::Value(cacheControl->maxAge()), allocator);
		root.AddMember("sharedMaxAge", rapidjson::Value(cacheControl->sharedMaxAge()), allocator);

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

	std::optional<ResponseCacheControl> convertStringToResponseCacheControl(const std::string& serializedResponseCacheControl) {
		if (serializedResponseCacheControl.empty()) {
			return std::nullopt;
		}

		rapidjson::Document document;
		document.Parse(serializedResponseCacheControl.c_str());
		return ResponseCacheControl{
			document["mustRevalidate"].GetBool(),
			document["noCache"].GetBool(),
			document["noStore"].GetBool(),
			document["noTransform"].GetBool(),
			document["accessControlPublic"].GetBool(),
			document["accessControlPrivate"].GetBool(),
			document["proxyRevalidate"].GetBool(),
			document["maxAge"].GetInt(),
			document["sharedMaxAge"].GetInt()
		};
	}
}