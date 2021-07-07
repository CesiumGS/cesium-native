#pragma once

#if __cplusplus >= 201703L

#define CESIUM_NODISCARD [[nodiscard]]
#define CESIUM_UNUSED [[maybe_unused]]

#else

#define CESIUM_NODISCARD
#define CESIUM_UNUSED

namespace std {
enum class byte : unsigned char {};
}

#endif
