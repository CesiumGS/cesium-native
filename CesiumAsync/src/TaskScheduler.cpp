#include "CesiumAsync/Impl/TaskScheduler.h"

#include <CesiumAsync/ITaskProcessor.h>

#include <async++.h>

#include <memory>
#include <utility>

using namespace CesiumAsync::CesiumImpl;

TaskScheduler::TaskScheduler(
    const std::shared_ptr<CesiumAsync::ITaskProcessor>& pTaskProcessor)
    : _pTaskProcessor(pTaskProcessor) {}

void TaskScheduler::schedule(async::task_run_handle t) {
  // std::function must be copyable, so we can't put a move-only
  // task_run_handle in the capture list of a lambda we want to use with it.
  // So, we wrap it with a copyable type (shared_ptr).
  // https://riptutorial.com/cplusplus/example/1950/generalized-capture has
  // a good explanation of this problem.

  struct Receiver {
    async::task_run_handle taskHandle;
  };

  std::shared_ptr<Receiver> pReceiver = std::make_shared<Receiver>();
  pReceiver->taskHandle = std::move(t);

  this->_pTaskProcessor->startTask([this, pReceiver]() mutable {
    auto scope = this->immediate.scope();
    pReceiver->taskHandle.run();
  });
}
