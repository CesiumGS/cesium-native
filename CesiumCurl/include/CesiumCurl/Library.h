#pragma once

/**
 * @brief Provides the ability to access HTTP and other network resources using
 * libcurl.
 *
 * @mermaid-interactive{dependencies/CesiumCurl}
 */
namespace CesiumCurl {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMCURL_BUILDING
#define CESIUMCURL_API __declspec(dllexport)
#else
#define CESIUMCURL_API __declspec(dllimport)
#endif
#else
#define CESIUMCURL_API
#endif
