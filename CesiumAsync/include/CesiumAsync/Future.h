#pragma once

#include "CesiumAsync/Impl/AsyncSystemSchedulers.h"
#include "CesiumAsync/Impl/CatchFunction.h"
#include "CesiumAsync/Impl/ContinuationFutureType.h"
#include "CesiumAsync/Impl/FutureWaitResult.h"
#include "CesiumAsync/Impl/WithTracing.h"
#include "CesiumAsync/ThreadPool.h"
#include "CesiumUtility/Tracing.h"
#include <variant>

namespace CesiumAsync {

namespace Impl {

template <typename Func, typename R> struct ParameterizedTaskUnwrapper;
template <typename Func> struct TaskUnwrapper;

} // namespace Impl

/**
 * @brief A value that will be available in the future, as produced by
 * {@link AsyncSystem}.
 *
 * @tparam T The type of the value.
 */
template <typename T> class Future final {
public:
  /**
   * @brief Move constructor
   */
  Future(Future<T>&& rhs) noexcept
      : _pSchedulers(std::move(rhs._pSchedulers)),
        _task(std::move(rhs._task)) {}

  Future(const Future<T>& rhs) = delete;
  Future<T>& operator=(const Future<T>& rhs) = delete;

  /**
   * @brief Registers a continuation function to be invoked in a worker thread
   * when this Future resolves, and invalidates this Future.
   *
   * If the function itself returns a `Future`, the function will not be
   * considered complete until that returned `Future` also resolves.
   *
   * If this Future is resolved from a designated worker thread, the
   * continuation function will be invoked immediately rather than in a
   * separate task. Similarly, if the Future is already resolved when
   * `thenInWorkerThread` is called from a designated worker thread, the
   * continuation function will be invoked immediately before this
   * method returns.
   *
   * @tparam Func The type of the function.
   * @param f The function.
   * @return A future that resolves after the supplied function completes.
   */
  template <typename Func>
  Impl::ContinuationFutureType_t<Func, T> thenInWorkerThread(Func&& f) && {
    return std::move(*this).thenWithScheduler(
        this->_pSchedulers->immediatelyInWorkerThreadScheduler,
        "waiting for worker thread",
        std::forward<Func>(f));
  }

  /**
   * @brief Registers a continuation function to be invoked in the main thread
   * when this Future resolves, and invalidates this Future.
   *
   * If this Future is resolved from the main thread, the
   * continuation function will be invoked immediately rather than queued for
   * later execution in the main thread. Similarly, if the Future is already
   * resolved when `thenInMainThread` is called from the main thread, the
   * continuation function will be invoked immediately before this
   * method returns.
   *
   * If the function itself returns a `Future`, the function will not be
   * considered complete until that returned `Future` also resolves.
   *
   * @tparam Func The type of the function.
   * @param f The function.
   * @return A future that resolves after the supplied function completes.
   */
  template <typename Func>
  Impl::ContinuationFutureType_t<Func, T> thenInMainThread(Func&& f) && {
    return std::move(*this).thenWithScheduler(
        this->_pSchedulers->immediatelyInMainThreadScheduler,
        "waiting for main thread",
        std::forward<Func>(f));
  }

  /**
   * @brief Registers a continuation function to be invoked immediately in
   * whichever thread causes the Future to be resolved, and invalidates this
   * Future.
   *
   * If the Future is already resolved, the supplied function will be called
   * immediately in the calling thread and this method will not return until
   * that function does.
   *
   * If the function itself returns a `Future`, the function will not be
   * considered complete until that returned `Future` also resolves.
   *
   * @tparam Func The type of the function.
   * @param f The function.
   * @return A future that resolves after the supplied function completes.
   */
  template <typename Func>
  Impl::ContinuationFutureType_t<Func, T> thenImmediately(Func&& f) && {
    return Impl::ContinuationFutureType_t<Func, T>(
        this->_pSchedulers,
        _task.then(
            async::inline_scheduler(),
            Impl::WithTracing<Func, T>::wrap(nullptr, std::forward<Func>(f))));
  }

  /**
   * @brief Registers a continuation function to be invoked in a thread pool
   * when this Future resolves, and invalidates this Future.
   *
   * If the function itself returns a `Future`, the function will not be
   * considered complete until that returned `Future` also resolves.
   *
   * If this Future is resolved from a thread pool thread, the
   * continuation function will be invoked immediately rather than in a
   * separate task. Similarly, if the Future is already resolved when
   * `thenInThreadPool` is called from a designated thread pool thread, the
   * continuation function will be invoked immediately before this
   * method returns.
   *
   * @tparam Func The type of the function.
   * @param f The function.
   * @return A future that resolves after the supplied function completes.
   */
  template <typename Func>
  Impl::ContinuationFutureType_t<Func, T>
  thenInThreadPool(const ThreadPool& threadPool, Func&& f) && {
    return std::move(*this).thenWithScheduler(
        *threadPool._pScheduler,
        "waiting for thread pool thread",
        std::forward<Func>(f));
  }

  /**
   * @brief Registers a continuation function to be invoked in the main thread
   * when this Future rejects, and invalidates this Future.
   *
   * If this Future is rejected from the main thread, the
   * continuation function will be invoked immediately rather than queued for
   * later execution in the main thread. Similarly, if the Future is already
   * rejected when `catchInMainThread` is called from the main thread, the
   * continuation function will be invoked immediately before this
   * method returns.
   *
   * If the function itself returns a `Future`, the function will not be
   * considered complete until that returned `Future` also resolves.
   *
   * Any `then` continuations chained after this one will be invoked with the
   * return value of the catch callback.
   *
   * @tparam Func The type of the function.
   * @param f The function.
   * @return A future that resolves after the supplied function completes.
   */
  template <typename Func> Future<T> catchInMainThread(Func&& f) && {
    return std::move(*this).catchWithScheduler(
        this->_pSchedulers->immediatelyInMainThreadScheduler,
        std::forward<Func>(f));
  }

  /**
   * @brief Waits for the future to resolve or reject and returns the result.
   *
   * This method must not be called from the main thread, the one that calls
   * {@link AsyncSystem::dispatchMainThreadTasks}. Doing so can lead to a
   * deadlock because the main thread tasks will never complete while this
   * method is blocking the main thread.
   *
   * @return The value if the future resolves successfully, or the exception if
   * it rejects.
   */
  Impl::FutureWaitResult_t<T> wait() {
    try {
      return Impl::FutureWaitResult<T>::getFromTask(this->_task);
    } catch (std::exception& e) {
      return e;
    } catch (...) {
      return std::runtime_error("Unknown exception.");
    }
  }

private:
  Future(
      const std::shared_ptr<Impl::AsyncSystemSchedulers>& pSchedulers,
      async::task<T>&& task) noexcept
      : _pSchedulers(pSchedulers), _task(std::move(task)) {}

  template <typename Func, typename Scheduler>
  Impl::ContinuationFutureType_t<Func, T> thenWithScheduler(
      Scheduler& scheduler,
      const char* tracingName,
      Func&& f) && {
    // It would be nice if tracingName were a template parameter instead of a
    // function parameter, but that triggers a bug in VS2017. It was previously
    // a bug in VS2019, too, but has been fixed there:
    // https://developercommunity.visualstudio.com/t/internal-compiler-error-when-compiling-a-template-1/534210
#if CESIUM_TRACING_ENABLED
    // When tracing is enabled, we measure the time between scheduling and
    // dispatching of the work.
    auto task = this->_task.then(
        async::inline_scheduler(),
        [tracingName, CESIUM_TRACE_LAMBDA_CAPTURE_TRACK()](T&& value) mutable {
          CESIUM_TRACE_USE_CAPTURED_TRACK();
          if (tracingName) {
            CESIUM_TRACE_BEGIN_IN_TRACK(tracingName);
          }
          return std::move(value);
        });
#else
    auto& task = this->_task;
#endif

    return Impl::ContinuationFutureType_t<Func, T>(
        this->_pSchedulers,
        task.then(
            scheduler,
            Impl::WithTracing<Func, T>::wrap(
                tracingName,
                std::forward<Func>(f))));
  }

  template <typename Func, typename Scheduler>
  Impl::ContinuationFutureType_t<Func, std::exception>
  catchWithScheduler(Scheduler& scheduler, Func&& f) && {
    return Impl::ContinuationFutureType_t<Func, std::exception>(
        this->_pSchedulers,
        this->_task.then(
            async::inline_scheduler(),
            Impl::CatchFunction<Func, T, Scheduler>{
                scheduler,
                std::forward<Func>(f),
                this->_pSchedulers->pTaskProcessor}));
  }

  std::shared_ptr<Impl::AsyncSystemSchedulers> _pSchedulers;
  async::task<T> _task;

  friend class AsyncSystem;

  template <typename Func, typename R>
  friend struct Impl::ParameterizedTaskUnwrapper;

  template <typename Func> friend struct Impl::TaskUnwrapper;

  template <typename R> friend class Future;
};

} // namespace CesiumAsync
