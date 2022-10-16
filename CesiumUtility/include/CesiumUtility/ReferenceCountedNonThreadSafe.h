#pragma once

#include <cstdint>

namespace CesiumUtility {

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
template <typename T> class ReferenceCountedNonThreadSafe {
public:
  ~ReferenceCountedNonThreadSafe() noexcept {
    assert(this->_referenceCount == 0);
  }

  /**
   * @brief Adds a counted reference to this object. Use
   * {@link CesiumUtility::IntrusivePointer} instead of calling this method
   * directly.
   *
   * This method is _not_ thread safe. Do not call it or use an
   * `IntrusivePointer` from multiple threads simultaneously.
   */
  void addReference() const noexcept { ++this->_referenceCount; }

  /**
   * @brief Removes a counted reference from this object. When the last
   * reference is removed, this method will delete this instance. Use
   * {@link CesiumUtility::IntrusivePointer} instead of calling this method
   * directly.
   *
   * This method is _not_ thread safe. Do not call it or use an
   * `IntrusivePointer` from multiple threads simultaneously.
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
  mutable std::int32_t _referenceCount{0};
};

} // namespace CesiumUtility
