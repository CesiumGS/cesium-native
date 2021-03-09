#pragma once

#if defined(_WIN32) && defined(CESIUM_SHARED)
#ifdef CESIUMGLTFREADER_BUILDING
#define CESIUMGLTFREADER_API __declspec(dllexport)
#else
#define CESIUMGLTFREADER_API __declspec(dllimport)
#endif
#else
#define CESIUMGLTFREADER_API
#endif
