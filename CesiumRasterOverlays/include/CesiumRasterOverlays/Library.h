#pragma once

/**
 * @brief Classes for raster overlays, which allow draping massive 2D textures
 * over a model.
 *
 * @mermaid-interactive{dependencies/CesiumRasterOverlays}
 */
namespace CesiumRasterOverlays {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMRASTEROVERLAYS_BUILDING
#define CESIUMRASTEROVERLAYS_API __declspec(dllexport)
#else
#define CESIUMRASTEROVERLAYS_API __declspec(dllimport)
#endif
#else
#define CESIUMRASTEROVERLAYS_API
#endif
