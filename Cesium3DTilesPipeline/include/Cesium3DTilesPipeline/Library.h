#pragma once

/**
 * @brief Classes that implement the 3D Tiles standard
 */
namespace Cesium3DTilesPipeline {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUM3DTILES_BUILDING
#define CESIUM3DTILESPIPELINE_API __declspec(dllexport)
#else
#define CESIUM3DTILESPIPELINE_API __declspec(dllimport)
#endif
#else
#define CESIUM3DTILESPIPELINE_API
#endif
