#pragma once

#include <atomic>
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

/**
 * @brief Initializes the tracing framework and begins recording to a given JSON
 * filename.
 *
 * @param filename The path and named of the file in which to record traces.
 */
#define CESIUM_TRACE_INIT(filename)                                            \
  CesiumUtility::Tracer::instance().startTracing(filename)

/**
 * @brief Shuts down tracing and closes the JSON tracing file.
 */
#define CESIUM_TRACE_SHUTDOWN() CesiumUtility::Tracer::instance().endTracing()

/**
 * @brief Measures and records the time spent in the current scope.
 *
 * The time is measured from the `CESIUM_TRACE` line to the end of the scope.
 * If the current thread is enlisted in an async process
 * ({@link CESIUM_TRACE_ASYNC_ENLIST}), the time is recorded against the async
 * process. Otherwise, it is recorded against the current thread.
 *
 * @param name The name of the measured operation.
 */
#define CESIUM_TRACE(name)                                                     \
  CesiumUtility::ScopedTrace TRACE_NAME_AUX2(tracer, __LINE__)(name)

/**
 * @brief Begins measuring an operation which may span scopes and even threads.
 *
 * Begins measuring the time of an operation which completes with a
 * corresponding call to {@link CESIUM_TRACE_END}. If the calling thread is
 * enlisted in an async process ({@link CESIUM_TRACE_ASYNC_ENLIST}), the
 * time is recorded against the async process. Otherwise, is is recorded
 * against the current thread.
 *
 * Extreme care must be taken to match calls to `CESIUM_TRACE_BEGIN` and
 * `CESIUM_TRACE_END`:
 *
 *   * Paired calls must use an identical `name`.
 *   * If either BEGIN or END is called from a thread enlisted into an async
 *     process, then both must be, and it must be the same enlisted async ID in
 *     both cases. In this case BEGIN and END may be called from different
 *     threads.
 *   * If either BEGIN or END is called from a thread _not_ enlisted into an
 *     async process, then both must be called from the same thread and neither
 *     thread may be enlisted.
 *   * Paired calls must not be interleaved with other BEGIN/END calls for the
 *     same thread or async process ID. Other BEGIN/END pairs may be fully
 *     nested within this one, but this pair must not END in between a nested
 *     measurement's BEGIN and END calls.
 *
 * Failure to ensure the above may lead to generation of a trace file that the
 * Chromium trace viewer is not able to correctly interpret.
 *
 * @param name The name of the measured operation.
 */
#define CESIUM_TRACE_BEGIN(name)                                               \
  CesiumUtility::Tracer::instance().writeAsyncTrace(                           \
      "cesium",                                                                \
      name,                                                                    \
      'b',                                                                     \
      CesiumUtility::Tracer::instance().getEnlistedID())

/**
 * @brief Ends measuring an operation which may span scopes and even threads.
 *
 * Finishes measuring the time of an operation that began with a call to
 * {@link CESIUM_TRACE_BEGIN}. See the documentation for that macro for more
 * details and caveats.
 *
 * @param name The name of the measured operation.
 */
#define CESIUM_TRACE_END(name)                                                 \
  CesiumUtility::Tracer::instance().writeAsyncTrace(                           \
      "cesium",                                                                \
      name,                                                                    \
      'e',                                                                     \
      CesiumUtility::Tracer::instance().getEnlistedID())

/**
 * @brief Allocates an ID for a new asynchronous process.
 *
 * This ID is used to identify an operation that may span multiple threads.
 * It may be supplied to {@link CESIUM_TRACE_BEGIN_ID} and
 * {@link CESIUM_TRACE_END_ID} to explicitly trace a part of an async process
 * across threads.  It may also be passed to {@link CESIUM_TRACE_ASYNC_ENLIST}
 * to set this async ID as the "ambient" async process for the current thread
 * and scope, in which case calls to {@link CESIUM_TRACE},
 * {@link CESIUM_TRACE_BEGIN}, and {@link CESIUM_TRACE_END} will automatically
 * create events that are part of this async process.
 */
#define CESIUM_TRACE_ALLOCATE_ASYNC_ID()                                       \
  CesiumUtility::Tracer::instance().allocateID()

/**
 * @brief Gets the ID of the async process that the calling thread is currently
 * enlisted in.
 *
 * A new async process ID can be allocated with
 * {@link CESIUM_TRACE_ALLOCATE_ASYNC_ID}, and the current thread can be enlisted
 * into the process with {@link CESIUM_TRACE_ASYNC_ENLIST}.
 *
 * @return The ID of the async process, or -1 if the current thread is not
 * enlisted in an async process.
 */
#define CESIUM_TRACE_CURRENT_ASYNC_ID()                                        \
  CesiumUtility::Tracer::instance().getEnlistedID()

/**
 * @brief Enlist the current thread into an async process for the duration of
 * the current scope.
 *
 * Async IDs are allocated with {@link CESIUM_TRACE_ALLOCATE_ASYNC_ID}. Once a
 * thread is enlisted into an async ID, all tracing operations that don't
 * explicitly take an ID are automatically associated with this ID for the
 * duration of the scope.
 */
#define CESIUM_TRACE_ASYNC_ENLIST(id)                                          \
  CesiumUtility::ScopedEnlist TRACE_NAME_AUX2(tracerEnlist, __LINE__)(id)

/**
 * @brief Starts a new async process by allocating an ID for it and enlisting
 * the current thread into the process for the current scope.
 *
 * This is equivalent to calling {@link CESIUM_TRACE_ALLOCATE_ASYNC_ID} and
 * then passing the return value to {@link CESIUM_TRACE_ASYNC_ENLIST}.
 */
#define CESIUM_TRACE_NEW_ASYNC()                                               \
  CESIUM_TRACE_ASYNC_ENLIST(CESIUM_TRACE_ALLOCATE_ASYNC_ID())

/**
 * @brief Begins measuring an operation for a particular async ID.
 *
 * This operation is identical to {@link CESIUM_TRACE_BEGIN} except that the
 * operation it measures is explicitly tied to a particular async ID allocated
 * with {@link CESIUM_TRACE_ALLOCATE_ASYNC_ID} rather than using the ambient async
 * ID, or the current thread ID if the thread is not enlisted into into an
 * async process.
 *
 * @param name The name of the measured operation.
 */
#define CESIUM_TRACE_BEGIN_ID(name, id)                                        \
  CesiumUtility::Tracer::instance().writeAsyncTrace("cesium", name, 'b', id)

/**
 * @brief Ends measuring an operation for a particular async ID.
 *
 * This operation is identical to {@link CESIUM_TRACE_END} except that the
 * operation it measures is explicitly tied to a particular async ID allocated
 * with {@link CESIUM_TRACE_ALLOCATE_ASYNC_ID} rather than using the ambient async
 * ID, or the current thread ID if the thread is not enlisted into into an
 * async process.
 *
 * @param name The name of the measured operation.
 */
#define CESIUM_TRACE_END_ID(name, id)                                          \
  CesiumUtility::Tracer::instance().writeAsyncTrace("cesium", name, 'e', id)

#else
#define CESIUM_TRACE_INIT(filename)
#define CESIUM_TRACE_SHUTDOWN()
#define CESIUM_TRACE(name)
#define CESIUM_TRACE_BEGIN(name)
#define CESIUM_TRACE_END(name)
#define CESIUM_TRACE_ALLOCATE_ASYNC_ID() -1
#define CESIUM_TRACE_ASYNC_ENLIST(id)
#define CESIUM_TRACE_NEW_ASYNC()
#define CESIUM_TRACE_BEGIN_ID(name, id)
#define CESIUM_TRACE_END_ID(name, id)
#endif

namespace CesiumUtility {
struct Trace {
  std::string name;
  int64_t start;
  int64_t duration;
  std::thread::id threadID;
};

class Tracer {
public:
  static Tracer& instance();

  ~Tracer();

  void startTracing(const std::string& filePath = "trace.json");

  void writeTrace(const Trace& trace);
  void writeAsyncTrace(
      const char* category,
      const char* name,
      char type,
      int64_t id);
  void enlist(int64_t id);
  int64_t getEnlistedID() const;

  int64_t allocateID();

  void endTracing();

private:
  Tracer();

  std::ofstream _output;
  uint32_t _numTraces;
  std::mutex _lock;
  std::atomic<int64_t> _lastAllocatedID;
  static thread_local int64_t _threadEnlistedID;
};

// Internal class used by CESIUM_TRACE and TRACE_LOG do not use directly
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

private:
  int64_t _previousID;
};

} // namespace CesiumUtility
