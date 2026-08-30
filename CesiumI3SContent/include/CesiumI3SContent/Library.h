#pragma once

/**
 * @brief Classes for converting I3S geometry to glTF.
 */
namespace CesiumI3SContent {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMI3SCONTENT_BUILDING
#define CESIUMI3SCONTENT_API __declspec(dllexport)
#else
#define CESIUMI3SCONTENT_API __declspec(dllimport)
#endif
#else
#define CESIUMI3SCONTENT_API
#endif
