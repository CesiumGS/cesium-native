#pragma once

#include <CesiumUtility/Assert.h>

#include <atomic>
#include <cstdint>

#ifndef NDEBUG
#include <thread>
#endif

namespace CesiumUtility {

/** \cond Doxygen_Suppress */
#ifndef NDEBUG
template <bool isThreadSafe> class ThreadIdHolder;

template <> class ThreadIdHolder<false> {
  ThreadIdHolder() : _threadID(std::this_thread::get_id()) {}

  std::thread::id _threadID;

  template <typename T, bool isThreadSafe> friend class ReferenceCounted;
};

template <> class ThreadIdHolder<true> {};
#endif
/** \endcond */

/**
 * @brief A reference-counted base class, meant to be used with
 * {@link IntrusivePointer}.
 *
 * Consider using {@link ReferenceCountedThreadSafe} or
 * {@link ReferenceCountedNonThreadSafe} instead of using this class directly.
 *
 * @tparam T The type that is _deriving_ from this class. For example, you
 * should declare your class as
 * `class MyClass : public ReferenceCounted<MyClass> { ... };`
 * @tparam isThreadSafe If `true`, the reference count will be thread-safe by
 * using `std::atomic`, allowing references to safely be added and removed from
 * any thread at any time. The object will be destroyed in the thread that
 * releases the last reference. If false, it uses a simple integer for the
 * reference count, which is not thread safe. In this case, references must be
 * added and removed (including automatically via `IntrusivePointer`) from only
 * one thread at a time. However, this mode has a bit less overhead for objects
 * that are only ever accessed from a single thread.
 */
template <typename T, bool isThreadSafe = true>
class ReferenceCounted
#ifndef NDEBUG
    : public ThreadIdHolder<isThreadSafe>
#endif
{
public:
  ~ReferenceCounted() noexcept { CESIUM_ASSERT(this->_referenceCount == 0); }

  /**
   * @brief Adds a counted reference to this object. Use
   * {@link CesiumUtility::IntrusivePointer} instead of calling this method
   * directly.
   */
  void addReference() const /*noexcept*/ {
#ifndef NDEBUG
    if constexpr (!isThreadSafe) {
      CESIUM_ASSERT(std::this_thread::get_id() == this->_threadID);
    }
#endif

    ++this->_referenceCount;
  }

  /**
   * @brief Removes a counted reference from this object. When the last
   * reference is removed, this method will delete this instance. Use
   * {@link CesiumUtility::IntrusivePointer} instead of calling this method
   * directly.
   */
  void releaseReference() const /*noexcept*/ {
#ifndef NDEBUG
    if constexpr (!isThreadSafe) {
      CESIUM_ASSERT(std::this_thread::get_id() == this->_threadID);
    }
#endif

    CESIUM_ASSERT(this->_referenceCount > 0);
    const int32_t references = --this->_referenceCount;
    if (references == 0) {
      delete static_cast<const T*>(this);
    }
  }

  /**
   * @brief Returns the current reference count of this instance.
   */
  std::int32_t getReferenceCount() const noexcept {
    return this->_referenceCount;
  }

private:
  ReferenceCounted() noexcept = default;
  friend T;

  using ThreadSafeCounter = std::atomic<std::int32_t>;
  using NonThreadSafeCounter = std::int32_t;
  using CounterType =
      std::conditional_t<isThreadSafe, ThreadSafeCounter, NonThreadSafeCounter>;

  mutable CounterType _referenceCount{0};
};

/**
 * @brief A reference-counted base class, meant to be used with
 * {@link IntrusivePointer}. The reference count is thread-safe, so references
 * may be added and removed from any thread at any time. The object will be
 * destroyed in the thread that releases the last reference.
 *
 * @tparam T The type that is _deriving_ from this class. For example, you
 * should declare your class as
 * `class MyClass : public ReferenceCountedThreadSafe<MyClass> { ... };`
 */
template <typename T>
using ReferenceCountedThreadSafe = ReferenceCounted<T, true>;

/**
 * @brief A reference-counted base class, meant to be used with
 * {@link IntrusivePointer}. The reference count is not thread-safe, so
 * references must be added and removed (including automatically via
 * `IntrusivePointer`) from only one thread at a time.
 *
 * @tparam T The type that is _deriving_ from this class. For example, you
 * should declare your class as
 * `class MyClass : public ReferenceCountedNonThreadSafe<MyClass> { ... };`
 */
template <typename T>
using ReferenceCountedNonThreadSafe = ReferenceCounted<T, false>;

} // namespace CesiumUtility
