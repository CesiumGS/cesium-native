#pragma once

#include "spdlog/spdlog.h"

#ifdef SPDLOG_ACTIVE_LEVEL
    #undef SPDLOG_ACTIVE_LEVEL
#endif
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE


// String formatting adapted from https://stackoverflow.com/a/26221725 
// (public domain), with some contortions to support str::string in
// variadic arguments
template<typename T>
auto convert(T&& t) {
    if constexpr (std::is_same<std::remove_cv_t<std::remove_reference_t<T>>, std::string>::value) {
        return std::forward<T>(t).c_str();
    } else {
        return std::forward<T>(t);
    }
}
template<typename ... Args>
std::string formatStringInternal(const std::string& format, Args ... args)
{
    // Determine the size, with extra space for '\0'
    size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1; 
    if (size <= 0) 
    { 
        return "Error in formatString: " + format;
    }
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1);
}
/**
 * @brief Creates a string from a "printf" style format template and arguments.
 *
 * @param format The format string
 * @param args The arguments
 */
template<typename ... Args>
std::string formatString(std::string fmt, Args&& ... args) {
    return formatStringInternal(fmt, convert(std::forward<Args>(args))...);
}

#define LOG_TRACE(...) SPDLOG_TRACE(formatString(__VA_ARGS__))
#define LOG_DEBUG(...) SPDLOG_DEBUG(formatString(__VA_ARGS__))
#define LOG_INFO(...) SPDLOG_INFO(formatString(__VA_ARGS__))
#define LOG_WARN(...) SPDLOG_WARN(formatString(__VA_ARGS__))
#define LOG_ERROR(...) SPDLOG_ERROR(formatString(__VA_ARGS__))
#define LOG_CRITICAL(...) SPDLOG_CRITICAL(formatString(__VA_ARGS__))

namespace Cesium3DTiles {
    namespace impl {

        /**
         * Initialize the underlying logging infrastructure.
         */
        void initializeLogging() noexcept;
    }
}