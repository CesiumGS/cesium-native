#include "DiskCache.h"
#include "CesiumAsync/IAssetResponse.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include <utility>
#include <stdexcept>

namespace CesiumAsync {
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

	static std::string convertHeadersToString(const std::map<std::string, std::string>& headers);

	static std::string convertCacheControlToString(const ResponseCacheControl* cacheControl);

	static std::map<std::string, std::string> convertStringToHeaders(const std::string& serializedHeaders);

	static std::optional<ResponseCacheControl> convertStringToResponseCacheControl(const std::string& serializedResponseCacheControl);

	DiskCache::DiskCache(const std::string &databaseName, uint64_t maxSize)
		: _maxSize{maxSize}
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
		std::string sqlStr = "CREATE TABLE IF NOT EXISTS " + CACHE_TABLE + "(" +
			KEY_COLUMN + " TEXT PRIMARY KEY NOT NULL," +
			EXPIRY_TIME_COLUMN + " DATETIME NOT NULL," +
			LAST_ACCESSED_TIME_COLUMN + " DATETIME NOT NULL," +
			RESPONSE_HEADER_COLUMN + " TEXT NOT NULL," +
			RESPONSE_CONTENT_TYPE_COLUMN + " TEXT NOT NULL," +
			RESPONSE_STATUS_CODE_COLUMN + " INTEGER NOT NULL," +
			RESPONSE_CACHE_CONTROL_COLUMN + " TEXT NOT NULL," +
			RESPONSE_DATA_COLUMN + " BLOB NOT NULL," +
			REQUEST_HEADER_COLUMN + " TEXT NOT NULL," +
			REQUEST_METHOD_COLUMN + " TEXT NOT NULL," +
			REQUEST_URL_COLUMN + " TEXT NOT NULL" +
			");";
		char* error;
		status = sqlite3_exec(this->_pConnection, sqlStr.c_str(), nullptr, nullptr, &error);
		if (status != SQLITE_OK) {
			std::string errorStr(error);
			sqlite3_free(error);
			throw std::runtime_error(errorStr);
		}
	}

	DiskCache::~DiskCache() noexcept
	{
		if (this->_pConnection) {
			sqlite3_close(this->_pConnection);
		}
	}

	DiskCache::DiskCache(DiskCache&& other) noexcept
	{
		this->_pConnection = other._pConnection;
		other._pConnection = nullptr;
	}

	DiskCache& DiskCache::operator=(DiskCache&& other) noexcept
	{
		if (this != &other) {
			std::swap(this->_pConnection, other._pConnection);
		}

		return *this;
	}

	bool DiskCache::getEntry(const std::string& key, std::optional<CacheItem>& item, std::string& error) const {
		sqlite3_stmt* stmt = nullptr;
		std::string sqlStr = "SELECT " +
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
			" FROM " + CACHE_TABLE + " WHERE " + KEY_COLUMN + "=?;";

		int status = sqlite3_prepare_v2(this->_pConnection, sqlStr.c_str(), -1, &stmt, nullptr);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_step(stmt);
		if (status == SQLITE_DONE) {
			sqlite3_finalize(stmt);
			return true;
		}
		else if (status != SQLITE_ROW) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		// parse response cache 
		std::time_t expiryTime = sqlite3_column_int64(stmt, 0);

		std::time_t lastAccessedTime = sqlite3_column_int64(stmt, 1);

		std::string serializedResponseHeaders = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		std::map<std::string, std::string> responseHeaders = convertStringToHeaders(serializedResponseHeaders);

		std::string responseContentType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

		uint16_t statusCode = static_cast<uint16_t>(sqlite3_column_int(stmt, 4));

		std::string serializedResponseCacheControl = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
		std::optional<ResponseCacheControl> responseCacheControl = convertStringToResponseCacheControl(serializedResponseCacheControl);

		const void* rawResponseData = sqlite3_column_blob(stmt, 6);
		int responseDataSize = sqlite3_column_bytes(stmt, 6);
		std::vector<uint8_t> responseData(responseDataSize);
		std::memcpy(responseData.data(), rawResponseData, responseDataSize);

		CacheResponse cacheResponse(statusCode, 
			std::move(responseContentType), 
			std::move(responseHeaders), 
			std::move(responseCacheControl), 
			std::move(responseData));
		
		// parse request
		std::string serializedRequestHeaders = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
		std::map<std::string, std::string> requestHeaders = convertStringToHeaders(serializedRequestHeaders);

		std::string requestMethod = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));

		std::string requestUrl = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));

		CacheRequest cacheRequest(std::move(requestHeaders), 
			std::move(requestMethod), 
			std::move(requestUrl));

		sqlite3_finalize(stmt);

		// update the last accessed time
		stmt = nullptr;
		sqlStr = "UPDATE " + CACHE_TABLE + " SET " + LAST_ACCESSED_TIME_COLUMN + " = strftime('%s','now') WHERE " + KEY_COLUMN + " =?;";

		status = sqlite3_prepare_v2(this->_pConnection, sqlStr.c_str(), -1, &stmt, nullptr);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_step(stmt);
		if (status != SQLITE_DONE) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		sqlite3_finalize(stmt);

		item = std::make_optional<CacheItem>(expiryTime, 
			lastAccessedTime, 
			std::move(cacheRequest), 
			std::move(cacheResponse));
;
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

		sqlite3_stmt* stmt = nullptr;
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
			"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
		int status = sqlite3_prepare_v2(this->_pConnection, sqlStr.c_str(), -1, &stmt, nullptr);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(expiryTime));
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(std::time(0)));
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		std::string responseHeader = convertHeadersToString(response->headers());
		status = sqlite3_bind_text(stmt, 4, responseHeader.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}
		
		const std::string& responseContentType = response->contentType();
		status = sqlite3_bind_text(stmt, 5, responseContentType.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_bind_int(stmt, 6, static_cast<int>(response->statusCode()));
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		std::string responseCacheControl = convertCacheControlToString(response->cacheControl());
		status = sqlite3_bind_text(stmt, 7, responseCacheControl.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_bind_blob(stmt, 8, responseData.data(), static_cast<int>(responseData.size()), SQLITE_STATIC);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		std::string requestHeader = convertHeadersToString(request.headers());
		status = sqlite3_bind_text(stmt, 9, requestHeader.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		const std::string& requestMethod = request.method();
		status = sqlite3_bind_text(stmt, 10, requestMethod.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		const std::string& requestUrl = request.url();
		status = sqlite3_bind_text(stmt, 11, requestUrl.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_step(stmt);
		if (status != SQLITE_DONE) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		sqlite3_finalize(stmt);

		return true;
	}

	bool DiskCache::removeEntry(const std::string& key, std::string& error) 
	{
		std::string sqlStr = "DELETE FROM " + CACHE_TABLE + " WHERE " + KEY_COLUMN + "=?;";

		sqlite3_stmt* stmt = nullptr;
		int status = sqlite3_prepare_v2(this->_pConnection, sqlStr.c_str(), -1, &stmt, nullptr);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_step(stmt);
		if (status != SQLITE_DONE) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		sqlite3_finalize(stmt);

		return true;
	}

	bool DiskCache::prune(std::string& error)
	{
		// query total size of response's data
		sqlite3_stmt* stmt = nullptr;
		std::string sqlStr = "SELECT SUM(LENGTH(" + RESPONSE_DATA_COLUMN + ")) AS " + 
			VIRTUAL_RESPONSE_DATA_SIZE + 
			" FROM " + 
			CACHE_TABLE + 
			";";
		int status = sqlite3_prepare_v2(this->_pConnection, sqlStr.c_str(), -1, &stmt, nullptr);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_step(stmt);
		if (status == SQLITE_DONE) {
			sqlite3_finalize(stmt);
			return true;
		}
		else if (status != SQLITE_ROW) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		// prune the row if over maximum
		int64_t totalDataSize = sqlite3_column_int64(stmt, 0);
		if (totalDataSize > 0 && totalDataSize <= static_cast<int64_t>(_maxSize)) {
			return true;
		}

		// sort by expiry time first
		stmt = nullptr;
		sqlStr = "SELECT " + KEY_COLUMN + ", LENGTH(" + RESPONSE_DATA_COLUMN + ") FROM " +
			CACHE_TABLE + " ORDER BY " + EXPIRY_TIME_COLUMN + " ASC;";
		status = sqlite3_prepare_v2(this->_pConnection, sqlStr.c_str(), -1, &stmt, nullptr);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		// prepare delete sqlite command
		sqlite3_stmt* delStmt = nullptr;
		std::string sqlDelStr = "DELETE FROM " + CACHE_TABLE + " WHERE " + KEY_COLUMN + "=?;";
		int delStatus = sqlite3_prepare_v2(this->_pConnection, sqlDelStr.c_str(), -1, &delStmt, nullptr);
		if (delStatus != SQLITE_OK) {
			error = std::string(sqlite3_errstr(delStatus));
			return false;
		}

		while (totalDataSize > static_cast<int64_t>(_maxSize) && ((status = sqlite3_step(stmt)) == SQLITE_ROW)) {
			const char *key = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
			int64_t size = sqlite3_column_int64(stmt, 1);
			totalDataSize -= size;

			delStatus = sqlite3_bind_text(delStmt, 1, key, -1, SQLITE_STATIC);
			if (delStatus != SQLITE_OK) {
				sqlite3_finalize(delStmt);
				error = std::string(sqlite3_errstr(delStatus));
				return false;
			}

			delStatus = sqlite3_step(delStmt);
			if (delStatus != SQLITE_DONE) {
				sqlite3_finalize(delStmt);
				error = std::string(sqlite3_errstr(delStatus));
				return false;
			}

			delStatus = sqlite3_reset(delStmt);
			if (delStatus != SQLITE_OK) {
				sqlite3_finalize(delStmt);
				error = std::string(sqlite3_errstr(delStatus));
				return false;
			}
		}

		sqlite3_finalize(delStmt);

		// check final status
		if (status != SQLITE_DONE && status != SQLITE_ROW) {
			sqlite3_finalize(stmt);
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		sqlite3_finalize(stmt);

		return true;
	}

	bool DiskCache::clearAll(std::string& error)
	{
		std::string sqlStr = "DELETE FROM " + CACHE_TABLE + ";";

		sqlite3_stmt* stmt = nullptr;
		int status = sqlite3_prepare_v2(this->_pConnection, sqlStr.c_str(), -1, &stmt, nullptr);
		if (status != SQLITE_OK) {
			error = std::string(sqlite3_errstr(status));
			return false;
		}

		status = sqlite3_step(stmt);
		if (status != SQLITE_DONE) {
			sqlite3_finalize(stmt);
			return false;
		}

		sqlite3_finalize(stmt);

		return true;
	}

	std::string convertHeadersToString(const std::map<std::string, std::string>& headers) {
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

	std::map<std::string, std::string> convertStringToHeaders(const std::string& serializedHeaders) {
		rapidjson::Document document;
		document.Parse(serializedHeaders.c_str());

		std::map<std::string, std::string> headers;
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