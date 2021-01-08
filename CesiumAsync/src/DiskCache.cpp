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

	static std::string convertHeadersToString(const std::map<std::string, std::string>& headers);

	static std::string convertCacheControlToString(const ResponseCacheControl* cacheControl);

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
		char* error;
		status = sqlite3_exec(this->_pConnection,
			("CREATE TABLE IF NOT EXISTS " + CACHE_TABLE + "(" + 
				KEY_COLUMN + " TEXT PRIMARY KEY NOT NULL, " +
				EXPIRY_TIME_COLUMN + " DATETIME NOT NULL" +
				LAST_ACCESSED_TIME_COLUMN+ " DATETIME NOT NULL" +
				RESPONSE_HEADER_COLUMN + " TEXT NOT NULL, " +
				RESPONSE_CONTENT_TYPE_COLUMN + " TEXT NOT NULL, " +
				RESPONSE_STATUS_CODE_COLUMN + " INTEGER NOT NULL, " +
				RESPONSE_CACHE_CONTROL_COLUMN + " TEXT NOT NULL, " +
				RESPONSE_DATA_COLUMN + " BLOB NOT NULL, " +
				REQUEST_HEADER_COLUMN + " TEXT NOT NULL, " +
				REQUEST_METHOD_COLUMN + " TEXT NOT NULL, " +
				REQUEST_URL_COLUMN + " TEXT NOT NULL" +
				");").c_str(), 
			nullptr, 
			nullptr, 
			&error);

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

	std::optional<CacheItem> DiskCache::getEntry(const std::string& ) const {
		return std::nullopt;
	}

	void DiskCache::storeResponse(const std::string& key, 
		std::time_t expiryTime,
		const IAssetRequest& request,
		std::function<void(std::string)> onError) 
	{
		const IAssetResponse* response = request.response();
		if (response == nullptr) {
			return;
		}

		gsl::span<const uint8_t> responseData = response->data();

		sqlite3_stmt* stmt;
		int status = sqlite3_prepare_v2(this->_pConnection,
			("REPLACE INTO " + CACHE_TABLE + " (" +
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
				REQUEST_URL_COLUMN + ")" +
				"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);").c_str(), -1, &stmt, nullptr);
		if (status != SQLITE_OK) {
			onError(std::string(sqlite3_errstr(status)));
			return;
		}

		status = sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			onError(std::string(sqlite3_errstr(status)));
			return;
		}

		status = sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(expiryTime));
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			onError(std::string(sqlite3_errstr(status)));
			return;
		}

		status = sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(std::time(0)));
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			onError(std::string(sqlite3_errstr(status)));
			return;
		}

		status = sqlite3_bind_text(stmt, 4, convertHeadersToString(response->headers()).c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			onError(std::string(sqlite3_errstr(status)));
			return;
		}
		
		status = sqlite3_bind_text(stmt, 5, response->contentType().c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			onError(std::string(sqlite3_errstr(status)));
			return;
		}

		status = sqlite3_bind_int(stmt, 6, static_cast<int>(response->statusCode()));
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			onError(std::string(sqlite3_errstr(status)));
			return;
		}

		status = sqlite3_bind_text(stmt, 7, convertCacheControlToString(response->cacheControl()).c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			onError(std::string(sqlite3_errstr(status)));
			return;
		}

		status = sqlite3_bind_blob(stmt, 8, responseData.data(), static_cast<int>(responseData.size()), SQLITE_STATIC);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			onError(std::string(sqlite3_errstr(status)));
			return;
		}

		status = sqlite3_bind_text(stmt, 9, convertHeadersToString(request.headers()).c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			onError(std::string(sqlite3_errstr(status)));
			return;
		}

		status = sqlite3_bind_text(stmt, 10, request.method().c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			onError(std::string(sqlite3_errstr(status)));
			return;
		}

		status = sqlite3_bind_text(stmt, 11, request.url().c_str(), -1, SQLITE_STATIC);
		if (status != SQLITE_OK) {
			sqlite3_finalize(stmt);
			onError(std::string(sqlite3_errstr(status)));
			return;
		}

		status = sqlite3_step(stmt);
		if (status != SQLITE_DONE) {
			sqlite3_finalize(stmt);
			onError(std::string(sqlite3_errstr(status)));
			return;
		}

		status = sqlite3_finalize(stmt);
		if (status != SQLITE_OK) {
			onError(std::string(sqlite3_errstr(status)));
		}
	}

	void DiskCache::removeEntry(const std::string&)
	{
	}

	void DiskCache::prune()
	{
	}

	void DiskCache::clearAll()
	{
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
}