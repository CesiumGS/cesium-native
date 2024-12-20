#pragma once

#include <CesiumAsync/ITaskProcessor.h>

#include <functional>

class MockTaskProcessor : public CesiumAsync::ITaskProcessor {
public:
  virtual void startTask(std::function<void()> f) override { f(); }
};
