#pragma once

#if __cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define CESIUM_CXX_17 1
#else
#define CESIUM_CXX_17 0
#endif

#if CESIUM_CXX_17

#define CESIUM_NODISCARD [[nodiscard]]

#else

#include <type_traits>

#define CESIUM_NODISCARD

namespace std {

enum class byte : unsigned char {};

// MSVC already has these templates, even in C++14.
#ifndef _MSC_VER
template <class T>
constexpr typename std::add_const<T>::type& as_const(T& t) noexcept {
  return t;
}

template <class T, class U> constexpr bool is_same_v = is_same<T, U>::value;
#endif

// Implement invoke_result in terms of result_of.
// See https://en.cppreference.com/w/cpp/types/result_of#Notes
template <typename F, typename... ArgTypes> struct invoke_result {
  using type = typename std::result_of<F && (ArgTypes && ...)>::type;
};

template <typename F, typename... ArgTypes>
using invoke_result_t = typename invoke_result<F, ArgTypes...>::type;

} // namespace std

#endif
