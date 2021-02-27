// Copyright CesiumGS, Inc. and Contributors

#pragma once

/**
 * @brief Classes that support asynchronous operations.
 */
namespace CesiumAsync {}

#if defined(_WIN32) && defined(CESIUM_SHARED)
    #ifdef CESIUMASYNC_BUILDING
        #define CESIUMASYNC_API __declspec(dllexport)
    #else
        #define CESIUMASYNC_API __declspec(dllimport)
    #endif
#else
    #define CESIUMASYNC_API
#endif
