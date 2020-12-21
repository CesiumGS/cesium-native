#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/TileRefine.h"
#include "Cesium3DTiles/TileContentLoadResult.h"
#include <functional>
#include <gsl/span>
#include <memory>
#include <optional>
#include <spdlog/fwd.h>
#include <unordered_map>

namespace Cesium3DTiles {
    class TileContent;
    class Tileset;

    /**
     * @brief Creates {@link TileContentLoadResult} from raw data.
     * 
     * The class offers a lookup functionality for functions that can create
     * {@link TileContentLoadResult} instances from raw data. It allows 
     * registering different {@link FactoryFunction} instances that are
     * used for creating the tile content.
     * 
     * The functions are registered based on the magic header or the content type
     * of the input data. The raw data is usually received as a response to a 
     * network request, and the first four bytes of the raw data form the magic 
     * header. Based on this header or the content type of the network response, 
     * the function that will be used for processing that raw data can be looked up.
     */
    class CESIUM3DTILES_API TileContentFactory final {
    public:
        TileContentFactory() = delete;

        /**
         * @brief The signature of a function that can create a {@link TileContentLoadResult}
         */
        typedef std::unique_ptr<TileContentLoadResult> FactoryFunctionSignature(
            std::shared_ptr<spdlog::logger> pLogger,
            const TileContext& context,
            const TileID& tileID,
            const BoundingVolume& tileBoundingVolume,
            double tileGeometricError,
            const glm::dmat4& tileTransform,
            const std::optional<BoundingVolume>& tileContentBoundingVolume,
            TileRefine tileRefine,
            const std::string& url,
            const gsl::span<const uint8_t>& data
        );

        /**
         * @brief A function that can create a {@link TileContentLoadResult}
         */
        typedef std::function<FactoryFunctionSignature> FactoryFunction;

        /**
         * @brief Register the given function for the given magic header.
         * 
         * The given magic header is a 4-character string. It will be compared
         * to the first 4 bytes of the raw input data, to decide whether the
         * given factory function should be used to create the 
         * {@link TileContentLoadResult} from the input data.
         * 
         * @param magic The string describing the magic header.
         * @param factoryFunction The function that will be used to create the tile content.
         */
        static void registerMagic(const std::string& magic, FactoryFunction factoryFunction);

        /**
         * @brief Register the given function for the given content type.
         *
         * The given string describes the content type of a network response.
         * It is used for deciding whether the given factory function should be 
         * used to create the {@link TileContentLoadResult} from the raw response data.
         *
         * @param contentType The string describing the content type.
         * @param factoryFunction The function that will be used to create the tile content
         */
        static void registerContentType(const std::string& contentType, FactoryFunction factoryFunction);

        /**
         * @brief Creates the {@link TileContentLoadResult} from the given parameters.
         * 
         * This will look up the {@link FactoryFunction} that can be used to
         * process the given input data, based on all factory functions that
         * have been registered with {@link TileContentFactory::registerMagic}
         * or {@link TileContentFactory::registerContentType}.
         * 
         * It will first try to find a factory function based on the magic header
         * of the given data. If no matching function is found, then it will look
         * up a function based on the content type. If no such function is found,
         * then `nullptr` is returned.
         * 
         * If a matching function is found, it will be applied to the given
         * input, and the result will be returned.
         * 
         * @param context The {@link TileContext}.
         * @param tileID The {@link TileID}
         * @param tileBoundingVolume The tile {@link BoundingVolume}
         * @param tileGeometricError The geometric error
         * @param tileTransform The tile transform
         * @param tileContentBoundingVolume Tile content {@link BoundingVolume}
         * @param tileRefine The {@link TileRefine}
         * @param url The source URL
         * @param contentType The content type
         * @param data The raw input data
         * @return The tile content load result, or `nullptr` if there is
         * no factory function registered for the magic header of the given
         * data, and no factory function for the given content type.
         */
        static std::unique_ptr<TileContentLoadResult> createContent(
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
        );

    private:
        static std::optional<std::string> getMagic(const gsl::span<const uint8_t>& data);

        static std::unordered_map<std::string, FactoryFunction> _factoryFunctionsByMagic;
        static std::unordered_map<std::string, FactoryFunction> _factoryFunctionsByContentType;
    };

}
