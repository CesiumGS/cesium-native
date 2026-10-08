#pragma once

/**
 * @brief Classes for reading [3D Tiles 1.0
 * and 1.1](https://github.com/CesiumGS/3d-tiles).
 *
 * @mermaid-interactive{dependencies/CesiumTilesetJsonReader}
 */
namespace CesiumTilesetJsonReader {}
#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMTILESETJSONREADER_BUILDING
#define CESIUMTILESETJSONREADER_API __declspec(dllexport)
#else
#define CESIUMTILESETJSONREADER_API __declspec(dllimport)
#endif
#else
#define CESIUMTILESETJSONREADER_API
#endif
