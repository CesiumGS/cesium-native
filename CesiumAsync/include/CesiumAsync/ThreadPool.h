#pragma once

#include "CesiumAsync/Impl/cesium-async++.h"
#include "CesiumAsync/Library.h"
#include <memory>

namespace CesiumAsync {

/**
 * @brief A thread pool created by {@link AsyncSystem::createThreadPool}.
 *
 * This object has no public methods, but can be used with
 * {@link AsyncSystem::runInThreadPool} and
 * {@link Future::thenInThreadPool}.
 */
class CESIUMASYNC_API ThreadPool {
private:
  ThreadPool(int32_t numberOfThreads);
  std::shared_ptr<async::threadpool_scheduler> _pScheduler;

  template <typename T> friend class Future;
  friend class AsyncSystem;
};

} // namespace CesiumAsync
