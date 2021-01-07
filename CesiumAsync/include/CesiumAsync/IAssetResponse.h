#pragma once

#include "CesiumAsync/Library.h"
#include <gsl/span>
#include <string>
#include <map>

namespace CesiumAsync {

    /**
     * @brief A completed response for a 3D Tiles asset.
     */
    class CESIUMASYNC_API IAssetResponse {
    public:
        struct CacheControl {
            bool mustRevalidate;
            bool noCache;
            bool noStore;
            bool noTransform;
            bool accessControlPublic;
            bool accessControlPrivate;
            bool proxyRevalidate;
            int maxAge;
            int sharedMaxAge;
        };

        /**
         * @brief Default destructor
         */
        virtual ~IAssetResponse() = default;

        /**
         * @brief Returns the HTTP response code.
         */
        virtual uint16_t statusCode() const = 0;

        /**
         * @brief Returns the HTTP content type
         */
        virtual std::string contentType() const = 0;

        /**
         * @brief Returns the HTTP headers of the response
         */
        virtual const std::map<std::string, std::string> &headers() const = 0;

        /**
         * @brief Returns the HTTP cache control of the response
         */
        virtual const CacheControl &cacheControl() const = 0;

        /**
         * @brief Returns the data of this response
         */
        virtual gsl::span<const uint8_t> data() const = 0;
    };

}
