#pragma once

#include "Impl/ContinuationFutureType.h"
#include "Impl/RemoveFuture.h"
#include "Impl/WithTracing.h"
#include "Impl/cesium-async++.h"

#include <CesiumAsync/Future.h>
#include <CesiumAsync/Library.h>
#include <CesiumAsync/Promise.h>
#include <CesiumAsync/ThreadPool.h>
#include <CesiumUtility/Tracing.h>

#include <memory>
#include <type_traits>

namespace CesiumAsync {
class ITaskProcessor;

class AsyncSystem;

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
  CesiumImpl::ContinuationFutureType_t<Func, void>
  runInWorkerThread(Func&& f) const {
    static const char* tracingName = "waiting for worker thread";

    CESIUM_TRACE_BEGIN_IN_TRACK(tracingName);

    return CesiumImpl::ContinuationFutureType_t<Func, void>(
        this->_pSchedulers,
        async::spawn(
            this->_pSchedulers->workerThread.immediate,
            CesiumImpl::WithTracing<void>::end(
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
  CesiumImpl::ContinuationFutureType_t<Func, void>
  runInMainThread(Func&& f) const {
    static const char* tracingName = "waiting for main thread";

    CESIUM_TRACE_BEGIN_IN_TRACK(tracingName);

    return CesiumImpl::ContinuationFutureType_t<Func, void>(
        this->_pSchedulers,
        async::spawn(
            this->_pSchedulers->mainThread.immediate,
            CesiumImpl::WithTracing<void>::end(
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
  CesiumImpl::ContinuationFutureType_t<Func, void>
  runInThreadPool(const ThreadPool& threadPool, Func&& f) const {
    static const char* tracingName = "waiting for thread pool";

    CESIUM_TRACE_BEGIN_IN_TRACK(tracingName);

    return CesiumImpl::ContinuationFutureType_t<Func, void>(
        this->_pSchedulers,
        async::spawn(
            threadPool._pScheduler->immediate,
            CesiumImpl::WithTracing<void>::end(
                tracingName,
                std::forward<Func>(f))));
  }

  /**
   * @brief The value type of the Future returned by {@link all}.
   *
   * This will be either `std::vector<T>`, if the input Futures passed to the
   * `all` function return values, or `void` if they do not.
   *
   * @tparam T The value type of the input Futures passed to the function.
   */
  template <typename T>
  using AllValueType =
      std::conditional_t<std::is_void_v<T>, void, std::vector<T>>;

  /**
   * @brief Creates a Future that resolves when every Future in a vector
   * resolves, and rejects when any Future in the vector rejects.
   *
   * If the input Futures resolve to non-void values, the returned Future
   * resolves to a vector of the values, in the same order as the input Futures.
   * If the input Futures resolve to void, the returned Future resolves to void
   * as well.
   *
   * If any of the Futures rejects, the returned Future rejects as well. The
   * exception included in the rejection will be from the first Future in the
   * vector that rejects.
   *
   * To get detailed rejection information from each of the Futures,
   * attach a `catchInMainThread` continuation prior to passing the
   * list into `all`.
   *
   * @tparam T The type that each Future resolves to.
   * @param futures The list of futures.
   * @return A Future that resolves when all the given Futures resolve, and
   * rejects when any Future in the vector rejects.
   */
  template <typename T>
  Future<AllValueType<T>> all(std::vector<Future<T>>&& futures) const {
    return this->all<T, Future<T>>(
        std::forward<std::vector<Future<T>>>(futures));
  }

  /**
   * @brief Creates a Future that resolves when every Future in a vector
   * resolves, and rejects when any Future in the vector rejects.
   *
   * If the input SharedFutures resolve to non-void values, the returned Future
   * resolves to a vector of the values, in the same order as the input
   * SharedFutures. If the input SharedFutures resolve to void, the returned
   * Future resolves to void as well.
   *
   * If any of the SharedFutures rejects, the returned Future rejects as well.
   * The exception included in the rejection will be from the first SharedFuture
   * in the vector that rejects.
   *
   * To get detailed rejection information from each of the SharedFutures,
   * attach a `catchInMainThread` continuation prior to passing the
   * list into `all`.
   *
   * @tparam T The type that each SharedFuture resolves to.
   * @param futures The list of shared futures.
   * @return A Future that resolves when all the given SharedFutures resolve,
   * and rejects when any SharedFuture in the vector rejects.
   */
  template <typename T>
  Future<AllValueType<T>> all(std::vector<SharedFuture<T>>&& futures) const {
    return this->all<T, SharedFuture<T>>(
        std::forward<std::vector<SharedFuture<T>>>(futures));
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
   * thread. If there are no tasks waiting, it returns immediately without
   * running any tasks.
   *
   * The task is run in the calling thread.
   *
   * @return true A single task was executed.
   * @return false No task was executed because none are waiting.
   */
  bool dispatchOneMainThreadTask();

  /**
   * @brief Creates a new thread pool that can be used to run continuations.
   *
   * @param numberOfThreads The number of threads in the pool.
   * @return The thread pool.
   */
  ThreadPool createThreadPool(int32_t numberOfThreads) const;

  /**
   * Returns true if this instance and the right-hand side can be used
   * interchangeably because they schedule continuations identically. Otherwise,
   * returns false.
   */
  bool operator==(const AsyncSystem& rhs) const noexcept;

  /**
   * Returns true if this instance and the right-hand side can _not_ be used
   * interchangeably because they schedule continuations differently. Otherwise,
   * returns false.
   */
  bool operator!=(const AsyncSystem& rhs) const noexcept;

private:
  // Common implementation of 'all' for both Future and SharedFuture.
  template <typename T, typename TFutureType>
  Future<AllValueType<T>> all(std::vector<TFutureType>&& futures) const {
    using TTaskType = decltype(TFutureType::_task);
    std::vector<TTaskType> tasks;
    tasks.reserve(futures.size());

    for (auto it = futures.begin(); it != futures.end(); ++it) {
      tasks.emplace_back(std::move(it->_task));
    }

    futures.clear();

    async::task<AllValueType<T>> task =
        async::when_all(tasks.begin(), tasks.end())
            .then(
                async::inline_scheduler(),
                [](std::vector<TTaskType>&& tasks) {
                  if constexpr (std::is_void_v<T>) {
                    // Tasks return void. "Get" each task so that error
                    // information is propagated.
                    for (auto it = tasks.begin(); it != tasks.end(); ++it) {
                      it->get();
                    }
                  } else {
                    // Get all the results. If any tasks rejected, we'll bail
                    // with an exception.
                    std::vector<T> results;
                    results.reserve(tasks.size());

                    for (auto it = tasks.begin(); it != tasks.end(); ++it) {
                      results.emplace_back(std::move(it->get()));
                    }
                    return results;
                  }
                });
    return Future<AllValueType<T>>(this->_pSchedulers, std::move(task));
  }

  std::shared_ptr<CesiumImpl::AsyncSystemSchedulers> _pSchedulers;

  template <typename T> friend class Future;
};
} // namespace CesiumAsync
