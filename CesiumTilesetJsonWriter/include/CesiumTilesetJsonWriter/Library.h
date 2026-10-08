#pragma once

/**
 * @brief Classes for writing [3D Tiles 1.0
 * and 1.1](https://github.com/CesiumGS/3d-tiles).
 *
 * @mermaid-interactive{dependencies/CesiumTilesetJsonWriter}
 */
namespace CesiumTilesetJsonWriter {}
#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMTILESETJSONWRITER_BUILDING
#define CESIUMTILESETJSONWRITER_API __declspec(dllexport)
#else
#define CESIUMTILESETJSONWRITER_API __declspec(dllimport)
#endif
#else
#define CESIUMTILESETJSONWRITER_API
#endif
