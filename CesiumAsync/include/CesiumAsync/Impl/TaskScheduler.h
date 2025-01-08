#pragma once

#include "../ITaskProcessor.h"
#include "ImmediateScheduler.h"

#include <memory>

namespace CesiumAsync {
namespace CesiumImpl {

// Begin omitting doxygen warnings for Impl namespace
//! @cond Doxygen_Suppress
class TaskScheduler {
public:
  TaskScheduler(const std::shared_ptr<ITaskProcessor>& pTaskProcessor);
  void schedule(async::task_run_handle t);

  ImmediateScheduler<TaskScheduler> immediate{this};

private:
  std::shared_ptr<ITaskProcessor> _pTaskProcessor;
};
//! @endcond
// End omitting doxygen warnings for Impl namespace

} // namespace CesiumImpl
} // namespace CesiumAsync
