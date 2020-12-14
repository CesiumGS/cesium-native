#pragma once

// The compile-time log level. The spdlog LOG-macros for
// lower log levels will be omitted at compile time.
// The spdlog levels are
// SPDLOG_LEVEL_TRACE 0
// SPDLOG_LEVEL_DEBUG 1
// SPDLOG_LEVEL_INFO 2
// SPDLOG_LEVEL_WARN 3
// SPDLOG_LEVEL_ERROR 4
// SPDLOG_LEVEL_CRITICAL 5
// SPDLOG_LEVEL_OFF 6

#ifdef SPDLOG_ACTIVE_LEVEL
#undef SPDLOG_ACTIVE_LEVEL
#endif
#define SPDLOG_ACTIVE_LEVEL 0

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Werror=sign-conversion"
#endif

#include <spdlog/spdlog.h>

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

#define CESIUM_LOG_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define CESIUM_LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define CESIUM_LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define CESIUM_LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define CESIUM_LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define CESIUM_LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)
