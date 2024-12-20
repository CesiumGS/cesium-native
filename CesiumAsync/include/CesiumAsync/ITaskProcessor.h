#pragma once

#include <CesiumAsync/Library.h>

#include <functional>

namespace CesiumAsync {
/**
 * @brief When implemented by a rendering engine, allows tasks to be
 * asynchronously executed in background threads.
 *
 * Not supposed to be used by clients.
 */
class ITaskProcessor {
public:
  /**
   * @brief Default destructor
   */
  virtual ~ITaskProcessor() = default;

  /**
   * @brief Starts a task that executes the given function in a background
   * thread.
   *
   * @param f The function to execute
   */
  virtual void startTask(std::function<void()> f) = 0;
};
} // namespace CesiumAsync
