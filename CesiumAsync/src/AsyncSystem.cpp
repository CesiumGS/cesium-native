#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/ITaskProcessor.h"
#include <future>

namespace CesiumAsync {
AsyncSystem::AsyncSystem(
    const std::shared_ptr<ITaskProcessor>& pTaskProcessor) noexcept
    : _pSchedulers(
          std::make_shared<Impl::AsyncSystemSchedulers>(pTaskProcessor)) {}

void AsyncSystem::dispatchMainThreadTasks() {
  this->_pSchedulers->startInMainThread();

  try {
    this->_pSchedulers->mainThreadScheduler.run_all_tasks();
  } catch (...) {
    this->_pSchedulers->endInMainThread();
    throw;
  }

  this->_pSchedulers->endInMainThread();
}

namespace Impl {

void ImmediatelyInMainThreadScheduler::schedule(async::task_run_handle t) {
  // Are we already in the main thread?
  const std::vector<AsyncSystemSchedulers*>& inMain =
      AsyncSystemSchedulers::schedulersInMainThread;
  if (std::find(inMain.begin(), inMain.end(), this->_pSchedulers) !=
      inMain.end()) {
    // Yes, run this task directly.
    t.run();
  } else {
    // No, schedule this thread in the main thread.
    this->_pSchedulers->mainThreadScheduler.schedule(std::move(t));
  }
}

void WorkerThreadScheduler::schedule(async::task_run_handle t) {
  struct Receiver {
    async::task_run_handle taskHandle;
  };

  std::shared_ptr<Receiver> pReceiver = std::make_shared<Receiver>();
  pReceiver->taskHandle = std::move(t);

  this->_pSchedulers->pTaskProcessor->startTask(
      [pReceiver, pScheduler = this->_pSchedulers]() mutable {
        pScheduler->startInWorkerThread();

        try {
          pReceiver->taskHandle.run();
        } catch (...) {
          pScheduler->endInWorkerThread();
          throw;
        }

        pScheduler->endInWorkerThread();
      });
}

void ImmediatelyInWorkerThreadScheduler::schedule(async::task_run_handle t) {
  // Are we already in the worker thread?
  const std::vector<AsyncSystemSchedulers*>& inWorker =
      AsyncSystemSchedulers::schedulersInWorkerThread;
  if (std::find(inWorker.begin(), inWorker.end(), this->_pSchedulers) !=
      inWorker.end()) {
    // Yes, run this task directly.
    t.run();
  } else {
    // No, schedule this thread in the worker thread.
    this->_pSchedulers->workerThreadScheduler.schedule(std::move(t));
  }
}

/*static*/ thread_local std::vector<AsyncSystemSchedulers*>
    AsyncSystemSchedulers::schedulersInMainThread;
/*static*/ thread_local std::vector<AsyncSystemSchedulers*>
    AsyncSystemSchedulers::schedulersInWorkerThread;

AsyncSystemSchedulers::AsyncSystemSchedulers(
    const std::shared_ptr<ITaskProcessor>& pTaskProcessor_)
    : pTaskProcessor(pTaskProcessor_),
      mainThreadScheduler(),
      workerThreadScheduler(this),
      immediatelyInMainThreadScheduler(this),
      immediatelyInWorkerThreadScheduler(this) {}

void AsyncSystemSchedulers::startInMainThread() {
  AsyncSystemSchedulers::schedulersInMainThread.push_back(this);
}

void AsyncSystemSchedulers::endInMainThread() {
  assert(AsyncSystemSchedulers::schedulersInMainThread.back() == this);
  AsyncSystemSchedulers::schedulersInMainThread.pop_back();
}

void AsyncSystemSchedulers::startInWorkerThread() {
  AsyncSystemSchedulers::schedulersInWorkerThread.push_back(this);
}

void AsyncSystemSchedulers::endInWorkerThread() {
  assert(AsyncSystemSchedulers::schedulersInWorkerThread.back() == this);
  AsyncSystemSchedulers::schedulersInWorkerThread.pop_back();
}

} // namespace Impl

} // namespace CesiumAsync
