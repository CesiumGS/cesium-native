#include <CesiumGltfConverters/Library.h>

namespace CesiumGltfConverters {

/**
 * @brief Register all \ref Cesium3DTilesSelection::Tile "Tile" content types
 * that can be loaded.
 *
 * This is supposed to be called during the initialization, before
 * any \ref Cesium3DTilesSelection::Tileset "Tileset" is loaded. It will
 * register loaders for the different types of tiles that can be encountered.
 */
CESIUMGLTFCONVERTERS_API void registerAllConverterTypes();

} // namespace CesiumGltfConverters
