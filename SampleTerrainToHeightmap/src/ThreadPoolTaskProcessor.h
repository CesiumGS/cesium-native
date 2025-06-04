#pragma once

#include <CesiumAsync/ITaskProcessor.h>

#include <cstdint>
#include <functional>
#include <memory>

/**
 * @brief An `ITaskProcessor` that executes tasks in a thread pool.
 */
class ThreadPoolTaskProcessor : public CesiumAsync::ITaskProcessor {
public:
  /**
   * @brief Initializes a new instance to use a thread pool with one thread per
   * logical processor available on the system.
   */
  ThreadPoolTaskProcessor();

  /**
   * @brief Initializes a new instance to use a thread pool with a specified
   * number of threads.
   *
   * @param numberOfThreads The number of threads in the thread pool.
   */
  ThreadPoolTaskProcessor(int32_t numberOfThreads);

  ~ThreadPoolTaskProcessor() override;

  void startTask(std::function<void()> f) override;

private:
  struct Impl;
  std::unique_ptr<Impl> _pImpl;
};
