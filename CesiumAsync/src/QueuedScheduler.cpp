#include "CesiumAsync/Impl/QueuedScheduler.h"

#include <condition_variable>
#include <mutex>

// Hackily use Async++'s internal fifo_queue. We could copy it instead - it's
// not much code - but why create the duplication? However, we are assuming that
// Async++'s source (not just headers) are available while building
// cesium-native. If that's a problem in some context, we'll need to do that
// duplication of fifo_queue after all.
#include <async++/../../src/fifo_queue.h>

namespace CesiumAsync::CesiumImpl {

struct QueuedScheduler::Impl {
  async::detail::fifo_queue queue;
  std::mutex mutex;
  std::condition_variable conditionVariable;
};

QueuedScheduler::QueuedScheduler() : _pImpl(std::make_unique<Impl>()) {}
QueuedScheduler::~QueuedScheduler() = default;

void QueuedScheduler::schedule(async::task_run_handle t) {
  std::unique_lock<std::mutex> guard(this->_pImpl->mutex);
  this->_pImpl->queue.push(std::move(t));

  // Notify listeners that there is new work.
  this->_pImpl->conditionVariable.notify_all();
}

void QueuedScheduler::dispatchQueuedContinuations() {
  while (this->dispatchZeroOrOneContinuation()) {
  }
}

bool QueuedScheduler::dispatchZeroOrOneContinuation() {
  return this->dispatchInternal(false);
}

bool QueuedScheduler::dispatchInternal(bool blockIfNoTasks) {
  async::task_run_handle t;

  {
    std::unique_lock<std::mutex> guard(this->_pImpl->mutex);
    t = this->_pImpl->queue.pop();
    if (blockIfNoTasks && !t) {
      this->_pImpl->conditionVariable.wait(guard);
    }
  }

  if (t) {
    auto scope = this->immediate.scope();
    t.run();
    return true;
  } else {
    return false;
  }
}

void QueuedScheduler::unblock() {
  std::unique_lock<std::mutex> guard(this->_pImpl->mutex);
  this->_pImpl->conditionVariable.notify_all();
}

} // namespace CesiumAsync::CesiumImpl
