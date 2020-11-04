#pragma once

#include <string>
#include <map>
#include <vector>
#include <gsl/span>
#include <gsl/string_span>
#include "Cesium3DTiles/Library.h"

namespace Cesium3DTiles {

    /**
     * @brief A completed response for a 3D Tiles asset.
     */
    class CESIUM3DTILES_API IAssetResponse {
    public:

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
        //virtual const std::map<std::string, std::string>& headers() = 0;

        /**
         * @brief Returns the data of this response
         */
        virtual gsl::span<const uint8_t> data() const = 0;
    };

}
