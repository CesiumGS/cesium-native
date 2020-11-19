#include "Cesium3DTiles/registerAllTileContentTypes.h"
#include "Cesium3DTiles/TileContentFactory.h"
#include "Cesium3DTiles/GltfContent.h"
#include "Cesium3DTiles/ExternalTilesetContent.h"
#include "Batched3DModelContent.h"
#include "QuantizedMeshContent.h"
#include "Logging.h"

namespace Cesium3DTiles {

    void registerAllTileContentTypes() {

        // TODO This should not be done here, but in a more generic "initialize3DTiles" function
        impl::initializeLogging();

        TileContentFactory::registerMagic("glTF", GltfContent::load);
        TileContentFactory::registerMagic("b3dm", Batched3DModelContent::load);
        TileContentFactory::registerMagic("json", ExternalTilesetContent::load);

        TileContentFactory::registerContentType(QuantizedMeshContent::CONTENT_TYPE, QuantizedMeshContent::load);
    }

}
