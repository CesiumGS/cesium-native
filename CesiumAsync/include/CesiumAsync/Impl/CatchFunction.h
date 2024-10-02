#pragma once

#include "unwrapFuture.h"

namespace CesiumAsync {
namespace CesiumImpl {
// Begin omitting doxgen warnings for Impl namespace
//! @cond Doxygen_Suppress

template <
    typename Func,
    typename T,
    typename Scheduler,
    typename TaskParameter = async::task<T>&&>
struct CatchFunction {
  Scheduler& scheduler;
  Func f;

  async::task<T> operator()(TaskParameter t) {
    try {
      return async::make_task(t.get());
    } catch (...) {
      // Make an exception_ptr task, then scheduler to a wrapper around f that
      // throws it, catches it, and calls f with a reference to it.
      auto ptrToException = [f = std::move(f)](std::exception_ptr&& e) mutable {
        try {
          std::rethrow_exception(e);
        } catch (std::exception& e) {
          return f(std::move(e));
        } catch (...) {
          return f(std::runtime_error("Unknown exception"));
        }
      };
      return async::make_task(std::current_exception())
          .then(
              scheduler,
              unwrapFuture<decltype(ptrToException), std::exception_ptr>(
                  std::move(ptrToException)));
    }
  }
};

template <typename Func, typename Scheduler, typename TaskParameter>
struct CatchFunction<Func, void, Scheduler, TaskParameter> {
  Scheduler& scheduler;
  Func f;

  async::task<void> operator()(TaskParameter t) {
    try {
      t.get();
      return async::make_task();
    } catch (...) {
      // Make an exception_ptr task, then scheduler to a wrapper around f that
      // throws it, catches it, and calls f with a reference to it.
      auto ptrToException = [f = std::move(f)](std::exception_ptr&& e) mutable {
        try {
          std::rethrow_exception(e);
        } catch (std::exception& e) {
          return f(std::move(e));
        } catch (...) {
          return f(std::runtime_error("Unknown exception"));
        }
      };
      return async::make_task(std::current_exception())
          .then(
              scheduler,
              unwrapFuture<decltype(ptrToException), std::exception_ptr>(
                  std::move(ptrToException)));
    }
  }
};

//! @endcond
// End omitting doxgen warnings for Impl namespace
} // namespace CesiumImpl
} // namespace CesiumAsync
