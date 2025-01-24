#pragma once

#include "ImmediateScheduler.h"
#include "cesium-async++.h"

#include <atomic>

namespace CesiumAsync {
// Begin omitting doxygen warnings for Impl namespace
//! @cond Doxygen_Suppress
namespace CesiumImpl {

class QueuedScheduler {
public:
  QueuedScheduler();
  ~QueuedScheduler();

  void schedule(async::task_run_handle t);
  void dispatchQueuedContinuations();
  bool dispatchZeroOrOneContinuation();

  template <typename T> T dispatchUntilTaskCompletes(async::task<T>&& task) {
    // Set up a continuation to unblock the blocking dispatch when this task
    // completes.
    //
    // We use the `isDone` flag as the loop termination condition to
    // avoid a race condition that can lead to a deadlock. If we used
    // `unblockTask.ready()` as the termination condition instead, then it's
    // possible for events to happen as follows:
    //
    // 1. The original `task` completes in a worker thread and the `unblockTask`
    // continuation is invoked immediately in the same thread.
    // 2. The unblockTask continuation calls `unblock`, which terminates the
    // `wait` on the condition variable in the main thread.
    // 3. The main thread resumes and the while loop in this function spins back
    // around and evaluates `unblockTask.ready()`. This returns false because
    // the unblockTask continuation has not actually finished running in the
    // worker thread yet. The main thread starts waiting on the condition
    // variable again.
    // 4. The `unblockTask` continuation finally finishes, making
    // `unblockTask.ready()` return true, but it's too late. The main thread is
    // already waiting on the condition variable.
    //
    // By setting the atomic `isDone` flag before calling `unblock`, we ensure
    // that the loop termination condition is satisfied before the main thread
    // is awoken, avoiding the potential deadlock.

    std::atomic<bool> isDone = false;
    async::task<T> unblockTask = task.then(
        async::inline_scheduler(),
        [this, &isDone](async::task<T>&& task) {
          isDone = true;
          this->unblock();
          return task.get();
        });

    while (!isDone) {
      this->dispatchInternal(true);
    }

    return std::move(unblockTask).get();
  }

  template <typename T>
  T dispatchUntilTaskCompletes(const async::shared_task<T>& task) {
    // Set up a continuation to unblock the blocking dispatch when this task
    // completes. This case is simpler than the one above because a SharedFuture
    // supports multiple continuations. We can use readiness of the _original_
    // task to terminate the loop while unblocking in a separate continuation
    // guaranteed to run only after that termination condition is satisfied.
    async::task<void> unblockTask = task.then(
        async::inline_scheduler(),
        [this](const async::shared_task<T>&) { this->unblock(); });

    while (!task.ready()) {
      this->dispatchInternal(true);
    }

    return task.get();
  }

  ImmediateScheduler<QueuedScheduler> immediate{this};

private:
  bool dispatchInternal(bool blockIfNoTasks);
  void unblock();

  struct Impl;
  std::unique_ptr<Impl> _pImpl;
};
//! @endcond
// End omitting doxygen warnings for Impl namespace

} // namespace CesiumImpl
} // namespace CesiumAsync
