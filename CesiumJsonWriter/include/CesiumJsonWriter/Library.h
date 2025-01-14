#pragma once

/**
 * @brief Classes for writing JSON.
 *
 * @mermaid-interactive{dependencies/CesiumJsonWriter}
 */
namespace CesiumJsonWriter {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMJSONWRITER_BUILDING
#define CESIUMJSONWRITER_API __declspec(dllexport)
#else
#define CESIUMJSONWRITER_API __declspec(dllimport)
#endif
#else
#define CESIUMJSONWRITER_API
#endif
