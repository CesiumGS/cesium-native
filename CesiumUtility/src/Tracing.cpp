
#ifndef CESIUM_OVERRIDE_TRACING

#include <CesiumUtility/Tracing.h>

#if CESIUM_TRACING_ENABLED

namespace CesiumUtility {
namespace CesiumImpl {

Tracer& Tracer::instance() {
  static Tracer instance;
  return instance;
}

Tracer::~Tracer() { endTracing(); }

void Tracer::startTracing(const std::string& filePath) {
  this->_output.open(filePath);
  this->_output << "{\"otherData\": {},\"traceEvents\":[";
}

void Tracer::endTracing() {
  this->_output << "]}";
  this->_output.close();
}

void Tracer::writeCompleteEvent(const Trace& trace) {
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

void Tracer::writeAsyncEventBegin(const char* name, int64_t id) {
  this->writeAsyncEvent("cesium", name, 'b', id);
}

void Tracer::writeAsyncEventBegin(const char* name) {
  this->writeAsyncEventBegin(name, this->getCurrentThreadTrackID());
}

void Tracer::writeAsyncEventEnd(const char* name, int64_t id) {
  this->writeAsyncEvent("cesium", name, 'e', id);
}

void Tracer::writeAsyncEventEnd(const char* name) {
  this->writeAsyncEventEnd(name, this->getCurrentThreadTrackID());
}

int64_t Tracer::allocateTrackID() { return ++this->_lastAllocatedID; }

Tracer::Tracer() : _output{}, _numTraces{0}, _lock{}, _lastAllocatedID(0) {}

int64_t Tracer::getCurrentThreadTrackID() const {
  const TrackReference* pTrack = TrackReference::current();
  return pTrack->getTracingID();
}

void Tracer::writeAsyncEvent(
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

ScopedTrace::ScopedTrace(const std::string& message)
    : _name{message},
      _startTime{std::chrono::steady_clock::now()},
      _threadId{std::this_thread::get_id()},
      _reset{false} {
  CESIUM_TRACE_BEGIN_IN_TRACK(_name.c_str());
}

ScopedTrace::~ScopedTrace() {
  if (!this->_reset) {
    this->reset();
  }
}

void ScopedTrace::reset() {
  this->_reset = true;

  if (TrackReference::current() != nullptr) {
    CESIUM_TRACE_END(_name.c_str());
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
    Tracer::instance().writeCompleteEvent(
        {this->_name, start, end - start, this->_threadId});
  }
}

TrackSet::TrackSet(const char* name_) : name(name_) {}

TrackSet::~TrackSet() {
  std::scoped_lock lock(this->mutex);
  for (auto& track : this->tracks) {
    CESIUM_ASSERT(!track.inUse);
    Tracer::instance().writeAsyncEventEnd(
        (this->name + " " + std::to_string(track.id)).c_str(),
        track.id);
  }
}

size_t TrackSet::acquireTrack() {
  std::scoped_lock lock(this->mutex);

  auto it =
      std::find_if(this->tracks.begin(), this->tracks.end(), [](auto& track) {
        return track.inUse == false;
      });

  if (it != this->tracks.end()) {
    it->inUse = true;
    return size_t(it - this->tracks.begin());
  } else {
    Track track{Tracer::instance().allocateTrackID(), true};
    Tracer::instance().writeAsyncEventBegin(
        (this->name + " " + std::to_string(track.id)).c_str(),
        track.id);
    size_t index = this->tracks.size();
    this->tracks.emplace_back(track);
    return index;
  }
}

void TrackSet::addReference(size_t trackIndex) noexcept {
  std::scoped_lock lock(this->mutex);
  ++this->tracks[trackIndex].referenceCount;
}

void TrackSet::releaseReference(size_t trackIndex) noexcept {
  std::scoped_lock lock(this->mutex);
  Track& track = this->tracks[trackIndex];
  CESIUM_ASSERT(track.referenceCount > 0);
  --track.referenceCount;
  if (track.referenceCount == 0) {
    track.inUse = false;
  }
}

int64_t TrackSet::getTracingID(size_t trackIndex) noexcept {
  std::scoped_lock lock(this->mutex);
  if (trackIndex >= this->tracks.size()) {
    return -1;
  }
  return this->tracks[trackIndex].id;
}

TrackSet::TrackSet(TrackSet&& rhs) noexcept : name(), tracks(), mutex() {
  std::scoped_lock lock(rhs.mutex);
  name = rhs.name;
  tracks = rhs.tracks;
}

TrackSet& TrackSet::operator=(TrackSet&& rhs) noexcept {
  std::scoped_lock lock(rhs.mutex);
  std::scoped_lock lock2(this->mutex);
  name = std::move(rhs.name);
  tracks = std::move(rhs.tracks);
  return *this;
}

LambdaCaptureTrack::LambdaCaptureTrack() : pSet(nullptr), index(0) {
  const TrackReference* pTrack = TrackReference::current();
  if (pTrack) {
    this->pSet = pTrack->pSet;
    this->index = pTrack->index;

    if (this->pSet) {
      this->pSet->addReference(this->index);
    }
  }
}

LambdaCaptureTrack::LambdaCaptureTrack(const LambdaCaptureTrack& rhs) noexcept
    : pSet(rhs.pSet), index(rhs.index) {
  if (this->pSet) {
    this->pSet->addReference(this->index);
  }
}

LambdaCaptureTrack::LambdaCaptureTrack(LambdaCaptureTrack&& rhs) noexcept
    : pSet(rhs.pSet), index(rhs.index) {
  rhs.pSet = nullptr;
  rhs.index = 0;
}

LambdaCaptureTrack::~LambdaCaptureTrack() {
  if (this->pSet) {
    this->pSet->releaseReference(this->index);
  }
}

LambdaCaptureTrack&
LambdaCaptureTrack::operator=(const LambdaCaptureTrack& rhs) noexcept {
  if (rhs.pSet) {
    rhs.pSet->addReference(rhs.index);
  }
  if (this->pSet) {
    this->pSet->releaseReference(this->index);
  }

  this->pSet = rhs.pSet;
  this->index = rhs.index;

  return *this;
}

LambdaCaptureTrack&
LambdaCaptureTrack::operator=(LambdaCaptureTrack&& rhs) noexcept {
  if (this->pSet) {
    this->pSet->releaseReference(this->index);
  }

  this->pSet = rhs.pSet;
  this->index = rhs.index;

  rhs.pSet = nullptr;
  rhs.index = 0;

  return *this;
}

/*static*/ thread_local std::vector<TrackReference*>
    TrackReference::_threadEnlistedTracks{};

/*static*/ TrackReference* TrackReference::current() {
  return TrackReference::_threadEnlistedTracks.empty()
             ? nullptr
             : TrackReference::_threadEnlistedTracks.back();
}

TrackReference::TrackReference(TrackSet& set) noexcept
    : TrackReference(set, set.acquireTrack()) {}

TrackReference::TrackReference(TrackSet& set, size_t index_) noexcept
    : pSet(&set), index(index_) {
  this->pSet->addReference(this->index);
  this->enlistCurrentThread();
}

TrackReference::TrackReference(const LambdaCaptureTrack& lambdaCapture) noexcept
    : pSet(lambdaCapture.pSet), index(lambdaCapture.index) {
  if (this->pSet) {
    this->pSet->addReference(this->index);
    this->enlistCurrentThread();
  }
}

TrackReference::~TrackReference() noexcept {
  if (this->pSet) {
    this->dismissCurrentThread();
    this->pSet->releaseReference(this->index);
  }
}

TrackReference::operator bool() const noexcept { return this->pSet != nullptr; }

int64_t TrackReference::getTracingID() const noexcept {
  if (this->pSet) {
    return this->pSet->getTracingID(this->index);
  } else {
    return -1;
  }
}

void TrackReference::enlistCurrentThread() {
  if (!this->pSet) {
    return;
  }

  TrackReference::_threadEnlistedTracks.emplace_back(this);
}

void TrackReference::dismissCurrentThread() {
  if (!this->pSet) {
    return;
  }

  CESIUM_ASSERT(
      TrackReference::_threadEnlistedTracks.size() > 0 &&
      TrackReference::_threadEnlistedTracks.back() == this);

  TrackReference::_threadEnlistedTracks.pop_back();
}

} // namespace CesiumImpl
} // namespace CesiumUtility

#endif // CESIUM_TRACING_ENABLED

#endif
