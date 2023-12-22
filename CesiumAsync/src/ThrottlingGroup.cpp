#include <CesiumAsync/ThrottlingGroup.h>

namespace CesiumAsync {

ThrottlingGroup::ThrottlingGroup(
    const AsyncSystem& asyncSystem,
    int32_t maximumRunning)
    : _asyncSystem(asyncSystem), _maximumRunning(maximumRunning) {}

void ThrottlingGroup::onTaskComplete() {
  --this->_currentRunning;
  this->startTasks();
}

void CesiumAsync::ThrottlingGroup::startTasks() {
  while (!this->_priorityQueue.empty() &&
         this->_currentRunning < this->_maximumRunning) {
    ++this->_currentRunning;
    Task task = this->_priorityQueue.top();
    this->_priorityQueue.pop();
    task.invoke();
  }
}

} // namespace CesiumAsync
