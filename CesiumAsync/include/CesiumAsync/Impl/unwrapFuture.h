#pragma once

#include "CesiumAsync/Impl/ContinuationFutureType.h"
#include "CesiumAsync/Impl/ContinuationReturnType.h"
#include "CesiumAsync/SharedFutureResult.h"
#include <type_traits>

namespace CesiumAsync {
namespace Impl {
// Begin omitting doxgen warnings for Impl namespace
//! @cond Doxygen_Suppress

template <typename T>
inline constexpr bool isFuture =
    !std::is_same_v<T, typename RemoveFuture<T>::type>;

template <typename Func, typename T>
auto futureFunctionToTaskFunction(Func&& f) {
  if constexpr (std::is_invocable_v<Func, SharedFutureResult<T>>) {
    // Function taking a SharedFuture<T>
    if constexpr (isFuture<typename ContinuationReturnType<Func, T>::type>) {
      // And returning a Future
      return [f = std::forward<Func>(f)](const async::shared_task<T>& task) {
        return f(SharedFutureResult<T>(task))._task;
      };
    } else {
      // And returning a regular value
      return [f = std::forward<Func>(f)](const async::shared_task<T>& task) {
        return f(SharedFutureResult<T>(task));
      };
    }
  } else {
    // Function taking a normal value T
    if constexpr (isFuture<typename ContinuationReturnType<Func, T>::type>) {
      // And returning a Future
      return
          [f = std::forward<Func>(f)](T&& t) { return f(std::move(t))._task; };
    } else {
      // And returning a regular value
      return std::forward<Func>(f);
    }
  }
}

struct IdentityUnwrapper {
  template <typename Func> static Func unwrap(Func&& f) {
    return std::forward<Func>(f);
  }
};

struct TaskUnwrapper {
  template <typename Func> static auto unwrap(Func&& f) {
    return [f = std::forward<Func>(f)]() { return f()._task; };
  }
};

template <typename Func, typename T> auto unwrapFuture(Func&& f) {
  return futureFunctionToTaskFunction<Func, T>(std::forward<Func>(f));
}

template <typename Func> auto unwrapFuture(Func&& f) {
  return std::conditional<
      std::is_same<
          typename ContinuationReturnType<Func, void>::type,
          typename RemoveFuture<
              typename ContinuationFutureType<Func, void>::type>::type>::value,
      IdentityUnwrapper,
      TaskUnwrapper>::type::unwrap(std::forward<Func>(f));
}

//! @endcond
// End omitting doxgen warnings for Impl namespace
} // namespace Impl
} // namespace CesiumAsync
