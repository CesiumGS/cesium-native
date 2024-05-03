#pragma once

// Allow assertion to be forcebly turned on, if we're configured
#ifdef CESIUM_FORCE_ASSERTIONS
// Asserts are normally compiled out when NDEBUG if defined. So undefine it
// before including it
#ifdef NDEBUG
#undef NDEBUG
#include <cassert>
#define NDEBUG
#else
#include <cassert>
#endif
#else
// Let assertions turn on with default behavior
#include <cassert>
#endif

// Define our own assertion, so that users of this library can forcebly turn
// them on if needed
#define ASSERT(expression) assert(expression)
