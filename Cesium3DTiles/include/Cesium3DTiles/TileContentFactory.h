#pragma once

#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/TileRefine.h"
#include <functional>
#include <gsl/span>
#include <memory>
#include <optional>

namespace Cesium3DTiles {
    class TileContent;
    class Tileset;

    class CESIUM3DTILES_API TileContentFactory {
    public:
        TileContentFactory() = delete;

        typedef std::unique_ptr<TileContent> FactoryFunctionSignature(
            Tileset& tileset,
            const TileID& tileID,
            const BoundingVolume& tileBoundingVolume,
            double tileGeometricError,
            const glm::dmat4& tileTransform,
            const std::optional<BoundingVolume>& tileContentBoundingVolume,
            TileRefine tileRefine,
            const std::string& url,
            const gsl::span<const uint8_t>& data
        );
        typedef std::function<FactoryFunctionSignature> FactoryFunction;

        static void registerMagic(const std::string& magic, FactoryFunction factoryFunction);
        static void registerContentType(const std::string& contentType, FactoryFunction factoryFunction);
        static std::unique_ptr<TileContent> createContent(
            Tileset& tileset,
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
