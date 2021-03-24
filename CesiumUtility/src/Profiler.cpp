#include "CesiumUtility/Profiler.h"

namespace CesiumUtility {
Profiler& Profiler::instance() {
  static Profiler instance;
  return instance;
}

Profiler::Profiler() : _output(), _numTraces(0), _lock() {}

Profiler::~Profiler() { endTracing(); }

void Profiler::startTracing(const std::string& filePath) {
  this->_output.open(filePath);
  this->_output << "{\"otherData\": {},\"traceEvents\":[";
}

void Profiler::writeTrace(const Trace& trace) {
  std::lock_guard<std::mutex> lock(_lock);
  // Chrome tracing wants the text like this
  if (this->_numTraces++ > 0) {
    this->_output << ",";
  }
  this->_output << "{";
  this->_output << "\"cat\":\"function\",";
  this->_output << "\"dur\":" << trace.duration << ',';
  this->_output << "\"name\":\"" << trace.name << "\",";
  this->_output << "\"ph\":\"X\",";
  this->_output << "\"pid\":0,";
  this->_output << "\"tid\":" << trace.threadID << ",";
  this->_output << "\"ts\":" << trace.start;
  this->_output << "}";
}

void Profiler::endTracing() {
  this->_output << "]}";
  this->_output.close();
}

ScopedTrace::ScopedTrace(const std::string& message)
    : _name(message),
      _startTime(std::chrono::steady_clock::now()),
      _threadId(std::this_thread::get_id()) {}

ScopedTrace::~ScopedTrace() {
  auto endTimePoint = std::chrono::steady_clock::now();
#ifdef TRACING_ENABLED
  int64_t start =
      std::chrono::time_point_cast<std::chrono::microseconds>(this->_startTime)
          .time_since_epoch()
          .count();
  int64_t end =
      std::chrono::time_point_cast<std::chrono::microseconds>(endTimePoint)
          .time_since_epoch()
          .count();
  Profiler::instance().writeTrace(
      {this->_name, start, end - start, this->_threadId});
#endif
}
} // namespace CesiumUtility