#pragma once

#include <type_traits>

namespace CesiumAsync {

template <typename T> class SharedFutureResult;

namespace Impl {
// Begin omitting doxgen warnings for Impl namespace
//! @cond Doxygen_Suppress

template <typename Func, typename T, typename Enable = void>
struct ContinuationReturnType {};

template <typename Func, typename T>
struct ContinuationReturnType<
    Func,
    T,
    std::enable_if_t<std::is_invocable_v<Func, SharedFutureResult<T>>>> {
  using type = typename std::invoke_result<Func, SharedFutureResult<T>>::type;
};

template <typename Func, typename T>
struct ContinuationReturnType<
    Func,
    T,
    std::enable_if_t<std::is_invocable_v<Func, T>>> {
  using type = typename std::invoke_result<Func, T>::type;
};

template <typename Func> struct ContinuationReturnType<Func, void> {
  using type = typename std::invoke_result<Func>::type;
};

//! @endcond
// End omitting doxgen warnings for Impl namespace
} // namespace Impl
} // namespace CesiumAsync
