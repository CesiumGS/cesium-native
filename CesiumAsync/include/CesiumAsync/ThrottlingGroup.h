#pragma once

#include "AsyncSystem.h"
#include "Impl/ContinuationFutureType.h"
#include "TaskController.h"

#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCountedNonThreadSafe.h>

#include <functional>
#include <queue>
#include <vector>

namespace CesiumAsync {

// TODO: need a thread safe reference count
class ThrottlingGroup
    : public CesiumUtility::ReferenceCountedNonThreadSafe<ThrottlingGroup> {
public:
  ThrottlingGroup(const AsyncSystem& asyncSystem, int32_t maximumRunning);

  template <typename Func>
  CesiumImpl::ContinuationFutureType_t<Func, void> runInAnyThread(
      const CesiumUtility::IntrusivePointer<TaskController>& pController,
      Func&& f) {
    using FutureType =
        typename CesiumImpl::ContinuationFutureType_t<Func, void>;
    using ValueType = typename CesiumImpl::RemoveFuture<FutureType>::type;
    Promise<ValueType> promise = this->_asyncSystem.createPromise<ValueType>();

    CesiumUtility::IntrusivePointer<ThrottlingGroup> that = this;
    auto runFunction = [that,
                        promise = std::move(promise),
                        f = std::forward<Func>(f)]() mutable {
      try {
        promise.resolve(f());
      } catch (...) {
        promise.reject(std::current_exception());
      }

      that->onTaskComplete();
    };

    pController->_groupStack.emplace_back(this);
    this->_priorityQueue.push(Task{runFunction, pController});

    this->startTasks();

    return promise.getFuture();
  }

  template <typename Func>
  CesiumImpl::ContinuationFutureType_t<Func, void> runInMainThread(
      const CesiumUtility::IntrusivePointer<TaskController>& pController,
      Func&& f) {
    return this->runInAnyThread(
        pController,
        [asyncSystem, f = std::forward<Func>(f)]() mutable {
          return asyncSystem.runInMainThread(std::move(f));
        });
  }

  template <typename Func>
  CesiumImpl::ContinuationFutureType_t<Func, void> runInWorkerThread(
      const CesiumUtility::IntrusivePointer<TaskController>& pController,
      Func&& f) {
    return this->runInAnyThread(
        pController,
        [asyncSystem = this->_asyncSystem, f = std::move(f)]() mutable {
          return asyncSystem.runInWorkerThread(std::move(f));
        });
  }

  template <typename T, typename Func>
  CesiumImpl::ContinuationFutureType_t<Func, T> continueInWorkerThread(
      Future<T>&& continueAfter,
      const CesiumUtility::IntrusivePointer<TaskController>& pController,
      Func&& f) {
    CesiumUtility::IntrusivePointer<ThrottlingGroup> me = this;
    return std::move(continueAfter)
        .thenImmediately(
            [me, pController, f = std::move(f)](T&& value) mutable {
              return me->runInWorkerThread(
                  pController,
                  [value = std::move(value), f = std::move(f)]() mutable {
                    return f(std::move(value));
                  });
            });
  }

private:
  void onTaskComplete();
  void startTasks();

  struct Task {
    std::function<void()> invoke;
    CesiumUtility::IntrusivePointer<TaskController> pController;
    bool operator<(const Task& rhs) const {
      PriorityGroup pgLeft = pController->getPriorityGroup();
      PriorityGroup pgRight = rhs.pController->getPriorityGroup();
      if (pgLeft != pgRight) {
        return pgLeft < pgRight;
      } else {
        return pController->getPriorityRank() >
               rhs.pController->getPriorityRank();
      }
    }
  };

  AsyncSystem _asyncSystem;
  int32_t _maximumRunning;
  int32_t _currentRunning;
  std::priority_queue<Task> _priorityQueue;
};

} // namespace CesiumAsync
