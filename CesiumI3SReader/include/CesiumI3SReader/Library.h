#pragma once

/**
 * @brief Classes for reading I3S scene layers from JSON.
 */
namespace CesiumI3SReader {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMI3SREADER_BUILDING
#define CESIUMI3SREADER_API __declspec(dllexport)
#else
#define CESIUMI3SREADER_API __declspec(dllimport)
#endif
#else
#define CESIUMI3SREADER_API
#endif
