#include "Cesium3DTiles/registerAllTileContentTypes.h"
#include "Cesium3DTiles/TileContentFactory.h"
#include "Cesium3DTiles/GltfContent.h"
#include "Cesium3DTiles/ExternalTilesetContent.h"
#include "Batched3DModelContent.h"
#include "QuantizedMeshContent.h"
#include "CompositeContent.h"

namespace Cesium3DTiles {

    void registerAllTileContentTypes() {

        TileContentFactory::registerMagic("glTF", std::make_shared<GltfContent>());
        TileContentFactory::registerMagic("b3dm", std::make_shared<Batched3DModelContent>());
        TileContentFactory::registerMagic("cmpt", std::make_shared<CompositeContent>());
        TileContentFactory::registerMagic("json", std::make_shared<ExternalTilesetContent>());

        TileContentFactory::registerContentType(QuantizedMeshContent::CONTENT_TYPE, std::make_shared<QuantizedMeshContent>());
    }

}
