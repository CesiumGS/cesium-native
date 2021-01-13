#pragma once

#include "CesiumAsync/Library.h"
#include "CesiumAsync/IAssetRequest.h"
#include <memory>
#include <string>
#include <vector>

namespace CesiumAsync {
    class AsyncSystem;

    /**
     * @brief Provides asynchronous access to assets, usually files downloaded via HTTP.
     */
    class CESIUMASYNC_API IAssetAccessor {
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
        virtual void requestAsset(
            const AsyncSystem* asyncSystem,
            const std::string& url,
            const std::vector<THeader>& headers,
            std::function<void(std::unique_ptr<IAssetRequest>)> callback
        ) = 0;

        /**
         * @brief Ticks the asset accessor system while the main thread is blocked.
         * 
         * If the asset accessor is not dependent on the main thread to
         * dispatch requests, this method does not need to do anything.
         */
        virtual void tick() noexcept = 0;
    };

}
