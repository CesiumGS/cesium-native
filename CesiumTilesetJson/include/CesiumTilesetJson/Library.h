#pragma once

/**
 * @brief Classes to represent a tileset.json from [3D
 * Tiles 1.1](https://github.com/CesiumGS/3d-tiles).
 *
 * @mermaid-interactive{dependencies/CesiumTilesetJson}
 */
namespace CesiumTilesetJson {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMTILESETJSON_BUILDING
#define CESIUMTILESETJSON_API __declspec(dllexport)
#else
#define CESIUMTILESETJSON_API __declspec(dllimport)
#endif
#else
#define CESIUMTILESETJSON_API
#endif
