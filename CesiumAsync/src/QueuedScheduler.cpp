#include "CesiumAsync/Impl/QueuedScheduler.h"

using namespace CesiumAsync::CesiumImpl;

void QueuedScheduler::schedule(async::task_run_handle t) {
  this->_scheduler.schedule(std::move(t));
}

void QueuedScheduler::dispatchQueuedContinuations() {
  auto scope = this->immediate.scope();
  this->_scheduler.run_all_tasks();
}

bool QueuedScheduler::dispatchZeroOrOneContinuation() {
  auto scope = this->immediate.scope();
  return this->_scheduler.try_run_one_task();
}
