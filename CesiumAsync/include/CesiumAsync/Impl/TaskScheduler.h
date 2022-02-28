#pragma once

#include "../ITaskProcessor.h"
#include "ImmediateScheduler.h"

#include <memory>

namespace CesiumAsync {
namespace CesiumImpl {

class TaskScheduler {
public:
  TaskScheduler(const std::shared_ptr<ITaskProcessor>& pTaskProcessor);
  void schedule(async::task_run_handle t);

  ImmediateScheduler<TaskScheduler> immediate{this};

private:
  std::shared_ptr<ITaskProcessor> _pTaskProcessor;
};

} // namespace CesiumImpl
} // namespace CesiumAsync
