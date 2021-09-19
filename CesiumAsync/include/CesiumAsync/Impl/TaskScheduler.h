#pragma once

#include "CesiumAsync/ITaskProcessor.h"
#include "CesiumAsync/Impl/ImmediateScheduler.h"

#include <memory>

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
