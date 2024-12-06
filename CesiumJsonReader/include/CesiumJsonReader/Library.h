#pragma once

/**
 * @brief Classes for reading JSON.
 *
 * @mermaid-interactive{dependencies/CesiumJsonReader}
 */
namespace CesiumJsonReader {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMJSONREADER_BUILDING
#define CESIUMJSONREADER_API __declspec(dllexport)
#else
#define CESIUMJSONREADER_API __declspec(dllimport)
#endif
#else
#define CESIUMJSONREADER_API
#endif
