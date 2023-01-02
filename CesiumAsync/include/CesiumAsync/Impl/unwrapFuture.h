#pragma once

#include "ContinuationFutureType.h"
#include "ContinuationReturnType.h"

namespace CesiumAsync {
namespace CesiumImpl {
// Begin omitting doxgen warnings for Impl namespace
//! @cond Doxygen_Suppress

struct IdentityUnwrapper {
  template <typename Func> static Func unwrap(Func&& f) {
    return std::forward<Func>(f);
  }

  template <typename Func> static Func unwrapShared(Func&& f) {
    return std::forward<Func>(f);
  }
};

template <typename T> struct ParameterizedTaskUnwrapper {
  template <typename Func> static auto unwrap(Func&& f) {
    return [f = std::forward<Func>(f)](T&& t) mutable {
      return f(std::move(t))._task;
    };
  }

  template <typename Func> static auto unwrapShared(Func&& f) {
    return
        [f = std::forward<Func>(f)](const T& t) mutable { return f(t)._task; };
  }
};

struct TaskUnwrapper {
  template <typename Func> static auto unwrap(Func&& f) {
    return [f = std::forward<Func>(f)]() mutable { return f()._task; };
  }
};

template <typename Func, typename T> auto unwrapFuture(Func&& f) {
  return std::conditional<
      std::is_same<
          typename ContinuationReturnType<Func, T>::type,
          typename RemoveFuture<
              typename ContinuationFutureType<Func, T>::type>::type>::value,
      IdentityUnwrapper,
      ParameterizedTaskUnwrapper<T>>::type::unwrap(std::forward<Func>(f));
}

template <typename Func, typename T> auto unwrapSharedFuture(Func&& f) {
  return std::conditional<
      std::is_same<
          typename ContinuationReturnType<Func, T>::type,
          typename RemoveFuture<
              typename ContinuationFutureType<Func, T>::type>::type>::value,
      IdentityUnwrapper,
      ParameterizedTaskUnwrapper<T>>::type::unwrapShared(std::forward<Func>(f));
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

template <typename Func> auto unwrapSharedFuture(Func&& f) {
  return unwrapFuture(std::forward<Func>(f));
}

//! @endcond
// End omitting doxgen warnings for Impl namespace
} // namespace CesiumImpl
} // namespace CesiumAsync
