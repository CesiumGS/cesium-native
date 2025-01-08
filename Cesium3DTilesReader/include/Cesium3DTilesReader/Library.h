#pragma once

/**
 * @brief Classes for reading [3D Tiles](https://github.com/CesiumGS/3d-tiles).
 *
 * @mermaid-interactive{dependencies/Cesium3DTilesReader}
 */
namespace Cesium3DTilesReader {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUM3DTILESREADER_BUILDING
#define CESIUM3DTILESREADER_API __declspec(dllexport)
#else
#define CESIUM3DTILESREADER_API __declspec(dllimport)
#endif
#else
#define CESIUM3DTILESREADER_API
#endif
