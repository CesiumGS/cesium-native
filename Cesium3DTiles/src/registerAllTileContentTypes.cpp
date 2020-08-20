#include "Cesium3DTiles/registerAllTileContentTypes.h"
#include "Cesium3DTiles/TileContentFactory.h"
#include "Cesium3DTiles/GltfContent.h"
#include "Cesium3DTiles/ExternalTilesetContent.h"
#include "Batched3DModel.h"
#include "QuantizedMesh.h"

namespace Cesium3DTiles {

    void registerAllTileContentTypes() {
        TileContentFactory::registerMagic(GltfContent::TYPE, [](const Tile& tile, const gsl::span<const uint8_t>& data, const std::string& url) {
            return std::make_unique<GltfContent>(tile, data, url);
        });

        TileContentFactory::registerMagic(ExternalTilesetContent::TYPE, [](const Tile& tile, const gsl::span<const uint8_t>& data, const std::string& url) {
            return std::make_unique<ExternalTilesetContent>(tile, data, url);
        });

        TileContentFactory::registerMagic(Batched3DModel::MAGIC, Batched3DModel::load);
        TileContentFactory::registerContentType(QuantizedMesh::CONTENT_TYPE, QuantizedMesh::load);
    }

}
