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
    //
    // `waitAndDispatchOne` re-checks `isDone` while holding the mutex before it
    // waits, so a completion that lands between the loop's check and the wait
    // is not missed.

    std::atomic<bool> isDone = false;
    async::task<T> unblockTask = task.then(
        async::inline_scheduler(),
        [this, &isDone](async::task<T>&& task) {
          isDone = true;
          this->unblock();
          return task.get();
        });

    while (!isDone) {
      this->waitAndDispatchOne(isDone);
    }

    return std::move(unblockTask).get();
  }

  template <typename T>
  T dispatchUntilTaskCompletes(const async::shared_task<T>& task) {
    // Convert to a regular task so that the overload above handles the wait.
    return this->dispatchUntilTaskCompletes(task.then(
        async::inline_scheduler(),
        [](const async::shared_task<T>& sharedTask) {
          return sharedTask.get();
        }));
  }

  ImmediateScheduler<QueuedScheduler> immediate{this};

private:
  // Blocks until a task is queued or `stopWaiting` becomes true, then runs at
  // most one queued task.
  void waitAndDispatchOne(const std::atomic<bool>& stopWaiting);

  bool runTask(async::task_run_handle t);
  void unblock();

  struct Impl;
  std::unique_ptr<Impl> _pImpl;
};
//! @endcond
// End omitting doxygen warnings for Impl namespace

} // namespace CesiumImpl
} // namespace CesiumAsync
