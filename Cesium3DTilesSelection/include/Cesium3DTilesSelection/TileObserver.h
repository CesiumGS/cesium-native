#pragma once

#include <CesiumUtility/Assert.h>

#include <cstddef>

namespace Cesium3DTilesSelection {

/**
 * @brief A non-owning, non-reference-counted observer pointer.
 *
 * This type documents that the holder **does not** manage the lifetime of the
 * pointed-to object. It provides the same null-check and dereference interface
 * as a raw pointer, but makes the ownership intent explicit at the type level.
 *
 * Unlike `std::shared_ptr` or `CesiumUtility::IntrusivePointer`, constructing
 * or destroying a `TileObserver` never increments or decrements any reference
 * count. The pointed-to object must stay alive by some other means for the
 * duration of the observer's use.
 *
 * Intended use: parent-back-pointer in the `Tile` hierarchy, where children
 * observe (but do not own) their parent. The parent's lifetime is guaranteed
 * by the child holding an *intrusive reference count* on the parent — that
 * reference counting is done explicitly via `addReference` / `releaseReference`
 * in `Tile.cpp`, and is a separate mechanism from this type.
 *
 * @tparam T The pointed-to type.
 */
template <typename T> class TileObserver {
public:
  /** @brief Constructs a null observer. */
  constexpr TileObserver() noexcept : _ptr(nullptr) {}

  /**
   * @brief Constructs an observer pointing at `ptr`.
   *
   * Accepts `nullptr` explicitly because the field is frequently initialized
   * to `nullptr` before a parent is set.
   */
  constexpr explicit TileObserver(T* ptr) noexcept : _ptr(ptr) {}

  /** @brief Constructs a null observer from a null pointer literal. */
  constexpr TileObserver(std::nullptr_t) noexcept : _ptr(nullptr) {}

  // Non-owning — no special copy/move/destructor semantics needed.
  /** @brief Copy constructor. */
  TileObserver(const TileObserver&) noexcept = default;
  /** @brief Move constructor. */
  TileObserver(TileObserver&&) noexcept = default;
  /** @brief Copy assignment. */
  TileObserver& operator=(const TileObserver&) noexcept = default;
  /** @brief Move assignment. */
  TileObserver& operator=(TileObserver&&) noexcept = default;
  ~TileObserver() noexcept = default;

  /**
   * @brief Returns the observed pointer. May be `nullptr`.
   */
  constexpr T* get() const noexcept { return _ptr; }

  /**
   * @brief Arrow operator. Debug-asserts that the pointer is not null.
   */
  constexpr T* operator->() const noexcept {
    CESIUM_ASSERT(_ptr != nullptr);
    return _ptr;
  }

  /**
   * @brief Dereference operator. Debug-asserts that the pointer is not null.
   */
  constexpr T& operator*() const noexcept {
    CESIUM_ASSERT(_ptr != nullptr);
    return *_ptr;
  }

  /** @brief Returns `true` if the observed pointer is non-null. */
  constexpr explicit operator bool() const noexcept { return _ptr != nullptr; }

  /** @brief Sets the observed pointer, or resets to null. */
  void reset(T* ptr = nullptr) noexcept { _ptr = ptr; }

  /** @copydoc TileObserver::reset() */
  TileObserver& operator=(std::nullptr_t) noexcept {
    _ptr = nullptr;
    return *this;
  }

  /** @brief Equality comparison. */
  friend constexpr bool
  operator==(const TileObserver& lhs, const TileObserver& rhs) noexcept {
    return lhs._ptr == rhs._ptr;
  }
  /** @brief Inequality comparison. */
  friend constexpr bool
  operator!=(const TileObserver& lhs, const TileObserver& rhs) noexcept {
    return lhs._ptr != rhs._ptr;
  }

  /** @brief Equality comparison with nullptr. */
  friend constexpr bool
  operator==(const TileObserver& lhs, std::nullptr_t) noexcept {
    return lhs._ptr == nullptr;
  }
  /** @brief Inequality comparison with nullptr. */
  friend constexpr bool
  operator!=(const TileObserver& lhs, std::nullptr_t) noexcept {
    return lhs._ptr != nullptr;
  }
  /** @brief Equality comparison with nullptr (reversed order). */
  friend constexpr bool
  operator==(std::nullptr_t, const TileObserver& rhs) noexcept {
    return rhs._ptr == nullptr;
  }
  /** @brief Inequality comparison with nullptr (reversed order). */
  friend constexpr bool
  operator!=(std::nullptr_t, const TileObserver& rhs) noexcept {
    return rhs._ptr != nullptr;
  }

  /** @brief Raw-pointer equality (useful for CESIUM_ASSERT comparisons). */
  friend constexpr bool
  operator==(const TileObserver& lhs, const T* rhs) noexcept {
    return lhs._ptr == rhs;
  }
  /** @brief Raw-pointer inequality. */
  friend constexpr bool
  operator!=(const TileObserver& lhs, const T* rhs) noexcept {
    return lhs._ptr != rhs;
  }

private:
  T* _ptr;
};

} // namespace Cesium3DTilesSelection
