#include "Cesium3DTiles/Library.h"

namespace Cesium3DTiles {

    /**
     * @brief Register all {@link Tile} content types that can be loaded.
     * 
     * This is supposed to be called during the initialization, before
     * any {@link Tileset} is loaded. It will register loaders for the
     * different types of tiles that can be encountered.
     */
    CESIUM3DTILES_API void registerAllTileContentTypes();

}
