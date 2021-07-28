#pragma once

/**
 * @brief Classes for handling JSON extensions (e.g. in glTF, 3D Tiles).
 */
namespace CesiumJsonExtensions {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMJSONEXTENSIONS_BUILDING
#define CESIUMJSONEXTENSIONS_API __declspec(dllexport)
#else
#define CESIUMJSONEXTENSIONS_API __declspec(dllimport)
#endif
#else
#define CESIUMJSONEXTENSIONS_API
#endif
