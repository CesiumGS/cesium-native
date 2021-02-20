#pragma once

#include "CesiumAsync/Library.h"
#include "CesiumAsync/HttpHeaders.h"
#include "CesiumAsync/ResponseCacheControl.h"
#include "gsl/span"
#include <vector>
#include <map>
#include <ctime>

namespace CesiumAsync {

    /**
     * @brief Cache response retrieved from the cache database.
     */
    class CESIUMASYNC_API CacheResponse {
    public:
        /**
         * @brief Constructor. 
         * @param cacheStatusCode the status code of the response
         * @param cacheContentType the content type of the response
         * @param cacheHeaders the headers of the response
         * @param responseCacheControl the cache-control of the response
         * @param cacheData the body of the response
         */
        CacheResponse(
            uint16_t cacheStatusCode, 
            HttpHeaders&& cacheHeaders, 
            // std::optional<ResponseCacheControl> responseCacheControl,
            std::vector<uint8_t>&& cacheData)
            : statusCode{cacheStatusCode}
            // , contentType{std::move(cacheContentType)}
            , headers{std::move(cacheHeaders)}
            // , cacheControl{std::move(responseCacheControl)}
            , data{std::move(cacheData)}
        {}

        /**
         * @brief The status code of the response.
         */
        uint16_t statusCode;

        /**
         * @brief The content type of the response.
         */
        // std::string contentType;

        /**
         * @brief The headers of the response.
         */
        HttpHeaders headers;

        /**
         * @brief The Cache-Control of the response.
         */
        // std::optional<ResponseCacheControl> cacheControl;

        /**
         * @brief The body data of the response.
         */
        std::vector<uint8_t> data;
    };

    /**
     * @brief Cache request retrieved from the cache database.
     */
    class CESIUMASYNC_API CacheRequest {
    public:
        /**
         * @brief Constructor. 
         * @param cacheHeaders the headers of the request
         * @param method the method of the request
         * @param url the url of the request
         */
        CacheRequest(HttpHeaders cacheHeaders,
            std::string cacheMethod,
            std::string cacheUrl)
            : headers{std::move(cacheHeaders)}
            , method{std::move(cacheMethod)}
            , url{std::move(cacheUrl)}
        {}

        /**
         * @brief The headers of the request.
         */
        HttpHeaders headers;

        /**
         * @brief The method of the request.
         */
        std::string method;

        /**
         * @brief The url of the request.
         */
        std::string url;
    };

    /**
     * @brief Cache item retrieved from the cache database.
     * Cache item consists of {@link CacheRequest} and {@link CacheResponse} and other metadata.
     */
    class CESIUMASYNC_API CacheItem {
    public:
        /**
         * @brief Constructor. 
         * @param cacheExpiryTime the time point this cache item will be expired
         * @param cacheLastAccessedTime the latest time point this cache item was accessed 
         * @param request the cache request owned by this item
         * @param response the cache response owned by this item
         */
        CacheItem(
            std::time_t cacheExpiryTime, 
            // std::time_t cacheLastAccessedTime, 
            CacheRequest request,
            CacheResponse response) :
            expiryTime{cacheExpiryTime},
            // , lastAccessedTime{cacheLastAccessedTime}
            cacheRequest{std::move(request)},
            cacheResponse{std::move(response)}
        {}

        /**
         * @brief The time point that this cache item is expired.
         */
        std::time_t expiryTime;

        /**
         * @brief The latest time point that this cache item is accessed.
         */
        // std::time_t lastAccessedTime;

        /**
         * @brief The cache request owned by this cache item.
         */
        CacheRequest cacheRequest;

        /**
         * @brief The cache response owned by this cache item.
         */
        CacheResponse cacheResponse;
    };
}