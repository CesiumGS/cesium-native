#include <CesiumAsync/TaskController.h>
#include <CesiumAsync/ThrottlingGroup.h>

namespace CesiumAsync {

TaskController::TaskController(
    PriorityGroup initialPriorityGroup,
    float initialPriorityRank)
    : _groupStack(),
      _priorityGroup(initialPriorityGroup),
      _priorityRank(initialPriorityRank) {}

void TaskController::cancel() {
  // TODO
}

PriorityGroup TaskController::getPriorityGroup() const {
  return this->_priorityGroup;
}

float CesiumAsync::TaskController::getPriorityRank() const {
  return this->_priorityRank;
}

} // namespace CesiumAsync
