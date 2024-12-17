#pragma once

/**
 * @brief Classes that support manipulating the content of a glTF.
 *
 * @mermaid-interactive{dependencies/CesiumGltfContent}
 */
namespace CesiumGltfContent {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMGLTFCONTENT_BUILDING
#define CESIUMGLTFCONTENT_API __declspec(dllexport)
#else
#define CESIUMGLTFCONTENT_API __declspec(dllimport)
#endif
#else
#define CESIUMGLTFCONTENT_API
#endif
