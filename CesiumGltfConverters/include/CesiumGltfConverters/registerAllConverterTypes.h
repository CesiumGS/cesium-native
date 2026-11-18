#include <CesiumGltfConverters/Library.h>

namespace CesiumGltfConverters {

/**
 * @brief Register all content types that can be converted to glTF.
 *
 * Clients should call this before using @ref GltfConverters. It will register
 * loaders for the different types of tiles that can be encountered.
 */
CESIUMGLTFCONVERTERS_API void registerAllConverterTypes();

} // namespace CesiumGltfConverters
