#define _CRT_RAND_S
#include "fillWithRandomBytes.h"

#include <cassert>
#include <stdexcept>

#if defined(_WIN32)
#pragma comment(lib, "Bcrypt.lib")
#include <Windows.h>
#include <bcrypt.h>
using ProviderHandle = BCRYPT_ALG_HANDLE;

static ProviderHandle getProvider() {
  static ProviderHandle ALG_HANDLE = nullptr;
  if (ALG_HANDLE == nullptr) {
    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &ALG_HANDLE,
        BCRYPT_RNG_ALGORITHM,
        nullptr,
        0);
    if (status != 0) {
      throw std::runtime_error("Failed to open algorithm provider");
    }
  }
  return ALG_HANDLE;
}

static void fillWithRandomBytesImpl(
    ProviderHandle handle,
    const gsl::span<uint8_t>& buffer) {
  uint32_t cbBuffer = static_cast<uint32_t>(buffer.size());
  NTSTATUS status = BCryptGenRandom(handle, buffer.data(), cbBuffer, 0);
  if (status != 0) {
    throw std::runtime_error("Failed to generate random bytes");
  }
}

#else
#include <cstdio>
using ProviderHandle = FILE*;

static ProviderHandle getProvider() {
  static ProviderHandle HANDLE = nullptr;
  if (HANDLE == nullptr) {
    HANDLE = fopen("/dev/urandom", "rb");
    assert(HANDLE != nullptr);
  }
  return HANDLE;
}

static void fillWithRandomBytesImpl(
    ProviderHandle handle,
    const gsl::span<uint8_t>& buffer) {
  auto bufferSize = buffer.size();
  auto readCount = fread((char*)buffer.data(), 1, bufferSize, handle);
  if (readCount != bufferSize) {
    throw std::runtime_error("Failed to generate random bytes");
  }
}

#endif

namespace CesiumIonClient {

void fillWithRandomBytes(const gsl::span<uint8_t>& buffer) {
  if (buffer.empty()) {
    return;
  }
  ProviderHandle handle = getProvider();
  ::fillWithRandomBytesImpl(handle, buffer);
}

} // namespace CesiumIonClient
