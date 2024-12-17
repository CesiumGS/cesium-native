#pragma once

/**
 * @brief Classes that support loading and converting 3D Tiles tile content.
 *
 * @mermaid-interactive{dependencies/Cesium3DTilesContent}
 */
namespace Cesium3DTilesContent {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUM3DTILESCONTENT_BUILDING
#define CESIUM3DTILESCONTENT_API __declspec(dllexport)
#else
#define CESIUM3DTILESCONTENT_API __declspec(dllimport)
#endif
#else
#define CESIUM3DTILESCONTENT_API
#endif
