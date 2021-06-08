#include "CesiumUtility/Profiler.h"

namespace CesiumUtility {
Profiler& Profiler::instance() {
  static Profiler instance;
  return instance;
}

/*static*/ thread_local int64_t Profiler::_threadEnlistedID = -1;

Profiler::Profiler() : _output{}, _numTraces{0}, _lock{}, _lastAllocatedID(0) {}

Profiler::~Profiler() { endTracing(); }

void Profiler::startTracing(const std::string& filePath) {
  this->_output.open(filePath);
  this->_output << "{\"otherData\": {},\"traceEvents\":[";
}

void Profiler::writeTrace(const Trace& trace) {
  std::lock_guard<std::mutex> lock(_lock);
  if (!this->_output) {
    return;
  }

  // Chrome tracing wants the text like this
  if (this->_numTraces++ > 0) {
    this->_output << ",";
  }

  this->_output << "{";
  this->_output << "\"cat\":\"cesium\",";
  this->_output << "\"dur\":" << trace.duration << ',';
  this->_output << "\"name\":\"" << trace.name << "\",";
  this->_output << "\"ph\":\"X\",";
  this->_output << "\"pid\":0,";
  this->_output << "\"tid\":" << trace.threadID << ",";
  this->_output << "\"ts\":" << trace.start;
  this->_output << "}";
}

void Profiler::writeAsyncTrace(
    const char* category,
    const char* name,
    char type,
    int64_t id) {

  std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();
  int64_t microseconds =
      std::chrono::time_point_cast<std::chrono::microseconds>(time)
          .time_since_epoch()
          .count();

  std::lock_guard<std::mutex> lock(_lock);
  if (!this->_output) {
    return;
  }

  // Chrome tracing wants the text like this
  if (this->_numTraces++ > 0) {
    this->_output << ",";
  }
  this->_output << "{";
  this->_output << "\"cat\":\"" << category << "\",";
  this->_output << "\"id\":" << id << ",";
  this->_output << "\"name\":\"" << name << "\",";
  this->_output << "\"ph\":\"" << type << "\",";
  this->_output << "\"pid\":0,";
  this->_output << "\"ts\":" << microseconds;
  this->_output << "}";
}

void Profiler::enlist(int64_t id) { Profiler::_threadEnlistedID = id; }

int64_t Profiler::getEnlistedID() const { return Profiler::_threadEnlistedID; }

int64_t Profiler::allocateID() {
  return ++this->_lastAllocatedID;
}

void Profiler::endTracing() {
  this->_output << "]}";
  this->_output.close();
}

ScopedTrace::ScopedTrace(const std::string& message)
    : _name{message},
      _startTime{std::chrono::steady_clock::now()},
      _threadId{std::this_thread::get_id()},
      _reset{false} {
  Profiler& profiler = Profiler::instance();
  if (profiler.getEnlistedID() >= 0) {
    profiler.writeAsyncTrace(
        "cesium",
        _name.c_str(),
        'b',
        profiler.getEnlistedID());
  }
}

void ScopedTrace::reset() {
  this->_reset = true;
  Profiler& profiler = Profiler::instance();
  if (profiler.getEnlistedID() >= 0) {
    profiler.writeAsyncTrace(
        "cesium",
        _name.c_str(),
        'e',
        profiler.getEnlistedID());
  } else {
    auto endTimePoint = std::chrono::steady_clock::now();
    int64_t start = std::chrono::time_point_cast<std::chrono::microseconds>(
                        this->_startTime)
                        .time_since_epoch()
                        .count();
    int64_t end =
        std::chrono::time_point_cast<std::chrono::microseconds>(endTimePoint)
            .time_since_epoch()
            .count();
    profiler.writeTrace(
        {this->_name, start, end - start, this->_threadId});
  }
}

ScopedTrace::~ScopedTrace() {
  if (!this->_reset) {
    this->reset();
  }
}

ScopedEnlist::ScopedEnlist(int64_t id) {
  CesiumUtility::Profiler::instance().enlist(id);
}

ScopedEnlist::~ScopedEnlist() {
  CesiumUtility::Profiler::instance().enlist(-1);
}

} // namespace CesiumUtility