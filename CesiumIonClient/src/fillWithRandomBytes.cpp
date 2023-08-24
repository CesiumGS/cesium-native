#define _CRT_RAND_S
#include "fillWithRandomBytes.h"

#include <cassert>

// When WINAPI_FAMILY_PARTITION && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
// is true, this is a Univeral Windows Platform build. csprng doesn't work on
// UWP. So we use the Windows-only rand_s function instead.

#ifdef WINAPI_FAMILY
#if WINAPI_FAMILY == WINAPI_FAMILY_PC_APP ||                                   \
    WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
#define IS_UWP 1
#else
#define IS_UWP 0
#endif
#else
#define IS_UWP 0
#endif

#if IS_UWP
#include <cstdlib>
#else
#include <duthomhas/csprng.hpp>
#endif

namespace CesiumIonClient {

void fillWithRandomBytes(const gsl::span<uint8_t>& buffer) {
#if IS_UWP
  size_t i = 0;
  if (buffer.size() >= sizeof(uint32_t)) {
    for (; i <= buffer.size() - sizeof(std::uint32_t);
         i += sizeof(std::uint32_t)) {
      std::uint32_t r;
      if (rand_s(&r) != 0) {
        throw std::exception("Failed to generate random numbers.");
      }
      std::memcpy(&buffer[i], &r, sizeof(std::uint32_t));
    }
  }

  if (i < buffer.size()) {
    assert(buffer.size() - i < sizeof(uint32_t));

    std::uint32_t extra;
    if (rand_s(&extra) != 0) {
      throw std::exception("Failed to generate random numbers.");
    }

    std::uint8_t* pSource = reinterpret_cast<std::uint8_t*>(&extra);
    for (; i < buffer.size(); ++i) {
      buffer[i] = *pSource;
      ++pSource;
    }
  }
#else
  duthomhas::csprng rng;
  rng(buffer);
#endif
}

} // namespace CesiumIonClient
