#pragma once
#include <type_traits>
#include <utility>

namespace CesiumUtility {

/**
 * @brief A smart pointer that calls `addReference` and `releaseReference` on
 * the controlled object.
 *
 * Please note that the thread-safety of this type is entirely dependent on the
 * implementation of `addReference` and `releaseReference`. If these methods are
 * not thread safe on a particular type - which is common for objects that are
 * not meant to be used from multiple threads simultaneously - then using an
 * `IntrusivePointer` from multiple threads is also unsafe.
 *
 * @tparam T The type of object controlled.
 */
template <class T> class IntrusivePointer final {
public:
  /**
   * @brief Default constructor.
   */
  IntrusivePointer(T* p = nullptr) noexcept : _p(p) { this->addReference(); }

  /**
   * @brief Copy constructor.
   */
  IntrusivePointer(const IntrusivePointer<T>& rhs) noexcept : _p(rhs._p) {
    this->addReference();
  }

  /**
   * @brief Implicit conversion to a pointer to a base (or otherwise
   * convertible) type.
   *
   * @tparam U The new type, usually a base class.
   * @param rhs The pointer.
   */
  template <class U>
  IntrusivePointer(const IntrusivePointer<U>& rhs) noexcept : _p(rhs._p) {
    this->addReference();
  }

  /**
   * @brief Move constructor.
   */
  IntrusivePointer(IntrusivePointer<T>&& rhs) noexcept
      : _p(std::exchange(rhs._p, nullptr)) {
    // Reference count is unchanged
  }

  /**
   * @brief Implicit conversion of an r-value to a pointer to a base (or
   * otherwise convertible) type.
   *
   * @tparam U The new type, usually a base class.
   * @param rhs The pointer.
   */
  template <class U>
  IntrusivePointer(IntrusivePointer<U>&& rhs) noexcept
      : _p(std::exchange(rhs._p, nullptr)) {
    // Reference count is unchanged
  }

  /**
   * @brief Default destructor.
   */
  ~IntrusivePointer() noexcept { this->releaseReference(); }

  /**
   * @brief Constructs a new instance and assigns it to this IntrusivePointer.
   * If this IntrusivePointer already points to another instance,
   * {@link releaseReference} is called on it.
   *
   * @param constructorArguments The arguments to the constructor to create the
   * new instance.
   * @return A reference to the newly-created instance.
   */
  template <typename... ConstructorArgumentTypes>
  T& emplace(ConstructorArgumentTypes&&... constructorArguments) {
    *this =
        new T(std::forward<ConstructorArgumentTypes>(constructorArguments)...);
    return *this->get();
  }

  /**
   * @brief Reset this pointer to nullptr.
   */
  void reset() { *this = nullptr; }

  /**
   * @brief Assignment operator.
   */
  // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
  IntrusivePointer& operator=(const IntrusivePointer& rhs) noexcept {
    if (*this != rhs) {
      // addReference the new pointer before releaseReference'ing the old.
      T* pOld = this->_p;
      this->_p = rhs._p;
      addReference();

      if (pOld) {
        pOld->releaseReference();
      }
    }

    return *this;
  }

  /**
   * @brief Assigns an \ref IntrusivePointer of another type to this \ref
   * IntrusivePointer.
   */
  template <class U>
  IntrusivePointer& operator=(const IntrusivePointer<U>& rhs) noexcept {
    if (*this != rhs) {
      // addReference the new pointer before releaseReference'ing the old.
      T* pOld = this->_p;
      this->_p = rhs._p;
      addReference();

      if (pOld) {
        pOld->releaseReference();
      }
    }

    return *this;
  }

  /**
   * @brief Move assignment operator.
   */
  IntrusivePointer& operator=(IntrusivePointer&& rhs) noexcept {
    if (this->_p != rhs._p) {
      std::swap(this->_p, rhs._p);
    }

    return *this;
  }

  /**
   * @brief Assignment operator.
   */
  IntrusivePointer& operator=(T* p) noexcept {
    if (this->_p != p) {
      // addReference the new pointer before releaseReference'ing the old.
      T* pOld = this->_p;
      this->_p = p;
      addReference();

      if (pOld) {
        pOld->releaseReference();
      }
    }

    return *this;
  }

  /**
   * @brief Dereferencing operator.
   */
  T& operator*() const noexcept { return *this->_p; }

  /**
   * @brief Arrow operator.
   */
  T* operator->() const noexcept { return this->_p; }

  /**
   * @brief Implicit conversion to `bool`, being `true` iff this is not the
   * `nullptr`.
   */
  explicit operator bool() const noexcept { return this->_p != nullptr; }

  /**
   * @brief Returns the internal pointer.
   */
  T* get() const noexcept { return this->_p; }

  /**
   * @brief Returns `true` if two pointers are equal.
   */
  bool operator==(const IntrusivePointer<T>& rhs) const noexcept {
    return this->_p == rhs._p;
  }

  /** @brief Returns `true` if two pointers are equal. */
  template <class U>
  bool operator==(const IntrusivePointer<U>& rhs) const noexcept {
    return this->_p == rhs._p;
  }

  /**
   * @brief Returns `true` if two pointers are *not* equal.
   */
  bool operator!=(const IntrusivePointer<T>& rhs) const noexcept {
    return !(*this == rhs);
  }

  /**
   * @brief Returns `true` if two pointers are *not* equal.
   */
  template <class U>
  bool operator!=(const IntrusivePointer<U>& rhs) const noexcept {
    return !(*this == rhs);
  }

  /**
   * @brief Returns `true` if the contents of this pointer is equal to the given
   * pointer.
   */
  bool operator==(const T* pRhs) const noexcept { return this->_p == pRhs; }

  /**
   * @brief Returns `true` if the contents of this pointer is *not* equal to the
   * given pointer.
   */
  bool operator!=(const T* pRhs) const noexcept { return !(*this == pRhs); }

private:
  void addReference() noexcept {
    if (this->_p) {
      this->_p->addReference();
    }
  }

  void releaseReference() noexcept {
    if (this->_p) {
      this->_p->releaseReference();
    }
  }

  T* _p;
  template <typename U> friend class IntrusivePointer;
};

/**
 * @brief Casts a `const` \ref IntrusivePointer to its non-const equivalent.
 *
 * @param p The `const` \ref IntrusivePointer.
 * @returns A non-const \ref IntrusivePointer with the same underlying pointer.
 */
template <typename T, typename U>
IntrusivePointer<T>
const_intrusive_cast(const IntrusivePointer<U>& p) noexcept {
  return IntrusivePointer<T>(const_cast<T*>(p.get()));
}

} // namespace CesiumUtility
