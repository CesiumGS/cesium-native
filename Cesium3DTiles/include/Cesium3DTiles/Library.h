#pragma once

/**
 * @brief Classes for using [3D Tiles](https://github.com/CesiumGS/3d-tiles).
 *
 * @mermaid-interactive{dependencies/Cesium3DTiles}
 */
namespace Cesium3DTiles {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUM3DTILES_BUILDING
#define CESIUM3DTILES_API __declspec(dllexport)
#else
#define CESIUM3DTILES_API __declspec(dllimport)
#endif
#else
#define CESIUM3DTILES_API
#endif
