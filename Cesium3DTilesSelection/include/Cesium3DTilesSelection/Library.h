#pragma once

/**
 * @brief Classes that implement the 3D Tiles standard
 *
 * @mermaid-interactive{dependencies/Cesium3DTilesSelection}
 */
namespace Cesium3DTilesSelection {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUM3DTILESSELECTION_BUILDING
#define CESIUM3DTILESSELECTION_API __declspec(dllexport)
#else
#define CESIUM3DTILESSELECTION_API __declspec(dllimport)
#endif
#else
#define CESIUM3DTILESSELECTION_API
#endif
