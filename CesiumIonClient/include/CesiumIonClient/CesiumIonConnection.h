#pragma once

#include "CesiumAsync/AsyncSystem.h"
#include <optional>

namespace CesiumIonClient {

    class CesiumIonConnection {
    public:
        /**
         * @brief Connect to Cesium ion using the provided username and password.
         * 
         * @param username The username.
         * @param password The password.
         * @param apiUrl The base URL of the Cesium ion API.
         * @return A future that, when it resolves, provides a connection to Cesium ion under the given credentials.
         */
        static CesiumAsync::Future<std::optional<CesiumIonConnection>> connect(
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

    private:
        CesiumAsync::AsyncSystem _asyncSystem;
        std::string _accessToken;
        std::string _apiUrl;
    };

}
