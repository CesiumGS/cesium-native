#pragma once

/**
 * @brief Classes for working with [glTF](https://www.khronos.org/gltf/) models.
 *
 * @mermaid-interactive{dependencies/CesiumGltf}
 */
namespace CesiumGltf {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMGLTF_BUILDING
#define CESIUMGLTF_API __declspec(dllexport)
#else
#define CESIUMGLTF_API __declspec(dllimport)
#endif
#else
#define CESIUMGLTF_API
#endif
