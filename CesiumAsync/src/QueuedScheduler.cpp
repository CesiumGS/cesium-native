#include "CesiumAsync/Impl/QueuedScheduler.h"

#include <async++.h>

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <utility>

namespace {

// This class is copied from async++'s fifo_queue.h:
// https://github.com/Amanieu/asyncplusplus/blob/4159da79e20ad6d0eb1f13baa0f10e989edd9fba/src/fifo_queue.h
// It's private within Async++, meaning it is defined in a header in the src
// directory rather than the include directory. When we use Async++ via vcpkg,
// we have no way to access files in the src directory. So, we copy it here
// instead.
class fifo_queue {
  async::detail::aligned_array<void*, LIBASYNC_CACHELINE_SIZE> items;
  std::size_t head, tail;

public:
  fifo_queue() : items(32), head(0), tail(0) {}
  ~fifo_queue() {
    // Free any unexecuted tasks
    for (std::size_t i = head; i != tail; i = (i + 1) & (items.size() - 1))
      async::task_run_handle::from_void_ptr(items[i]);
  }

  // Push a task to the end of the queue
  void push(async::task_run_handle t) {
    // Resize queue if it is full
    if (head == ((tail + 1) & (items.size() - 1))) {
      async::detail::aligned_array<void*, LIBASYNC_CACHELINE_SIZE> new_items(
          items.size() * 2);
      for (std::size_t i = 0; i != items.size(); i++)
        new_items[i] = items[(i + head) & (items.size() - 1)];
      head = 0;
      tail = items.size() - 1;
      items = std::move(new_items);
    }

    // Push the item
    items[tail] = t.to_void_ptr();
    tail = (tail + 1) & (items.size() - 1);
  }

  // Pop a task from the front of the queue
  async::task_run_handle pop() {
    // See if an item is available
    if (head == tail)
      return async::task_run_handle();
    else {
      void* x = items[head];
      head = (head + 1) & (items.size() - 1);
      return async::task_run_handle::from_void_ptr(x);
    }
  }
};

} // namespace

namespace CesiumAsync::CesiumImpl {

struct QueuedScheduler::Impl {
  fifo_queue queue;
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
