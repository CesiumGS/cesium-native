#pragma once

/**
 * @brief Classes for interacting with the iTwin API.
 *
 * @mermaid-interactive{dependencies/CesiumITwinClient}
 */
namespace CesiumITwinClient {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMITWINCLIENT_BUILDING
#define CESIUMITWINCLIENT_API __declspec(dllexport)
#else
#define CESIUMITWINCLIENT_API __declspec(dllimport)
#endif
#else
#define CESIUMITWINCLIENT_API
#endif
