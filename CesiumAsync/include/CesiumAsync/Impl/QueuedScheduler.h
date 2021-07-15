#pragma once

#include "CesiumAsync/Impl/ImmediateScheduler.h"
#include "CesiumAsync/Impl/cesium-async++.h"

namespace CesiumAsync {
namespace Impl {

class QueuedScheduler {
public:
  void schedule(async::task_run_handle t);
  void dispatchQueuedContinuations();
  bool dispatchZeroOrOneContinuation();

  ImmediateScheduler<QueuedScheduler> immediate{this};

private:
  async::fifo_scheduler _scheduler;
};

} // namespace Impl
} // namespace CesiumAsync
