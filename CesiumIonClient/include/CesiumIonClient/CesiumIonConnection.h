#pragma once

#include "CesiumAsync/AsyncSystem.h"
#include <optional>

namespace CesiumIonClient {
    struct CesiumIonAssets;
    struct CesiumIonProfile;

    class CesiumIonConnection final {
    public:
        /**
         * @brief A response from Cesium ion.
         * 
         * @tparam T The type of the response object.
         */
        template <typename T>
        struct Response final {
            /**
             * @brief The response value, or `std::nullopt` if the response was unsuccessful.
             */
            std::optional<T> value;

            /**
             * @brief The HTTP status code returned by Cesium ion.
             */
            uint16_t httpStatusCode;

            /**
             * @brief The error code, or empty string if there was no error.
             * 
             * If no response is received at all, the code will be `"NoResponse"`.
             * 
             * If Cesium ion returns an error, this will be the `code` reported by Cesium ion.
             * 
             * If Cesium ion reports success but an error occurs while attempting to parse the response, the code will be
             * `"ParseError"`.
             */
            std::string errorCode;

            /**
             * @brief The error message returned, or an empty string if there was no error.
             * 
             * If Cesium ion returns an error, this will be the `message` reported by Cesium ion. If Cesium ion
             * reports success but another error occurs, the message will contain further details of the error.
             */
            std::string errorMessage;
        };

        /**
         * @brief Connect to Cesium ion using the provided username and password.
         * 
         * @param username The username.
         * @param password The password.
         * @param apiUrl The base URL of the Cesium ion API.
         * @return A future that, when it resolves, provides a connection to Cesium ion under the given credentials.
         */
        static CesiumAsync::Future<Response<CesiumIonConnection>> connect(
            const CesiumAsync::AsyncSystem& asyncSystem,
            const std::string& username,
            const std::string& password,
            const std::string& apiUrl = "https://api.cesium.com"
        );

        /**
         * @brief Creates a connection to Cesium ion using the provided access token.
         * 
         * @param accessToken The access token
         * @param apiUrl The base URL of the Cesium ion API.
         */
        CesiumIonConnection(
            const CesiumAsync::AsyncSystem& asyncSystem,
            const std::string& accessToken,
            const std::string& apiUrl = "https://api.cesium.com"
        );

        CesiumAsync::AsyncSystem& getAsyncSystem() { return this->_asyncSystem; }
        const CesiumAsync::AsyncSystem& getAsyncSystem() const { return this->_asyncSystem; }

        const std::string& getAccessToken() const { return this->_accessToken; }

        /**
         * @brief Retrieves profile information for the access token currently being used to make API calls.
         * 
         * This route works with any valid token, but additional information is returned if the token uses the `profile:read` scope.
         * 
         * @return A future that resolves to the profile information.
         */
        CesiumAsync::Future<Response<CesiumIonProfile>> me() const;

        CesiumAsync::Future<Response<CesiumIonAssets>> assets() const;

    private:
        CesiumAsync::AsyncSystem _asyncSystem;
        std::string _accessToken;
        std::string _apiUrl;
    };

}
