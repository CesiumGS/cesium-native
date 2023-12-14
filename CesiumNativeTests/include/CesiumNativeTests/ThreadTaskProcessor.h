#pragma once

#include <CesiumAsync/ITaskProcessor.h>

#include <thread>

namespace CesiumNativeTests {
class ThreadTaskProcessor : public CesiumAsync::ITaskProcessor {
public:
  virtual void startTask(std::function<void()> f) override {
    std::thread(f).detach();
  }
};
} // namespace CesiumNativeTests
