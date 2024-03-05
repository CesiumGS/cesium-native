#pragma once

/**
 * @brief Classes for accessing legacy terrain based on layer.json and
 * quantized-mesh-1.0.
 */
namespace CesiumLegacyTerrain {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMLEGACYTERRAIN_BUILDING
#define CESIUMLEGACYTERRAIN_API __declspec(dllexport)
#else
#define CESIUMLEGACYTERRAIN_API __declspec(dllimport)
#endif
#else
#define CESIUMLEGACYTERRAIN_API
#endif
