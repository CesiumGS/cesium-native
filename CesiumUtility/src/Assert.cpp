#include "CesiumUtility/Assert.h"

#if defined CESIUM_FORCE_ASSERTIONS && defined NDEBUG

#undef NDEBUG
#include <cassert>
#define NDEBUG

namespace CesiumUtility {
void forceAssertFailure() { assert(0 && "Assertion failed"); }
}; // namespace CesiumUtility

#endif
