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
#define LAMBDA_CAPTURE_TRACE_START(name)                                       \
  TRACE_NAME_AUX2(tracer, name) = CesiumUtility::ScopedTrace(name),
#define LAMBDA_CAPTURE_TRACE_END(name) TRACE_NAME_AUX2(tracer, name).reset();
#define TRACE(name)                                                            \
  CesiumUtility::ScopedTrace TRACE_NAME_AUX2(tracer, __LINE__)(name);
#define TRACE_START(filename)                                                  \
  CesiumUtility::Profiler::instance().startTracing(filename);
#define TRACE_END() CesiumUtility::Profiler::instance().endTracing();
#define TRACE_ASYNC_BEGIN(name, id)                                            \
  CesiumUtility::Profiler::instance().writeAsyncTrace("cesium", name, 'b', id);
#define TRACE_ASYNC_END(name, id)                                              \
  CesiumUtility::Profiler::instance().writeAsyncTrace("cesium", name, 'e', id);

/**
 * @brief Enlist the current thread into an async process for the duration of
 * the current scope.
 */
#define TRACE_ASYNC_ENLIST(id) CesiumUtility::ScopedEnlist TRACE_NAME_AUX2(tracerEnlist, __LINE__)(id);
#else
#define LAMBDA_CAPTURE_TRACE_START(name)
#define LAMBDA_CAPTURE_TRACE_END(name)
#define TRACE(name)
#define TRACE_START(filename)
#define TRACE_END()
#define TRACE_ASYNC_BEGIN(name, id)
#define TRACE_ASYNC_END(name, id)
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
  void writeAsyncTrace(
      const char* category,
      const char* name,
      char type,
      int64_t id);
  void enlist(int64_t id);
  int64_t getEnlistedID() const;

  void endTracing();

private:
  Profiler();

  std::ofstream _output;
  uint32_t _numTraces;
  std::mutex _lock;
  static thread_local int64_t _threadEnlistedID;
};

// Internal class used by TRACE and TRACE_LOG do not use directly
class ScopedTrace {
public:
  explicit ScopedTrace(const std::string& message);

  void reset();

  ~ScopedTrace();

private:
  std::string _name;
  std::chrono::steady_clock::time_point _startTime;
  std::thread::id _threadId;
  bool _reset;
};

class ScopedEnlist {
public:
  explicit ScopedEnlist(int64_t id);
  ~ScopedEnlist();
};

} // namespace CesiumUtility
