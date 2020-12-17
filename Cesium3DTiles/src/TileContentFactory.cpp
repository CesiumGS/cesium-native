#include "Cesium3DTiles/TileContentFactory.h"
#include <algorithm>
#include "spdlog.h"

namespace Cesium3DTiles {

    void TileContentFactory::registerMagic(const std::string& magic, TileContentFactory::FactoryFunction factoryFunction) {

        SPDLOG_INFO("Registering magic header {}", magic);

        TileContentFactory::_factoryFunctionsByMagic[magic] = factoryFunction;
    }

    void TileContentFactory::registerContentType(const std::string& contentType, TileContentFactory::FactoryFunction factoryFunction) {

        SPDLOG_INFO("Registering content type {}", contentType);

        std::string lowercaseContentType;
        std::transform(contentType.begin(), contentType.end(), std::back_inserter(lowercaseContentType), [](char c) { return static_cast<char>(::tolower(c)); });
        TileContentFactory::_factoryFunctionsByContentType[lowercaseContentType] = factoryFunction;
    }

    std::unique_ptr<TileContentLoadResult> TileContentFactory::createContent(
        std::shared_ptr<spdlog::logger> pLogger,
        const TileContext& context,
        const TileID& tileID,
        const BoundingVolume& tileBoundingVolume,
        double tileGeometricError,
        const glm::dmat4& tileTransform,
        const std::optional<BoundingVolume>& tileContentBoundingVolume,
        TileRefine tileRefine,
        const std::string& url,
        const std::string& contentType,
        const gsl::span<const uint8_t>& data
    ) {
        std::string magic = TileContentFactory::getMagic(data).value_or("json");

        auto itMagic = TileContentFactory::_factoryFunctionsByMagic.find(magic);
        if (itMagic != TileContentFactory::_factoryFunctionsByMagic.end()) {
            return itMagic->second(pLogger, context, tileID, tileBoundingVolume, tileGeometricError, tileTransform, tileContentBoundingVolume, tileRefine, url, data);
        }

        std::string baseContentType = contentType.substr(0, contentType.find(';'));

        auto itContentType = TileContentFactory::_factoryFunctionsByContentType.find(baseContentType);
        if (itContentType != TileContentFactory::_factoryFunctionsByContentType.end()) {
            return itContentType->second(pLogger, context, tileID, tileBoundingVolume, tileGeometricError, tileTransform, tileContentBoundingVolume, tileRefine, url, data);
        }

        itMagic = TileContentFactory::_factoryFunctionsByMagic.find("json");
        if (itMagic != TileContentFactory::_factoryFunctionsByMagic.end()) {
            return itMagic->second(pLogger, context, tileID, tileBoundingVolume, tileGeometricError, tileTransform, tileContentBoundingVolume, tileRefine, url, data);
        }

        // No content type registered for this magic or content type
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

        return std::optional<std::string>();
    }

    std::unordered_map<std::string, TileContentFactory::FactoryFunction> TileContentFactory::_factoryFunctionsByMagic;
    std::unordered_map<std::string, TileContentFactory::FactoryFunction> TileContentFactory::_factoryFunctionsByContentType;

}
