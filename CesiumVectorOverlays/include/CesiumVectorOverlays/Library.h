#pragma once

/**
 * @brief Raster overlays for displaying vector data.
 *
 * @mermaid-interactive{dependencies/CesiumVectorOverlays}
 */
namespace CesiumVectorOverlays {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMVECTOROVERLAYS_BUILDING
#define CESIUMVECTOROVERLAYS_API __declspec(dllexport)
#else
#define CESIUMVECTOROVERLAYS_API __declspec(dllimport)
#endif
#else
#define CESIUMVECTOROVERLAYS_API
#endif
