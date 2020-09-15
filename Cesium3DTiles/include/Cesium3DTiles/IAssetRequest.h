#pragma once

#include <functional>
#include "Cesium3DTiles/Library.h"

namespace Cesium3DTiles {

    class IAssetResponse;

    /**
     * @brief An asynchronous request for an asset, usually a file
     * downloaded via HTTP.
     */
    class CESIUM3DTILES_API IAssetRequest {
    public:
        virtual ~IAssetRequest() = default;

        /**
         * @brief Gets the response, or nullptr if the request is still in progress.
         * This method may be called from any thread.
         */
        virtual IAssetResponse* response() = 0;

        /**
         * @brief Binds a callback function that will be invoked when the request's response is
         * received. This method may only be called from the thread that created the request.
         * 
         * @param callback The callback.
         */
        virtual void bind(std::function<void(IAssetRequest*)> callback) = 0;

        /**
         * @brief Gets the requested URL. This method may be called from any thread.
         */
        virtual std::string url() const = 0;

        /**
         * @brief Cancels the request.
         * This method may only be called from the thread that created the request.
         */
        virtual void cancel() = 0;
    };

}
