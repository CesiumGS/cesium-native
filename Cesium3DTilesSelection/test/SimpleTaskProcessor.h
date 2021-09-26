#pragma once

#include "CesiumAsync/ITaskProcessor.h"

class SimpleTaskProcessor : public CesiumAsync::ITaskProcessor {
public:
  virtual void startTask(std::function<void()> f) override { f(); }
};
