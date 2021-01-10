#pragma once

#include "CesiumAsync/ResponseCacheControl.h"
#include "gsl/span"
#include <vector>
#include <map>
#include <ctime>

namespace CesiumAsync {
	class CacheResponse {
	public:
		CacheResponse(uint16_t statusCode, 
			std::string contentType, 
			std::map<std::string, std::string> headers, 
			std::optional<ResponseCacheControl> cacheControl,
			std::vector<uint8_t> data)
			: _statusCode{statusCode}
			, _contentType{std::move(contentType)}
			, _headers{std::move(headers)}
			, _cacheControl{std::move(cacheControl)}
			, _data{std::move(data)}
		{}

		inline uint16_t statusCode() const { return _statusCode; }

		inline const std::string& contentType() const { return _contentType; }

		inline const std::map<std::string, std::string>& headers() const { return _headers; }

		inline const ResponseCacheControl* cacheControl() const { return _cacheControl ? &*_cacheControl : nullptr; }

		inline gsl::span<const uint8_t> data() const { return gsl::span<const uint8_t>(_data.data(), _data.size()); }

	private:
		uint16_t _statusCode;
		std::string _contentType;
		std::map<std::string, std::string> _headers;
		std::optional<ResponseCacheControl> _cacheControl;
		std::vector<uint8_t> _data;
	};

	class CacheRequest {
	public:
		CacheRequest(std::map<std::string, std::string> headers,
			std::string method,
			std::string url)
			: _headers{std::move(headers)}
			, _method{std::move(method)}
			, _url{std::move(url)}
		{}

		inline const std::map<std::string, std::string>& headers() const { return _headers; }

		inline const std::string& method() const { return _method; }

		inline const std::string& url() const { return _url; }

	private:
		std::map<std::string, std::string> _headers;
		std::string _method;
		std::string _url;
	};

	class CacheItem {
	public:
		CacheItem(std::time_t expiryTime, 
			std::time_t lastAccessedTime, 
			CacheRequest cacheRequest,
			CacheResponse cacheResponse) 
			: _expiryTime{expiryTime}
			, _lastAccessedTime{lastAccessedTime}
			, _cacheRequest{std::move(cacheRequest)}
			, _cacheResponse{std::move(cacheResponse)}
		{}

		inline std::time_t expiryTime() const { return this->_expiryTime; }

		inline std::time_t lastAccessedTime() const { return this->_lastAccessedTime; }

		inline const CacheRequest& cacheRequest() const { return *this->_cacheRequest; }

		inline const CacheResponse& cacheResponse() const { return *this->_cacheResponse; }

	private:
		std::time_t _expiryTime;
		std::time_t _lastAccessedTime;
		std::optional<CacheRequest> _cacheRequest;
		std::optional<CacheResponse> _cacheResponse;
	};
}