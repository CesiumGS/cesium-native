#pragma once

/**
 * @brief Classes for working with Cesium ion clients
 *
 * @mermaid-interactive{dependencies/CesiumIonClient}
 */
namespace CesiumIonClient {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMIONCLIENT_BUILDING
#define CESIUMIONCLIENT_API __declspec(dllexport)
#else
#define CESIUMIONCLIENT_API __declspec(dllimport)
#endif
#else
#define CESIUMIONCLIENT_API
#endif
