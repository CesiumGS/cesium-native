#pragma once

namespace CesiumAsync {
namespace Impl {
// Begin omitting doxgen warnings for Impl namespace
//! @cond Doxygen_Suppress

template <class Func> struct IdentityUnwrapper {
  static Func unwrap(Func&& f) { return std::forward<Func>(f); }
};

template <class Func, class T> struct ParameterizedTaskUnwrapper {
  static auto unwrap(Func&& f) {
    return [f = std::move(f)](T&& t) { return f(std::forward<T>(t))._task; };
  }
};

template <class Func> struct TaskUnwrapper {
  static auto unwrap(Func&& f) {
    return [f = std::move(f)]() { return f()._task; };
  }
};

template <class Func, class T> auto unwrapFuture(Func&& f) {
  return std::conditional<
      std::is_same<
          typename ContinuationReturnType<Func, T>::type,
          typename ContinuationFutureType<Func, T>::type>::value,
      IdentityUnwrapper<Func>,
      ParameterizedTaskUnwrapper<Func, T>>::type::unwrap(std::forward<Func>(f));
}

template <class Func> auto unwrapFuture(Func&& f) {
  return std::conditional<
      std::is_same<
          typename ContinuationReturnType<Func, void>::type,
          typename ContinuationFutureType<Func, void>::type>::value,
      IdentityUnwrapper<Func>,
      TaskUnwrapper<Func>>::type::unwrap(std::forward<Func>(f));
}

//! @endcond
// End omitting doxgen warnings for Impl namespace
} // namespace Impl
} // namespace CesiumAsync
