#include "CesiumUtility/Assert.h"

#if defined CESIUM_FORCE_ASSERTIONS && defined NDEBUG

#undef NDEBUG
#include <cassert>
#define NDEBUG

namespace CesiumUtility {
std::int32_t forceAssertFailure() {
  assert(0 && "Assertion failed");
  return 1;
}
}; // namespace CesiumUtility

#endif
