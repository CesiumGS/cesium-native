#pragma once

#include "Impl/ImmediateScheduler.h"
#include "Impl/cesium-async++.h"

#include <CesiumAsync/Library.h>

#include <memory>

namespace CesiumAsync {

/**
 * @brief A thread pool created by {@link AsyncSystem::createThreadPool}.
 *
 * This object has no public methods, but can be used with
 * {@link AsyncSystem::runInThreadPool} and
 * {@link Future::thenInThreadPool}.
 */
class CESIUMASYNC_API ThreadPool {
public:
  /**
   * @brief Creates a new thread pool with the given number of threads.
   *
   * @param numberOfThreads The number of threads to create in this ThreadPool.
   */
  ThreadPool(int32_t numberOfThreads);

private:
  struct Scheduler {
    Scheduler(int32_t numberOfThreads);
    void schedule(async::task_run_handle t);

    CesiumImpl::ImmediateScheduler<Scheduler> immediate{this};

    async::threadpool_scheduler scheduler;
  };

  static auto createPreRun(ThreadPool::Scheduler* pScheduler) {
    return
        [pScheduler]() { ThreadPool::_scope = pScheduler->immediate.scope(); };
  }

  static auto createPostRun() noexcept {
    return []() noexcept { ThreadPool::_scope.reset(); };
  }

  static thread_local CesiumImpl::ImmediateScheduler<Scheduler>::SchedulerScope
      _scope;

  std::shared_ptr<Scheduler> _pScheduler;

  template <typename T> friend class Future;
  template <typename T> friend class SharedFuture;
  friend class AsyncSystem;
};

} // namespace CesiumAsync
