#pragma once

/**
 * @brief Classes for loading I3S scene layers as 3D Tiles.
 */
namespace CesiumI3SSelection {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMI3SSELECTION_BUILDING
#define CESIUMI3SSELECTION_API __declspec(dllexport)
#else
#define CESIUMI3SSELECTION_API __declspec(dllimport)
#endif
#else
#define CESIUMI3SSELECTION_API
#endif
