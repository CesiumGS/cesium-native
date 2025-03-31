#pragma once

/**
 * @brief Shared classes for interacting with Cesium ion and iTwin APIs.
 *
 * @mermaid-interactive{dependencies/CesiumClientCommon}
 */
namespace CesiumClientCommon {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMCLIENTCOMMON_BUILDING
#define CESIUMCLIENTCOMMON_API __declspec(dllexport)
#else
#define CESIUMCLIENTCOMMON_API __declspec(dllimport)
#endif
#else
#define CESIUMCLIENTCOMMON_API
#endif
