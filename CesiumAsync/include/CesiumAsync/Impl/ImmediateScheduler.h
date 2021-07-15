#pragma once

#include "CesiumAsync/Impl/cesium-async++.h"
#include <spdlog/spdlog.h>

namespace CesiumAsync {
namespace Impl {

template <typename TScheduler> class ImmediateScheduler {
public:
  explicit ImmediateScheduler(TScheduler* pScheduler)
      : _pScheduler(pScheduler) {}

  void schedule(async::task_run_handle t) {
    // Are we already in a suitable thread?
    std::vector<TScheduler*>& inSuitable =
        ImmediateScheduler<TScheduler>::getSchedulersCurrentlyDispatching();
    if (std::find(inSuitable.begin(), inSuitable.end(), this->_pScheduler) !=
        inSuitable.end()) {
      // Yes, run this task directly.
      t.run();
    } else {
      // No, schedule this task with the deferred scheduler.
      this->_pScheduler->schedule(std::move(t));
    }
  }

  void markBegin() {
    std::vector<TScheduler*>& inSuitable =
        ImmediateScheduler<TScheduler>::getSchedulersCurrentlyDispatching();
    inSuitable.push_back(this->_pScheduler);
  }

  void markEnd() {
    std::vector<TScheduler*>& inSuitable =
        ImmediateScheduler<TScheduler>::getSchedulersCurrentlyDispatching();
    assert(inSuitable.back() == this->_pScheduler);
    inSuitable.pop_back();
  }

private:
  TScheduler* _pScheduler;

  // If a TScheduler instance is found in this thread-local vector, then the
  // current thread has been dispatched by this scheduler and therefore we can
  // dispatch immediately.
  static std::vector<TScheduler*>& getSchedulersCurrentlyDispatching() {
    // We're using a static local here rather than a static field because, on
    // at least some Linux systems (mine), with Clang 12, in a Debug build, a
    // thread_local static field causes a SEGFAULT on access.
    // I don't understand why (despite hours trying), but making it a static
    // local instead solves the problem and is arguably cleaner, anyway.
    static thread_local std::vector<TScheduler*> schedulersCurrentlyDispatching;
    return schedulersCurrentlyDispatching;
  }
};

} // namespace Impl
} // namespace CesiumAsync
