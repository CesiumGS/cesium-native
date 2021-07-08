#pragma once

#if __cplusplus >= 201703L

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

} // namespace std

#endif
