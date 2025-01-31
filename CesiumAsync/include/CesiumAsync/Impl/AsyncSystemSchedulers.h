#pragma once

#include "QueuedScheduler.h"
#include "TaskScheduler.h"
#include "cesium-async++.h"

namespace CesiumAsync {

class ITaskProcessor;

namespace CesiumImpl {
// Begin omitting doxygen warnings for Impl namespace
//! @cond Doxygen_Suppress

class AsyncSystemSchedulers {
public:
  AsyncSystemSchedulers(const std::shared_ptr<ITaskProcessor>& pTaskProcessor)
      : mainThread(), workerThread(pTaskProcessor) {}

  QueuedScheduler mainThread;
  TaskScheduler workerThread;
};

//! @endcond
// End omitting doxygen warnings for Impl namespace
} // namespace CesiumImpl
} // namespace CesiumAsync
