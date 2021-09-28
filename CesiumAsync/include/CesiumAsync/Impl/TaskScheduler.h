#pragma once

#include "ImmediateScheduler.h"

#include <memory>

#include "../ITaskProcessor.h"

namespace CesiumAsync {
namespace Impl {

class TaskScheduler {
public:
  TaskScheduler(const std::shared_ptr<ITaskProcessor>& pTaskProcessor);
  void schedule(async::task_run_handle t);

  ImmediateScheduler<TaskScheduler> immediate{this};

private:
  std::shared_ptr<ITaskProcessor> _pTaskProcessor;
};

} // namespace Impl
} // namespace CesiumAsync
