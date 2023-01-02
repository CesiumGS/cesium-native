#pragma once

#include <CesiumAsync/ITaskProcessor.h>

namespace Cesium3DTilesSelection {
class SimpleTaskProcessor : public CesiumAsync::ITaskProcessor {
public:
  virtual void startTask(std::function<void()> f) override { f(); }
};
} // namespace Cesium3DTilesSelection
