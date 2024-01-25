#pragma once

#include <atomic>
#include <cstdint>

namespace CesiumUtility {

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
template <typename T> class ReferenceCountedThreadSafe {
public:
  /**
   * @brief Adds a counted reference to this object. Use
   * {@link CesiumUtility::IntrusivePointer} instead of calling this method
   * directly.
   */
  void addReference() const noexcept { ++this->_referenceCount; }

  /**
   * @brief Removes a counted reference from this object. When the last
   * reference is removed, this method will delete this instance. Use
   * {@link CesiumUtility::IntrusivePointer} instead of calling this method
   * directly.
   */
  void releaseReference() const noexcept {
    assert(this->_referenceCount > 0);
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
  mutable std::atomic<std::int32_t> _referenceCount{0};
};

} // namespace CesiumUtility
