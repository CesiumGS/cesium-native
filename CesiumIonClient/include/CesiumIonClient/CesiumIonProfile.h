#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace CesiumIonClient {

    /**
     * @brief Information about the amount of storage available in a user's account.
     */
    struct CesiumIonProfileStorage final {
        /**
         * @brief The number of bytes currently being used by this account.
         */
        int64_t used;

        /**
         * @brief The number of bytes available for additional asset uploads to this account.
         */
        int64_t available;

        /**
         * @brief The total number of bytes currently allowed to be used by this account.
         */
        int64_t total;
    };

    /**
     * @brief Contains of a Cesium ion user.
     * 
     * @see CesiumIonConnection#me
     */
    struct CesiumIonProfile final {
        /**
         * @brief The unique identifier for this account.
         */
        int64_t id;

        /**
         * @brief The array of scopes available with this token.
         */
        std::vector<std::string> scopes;

        /**
         * @brief The account username.
         */
        std::string username;

        /**
         * @brief The primary email address associated with this account.
         */
        std::string email;

        /**
         * @brief If true, the email address has been verified for this account.
         */
        bool emailVerified;

        /**
         * @brief A url to the profile image associated with this account.
         */
        std::string avatar;

        /**
         * @brief Information about the amount of storage available in the user's account.
         */
        CesiumIonProfileStorage storage;
    };

}
