#include "CesiumAsync/Impl/QueuedScheduler.h"

using namespace CesiumAsync::Impl;

void QueuedScheduler::schedule(async::task_run_handle t) {
  this->_scheduler.schedule(std::move(t));
}

void QueuedScheduler::dispatchQueuedContinuations() {
  this->immediate.markBegin();

  try {
    this->_scheduler.run_all_tasks();
  } catch (...) {
    this->immediate.markEnd();
    throw;
  }

  this->immediate.markEnd();
}

bool QueuedScheduler::dispatchZeroOrOneContinuation() {
  this->immediate.markBegin();

  try {
    bool result = this->_scheduler.try_run_one_task();
    this->immediate.markEnd();
    return result;
  } catch (...) {
    this->immediate.markEnd();
    throw;
  }
}
