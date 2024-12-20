#include <Cesium3DTilesContent/Library.h>

namespace Cesium3DTilesContent {

/**
 * @brief Register all \ref Cesium3DTilesSelection::Tile "Tile" content types
 * that can be loaded.
 *
 * This is supposed to be called during the initialization, before
 * any \ref Cesium3DTilesSelection::Tileset "Tileset" is loaded. It will
 * register loaders for the different types of tiles that can be encountered.
 */
CESIUM3DTILESCONTENT_API void registerAllTileContentTypes();

} // namespace Cesium3DTilesContent
