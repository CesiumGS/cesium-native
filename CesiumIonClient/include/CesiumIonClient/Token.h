#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace CesiumIonClient {
    /**
     * @brief A Cesium ion access token.
     */
    struct Token {
        /**
         * @brief The identifier of the token.
         */
        std::string jti;

        /**
         * @brief The name of the token.
         */
        std::string name;

        /**
         * @brief The token value.
         */
        std::string token;

        /**
         * @brief True if this is the default token.
         */
        bool isDefault;

        /**
         * @brief The date when this token was last used.
         */
        std::string lastUsed;

        /**
         * @brief The scopes granted by this token.
         */
        std::vector<std::string> scopes;

        /**
         * @brief The assets that this token my access.
         * 
         * If `std::nullopt`, the token allows access to all assets.
         */
        std::optional<std::vector<int64_t>> assets;
    };
}
