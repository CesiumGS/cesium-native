#include "CesiumUtility/Tracing.h"
#include <algorithm>
#include <cassert>

#if CESIUM_TRACING_ENABLED

namespace CesiumUtility {
Tracer& Tracer::instance() {
  static Tracer instance;
  return instance;
}

/*static*/ thread_local std::vector<SlotReference*>
    Tracer::_threadEnlistedSlots{};

Tracer::Tracer() : _output{}, _numTraces{0}, _lock{}, _lastAllocatedID(0) {}

Tracer::~Tracer() { endTracing(); }

void Tracer::startTracing(const std::string& filePath) {
  this->_output.open(filePath);
  this->_output << "{\"otherData\": {},\"traceEvents\":[";
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
  this->writeAsyncEventBegin(name, this->getIDFromEnlistedSlotReference());
}

void Tracer::writeAsyncEventEnd(const char* name, int64_t id) {
  this->writeAsyncEvent("cesium", name, 'e', id);
}

void Tracer::writeAsyncEventEnd(const char* name) {
  this->writeAsyncEventEnd(name, this->getIDFromEnlistedSlotReference());
}

int64_t Tracer::getIDFromEnlistedSlotReference() const {
  const SlotReference* pSlot =
      CesiumUtility::Tracer::instance().getEnlistedSlotReference();
  if (pSlot && *pSlot) {
    std::scoped_lock lock(pSlot->pSlots->mutex);
    return pSlot->pSlots->slots[pSlot->index].id;
  } else {
    return -1;
  }
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

void Tracer::enlist(SlotReference& slotReference) {
  if (!slotReference.pSlots) {
    return;
  }

  Tracer::_threadEnlistedSlots.emplace_back(&slotReference);
}

void Tracer::unEnlist([[maybe_unused]] SlotReference& slotReference) {
  if (!slotReference.pSlots) {
    return;
  }

  assert(
      Tracer::_threadEnlistedSlots.size() > 0 &&
      Tracer::_threadEnlistedSlots.back() == &slotReference);
  Tracer::_threadEnlistedSlots.pop_back();
}

SlotReference* Tracer::getEnlistedSlotReference() const {
  return Tracer::_threadEnlistedSlots.empty()
             ? nullptr
             : Tracer::_threadEnlistedSlots.back();
}

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
  CESIUM_TRACE_BEGIN_IN_TRACK(_name.c_str());
}

void ScopedTrace::reset() {
  this->_reset = true;

  if (CesiumUtility::Tracer::instance().getEnlistedSlotReference() != nullptr) {
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

ScopedTrace::~ScopedTrace() {
  if (!this->_reset) {
    this->reset();
  }
}

TraceAsyncSlots::TraceAsyncSlots(const char* name_) : name(name_) {}

TraceAsyncSlots::~TraceAsyncSlots() {
  std::scoped_lock lock(this->mutex);
  for (auto& slot : this->slots) {
    assert(!slot.inUse);
    Tracer::instance().writeAsyncEventEnd(
        (this->name + " " + std::to_string(slot.id)).c_str(),
        slot.id);
  }
}

void TraceAsyncSlots::addReference(size_t index) noexcept {
  std::scoped_lock lock(this->mutex);
  ++this->slots[index].referenceCount;
}

void TraceAsyncSlots::releaseReference(size_t index) noexcept {
  std::scoped_lock lock(this->mutex);
  Slot& slot = this->slots[index];
  assert(slot.referenceCount > 0);
  --slot.referenceCount;
  if (slot.referenceCount == 0) {
    slot.inUse = false;
  }
}

size_t TraceAsyncSlots::acquireSlot() {
  std::scoped_lock lock(this->mutex);

  auto it =
      std::find_if(this->slots.begin(), this->slots.end(), [](auto& slot) {
        return slot.inUse == false;
      });

  if (it != this->slots.end()) {
    it->inUse = true;
    return size_t(it - this->slots.begin());
  } else {
    Slot slot{CesiumUtility::Tracer::instance().allocateID(), true};
    CesiumUtility::Tracer::instance().writeAsyncEventBegin(
        (this->name + " " + std::to_string(slot.id)).c_str(),
        slot.id);
    size_t index = this->slots.size();
    this->slots.emplace_back(slot);
    return index;
  }
}

LambdaCaptureSlot::LambdaCaptureSlot(const char* file_, int32_t line_)
    : pSlots(nullptr), index(0), file(file_), line(line_) {
  const SlotReference* pSlot = Tracer::instance().getEnlistedSlotReference();
  if (pSlot) {
    this->pSlots = pSlot->pSlots;
    this->index = pSlot->index;

    if (this->pSlots) {
      this->pSlots->addReference(this->index);
    }
  }
}

LambdaCaptureSlot::LambdaCaptureSlot(const LambdaCaptureSlot& rhs) noexcept
    : pSlots(rhs.pSlots), index(rhs.index), file(rhs.file), line(rhs.line) {
  if (this->pSlots) {
    this->pSlots->addReference(this->index);
  }
}

LambdaCaptureSlot::LambdaCaptureSlot(LambdaCaptureSlot&& rhs) noexcept
    : pSlots(rhs.pSlots), index(rhs.index), file(rhs.file), line(rhs.line) {
  rhs.pSlots = nullptr;
  rhs.index = 0;
}

LambdaCaptureSlot::~LambdaCaptureSlot() {
  if (this->pSlots) {
    this->pSlots->releaseReference(this->index);
  }
}

LambdaCaptureSlot&
LambdaCaptureSlot::operator=(const LambdaCaptureSlot& rhs) noexcept {
  if (rhs.pSlots) {
    rhs.pSlots->addReference(rhs.index);
  }
  if (this->pSlots) {
    this->pSlots->releaseReference(this->index);
  }

  this->pSlots = rhs.pSlots;
  this->index = rhs.index;
  this->file = rhs.file;
  this->line = rhs.line;

  return *this;
}

LambdaCaptureSlot&
LambdaCaptureSlot::operator=(LambdaCaptureSlot&& rhs) noexcept {
  if (this->pSlots) {
    this->pSlots->releaseReference(this->index);
  }

  this->pSlots = rhs.pSlots;
  this->index = rhs.index;

  rhs.pSlots = nullptr;
  rhs.index = 0;

  return *this;
}

SlotReference::SlotReference(
    TraceAsyncSlots& slots_,
    const char* file,
    int32_t line) noexcept
    : SlotReference(slots_, slots_.acquireSlot(), file, line) {}

SlotReference::SlotReference(
    TraceAsyncSlots& slots_,
    size_t index_,
    const char* file_,
    int32_t line_) noexcept
    : pSlots(&slots_), index(index_), file(file_), line(line_) {
  this->pSlots->addReference(this->index);
  Tracer::instance().enlist(*this);
}

SlotReference::SlotReference(
    const LambdaCaptureSlot& lambdaCapture,
    const char* file_,
    int32_t line_) noexcept
    : pSlots(lambdaCapture.pSlots),
      index(lambdaCapture.index),
      file(file_),
      line(line_) {
  if (this->pSlots) {
    this->pSlots->addReference(this->index);
    Tracer::instance().enlist(*this);
  }
}

// SlotReference::SlotReference(const SlotReference& rhs) noexcept
//     : pSlots(rhs.pSlots), index(rhs.index) {
//   if (this->pSlots) {
//     this->pSlots->addReference(this->index);
//     Tracer::instance().enlist(*this);
//   }
// }

SlotReference::~SlotReference() noexcept {
  if (this->pSlots) {
    Tracer::instance().unEnlist(*this);
    this->pSlots->releaseReference(this->index);
  }
}

SlotReference::operator bool() const noexcept {
  return this->pSlots != nullptr;
}

} // namespace CesiumUtility

#endif // CESIUM_TRACING_ENABLED
