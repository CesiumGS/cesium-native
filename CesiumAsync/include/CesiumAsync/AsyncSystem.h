#pragma once

#include "CesiumAsync/Library.h"
#include "CesiumUtility/Profiler.h"
#include <memory>
#include <string>
#include <variant>
#include <vector>

#pragma warning(push)

#pragma warning(disable : 4458 4324)

#ifndef _MSC_VER
#pragma GCC diagnostic ignored "-Wshadow"
#endif

#include <async++.h>

#pragma warning(pop)

namespace CesiumAsync {
class ITaskProcessor;

template <class T> class Future;

namespace Impl {
// Begin omitting doxgen warnings for Impl namespace
//! @cond Doxygen_Suppress

struct AsyncSystemSchedulers {
  AsyncSystemSchedulers(std::shared_ptr<ITaskProcessor> pTaskProcessor);

  std::shared_ptr<ITaskProcessor> pTaskProcessor;
  async::fifo_scheduler mainThreadScheduler;

  void schedule(async::task_run_handle t);
};

template <typename T> struct RemoveFuture { typedef T type; };
template <typename T> struct RemoveFuture<Future<T>> { typedef T type; };
template <typename T> struct RemoveFuture<const Future<T>> { typedef T type; };
template <typename T> struct RemoveFuture<async::task<T>> { typedef T type; };
template <typename T> struct RemoveFuture<const async::task<T>> {
  typedef T type;
};

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
          typename std::invoke_result<Func, T>::type,
          typename Impl::RemoveFuture<
              typename std::invoke_result<Func, T>::type>::type>::value,
      IdentityUnwrapper<Func>,
      ParameterizedTaskUnwrapper<Func, T>>::type::unwrap(std::forward<Func>(f));
}

template <class Func> auto unwrapFuture(Func&& f) {
  return std::conditional<
      std::is_same<
          typename std::invoke_result<Func>::type,
          typename Impl::RemoveFuture<
              typename std::invoke_result<Func>::type>::type>::value,
      IdentityUnwrapper<Func>,
      TaskUnwrapper<Func>>::type::unwrap(std::forward<Func>(f));
}

template <class Func, class T>
auto withTracing(const char* tracingName, Func&& f) {
#if TRACING_ENABLED
  int64_t tracingID = CesiumUtility::Profiler::instance().getEnlistedID();
  return [tracingID,
          tracingName,
          f = Impl::unwrapFuture<Func, T>(std::forward<Func>(f))](
             T&& result) mutable {
    TRACE_ASYNC_ENLIST(tracingID);
    if (tracingName && tracingID >= 0) {
      TRACE_ASYNC_END_ID(tracingName, tracingID);
    }
    return f(std::move(result));
  };
#else
  return Impl::unwrapFuture<Func, T>(std::forward<Func>(f));
#endif
}

template <class Func> auto withTracing(const char* tracingName, Func&& f) {
#if TRACING_ENABLED
  int64_t tracingID = CesiumUtility::Profiler::instance().getEnlistedID();
  return [tracingID,
          tracingName,
          f = Impl::unwrapFuture<Func>(std::forward<Func>(f))]() mutable {
    TRACE_ASYNC_ENLIST(tracingID);
    if (tracingName && tracingID >= 0) {
      TRACE_ASYNC_END_ID(tracingName, tracingID);
    }
    return f();
  };
#else
  return Impl::unwrapFuture<Func>(std::forward<Func>(f));
#endif
}

//! @endcond
// End omitting doxgen warnings for Impl namespace
} // namespace Impl

/**
 * @brief A value that will be available in the future, as produced by
 * {@link AsyncSystem}.
 *
 * @tparam T The type of the value.
 */
template <class T> class Future final {
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
   * @tparam Func The type of the function.
   * @param f The function.
   * @return A future that resolves after the supplied function completes.
   */
  template <class Func>
  Future<typename Impl::RemoveFuture<
      typename std::invoke_result<Func, T>::type>::type>
  thenInWorkerThread(Func&& f) && {
    // When tracing is enabled, we measure the time between scheduling and
    // dispatching of the work.
#if TRACING_ENABLED
    static const char* tracingName = "waiting for worker thread";
    int64_t tracingID = CesiumUtility::Profiler::instance().getEnlistedID();
#endif

    return Future<typename Impl::RemoveFuture<
        typename std::invoke_result<Func, T>::type>::type>(
        this->_pSchedulers,
        this->_task
#if TRACING_ENABLED
            .then(
                async::inline_scheduler(),
                [tracingID](T&& value) mutable {
                  if (tracingID >= 0) {
                    TRACE_ASYNC_BEGIN_ID(tracingName, tracingID);
                  }
                  return std::move(value);
                })
#endif
            .then(
                *this->_pSchedulers,
                Impl::withTracing<Func, T>(
                    tracingName,
                    std::forward<Func>(f))));
  }

  /**
   * @brief Registers a continuation function to be invoked in the main thread
   * when this Future resolves, and invalidates this Future.
   *
   * The supplied function will not be invoked immediately, even if this method
   * is called from the main thread. Instead, it will be queued and invoked the
   * next time {@link AsyncSystem::dispatchMainThreadTasks} is called.
   *
   * If the function itself returns a `Future`, the function will not be
   * considered complete until that returned `Future` also resolves.
   *
   * @tparam Func The type of the function.
   * @param f The function.
   * @return A future that resolves after the supplied function completes.
   */
  template <class Func>
  Future<typename Impl::RemoveFuture<
      typename std::invoke_result<Func, T>::type>::type>
  thenInMainThread(Func&& f) && {
    // When tracing is enabled, we measure the time between scheduling and
    // dispatching of the work.
#if TRACING_ENABLED
    static const char* tracingName = "waiting for main thread";
    int64_t tracingID = CesiumUtility::Profiler::instance().getEnlistedID();
#endif

    return Future<typename Impl::RemoveFuture<
        typename std::invoke_result<Func, T>::type>::type>(
        this->_pSchedulers,
        this->_task
#if TRACING_ENABLED
            .then(
                async::inline_scheduler(),
                [tracingID](T&& value) mutable {
                  if (tracingID >= 0) {
                    TRACE_ASYNC_BEGIN_ID(tracingName, tracingID);
                  }
                  return std::move(value);
                })
#endif
            .then(
                this->_pSchedulers->mainThreadScheduler,
                Impl::withTracing<Func, T>(
                    tracingName,
                    std::forward<Func>(f))));
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
  template <class Func>
  Future<typename Impl::RemoveFuture<
      typename std::invoke_result<Func, T>::type>::type>
  thenImmediately(Func&& f) && {
    return Future<typename Impl::RemoveFuture<
        typename std::invoke_result<Func, T>::type>::type>(
        this->_pSchedulers,
        _task.then(
            async::inline_scheduler(),
            Impl::withTracing<Func, T>(nullptr, std::forward<Func>(f))));
  }

  /**
   * @brief Registers a continuation function to be invoked in the main thread
   * when this Future rejects, and invalidates this Future.
   *
   * The supplied function will not be invoked immediately, even if this method
   * is called from the main thread. Instead, it will be queued and invoked the
   * next time {@link AsyncSystem::dispatchMainThreadTasks} is called.
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
  template <class Func> Future<T> catchInMainThread(Func&& f) && {
    // When tracing is enabled, we measure the time between scheduling and
    // dispatching of the work.
#if TRACING_ENABLED
    static const char* tracingName = "waiting for main thread catch";
    int64_t tracingID = CesiumUtility::Profiler::instance().getEnlistedID();
#endif

    auto catcher = [f = std::forward<Func>(f)](async::task<T>&& t) mutable {
      try {
        return t.get();
      } catch (std::exception& e) {
        return f(std::move(e));
      } catch (...) {
        return f(std::runtime_error("Unknown exception"));
      }
    };

    return Future<T>(
        this->_pSchedulers,
        _task
#if TRACING_ENABLED
            .then(
                async::inline_scheduler(),
                [tracingID](async::task<T>&& t) mutable {
                  if (tracingID >= 0) {
                    TRACE_ASYNC_BEGIN_ID(tracingName, tracingID);
                  }
                  return std::move(t);
                })
#endif
            .then(
                this->_pSchedulers->mainThreadScheduler,
                Impl::withTracing<decltype(catcher), async::task<T>>(
                    tracingName,
                    std::move(catcher))));
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
  std::variant<T, std::exception> wait() {
    try {
      return this->_task.get();
    } catch (std::exception& e) {
      return e;
    } catch (...) {
      return std::runtime_error("Unknown exception.");
    }
  }

private:
  Future(
      std::shared_ptr<Impl::AsyncSystemSchedulers> pSchedulers,
      async::task<T>&& task) noexcept
      : _pSchedulers(pSchedulers), _task(std::move(task)) {}

  std::shared_ptr<Impl::AsyncSystemSchedulers> _pSchedulers;
  async::task<T> _task;

  friend class AsyncSystem;

  template <class Func, class R> friend struct Impl::ParameterizedTaskUnwrapper;

  template <class Func> friend struct Impl::TaskUnwrapper;

  template <class R> friend class Future;
};

/**
 * @brief A system for managing asynchronous requests and tasks.
 *
 * Instances of this class may be safely and efficiently stored and passed
 * around by value.
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

  template <class T, class Func> Future<T> createFuture(Func&& f) const {
    std::shared_ptr<async::event_task<T>> pEvent =
        std::make_shared<async::event_task<T>>();

    Promise<T> promise(pEvent);
    f(promise);

    return Future<T>(this->_pSchedulers, pEvent->get_task());
  }

  /**
   * @brief Runs a function in a worker thread, returning a promise that
   * resolves when the function completes.
   *
   * If the function itself returns a `Future`, the function will not be
   * considered complete until that returned `Future` also resolves.
   *
   * @tparam Func The type of the function.
   * @param f The function.
   * @return A future that resolves after the supplied function completes.
   */
  template <class Func>
  Future<typename Impl::RemoveFuture<
      typename std::invoke_result<Func>::type>::type>
  runInWorkerThread(Func&& f) const {
#if TRACING_ENABLED
    static const char* tracingName = "waiting for worker thread";
    int64_t tracingID = CesiumUtility::Profiler::instance().getEnlistedID();
    if (tracingID >= 0) {
      TRACE_ASYNC_BEGIN_ID(tracingName, tracingID);
    }
#endif

    return Future<typename Impl::RemoveFuture<
        typename std::invoke_result<Func>::type>::type>(
        this->_pSchedulers,
        async::spawn(
            *this->_pSchedulers,
            Impl::withTracing<Func>(tracingName, std::forward<Func>(f))));
  }

  /**
   * @brief Runs a function in the main thread, returning a promise that
   * resolves when the function completes.
   *
   * The supplied function will not be called immediately, even if this method
   * is invoked from the main thread. Instead, it will be queued and called the
   * next time {@link dispatchMainThreadTasks} is called.
   *
   * If the function itself returns a `Future`, the function will not be
   * considered complete until that returned `Future` also resolves.
   *
   * @tparam Func The type of the function.
   * @param f The function.
   * @return A future that resolves after the supplied function completes.
   */
  template <class Func>
  Future<typename Impl::RemoveFuture<
      typename std::invoke_result<Func>::type>::type>
  runInMainThread(Func&& f) const {
#if TRACING_ENABLED
    static const char* tracingName = "waiting for main thread";
    int64_t tracingID = CesiumUtility::Profiler::instance().getEnlistedID();
    if (tracingID >= 0) {
      TRACE_ASYNC_BEGIN_ID(tracingName, tracingID);
    }
#endif

    return Future<typename Impl::RemoveFuture<
        typename std::invoke_result<Func>::type>::type>(
        this->_pSchedulers,
        async::spawn(
            this->_pSchedulers->mainThreadScheduler,
            Impl::withTracing<Func>(tracingName, std::forward<Func>(f))));
  }

  /**
   * @brief Creates a future that is already resolved.
   *
   * @tparam T The type of the future
   * @param value The value for the future
   * @return The future
   */
  template <class T> Future<T> createResolvedFuture(T&& value) const {
    return Future<T>(
        this->_pSchedulers,
        async::make_task<T>(std::forward<T>(value)));
  }

  Future<void> createResolvedFuture() const {
    return Future<void>(this->_pSchedulers, async::make_task());
  }

  /**
   * @brief Runs all tasks that are currently queued for the main thread.
   *
   * The tasks are run in the calling thread.
   */
  void dispatchMainThreadTasks();

private:
  std::shared_ptr<Impl::AsyncSystemSchedulers> _pSchedulers;

  template <class T> friend class Future;
};
} // namespace CesiumAsync
