#pragma once

/**
 * @brief Classes that support reading, decoding, and manipulating images.
 *
 * @mermaid-interactive{dependencies/CesiumImage}
 */
namespace CesiumImage {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMIMAGE_BUILDING
#define CESIUMIMAGE_API __declspec(dllexport)
#else
#define CESIUMIMAGE_API __declspec(dllimport)
#endif
#else
#define CESIUMIMAGE_API
#endif
