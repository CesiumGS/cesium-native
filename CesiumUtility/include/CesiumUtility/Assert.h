#pragma once

#include <cstdint>

//
// Define our own assertion, so that users can forcebly turn them on if needed
//
#if defined CESIUM_FORCE_ASSERTIONS && defined NDEBUG
// Asserts are defined in cassert and normally compiled out when NDEBUG is
// defined. Redirect to our own assertion that can't be compiled out
namespace CesiumUtility {
std::int32_t forceAssertFailure();
};
#define CESIUM_ASSERT(expression)                                              \
  ((expression) ? 0 : CesiumUtility::forceAssertFailure())
#else
// Let assertions get defined with default behavior
#include <cassert>
#define CESIUM_ASSERT(expression) assert(expression)
#endif
