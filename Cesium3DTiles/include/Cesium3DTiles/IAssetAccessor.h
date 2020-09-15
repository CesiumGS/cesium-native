#pragma once

#include <string>
#include "Cesium3DTiles/Library.h"
#include "IAssetRequest.h"
#include <memory>

namespace Cesium3DTiles {

    /**
     * @brief Provides asynchronous access to assets, usually files downloaded via HTTP.
     */
    class CESIUM3DTILES_API IAssetAccessor {
    public:
        /**
         * @brief An HTTP header represented as a key/value pair.
         */
        typedef std::pair<std::string, std::string> THeader;

        virtual ~IAssetAccessor() = default;

        /**
         * @brief Starts a new request for the asset with the given URL.
         * The request proceeds asynchronously without blocking the calling thread.
         * 
         * @param url The URL of the asset.
         * @param headers The headers to include in the request.
         * @return The in-progress asset request.
         */
        virtual std::unique_ptr<IAssetRequest> requestAsset(
            const std::string& url,
            const std::vector<THeader>& headers = std::vector<THeader>()
        ) = 0;

        /**
         * Ticks the asset accessor system while the main thread is blocked.
         * If the asset accessor is not dependent on the main thread to
         * dispatch requests, this method does not need to do anything.
         */
        virtual void tick() = 0;
    };

}
