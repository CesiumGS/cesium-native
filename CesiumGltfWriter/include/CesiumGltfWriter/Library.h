#pragma once

/**
 * @brief Classes for writing [glTF](https://www.khronos.org/gltf/) models.
 *
 * @mermaid-interactive{dependencies/CesiumGltfWriter}
 */
namespace CesiumGltfWriter {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMGLTFWRITER_BUILDING
#define CESIUMGLTFWRITER_API __declspec(dllexport)
#else
#define CESIUMGLTFWRITER_API __declspec(dllimport)
#endif
#else
#define CESIUMGLTFWRITER_API
#endif
