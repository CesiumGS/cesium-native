#pragma once

#ifndef CESIUM_OVERRIDE_TRACING

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
#define CESIUM_TRACE_BEGIN_IN_TRACK(name)
#define CESIUM_TRACE_END_IN_TRACK(name)
#define CESIUM_TRACE_DECLARE_TRACK_SET(id, name)
#define CESIUM_TRACE_USE_TRACK_SET(id)
#define CESIUM_TRACE_LAMBDA_CAPTURE_TRACK() tracingTrack = false
#define CESIUM_TRACE_USE_CAPTURED_TRACK()

#else

#include <atomic>
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
  CesiumUtility::CesiumImpl::Tracer::instance().startTracing(filename)

/**
 * @brief Shuts down tracing and closes the JSON tracing file.
 */
#define CESIUM_TRACE_SHUTDOWN()                                                \
  CesiumUtility::CesiumImpl::Tracer::instance().endTracing()

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
  CesiumUtility::CesiumImpl::ScopedTrace TRACE_NAME_AUX2(                      \
      cesiumTrace,                                                             \
      __LINE__)(name)

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
 *     threads. However, it safer to use {@link CESIUM_TRACE_BEGIN_IN_TRACK}
 *     in this scenario.
 *   * If either BEGIN or END is called from a thread _not_ enlisted into a
 *     track, then both must be called from the same thread and neither
 *     thread may be in a track.
 *   * Paired calls must not be interleaved with other BEGIN/END calls for the
 *     same thread or track. Other BEGIN/END pairs may be fully
 *     nested within this one, but this pair must not END in between a nested
 *     measurement's BEGIN and END calls.
 *
 * Failure to ensure the above may lead to generation of a trace file that the
 * Chromium trace viewer is not able to correctly interpret.
 *
 * @param name The name of the measured operation.
 */
#define CESIUM_TRACE_BEGIN(name)                                               \
  CesiumUtility::CesiumImpl::Tracer::instance().writeAsyncEventBegin(name)

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
  CesiumUtility::CesiumImpl::Tracer::instance().writeAsyncEventEnd(name)

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
  if (CesiumUtility::CesiumImpl::TrackReference::current() != nullptr) {       \
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
  if (CesiumUtility::CesiumImpl::TrackReference::current() != nullptr) {       \
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
  CesiumUtility::CesiumImpl::TrackSet id { name }

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
  CesiumUtility::CesiumImpl::TrackReference TRACE_NAME_AUX2(                   \
      cesiumTraceEnlistTrack,                                                  \
      __LINE__)(id);

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
  tracingTrack = CesiumUtility::CesiumImpl::LambdaCaptureTrack()

/**
 * @brief Uses a captured track for the current thread and the current scope.
 *
 * This macro should be used as the first line in a lambda that should inherit
 * the tracing track of the thread that created it. The lambda's capture list
 * must also contain {@link CESIUM_TRACE_USE_CAPTURED_TRACK}.
 */
#define CESIUM_TRACE_USE_CAPTURED_TRACK()                                      \
  CESIUM_TRACE_USE_TRACK_SET(tracingTrack)

namespace CesiumUtility {
namespace CesiumImpl {

// The following are internal classes used by the tracing framework, do not use
// directly.

struct Trace {
  std::string name;
  int64_t start;
  int64_t duration;
  std::thread::id threadID;
};

class TrackReference;

class Tracer {
public:
  static Tracer& instance();

  ~Tracer();

  void startTracing(const std::string& filePath = "trace.json");
  void endTracing();

  void writeCompleteEvent(const Trace& trace);
  void writeAsyncEventBegin(const char* name, int64_t id);
  void writeAsyncEventBegin(const char* name);
  void writeAsyncEventEnd(const char* name, int64_t id);
  void writeAsyncEventEnd(const char* name);

  int64_t allocateTrackID();

private:
  Tracer();

  int64_t getCurrentThreadTrackID() const;
  void writeAsyncEvent(
      const char* category,
      const char* name,
      char type,
      int64_t id);

  std::ofstream _output;
  uint32_t _numTraces;
  std::mutex _lock;
  std::atomic<int64_t> _lastAllocatedID;
};

class ScopedTrace {
public:
  explicit ScopedTrace(const std::string& message);
  ~ScopedTrace();

  void reset();

  ScopedTrace(const ScopedTrace& rhs) = delete;
  ScopedTrace(ScopedTrace&& rhs) = delete;
  ScopedTrace& operator=(const ScopedTrace& rhs) = delete;
  ScopedTrace& operator=(ScopedTrace&& rhs) = delete;

private:
  std::string _name;
  std::chrono::steady_clock::time_point _startTime;
  std::thread::id _threadId;
  bool _reset;
};

class TrackSet {
public:
  explicit TrackSet(const char* name);
  ~TrackSet();

  size_t acquireTrack();
  void addReference(size_t trackIndex) noexcept;
  void releaseReference(size_t trackIndex) noexcept;

  int64_t getTracingID(size_t trackIndex) noexcept;

  TrackSet(TrackSet&& rhs) noexcept;
  TrackSet& operator=(TrackSet&& rhs) noexcept;

private:
  struct Track {
    Track(int64_t id_, bool inUse_)
        : id(id_), referenceCount(0), inUse(inUse_) {}

    int64_t id;
    int32_t referenceCount;
    bool inUse;
  };

  std::string name;
  std::vector<Track> tracks;
  std::mutex mutex;
};

class LambdaCaptureTrack {
public:
  LambdaCaptureTrack();
  LambdaCaptureTrack(LambdaCaptureTrack&& rhs) noexcept;
  LambdaCaptureTrack(const LambdaCaptureTrack& rhs) noexcept;
  ~LambdaCaptureTrack();
  LambdaCaptureTrack& operator=(const LambdaCaptureTrack& rhs) noexcept;
  LambdaCaptureTrack& operator=(LambdaCaptureTrack&& rhs) noexcept;

private:
  TrackSet* pSet;
  size_t index;

  friend class TrackReference;
};

// An RAII object to reference an async operation track.
// When the last instance associated with a particular track index is destroyed,
// the track is released back to the track set.
class TrackReference {
public:
  static TrackReference* current();

  TrackReference(TrackSet& set) noexcept;
  TrackReference(TrackSet& set, size_t index) noexcept;
  TrackReference(const LambdaCaptureTrack& lambdaCapture) noexcept;
  ~TrackReference() noexcept;

  operator bool() const noexcept;
  int64_t getTracingID() const noexcept;

  TrackReference(const TrackReference& rhs) = delete;
  TrackReference(TrackReference&& rhs) = delete;
  TrackReference& operator=(const TrackReference& rhs) = delete;
  TrackReference& operator=(TrackReference&& rhs) = delete;

private:
  void enlistCurrentThread();
  void dismissCurrentThread();

  TrackSet* pSet;
  size_t index;

  static thread_local std::vector<TrackReference*> _threadEnlistedTracks;

  friend class LambdaCaptureTrack;
};

} // namespace CesiumImpl
} // namespace CesiumUtility

#endif // CESIUM_TRACING_ENABLED

#endif
