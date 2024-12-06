#pragma once

/**
 * @brief Classes for accessing terrain based on layer.json and
 * quantized-mesh-1.0.
 *
 * @mermaid-interactive{dependencies/CesiumQuantizedMeshTerrain}
 */
namespace CesiumQuantizedMeshTerrain {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMQUANTIZEDMESHTERRAIN_BUILDING
#define CESIUMQUANTIZEDMESHTERRAIN_API __declspec(dllexport)
#else
#define CESIUMQUANTIZEDMESHTERRAIN_API __declspec(dllimport)
#endif
#else
#define CESIUMQUANTIZEDMESHTERRAIN_API
#endif
