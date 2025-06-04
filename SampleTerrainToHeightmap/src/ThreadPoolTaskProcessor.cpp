#include "ThreadPoolTaskProcessor.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

#include "BS_thread_pool.hpp"

struct ThreadPoolTaskProcessor::Impl {
  Impl(int32_t numberOfThreads = 0) : threadPool(size_t(numberOfThreads)) {}
  BS::thread_pool<BS::tp::none> threadPool;
};

ThreadPoolTaskProcessor::ThreadPoolTaskProcessor()
    : _pImpl(std::make_unique<Impl>()) {}

ThreadPoolTaskProcessor::ThreadPoolTaskProcessor(int32_t numberOfThreads)
    : _pImpl(std::make_unique<Impl>(numberOfThreads)) {}

ThreadPoolTaskProcessor::~ThreadPoolTaskProcessor() = default;

void ThreadPoolTaskProcessor::startTask(std::function<void()> f) {
  this->_pImpl->threadPool.detach_task(std::move(f));
}
