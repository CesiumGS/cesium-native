#include "CesiumAsync/ThreadPool.h"

using namespace CesiumAsync;

/*static*/ thread_local std::vector<ThreadPool::Scheduler*>
    ThreadPool::schedulersInThreadPoolThread;

ThreadPool::ThreadPool(int32_t numberOfThreads)
    : _pScheduler(std::make_shared<Scheduler>(numberOfThreads)) {}

ThreadPool::Scheduler::Scheduler(int32_t numberOfThreads)
    : scheduler(size_t(numberOfThreads <= 0 ? 1 : numberOfThreads)) {}

void ThreadPool::Scheduler::schedule(async::task_run_handle t) {
  // Are we already in a thread pool thread?
  const std::vector<ThreadPool::Scheduler*>& inPool =
      ThreadPool::schedulersInThreadPoolThread;
  if (std::find(inPool.begin(), inPool.end(), this) != inPool.end()) {
    // Yes, run this task directly.
    t.run();
  } else {
    // No, schedule this thread in a worker thread.
    this->scheduler.schedule(std::move(t));
  }
}
