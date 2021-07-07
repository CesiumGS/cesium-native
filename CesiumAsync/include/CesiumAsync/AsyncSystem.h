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

class AsyncSystem;

/**
 * @brief A promise that can be resolved or rejected by an asynchronous task.
 *
 * @tparam T The type of the object that the promise will be resolved with.
 */
template <typename T> class Promise {
public:
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
  template <typename TException> void reject(TException error) const {
    this->_pEvent->set_exception(std::make_exception_ptr(error));
  }

  /**
   * @brief Will be called when the task failed.
   *
   * @param error The error, captured with `std::current_exception`, that
   * caused the task to fail.
   */
  void reject(const std::exception_ptr& error) const {
    this->_pEvent->set_exception(error);
  }

  /**
   * @brief Gets the Future that resolves or rejects when this Promise is
   * resolved or rejected.
   *
   * This method may only be called once.
   *
   * @return The future.
   */
  Future<T> getFuture() const {
    return Future<T>(this->_pSchedulers, this->_pEvent->get_task());
  }

private:
  Promise(
      const std::shared_ptr<Impl::AsyncSystemSchedulers>& pSchedulers,
      const std::shared_ptr<async::event_task<T>>& pEvent)
      : _pSchedulers(pSchedulers), _pEvent(pEvent) {}

  std::shared_ptr<Impl::AsyncSystemSchedulers> _pSchedulers;
  std::shared_ptr<async::event_task<T>> _pEvent;

  friend class AsyncSystem;
};

// Specialization for promises that resolve to no value.
template <> class Promise<void> {
public:
  void resolve() const { this->_pEvent->set(); }
  template <typename TException> void reject(TException error) const {
    this->_pEvent->set_exception(std::make_exception_ptr(error));
  }
  void reject(const std::exception_ptr& error) const {
    this->_pEvent->set_exception(error);
  }
  Future<void> getFuture() const {
    return Future<void>(this->_pSchedulers, this->_pEvent->get_task());
  }

private:
  Promise(
      const std::shared_ptr<Impl::AsyncSystemSchedulers>& pSchedulers,
      const std::shared_ptr<async::event_task<void>>& pEvent)
      : _pSchedulers(pSchedulers), _pEvent(pEvent) {}

  std::shared_ptr<Impl::AsyncSystemSchedulers> _pSchedulers;
  std::shared_ptr<async::event_task<void>> _pEvent;

  friend class AsyncSystem;
};

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
   * This method is very similar to {@link AsyncSystem::createPromise}, except
   * that that method returns the Promise directly. The advantage of using this
   * method instead is that it is more exception-safe.  If the callback `f`
   * throws an exception, the `Future` will be rejected automatically and the
   * exception will not escape the callback.
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

    Promise<T> promise(this->_pSchedulers, pEvent);

    try {
      f(promise);
    } catch (...) {
      promise.reject(std::current_exception());
    }

    return Future<T>(this->_pSchedulers, pEvent->get_task());
  }

  /**
   * @brief Create a Promise that can be used at a later time to resolve or
   * reject a Future.
   *
   * Use {@link Promise<T>::getFuture} to get the Future that is resolved
   * or rejected when this Promise is resolved or rejected.
   *
   * Consider using {@link AsyncSystem::createFuture} instead of this method.
   *
   * @tparam T The type that is provided when resolving the Promise and the type
   * that the associated Future resolves to. Future.
   * @return The Promise.
   */
  template <typename T> Promise<T> createPromise() const {
    return Promise<T>(
        this->_pSchedulers,
        std::make_shared<async::event_task<T>>());
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

    CESIUM_TRACE_BEGIN_IN_TRACK(tracingName);

    return Impl::ContinuationFutureType_t<Func, void>(
        this->_pSchedulers,
        async::spawn(
            this->_pSchedulers->workerThread.immediate,
            Impl::WithTracing<Func, void>::end(
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

    CESIUM_TRACE_BEGIN_IN_TRACK(tracingName);

    return Impl::ContinuationFutureType_t<Func, void>(
        this->_pSchedulers,
        async::spawn(
            this->_pSchedulers->mainThread.immediate,
            Impl::WithTracing<Func, void>::end(
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

    CESIUM_TRACE_BEGIN_IN_TRACK(tracingName);

    return Impl::ContinuationFutureType_t<Func, void>(
        this->_pSchedulers,
        async::spawn(
            threadPool._pScheduler->immediate,
            Impl::WithTracing<Func, void>::end(
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
   * @brief Runs a single waiting task that is currently queued for the main
   * thread, if there is one.
   *
   * The task is run in the calling thread.
   *
   * @return true A single task was executed.
   * @return false No task was executed because none are waiting.
   */
  bool dispatchZeroOrOneMainThreadTask();

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
