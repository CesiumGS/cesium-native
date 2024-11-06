#pragma once

#if PRIVATE_CESIUM_SQLITE
#define CESIUM_SQLITE(name) cesium_##name
#else
#define CESIUM_SQLITE(name) name
#endif
