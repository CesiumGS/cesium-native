#pragma once

#include "CesiumAsync/Impl/cesium-async++.h"
#include "CesiumAsync/Library.h"
#include <memory>

namespace CesiumAsync {

class CESIUMASYNC_API ThreadPool {
private:
  ThreadPool(int32_t numberOfThreads);
  std::shared_ptr<async::threadpool_scheduler> _pScheduler;

  template <typename T> friend class Future;
  friend class AsyncSystem;
};

} // namespace CesiumAsync
