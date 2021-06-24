#pragma once

#include "CesiumAsync/Future.h"
#include "CesiumAsync/Impl/ContinuationFutureType.h"
#include "CesiumAsync/Impl/RemoveFuture.h"
#include "CesiumAsync/Impl/WithTracing.h"
#include "CesiumAsync/Impl/cesium-async++.h"
#include "CesiumAsync/Library.h"
#include "CesiumAsync/ThreadPool.h"
#include "CesiumUtility/Tracing.h"
#include <memory>

namespace CesiumAsync {
class ITaskProcessor;

template <typename T> class Future;

/**
 * @brief A system for managing asynchronous requests and tasks.
 *
 * Instances of this class may be safely and efficiently stored and passed
 * around by value. However, it is essential that the _last_ AsyncSystem
 * instance be destroyed only after all continuations have run to completion.
 * Otherwise, continuations may be scheduled using invalid scheduler instances,
 * leading to a crash. Broadly, there are two ways to achieve this:
 *
 *   * Wait until all Futures complete before destroying the "owner" of the
 *     AsyncSystem.
 *   * Make the AsyncSystem a global or static local in order to extend its
 *     lifetime all the way until program termination.
 */
class CESIUMASYNC_API AsyncSystem final {
public:
  /**
   * @brief A promise that can be resolved or rejected by an asynchronous task.
   *
   * @tparam T The type of the object that the promise will be resolved with.
   */
  template <typename T> struct Promise {
    Promise(const std::shared_ptr<async::event_task<T>>& pEvent)
        : _pEvent(pEvent) {}

    /**
     * @brief Will be called when the task completed successfully.
     *
     * @param value The value that was computed by the asynchronous task.
     */
    void resolve(T&& value) const { this->_pEvent->set(std::move(value)); }

    /**
     * @brief Will be called when the task failed.
     *
     * @param error The error that caused the task to fail.
     */
    void reject(std::exception&& error) const {
      this->_pEvent->set_exception(std::make_exception_ptr(error));
    }

  private:
    std::shared_ptr<async::event_task<T>> _pEvent;
  };

  /**
   * @brief Constructs a new instance.
   *
   * @param pTaskProcessor The interface used to run tasks in background
   * threads.
   */
  AsyncSystem(const std::shared_ptr<ITaskProcessor>& pTaskProcessor) noexcept;

  /**
   * @brief Creates a new Future by immediately invoking a function and giving
   * it the opportunity to resolve or reject a {@link Promise}.
   *
   * The {@link Promise} passed to the callback `f` may be resolved or rejected
   * asynchronously, even after the function has returned.
   *
   * If the callback `f` throws an exception, the `Future` will be rejected.
   *
   * @tparam T The type that the Future resolves to.
   * @tparam Func The type of the callback function.
   * @param f The callback function to invoke immediately to create the Future.
   * @return A Future that will resolve when the callback function resolves the
   * supplied Promise.
   */
  template <typename T, typename Func> Future<T> createFuture(Func&& f) const {
    std::shared_ptr<async::event_task<T>> pEvent =
        std::make_shared<async::event_task<T>>();

    Promise<T> promise(pEvent);

    try {
      f(promise);
    } catch (std::exception& e) {
      promise.reject(std::move(e));
    } catch (...) {
      promise.reject(std::runtime_error("Unknown error"));
    }

    return Future<T>(this->_pSchedulers, pEvent->get_task());
  }

  /**
   * @brief Runs a function in a worker thread, returning a Future that
   * resolves when the function completes.
   *
   * If the function itself returns a `Future`, the function will not be
   * considered complete until that returned `Future` also resolves.
   *
   * If this method is called from a designated worker thread, the
   * callback will be invoked immediately and complete before this function
   * returns.
   *
   * @tparam Func The type of the function.
   * @param f The function.
   * @return A future that resolves after the supplied function completes.
   */
  template <typename Func>
  Impl::ContinuationFutureType_t<Func, void> runInWorkerThread(Func&& f) const {
    static const char* tracingName = "waiting for worker thread";
#if CESIUM_TRACING_ENABLED
    int64_t tracingID = CESIUM_TRACE_CURRENT_ASYNC_ID();
    if (tracingID >= 0) {
      CESIUM_TRACE_BEGIN_ID(tracingName, tracingID);
    }
#endif

    return Impl::ContinuationFutureType_t<Func, void>(
        this->_pSchedulers,
        async::spawn(
            this->_pSchedulers->immediatelyInWorkerThreadScheduler,
            Impl::WithTracing<Func, void>::wrap(
                tracingName,
                std::forward<Func>(f))));
  }

  /**
   * @brief Runs a function in the main thread, returning a Future that
   * resolves when the function completes.
   *
   * If the function itself returns a `Future`, the function will not be
   * considered complete until that returned `Future` also resolves.
   *
   * If this method is called from the main thread, the callback will be invoked
   * immediately and complete before this function returns.
   *
   * @tparam Func The type of the function.
   * @param f The function.
   * @return A future that resolves after the supplied function completes.
   */
  template <typename Func>
  Impl::ContinuationFutureType_t<Func, void> runInMainThread(Func&& f) const {
    static const char* tracingName = "waiting for main thread";
#if CESIUM_TRACING_ENABLED
    int64_t tracingID = CESIUM_TRACE_CURRENT_ASYNC_ID();
    if (tracingID >= 0) {
      CESIUM_TRACE_BEGIN_ID(tracingName, tracingID);
    }
#endif

    return Impl::ContinuationFutureType_t<Func, void>(
        this->_pSchedulers,
        async::spawn(
            this->_pSchedulers->mainThreadScheduler,
            Impl::WithTracing<Func, void>::wrap(
                tracingName,
                std::forward<Func>(f))));
  }

  /**
   * @brief Runs a function in a thread pool, returning a Future that resolves
   * when the function completes.
   *
   * @tparam Func The type of the function.
   * @param threadPool The thread pool in which to run the function.
   * @param f The function to run.
   * @return A future that resolves after the supplied function completes.
   */
  template <typename Func>
  Impl::ContinuationFutureType_t<Func, void>
  runInThreadPool(const ThreadPool& threadPool, Func&& f) const {
    static const char* tracingName = "waiting for thread pool";
#if CESIUM_TRACING_ENABLED
    int64_t tracingID = CESIUM_TRACE_CURRENT_ASYNC_ID();
    if (tracingID >= 0) {
      CESIUM_TRACE_BEGIN_ID(tracingName, tracingID);
    }
#endif

    return Impl::ContinuationFutureType_t<Func, void>(
        this->_pSchedulers,
        async::spawn(
            *threadPool._pScheduler,
            Impl::WithTracing<Func, void>::wrap(
                tracingName,
                std::forward<Func>(f))));
  }

  /**
   * @brief Creates a future that is already resolved.
   *
   * @tparam T The type of the future.
   * @param value The value for the future.
   * @return The future.
   */
  template <typename T> Future<T> createResolvedFuture(T&& value) const {
    return Future<T>(
        this->_pSchedulers,
        async::make_task<T>(std::forward<T>(value)));
  }

  /**
   * @brief Creates a future that is already resolved and resolves to no value.
   *
   * @return The future.
   */
  Future<void> createResolvedFuture() const {
    return Future<void>(this->_pSchedulers, async::make_task());
  }

  /**
   * @brief Runs all tasks that are currently queued for the main thread.
   *
   * The tasks are run in the calling thread.
   */
  void dispatchMainThreadTasks();

  /**
   * @brief Creates a new thread pool that can be used to run continuations.
   *
   * @param numberOfThreads The number of threads in the pool.
   * @return The thread pool.
   */
  ThreadPool createThreadPool(int32_t numberOfThreads) const;

private:
  std::shared_ptr<Impl::AsyncSystemSchedulers> _pSchedulers;

  template <typename T> friend class Future;
};
} // namespace CesiumAsync
