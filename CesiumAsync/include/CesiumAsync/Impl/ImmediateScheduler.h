#pragma once

#include "cesium-async++.h"

#include <CesiumUtility/Assert.h>

#include <spdlog/spdlog.h>

namespace CesiumAsync {
// Begin omitting doxygen warnings for Impl namespace
//! @cond Doxygen_Suppress
namespace CesiumImpl {

template <typename TScheduler> class ImmediateScheduler {
public:
  explicit ImmediateScheduler(TScheduler* pScheduler) noexcept
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

  class SchedulerScope {
  public:
    SchedulerScope(TScheduler* pScheduler = nullptr) : _pScheduler(pScheduler) {
      if (this->_pScheduler) {
        std::vector<TScheduler*>& inSuitable =
            ImmediateScheduler<TScheduler>::getSchedulersCurrentlyDispatching();
        inSuitable.push_back(this->_pScheduler);
      }
    }

    ~SchedulerScope() noexcept { this->reset(); }

    SchedulerScope(SchedulerScope&& rhs) noexcept
        : _pScheduler(rhs._pScheduler) {
      rhs._pScheduler = nullptr;
    }

    SchedulerScope& operator=(SchedulerScope&& rhs) noexcept {
      std::swap(this->_pScheduler, rhs._pScheduler);
      return *this;
    }

    void reset() noexcept {
      if (this->_pScheduler) {
        std::vector<TScheduler*>& inSuitable =
            ImmediateScheduler<TScheduler>::getSchedulersCurrentlyDispatching();
        CESIUM_ASSERT(!inSuitable.empty());
        CESIUM_ASSERT(inSuitable.back() == this->_pScheduler);
        inSuitable.pop_back();

        this->_pScheduler = nullptr;
      }
    }

    SchedulerScope(const SchedulerScope&) = delete;
    SchedulerScope& operator=(const SchedulerScope&) = delete;

  private:
    TScheduler* _pScheduler;
  };

  SchedulerScope scope() { return SchedulerScope(this->_pScheduler); }

private:
  TScheduler* _pScheduler;

  // If a TScheduler instance is found in this thread-local vector, then the
  // current thread has been dispatched by this scheduler and therefore we can
  // dispatch immediately.
  static std::vector<TScheduler*>&
  getSchedulersCurrentlyDispatching() noexcept {
    // We're using a static local here rather than a static field because, on
    // at least some Linux systems (mine), with Clang 12, in a Debug build, a
    // thread_local static field causes a SEGFAULT on access.
    // I don't understand why (despite hours trying), but making it a static
    // local instead solves the problem and is arguably cleaner, anyway.
    static thread_local std::vector<TScheduler*> schedulersCurrentlyDispatching;
    return schedulersCurrentlyDispatching;
  }
};
//! @endcond
// End omitting doxygen warnings for Impl namespace

} // namespace CesiumImpl
} // namespace CesiumAsync
