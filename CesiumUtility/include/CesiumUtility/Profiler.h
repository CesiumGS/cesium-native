#pragma once

#include <chrono>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

// If the build system doesn't enable the tracing support
// consider it disabled by default.
#ifndef TRACING_ENABLED
#define TRACING_ENABLED 0
#endif

// helper macros to avoid shadowing variables
#define TRACE_NAME_AUX1(A, B) A##B
#define TRACE_NAME_AUX2(A, B) TRACE_NAME_AUX1(A, B)

#if TRACING_ENABLED
// The TRACE macro
#define TRACE(name)                                                            \
  CesiumUtility::ScopedTrace TRACE_NAME_AUX2(tracer, __LINE__)(name, false);
#define TRACE_START(filename)                                                  \
  CesiumUtility::Profiler::instance().startTracing(filename);
#define TRACE_END() CesiumUtility::Profiler::instance().endTracing();
#else
#define TRACE(name)
#define TRACE_START(filename)
#define TRACE_END()
#endif

namespace CesiumUtility {
struct Trace {
  std::string name;
  int64_t start;
  int64_t duration;
  std::thread::id threadID;
};

class Profiler {
public:
  static Profiler& instance(); 

  ~Profiler();

  void startTracing(const std::string& filePath = "trace.json"); 

  void writeTrace(const Trace& trace); 

  void endTracing(); 

private:
  Profiler(); 

  std::ofstream _output;
  uint32_t _numTraces;
  std::mutex _lock;
};

// Internal class used by TRACE and TRACE_LOG do not use directly
class ScopedTrace {
public:
  explicit ScopedTrace(const std::string& message);

  ~ScopedTrace();

private:
  std::string _name;
  std::chrono::steady_clock::time_point _startTime;
  std::thread::id _threadId;
};
}
