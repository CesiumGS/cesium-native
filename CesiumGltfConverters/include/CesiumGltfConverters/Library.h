#pragma once

/**
 * @brief Classes that support converting various formats to glTF 2.1 content.
 *
 * @mermaid-interactive{dependencies/CesiumGltfConverters}
 */
namespace CesiumGltfConverters {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMGLTFCONVERTERS_BUILDING
#define CESIUMGLTFCONVERTERS_API __declspec(dllexport)
#else
#define CESIUMGLTFCONVERTERS_API __declspec(dllimport)
#endif
#else
#define CESIUMGLTFCONVERTERS_API
#endif
