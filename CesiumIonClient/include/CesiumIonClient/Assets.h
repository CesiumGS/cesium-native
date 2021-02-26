#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace CesiumIonClient {
    struct Asset {
        int64_t id;
        std::string name;
        std::string description;
        std::string attribution;
        std::string type;
        int64_t bytes;
        std::string dateAdded;
        std::string status;
        int8_t percentComplete;
    };

    struct Assets {
        std::string link;
        std::vector<Asset> items;
    };
}
