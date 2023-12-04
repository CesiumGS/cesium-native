#pragma once

#include <CesiumAsync/AsyncSystem.h>

namespace CesiumNativeTests {

template <typename T>
T waitForFuture(
    CesiumAsync::AsyncSystem& asyncSystem,
    CesiumAsync::Future<T>&& future) {
  while (!future.isReady()) {
    asyncSystem.dispatchMainThreadTasks();
  }

  return future.wait();
}

} // namespace CesiumNativeTests
