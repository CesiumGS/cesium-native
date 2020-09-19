#include "Cesium3DTiles/registerAllTileContentTypes.h"
#include "Cesium3DTiles/TileContentFactory.h"
#include "Cesium3DTiles/GltfContent.h"
#include "Cesium3DTiles/ExternalTilesetContent.h"
#include "Batched3DModel.h"
#include "QuantizedMeshContent.h"

namespace Cesium3DTiles {

    void registerAllTileContentTypes() {
        TileContentFactory::registerMagic(GltfContent::TYPE, [](
            const TileContext& /*tileContext*/,
            const CompleteTileDefinition& /*tile*/,
            const std::string& url,
            const gsl::span<const uint8_t>& data
        ) {
            return std::make_unique<GltfContent>(data, url);
        });

        TileContentFactory::registerMagic(ExternalTilesetContent::TYPE, [](
            const TileContext& tileContext,
            const CompleteTileDefinition& tile,
            const std::string& url,
            const gsl::span<const uint8_t>& data
        ) {
            return std::make_unique<ExternalTilesetContent>(tileContext, tile, data, url);
        });

        TileContentFactory::registerMagic(Batched3DModel::MAGIC, Batched3DModel::load);

        TileContentFactory::registerContentType(QuantizedMeshContent::CONTENT_TYPE, [](
            const TileContext& tileContext,
            const CompleteTileDefinition& tile,
            const std::string& url,
            const gsl::span<const uint8_t>& data
        ) {
            return std::make_unique<QuantizedMeshContent>(tileContext, tile, data, url);
        });
    }

}
