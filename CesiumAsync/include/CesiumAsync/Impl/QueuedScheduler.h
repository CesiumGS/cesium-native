#pragma once

#include "ImmediateScheduler.h"
#include "cesium-async++.h"

namespace CesiumAsync {
namespace CesiumImpl {

class QueuedScheduler {
public:
  void schedule(async::task_run_handle t);
  void dispatchQueuedContinuations();
  bool dispatchZeroOrOneContinuation();

  ImmediateScheduler<QueuedScheduler> immediate{this};

private:
  async::fifo_scheduler _scheduler;
};

} // namespace CesiumImpl
} // namespace CesiumAsync
