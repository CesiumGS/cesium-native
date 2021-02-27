// Copyright CesiumGS, Inc. and Contributors

#include "Cesium3DTiles/spdlog-cesium.h"
#include "Cesium3DTiles/TileContentFactory.h"
#include <algorithm>

namespace Cesium3DTiles {

    void TileContentFactory::registerMagic(const std::string& magic, const std::shared_ptr<TileContentLoader>& pLoader) {

        SPDLOG_INFO("Registering magic header {}", magic);

        TileContentFactory::_loadersByMagic[magic] = pLoader;
    }

    void TileContentFactory::registerContentType(const std::string& contentType, const std::shared_ptr<TileContentLoader>& pLoader) {

        SPDLOG_INFO("Registering content type {}", contentType);

        std::string lowercaseContentType;
        std::transform(contentType.begin(), contentType.end(), std::back_inserter(lowercaseContentType), [](char c) { return static_cast<char>(::tolower(c)); });
        TileContentFactory::_loadersByContentType[lowercaseContentType] = pLoader;
    }

    std::unique_ptr<TileContentLoadResult> TileContentFactory::createContent(
        const TileContentLoadInput& input) {

        const gsl::span<const uint8_t>& data = input.data;
        std::string magic = TileContentFactory::getMagic(data).value_or("json");

        auto itMagic = TileContentFactory::_loadersByMagic.find(magic);
        if (itMagic != TileContentFactory::_loadersByMagic.end()) {
            return itMagic->second->load(input);
        }

        const std::string& contentType = input.contentType;
        std::string baseContentType = contentType.substr(0, contentType.find(';'));

        auto itContentType = TileContentFactory::_loadersByContentType.find(baseContentType);
        if (itContentType != TileContentFactory::_loadersByContentType.end()) {
            return itContentType->second->load(input);
        }

        // Determine if this is plausibly a JSON external tileset.
        size_t i;
        for (i = 0; i < data.size(); ++i) {
            if (!std::isspace(data[i])) {
                break;
            }
        }

        if (i < data.size() && data[i] == '{') {
            // Might be an external tileset, try loading it that way.
            itMagic = TileContentFactory::_loadersByMagic.find("json");
            if (itMagic != TileContentFactory::_loadersByMagic.end()) {
                return itMagic->second->load(input);
            }
        }

        // No content type registered for this magic or content type
        SPDLOG_LOGGER_WARN(input.pLogger, "No loader registered for tile with content type '{}' and magic value '{}'.", baseContentType, magic);
        return nullptr;
    }

    /**
     * @brief Returns a string consisting of the first four ("magic") bytes of the given data
     * 
     * @param data The raw data.
     * @return The string, or an empty optional if the given data contains less than 4 bytes
     */
    std::optional<std::string> TileContentFactory::getMagic(const gsl::span<const uint8_t>& data) {
        if (data.size() >= 4) {
            gsl::span<const uint8_t> magicData = data.subspan(0, 4);
            return std::string(magicData.begin(), magicData.end());
        }

        return std::nullopt;
    }

    std::unordered_map<std::string, std::shared_ptr<TileContentLoader>> TileContentFactory::_loadersByMagic;
    std::unordered_map<std::string, std::shared_ptr<TileContentLoader>> TileContentFactory::_loadersByContentType;

}
