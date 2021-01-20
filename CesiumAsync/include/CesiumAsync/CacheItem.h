#pragma once

#include "CesiumAsync/HttpHeaders.h"
#include "CesiumAsync/ResponseCacheControl.h"
#include "gsl/span"
#include <vector>
#include <map>
#include <ctime>

namespace CesiumAsync {
	class CacheResponse {
	public:
		CacheResponse(uint16_t cacheStatusCode, 
			std::string cacheContentType, 
			HttpHeaders cacheHeaders, 
			std::optional<ResponseCacheControl> responseCacheControl,
			std::vector<uint8_t> cacheData)
			: statusCode{cacheStatusCode}
			, contentType{std::move(cacheContentType)}
			, headers{std::move(cacheHeaders)}
			, cacheControl{std::move(responseCacheControl)}
			, data{std::move(cacheData)}
		{}

		uint16_t statusCode;
		std::string contentType;
		HttpHeaders headers;
		std::optional<ResponseCacheControl> cacheControl;
		std::vector<uint8_t> data;
	};

	class CacheRequest {
	public:
		CacheRequest(HttpHeaders cacheHeaders,
			std::string cacheMethod,
			std::string cacheUrl)
			: headers{std::move(cacheHeaders)}
			, method{std::move(cacheMethod)}
			, url{std::move(cacheUrl)}
		{}

		HttpHeaders headers;
		std::string method;
		std::string url;
	};

	class CacheItem {
	public:
		CacheItem(std::time_t cacheExpiryTime, 
			std::time_t cacheLastAccessedTime, 
			CacheRequest request,
			CacheResponse response) 
			: expiryTime{cacheExpiryTime}
			, lastAccessedTime{cacheLastAccessedTime}
			, cacheRequest{std::move(request)}
			, cacheResponse{std::move(response)}
		{}

		std::time_t expiryTime;
		std::time_t lastAccessedTime;
		CacheRequest cacheRequest;
		CacheResponse cacheResponse;
	};
}