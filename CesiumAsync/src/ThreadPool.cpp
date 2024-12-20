#include "CesiumAsync/Impl/ImmediateScheduler.h"

#include <CesiumAsync/ThreadPool.h>

#include <async++.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

using namespace CesiumAsync;

// Each thread may be enrolled in a single scheduler scope.
// That means the thread is doing work on behalf of the scheduler.
/*static*/ thread_local CesiumImpl::ImmediateScheduler<
    ThreadPool::Scheduler>::SchedulerScope ThreadPool::_scope;

ThreadPool::ThreadPool(int32_t numberOfThreads)
    : _pScheduler(std::make_shared<Scheduler>(numberOfThreads)) {}

ThreadPool::Scheduler::Scheduler(int32_t numberOfThreads)
    : scheduler(
          size_t(numberOfThreads <= 0 ? 1 : numberOfThreads),
          createPreRun(this),
          createPostRun()) {}

void ThreadPool::Scheduler::schedule(async::task_run_handle t) {
  this->scheduler.schedule(std::move(t));
}
