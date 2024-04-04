#pragma once

#include "ImmediateScheduler.h"
#include "cesium-async++.h"

namespace CesiumAsync {
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
    async::task<T> unblockTask =
        task.then(async::inline_scheduler(), [this](auto&& value) {
          this->unblock();
          return std::move(value);
        });

    while (!unblockTask.ready()) {
      this->dispatchInternal(true);
    }

    return std::move(unblockTask).get();
  }

  template <typename T>
  T dispatchUntilTaskCompletes(const async::shared_task<T>& task) {
    // Set up a continuation to unblock the blocking dispatch when this task
    // completes. Unlike the non-shared future case above, we don't need to pass
    // the future value through because shared_task supports multiple
    // continuations.
    async::task<void> unblockTask =
        task.then(async::inline_scheduler(), [this](const auto&) {
          this->unblock();
        });

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

} // namespace CesiumImpl
} // namespace CesiumAsync
