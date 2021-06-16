#include "CesiumAsync/ThreadPool.h"

using namespace CesiumAsync;

ThreadPool::ThreadPool(int32_t numberOfThreads)
    : _pScheduler(
          std::make_shared<async::threadpool_scheduler>(numberOfThreads)) {}
