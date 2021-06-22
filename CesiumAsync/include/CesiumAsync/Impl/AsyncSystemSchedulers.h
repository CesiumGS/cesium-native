#pragma once

#include "CesiumAsync/Impl/cesium-async++.h"

namespace CesiumAsync {

class ITaskProcessor;

namespace Impl {
// Begin omitting doxgen warnings for Impl namespace
//! @cond Doxygen_Suppress

class AsyncSystemSchedulers;

class ImmediatelyInMainThreadScheduler {
public:
  ImmediatelyInMainThreadScheduler(AsyncSystemSchedulers* pSchedulers)
      : _pSchedulers(pSchedulers) {}

  void schedule(async::task_run_handle t);

private:
  AsyncSystemSchedulers* _pSchedulers;
};

class WorkerThreadScheduler {
public:
  WorkerThreadScheduler(AsyncSystemSchedulers* pSchedulers)
      : _pSchedulers(pSchedulers) {}

  void schedule(async::task_run_handle t);

private:
  AsyncSystemSchedulers* _pSchedulers;
};

class ImmediatelyInWorkerThreadScheduler {
public:
  ImmediatelyInWorkerThreadScheduler(AsyncSystemSchedulers* pSchedulers)
      : _pSchedulers(pSchedulers) {}

  void schedule(async::task_run_handle t);

private:
  AsyncSystemSchedulers* _pSchedulers;
};

class AsyncSystemSchedulers {
public:
  AsyncSystemSchedulers(const std::shared_ptr<ITaskProcessor>& pTaskProcessor);

  std::shared_ptr<ITaskProcessor> pTaskProcessor;

  async::fifo_scheduler mainThreadScheduler;
  WorkerThreadScheduler workerThreadScheduler;

  ImmediatelyInMainThreadScheduler immediatelyInMainThreadScheduler;
  ImmediatelyInWorkerThreadScheduler immediatelyInWorkerThreadScheduler;

  void startInMainThread();
  void endInMainThread();

  void startInWorkerThread();
  void endInWorkerThread();

  // If an AsyncSystemSchedulers instance is found in these thread-local vectors,
  // then the current thread is currently inside the "main thread" or a "worker
  // thread" for that scheduler.
  static thread_local std::vector<AsyncSystemSchedulers*>
      schedulersInMainThread;
  static thread_local std::vector<AsyncSystemSchedulers*>
      schedulersInWorkerThread;
};

//! @endcond
// End omitting doxgen warnings for Impl namespace
} // namespace Impl
} // namespace CesiumAsync
