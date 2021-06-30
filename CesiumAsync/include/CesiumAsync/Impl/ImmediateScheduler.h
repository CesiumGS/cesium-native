#pragma once

#include "CesiumAsync/Impl/cesium-async++.h"

namespace CesiumAsync {
namespace Impl {

template <typename TScheduler> class ImmediateScheduler {
public:
  explicit ImmediateScheduler(TScheduler* pScheduler)
      : _pScheduler(pScheduler) {}

  void schedule(async::task_run_handle t) {
    // Are we already in a suitable thread?
    const std::vector<TScheduler*>& inSuitable =
        _schedulersCurrentlyDispatching;
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
    _schedulersCurrentlyDispatching.push_back(this->_pScheduler);
  }

  void markEnd() {
    assert(_schedulersCurrentlyDispatching.back() == this->_pScheduler);
    _schedulersCurrentlyDispatching.pop_back();
  }

private:
  TScheduler* _pScheduler;

  // If a TScheduler instance is found in this thread-local vector, then the
  // current thread has been dispatched by this scheduler and therefore we can
  // dispatch immediately.
  static thread_local std::vector<TScheduler*> _schedulersCurrentlyDispatching;
};

template <typename TScheduler>
/*static*/ thread_local std::vector<TScheduler*>
    ImmediateScheduler<TScheduler>::_schedulersCurrentlyDispatching{};

} // namespace Impl
} // namespace CesiumAsync
