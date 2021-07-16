#pragma once

#include "CesiumAsync/Impl/ImmediateScheduler.h"
#include "CesiumAsync/Impl/cesium-async++.h"
#include "CesiumAsync/Library.h"
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
  ThreadPool(int32_t numberOfThreads);

private:
  struct Scheduler {
    Scheduler(int32_t numberOfThreads);
    void schedule(async::task_run_handle t);

    Impl::ImmediateScheduler<Scheduler> immediate{this};

    async::threadpool_scheduler scheduler;
  };

  static auto createPreRun(ThreadPool::Scheduler* pScheduler) {
    return
        [pScheduler]() { ThreadPool::_scope = pScheduler->immediate.scope(); };
  }

  static auto createPostRun() {
    return []() { ThreadPool::_scope.reset(); };
  }

  static thread_local Impl::ImmediateScheduler<Scheduler>::SchedulerScope
      _scope;

  std::shared_ptr<Scheduler> _pScheduler;

  template <typename T> friend class Future;
  template <typename T> friend class SharedFuture;
  friend class AsyncSystem;
};

} // namespace CesiumAsync
