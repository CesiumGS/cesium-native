#pragma once

/**
 * @brief Classes for using [I3S](https://github.com/Esri/i3s-spec).
 */
namespace CesiumI3S {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMI3S_BUILDING
#define CESIUMI3S_API __declspec(dllexport)
#else
#define CESIUMI3S_API __declspec(dllimport)
#endif
#else
#define CESIUMI3S_API
#endif
