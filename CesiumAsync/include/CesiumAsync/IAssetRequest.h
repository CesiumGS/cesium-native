#pragma once

#include "CesiumAsync/Library.h"
#include <map>
#include <functional>
#include <string>

namespace CesiumAsync {

    class IAssetResponse;

    /**
     * @brief An asynchronous request for an asset, usually a file
     * downloaded via HTTP.
     */
    class CESIUMASYNC_API IAssetRequest {
    public:
        virtual ~IAssetRequest() = default;

        /**
         * @brief Gets the request's method. This method may be called from any thread.
         */
        virtual const std::string& method() const = 0;

        /**
         * @brief Gets the requested URL. This method may be called from any thread.
         */
        virtual const std::string& url() const = 0;

        /**
         * @brief Gets the request's header. This method may be called from any thread.
         */
        virtual const std::map<std::string, std::string> &headers() const = 0;

        /**
         * @brief Gets the response, or nullptr if the request is still in progress.
         * This method may be called from any thread.
         */
        virtual const IAssetResponse* response() const = 0;
    };

}
