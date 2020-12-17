#pragma once

// The compile-time log level. The spdlog LOG-macros for
// lower log levels will be omitted at compile time.
// 
// The spdlog levels are
// SPDLOG_LEVEL_TRACE 0
// SPDLOG_LEVEL_DEBUG 1
// SPDLOG_LEVEL_INFO 2
// SPDLOG_LEVEL_WARN 3
// SPDLOG_LEVEL_ERROR 4
// SPDLOG_LEVEL_CRITICAL 5
// SPDLOG_LEVEL_OFF 6
// 
// For now, we use the default spdlog level, 
// which is SPDLOG_LEVEL_INFO
/*/
#ifdef SPDLOG_ACTIVE_LEVEL
#undef SPDLOG_ACTIVE_LEVEL
#endif
#define SPDLOG_ACTIVE_LEVEL 0
//*/

// #ifndef _MSC_VER
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wsign-conversion"
// #pragma GCC diagnostic ignored "-Werror=sign-conversion"
// #endif

#include <spdlog/spdlog.h>

// #ifndef _MSC_VER
// #pragma GCC diagnostic pop
// #endif
