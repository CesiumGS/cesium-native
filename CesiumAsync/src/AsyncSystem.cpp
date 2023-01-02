#include "CesiumAsync/AsyncSystem.h"

#include "CesiumAsync/ITaskProcessor.h"

#include <future>

namespace CesiumAsync {
AsyncSystem::AsyncSystem(
    const std::shared_ptr<ITaskProcessor>& pTaskProcessor) noexcept
    : _pSchedulers(
          std::make_shared<CesiumImpl::AsyncSystemSchedulers>(pTaskProcessor)) {
}

void AsyncSystem::dispatchMainThreadTasks() {
  this->_pSchedulers->mainThread.dispatchQueuedContinuations();
}

bool AsyncSystem::dispatchOneMainThreadTask() {
  return this->_pSchedulers->mainThread.dispatchZeroOrOneContinuation();
}

ThreadPool AsyncSystem::createThreadPool(int32_t numberOfThreads) const {
  return ThreadPool(numberOfThreads);
}

bool AsyncSystem::operator==(const AsyncSystem& rhs) const noexcept {
  return this->_pSchedulers == rhs._pSchedulers;
}

bool AsyncSystem::operator!=(const AsyncSystem& rhs) const noexcept {
  return this->_pSchedulers != rhs._pSchedulers;
}

} // namespace CesiumAsync
