#include "Cesium3DTiles/TileContentFactory.h"
#include "Cesium3DTiles/TileContent.h"
#include <algorithm>

namespace Cesium3DTiles {

    void TileContentFactory::registerMagic(const std::string& magic, TileContentFactory::FactoryFunction factoryFunction) {
        TileContentFactory::_factoryFunctionsByMagic[magic] = factoryFunction;
    }

    void TileContentFactory::registerContentType(const std::string& contentType, TileContentFactory::FactoryFunction factoryFunction) {
        std::string lowercaseContentType;
        std::transform(contentType.begin(), contentType.end(), std::back_inserter(lowercaseContentType), [](char c) { return static_cast<char>(::tolower(c)); });
        TileContentFactory::_factoryFunctionsByContentType[lowercaseContentType] = factoryFunction;
    }

    std::unique_ptr<TileContent> TileContentFactory::createContent(const Cesium3DTiles::Tile& tile, const gsl::span<const uint8_t>& data, const std::string& url, const std::string& contentType) {
        std::string magic = TileContentFactory::getMagic(data).value_or("json");

        auto itMagic = TileContentFactory::_factoryFunctionsByMagic.find(magic);
        if (itMagic != TileContentFactory::_factoryFunctionsByMagic.end()) {
            return itMagic->second(tile, data, url);
        }

        std::string baseContentType = contentType.substr(0, contentType.find(';'));

        auto itContentType = TileContentFactory::_factoryFunctionsByContentType.find(baseContentType);
        if (itContentType != TileContentFactory::_factoryFunctionsByContentType.end()) {
            return itContentType->second(tile, data, url);
        }

        itMagic = TileContentFactory::_factoryFunctionsByMagic.find("json");
        if (itMagic != TileContentFactory::_factoryFunctionsByMagic.end()) {
            return itMagic->second(tile, data, url);
        }

        // No content type registered for this magic or content type
        return nullptr;
    }

    std::optional<std::string> TileContentFactory::getMagic(const gsl::span<const uint8_t>& data) {
        if (data.size() >= 4) {
            gsl::span<const uint8_t> magicData = data.subspan(0, 4);
            return std::string(magicData.begin(), magicData.end());
        }

        return std::optional<std::string>();
    }

    std::unordered_map<std::string, TileContentFactory::FactoryFunction> TileContentFactory::_factoryFunctionsByMagic;
    std::unordered_map<std::string, TileContentFactory::FactoryFunction> TileContentFactory::_factoryFunctionsByContentType;

}
