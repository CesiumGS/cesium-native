#pragma once

#include "PriorityGroup.h"

#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCountedNonThreadSafe.h>

#include <vector>

namespace CesiumAsync {

class ThrottlingGroup;

// TODO: need a thread safe reference count
class TaskController
    : public CesiumUtility::ReferenceCountedNonThreadSafe<TaskController> {
public:
  TaskController(PriorityGroup initialPriorityGroup, float initialPriorityRank);

  void cancel();

  PriorityGroup getPriorityGroup() const;
  float getPriorityRank() const;

private:
  std::vector<CesiumUtility::IntrusivePointer<ThrottlingGroup>> _groupStack;
  PriorityGroup _priorityGroup;
  float _priorityRank;

  friend class ThrottlingGroup;
};

} // namespace CesiumAsync
