#pragma once

/**
 * @brief Classes for loading vector data such as GeoJSON.
 *
 * @mermaid-interactive{dependencies/CesiumVectorData}
 */
namespace CesiumVectorData {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMVECTORDATA_BUILDING
#define CESIUMVECTORDATA_API __declspec(dllexport)
#else
#define CESIUMVECTORDATA_API __declspec(dllimport)
#endif
#else
#define CESIUMVECTORDATA_API
#endif
