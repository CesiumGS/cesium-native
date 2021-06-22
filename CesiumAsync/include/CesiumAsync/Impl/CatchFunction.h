#pragma once

#include "CesiumAsync/Impl/unwrapFuture.h"

namespace CesiumAsync {
namespace Impl {
// Begin omitting doxgen warnings for Impl namespace
//! @cond Doxygen_Suppress

template <typename Func, typename T, typename Scheduler> struct CatchFunction {
  Scheduler& scheduler;
  Func f;
  std::shared_ptr<ITaskProcessor> pTaskProcessor;

  decltype(auto) operator()(async::task<T>&& t) {
    try {
      return async::make_task(t.get());
    } catch (std::exception& e) {
      return async::make_task(e).then(
          scheduler,
          unwrapFuture<Func, std::exception>(std::move(f)));
    } catch (...) {
      return async::make_task<std::exception>(
                 std::runtime_error("Unknown exception"))
          .then(scheduler, unwrapFuture<Func, std::exception>(std::move(f)));
    }
  }
};

template <typename Func, typename Scheduler>
struct CatchFunction<Func, void, Scheduler> {
  Scheduler& scheduler;
  Func f;
  std::shared_ptr<ITaskProcessor> pTaskProcessor;

  decltype(auto) operator()(async::task<void>&& t) {
    try {
      t.get();
      return async::make_task();
    } catch (std::exception& e) {
      return async::make_task(e).then(
          scheduler,
          unwrapFuture<Func, std::exception>(std::move(f)));
    } catch (...) {
      return async::make_task<std::exception>(
                 std::runtime_error("Unknown exception"))
          .then(scheduler, unwrapFuture<Func, std::exception>(std::move(f)));
    }
  }
};

//! @endcond
// End omitting doxgen warnings for Impl namespace
} // namespace Impl
} // namespace CesiumAsync
