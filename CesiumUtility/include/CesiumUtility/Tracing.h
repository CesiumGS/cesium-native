#pragma once

// If the build system doesn't enable the tracing support
// consider it disabled by default.
#ifndef CESIUM_TRACING_ENABLED
#define CESIUM_TRACING_ENABLED 0
#endif

#if !CESIUM_TRACING_ENABLED

#define CESIUM_TRACE_INIT(filename)
#define CESIUM_TRACE_SHUTDOWN()
#define CESIUM_TRACE(name)
#define CESIUM_TRACE_BEGIN(name)
#define CESIUM_TRACE_END(name)
#define CESIUM_TRACE_CURRENT_ASYNC_SLOT() nullptr
#define CESIUM_TRACE_ASYNC_ENLIST(id)
#define CESIUM_TRACE_NEW_ASYNC()
#define CESIUM_TRACE_DECLARE_TRACK_SET(id, name)
#define CESIUM_TRACE_USE_TRACK_SET(slotID)
#define CESIUM_TRACE_LAMBDA_CAPTURE_TRACK()
#define CESIUM_TRACE_USE_CAPTURED_TRACK()
#define CESIUM_TRACE_BEGIN_IN_TRACK(name)
#define CESIUM_TRACE_END_IN_TRACK(name)

#else

#include <atomic>
#include <cassert>
#include <chrono>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// helper macros to avoid shadowing variables
#define TRACE_NAME_AUX1(A, B) A##B
#define TRACE_NAME_AUX2(A, B) TRACE_NAME_AUX1(A, B)

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
  CesiumUtility::ScopedTrace TRACE_NAME_AUX2(cesiumTrace, __LINE__)(name)

/**
 * @brief Begins measuring an operation which may span scope but not threads.
 *
 * Begins measuring the time of an operation which completes with a
 * corresponding call to {@link CESIUM_TRACE_END}. If the calling thread is
 * operating in a track ({@link CESIUM_TRACE_USE_TRACK_SET}), the
 * time is recorded in the track. Otherwise, is is recorded against the current
 * thread.
 *
 * Extreme care must be taken to match calls to `CESIUM_TRACE_BEGIN` and
 * `CESIUM_TRACE_END`:
 *
 *   * Paired calls must use an identical `name`.
 *   * If either BEGIN or END is called from a thread operating in a track,
 *     then both must be, and it must be the same track in
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
  CesiumUtility::Tracer::instance().writeAsyncEventBegin(name)

/**
 * @brief Ends measuring an operation which may span scopes but not threads.
 *
 * Finishes measuring the time of an operation that began with a call to
 * {@link CESIUM_TRACE_BEGIN}. See the documentation for that macro for more
 * details and caveats.
 *
 * @param name The name of the measured operation.
 */
#define CESIUM_TRACE_END(name)                                                 \
  CesiumUtility::Tracer::instance().writeAsyncEventEnd(name)

/**
 * @brief Begins measuring an operation that may span both scopes and threads.
 * 
 * This macro is identical to {@link CESIUM_TRACE_BEGIN} except that it does
 * nothing if the calling thread and scope are not operating as part of a
 * track. This allows it to be safely used to measure operations that span
 * threads. Use {@link CESIUM_TRACE_USE_TRACK_SET} to use a track from a set.
 * 
 * @param name The name of the measured operation.
 */
#define CESIUM_TRACE_BEGIN_IN_TRACK(name)                                      \
  if (CesiumUtility::Tracer::instance().getEnlistedSlotReference() !=          \
      nullptr) {                                                               \
    CESIUM_TRACE_BEGIN(name);                                                  \
  }

/**
 * @brief Ends measuring an operation that may span both scopes and threads.
 * 
 * This macro is identical to {@link CESIUM_TRACE_END} except that it does
 * nothing if the calling thread and scope are not operating as part of a
 * track. This allows it to be safely used to measure operations that span
 * threads. Use {@link CESIUM_TRACE_USE_TRACK_SET} to use a track from a set.
 * 
 * @param name The name of the measured operation.
 */
#define CESIUM_TRACE_END_IN_TRACK(name)                                        \
  if (CesiumUtility::Tracer::instance().getEnlistedSlotReference() !=          \
      nullptr) {                                                               \
    CESIUM_TRACE_END(name);                                                    \
  }

/**
 * @brief Declares a set of tracing tracks as a field inside a class.
 *
 * A track is a sequential process that may take place across multiple threads.
 * An instance of a class may have multiple such tracks running simultaneously.
 * For example, a single 3D Tiles tile will load in a particular track, while
 * other tiles will be loading in other parallel tracks in the same set.
 *
 * Note that when the track set is destroyed, an assertion will check that no
 * tracks are still in progress.
 *
 * @param id The name of the field to hold the track set.
 * @param name A human-friendly name for this set of tracks.
 */
#define CESIUM_TRACE_DECLARE_TRACK_SET(id, name)                               \
  CesiumUtility::TraceAsyncSlots id { name }

/**
 * @brief Begins using a track set in this thread.
 *
 * The calling thread will be allocated a track from the track set, and will
 * continue using it for the remainder of the current scope. In addition, if
 * the thread starts an async operation using {@link CesiumAsync::AsyncSystem},
 * all continuations of that async operation will use the same track as well.
 *
 * @param id The ID (field name) of the track set declared with
 *           {@link CESIUM_TRACE_DECLARE_TRACK_SET}.
 */
#define CESIUM_TRACE_USE_TRACK_SET(id)                                         \
  CesiumUtility::SlotReference TRACE_NAME_AUX2(                                \
      cesiumTraceEnlistSlot,                                                   \
      __LINE__)(id, __FILE__, __LINE__);

/**
 * @brief Capture the current tracing track for a lambda, so that the lambda may
 * use the same track.
 *
 * This macro should be used in a lambda's capture list to capture the track of
 * the current thread so that the lambda (which may execute in a different
 * thread) can use the same track by executing
 * {@link CESIUM_TRACE_USE_CAPTURED_TRACK}.
 */
#define CESIUM_TRACE_LAMBDA_CAPTURE_TRACK()                                    \
  tracingSlot = CesiumUtility::LambdaCaptureSlot(__FILE__, __LINE__)

/**
 * @brief Uses a captured track for the current thread and the current scope.
 *
 * This macro should be used as the first line in a lambda that should inherit
 * the tracing track of the thread that created it. The lambda's capture list
 * must also contain {@link CESIUM_TRACE_USE_CAPTURED_TRACK}.
 */
#define CESIUM_TRACE_USE_CAPTURED_TRACK()                                      \
  CESIUM_TRACE_USE_TRACK_SET(tracingSlot)

namespace CesiumUtility {

// The following are internal classes used by the tracing framework, do not use
// directly.

struct Trace {
  std::string name;
  int64_t start;
  int64_t duration;
  std::thread::id threadID;
};

struct SlotReference;

class Tracer {
public:
  static Tracer& instance();

  ~Tracer();

  void startTracing(const std::string& filePath = "trace.json");

  void writeCompleteEvent(const Trace& trace);
  void writeAsyncEventBegin(const char* name, int64_t id);
  void writeAsyncEventBegin(const char* name);
  void writeAsyncEventEnd(const char* name, int64_t id);
  void writeAsyncEventEnd(const char* name);

  void enlist(SlotReference& slotReference);
  void unEnlist(SlotReference& slotReference);
  SlotReference* getEnlistedSlotReference() const;

  int64_t allocateID();

  void endTracing();

private:
  Tracer();

  int64_t getIDFromEnlistedSlotReference() const;
  void writeAsyncEvent(
      const char* category,
      const char* name,
      char type,
      int64_t id);

  std::ofstream _output;
  uint32_t _numTraces;
  std::mutex _lock;
  std::atomic<int64_t> _lastAllocatedID;
  static thread_local std::vector<SlotReference*> _threadEnlistedSlots;
};

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
  explicit ScopedEnlist(size_t index);
  ~ScopedEnlist();
};

struct TraceAsyncSlots {
  struct Slot {
    Slot(int64_t id_, bool inUse_)
        : id(id_), referenceCount(0), inUse(inUse_) {}

    int64_t id;
    int32_t referenceCount;
    bool inUse;
  };

  TraceAsyncSlots(const char* name);
  ~TraceAsyncSlots();

  void addReference(size_t index) noexcept;
  void releaseReference(size_t index) noexcept;

  std::string name;
  std::vector<Slot> slots;
  std::mutex mutex;

  size_t acquireSlot();
};

struct SlotReference;

struct LambdaCaptureSlot {
  LambdaCaptureSlot(const char* file, int32_t line);
  LambdaCaptureSlot(LambdaCaptureSlot&& rhs) noexcept;
  LambdaCaptureSlot(const LambdaCaptureSlot& rhs) noexcept;
  ~LambdaCaptureSlot();
  LambdaCaptureSlot& operator=(const LambdaCaptureSlot& rhs) noexcept;
  LambdaCaptureSlot& operator=(LambdaCaptureSlot&& rhs) noexcept;

  TraceAsyncSlots* pSlots;
  size_t index;
  const char* file;
  int32_t line;
};

// An RAII object to reference an async operation slot.
// When the last instance associated with a particular slot index is destroyed,
// the slot is released.
struct SlotReference {
  SlotReference(
      TraceAsyncSlots& slots,
      const char* file = nullptr,
      int32_t line = -1) noexcept;
  SlotReference(
      TraceAsyncSlots& slots,
      size_t index,
      const char* file = nullptr,
      int32_t line = -1) noexcept;
  SlotReference(
      const LambdaCaptureSlot& lambdaCapture,
      const char* file = nullptr,
      int32_t line = -1) noexcept;
  SlotReference(const SlotReference& rhs) = delete;
  SlotReference(SlotReference&& rhs) = delete;
  ~SlotReference() noexcept;

  SlotReference& operator=(const SlotReference& rhs) = delete;
  SlotReference& operator=(SlotReference&& rhs) = delete;

  operator bool() const noexcept;

  TraceAsyncSlots* pSlots;
  size_t index;
  const char* file;
  int32_t line;
};

} // namespace CesiumUtility

#endif // CESIUM_TRACING_ENABLED
