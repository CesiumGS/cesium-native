#pragma once

#include "CesiumAsync/Impl/QueuedScheduler.h"
#include "CesiumAsync/Impl/TaskScheduler.h"
#include "CesiumAsync/Impl/cesium-async++.h"

namespace CesiumAsync {

class ITaskProcessor;

namespace Impl {
// Begin omitting doxgen warnings for Impl namespace
//! @cond Doxygen_Suppress

class AsyncSystemSchedulers {
public:
  AsyncSystemSchedulers(const std::shared_ptr<ITaskProcessor>& pTaskProcessor)
      : mainThread(), workerThread(pTaskProcessor) {}

  QueuedScheduler mainThread;
  TaskScheduler workerThread;
};

//! @endcond
// End omitting doxgen warnings for Impl namespace
} // namespace Impl
} // namespace CesiumAsync
