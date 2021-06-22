#include "CesiumUtility/Tracing.h"

#if CESIUM_TRACING_ENABLED

namespace CesiumUtility {
Tracer& Tracer::instance() {
  static Tracer instance;
  return instance;
}

/*static*/ thread_local int64_t Tracer::_threadEnlistedID = -1;

Tracer::Tracer() : _output{}, _numTraces{0}, _lock{}, _lastAllocatedID(0) {}

Tracer::~Tracer() { endTracing(); }

void Tracer::startTracing(const std::string& filePath) {
  this->_output.open(filePath);
  this->_output << "{\"otherData\": {},\"traceEvents\":[";
}

void Tracer::writeTrace(const Trace& trace) {
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

void Tracer::writeAsyncTrace(
    const char* category,
    const char* name,
    char type,
    int64_t id) {

  bool isAsync = true;
  if (id < 0) {
    // Use a standard Duration event for slices without an async ID.
    isAsync = false;
    if (type == 'b') {
      type = 'B';
    } else if (type == 'e') {
      type = 'E';
    }
  }

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
  if (isAsync) {
    this->_output << "\"id\":" << id << ",";
  } else {
    this->_output << "\"tid\":" << std::this_thread::get_id() << ",";
  }
  this->_output << "\"name\":\"" << name << "\",";
  this->_output << "\"ph\":\"" << type << "\",";
  this->_output << "\"pid\":0,";
  this->_output << "\"ts\":" << microseconds;
  this->_output << "}";
}

void Tracer::enlist(int64_t id) { Tracer::_threadEnlistedID = id; }

int64_t Tracer::getEnlistedID() const { return Tracer::_threadEnlistedID; }

int64_t Tracer::allocateID() { return ++this->_lastAllocatedID; }

void Tracer::endTracing() {
  this->_output << "]}";
  this->_output.close();
}

ScopedTrace::ScopedTrace(const std::string& message)
    : _name{message},
      _startTime{std::chrono::steady_clock::now()},
      _threadId{std::this_thread::get_id()},
      _reset{false} {
  Tracer& profiler = Tracer::instance();
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
  Tracer& profiler = Tracer::instance();
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
    profiler.writeTrace({this->_name, start, end - start, this->_threadId});
  }
}

ScopedTrace::~ScopedTrace() {
  if (!this->_reset) {
    this->reset();
  }
}

ScopedEnlist::ScopedEnlist(int64_t id)
    : _previousID(CesiumUtility::Tracer::instance().getEnlistedID()) {
  CesiumUtility::Tracer::instance().enlist(id);
}

ScopedEnlist::~ScopedEnlist() {
  CesiumUtility::Tracer::instance().enlist(this->_previousID);
}

} // namespace CesiumUtility

#endif // CESIUM_TRACING_ENABLED
