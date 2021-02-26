#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace CesiumIonClient {

    struct Token {
        std::string jti;
        std::string name;
        std::string token;
        bool isDefault;
        std::string lastUsed;
        std::vector<std::string> scopes;
        std::vector<int64_t> assets;
    };
}
