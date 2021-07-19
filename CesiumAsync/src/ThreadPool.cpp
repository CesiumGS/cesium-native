#include "CesiumAsync/ThreadPool.h"

using namespace CesiumAsync;

ThreadPool::ThreadPool(int32_t numberOfThreads)
    : _pScheduler(std::make_shared<Scheduler>(numberOfThreads)) {}

ThreadPool::Scheduler::Scheduler(int32_t numberOfThreads)
    : scheduler(
          size_t(numberOfThreads <= 0 ? 1 : numberOfThreads),
          createPreRun(this),
          createPostRun(this)) {}

void ThreadPool::Scheduler::schedule(async::task_run_handle t) {
  this->scheduler.schedule(std::move(t));
}
