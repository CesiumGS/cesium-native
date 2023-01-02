#pragma once

#include "Impl/AsyncSystemSchedulers.h"
#include "Impl/CatchFunction.h"
#include "Impl/ContinuationFutureType.h"
#include "Impl/WithTracing.h"
#include "SharedFuture.h"
#include "ThreadPool.h"

#include <CesiumUtility/Tracing.h>

#include <variant>

namespace CesiumAsync {

namespace CesiumImpl {

template <typename R> struct ParameterizedTaskUnwrapper;
struct TaskUnwrapper;

} // namespace CesiumImpl

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

  Future<T>& operator=(Future<T>&& rhs) noexcept {
    this->_pSchedulers = std::move(rhs._pSchedulers);
    this->_task = std::move(rhs._task);
    return *this;
  }

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
  CesiumImpl::ContinuationFutureType_t<Func, T>
  thenInWorkerThread(Func&& f) && {
    return std::move(*this).thenWithScheduler(
        this->_pSchedulers->workerThread.immediate,
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
  CesiumImpl::ContinuationFutureType_t<Func, T> thenInMainThread(Func&& f) && {
    return std::move(*this).thenWithScheduler(
        this->_pSchedulers->mainThread.immediate,
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
  CesiumImpl::ContinuationFutureType_t<Func, T> thenImmediately(Func&& f) && {
    return CesiumImpl::ContinuationFutureType_t<Func, T>(
        this->_pSchedulers,
        _task.then(
            async::inline_scheduler(),
            CesiumImpl::WithTracing<T>::end(nullptr, std::forward<Func>(f))));
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
  CesiumImpl::ContinuationFutureType_t<Func, T>
  thenInThreadPool(const ThreadPool& threadPool, Func&& f) && {
    return std::move(*this).thenWithScheduler(
        threadPool._pScheduler->immediate,
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
        this->_pSchedulers->mainThread.immediate,
        std::forward<Func>(f));
  }

  /**
   * @brief Registers a continuation function to be invoked immediately, and
   * invalidates this Future.
   *
   * When this Future is rejected, the continuation function will be invoked
   * in whatever thread does the rejection. Similarly, if the Future is already
   * rejected when `catchImmediately` is called, the continuation function will
   * be invoked immediately before this method returns.
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
  template <typename Func> Future<T> catchImmediately(Func&& f) && {
    return std::move(*this).catchWithScheduler(
        async::inline_scheduler(),
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
   * @return The value if the future resolves successfully.
   * @throws An exception if the future rejected.
   */
  T wait() { return this->_task.get(); }

  /**
   * @brief Determines if this future is already resolved or rejected.
   *
   * If this method returns true, it is guaranteed that {@link wait} will
   * not block but will instead immediately return a value or throw an
   * exception.
   *
   * @return True if the future is already resolved or rejected and {@link wait}
   * will not block; otherwise, false.
   */
  bool isReady() const { return this->_task.ready(); }

  /**
   * @brief Creates a version of this future that can be shared, meaning that
   * its value may be accessed multiple times and multiple continuations may be
   * attached to it.
   *
   * Calling this method invalidates the original Future.
   *
   * @return The `SharedFuture`.
   */
  SharedFuture<T> share() && {
    return SharedFuture<T>(this->_pSchedulers, this->_task.share());
  }

private:
  Future(
      const std::shared_ptr<CesiumImpl::AsyncSystemSchedulers>& pSchedulers,
      async::task<T>&& task) noexcept
      : _pSchedulers(pSchedulers), _task(std::move(task)) {}

  template <typename Func, typename Scheduler>
  CesiumImpl::ContinuationFutureType_t<Func, T> thenWithScheduler(
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
        CesiumImpl::WithTracing<T>::begin(tracingName, std::forward<Func>(f)));
#else
    auto& task = this->_task;
#endif

    return CesiumImpl::ContinuationFutureType_t<Func, T>(
        this->_pSchedulers,
        task.then(
            scheduler,
            CesiumImpl::WithTracing<T>::end(
                tracingName,
                std::forward<Func>(f))));
  }

  template <typename Func, typename Scheduler>
  CesiumImpl::ContinuationFutureType_t<Func, std::exception>
  catchWithScheduler(Scheduler& scheduler, Func&& f) && {
    return CesiumImpl::ContinuationFutureType_t<Func, std::exception>(
        this->_pSchedulers,
        this->_task.then(
            async::inline_scheduler(),
            CesiumImpl::CatchFunction<Func, T, Scheduler>{
                scheduler,
                std::forward<Func>(f)}));
  }

  std::shared_ptr<CesiumImpl::AsyncSystemSchedulers> _pSchedulers;
  async::task<T> _task;

  friend class AsyncSystem;

  template <typename R> friend struct CesiumImpl::ParameterizedTaskUnwrapper;

  friend struct CesiumImpl::TaskUnwrapper;

  template <typename R> friend class Future;
  template <typename R> friend class SharedFuture;
  template <typename R> friend class Promise;
};

} // namespace CesiumAsync
