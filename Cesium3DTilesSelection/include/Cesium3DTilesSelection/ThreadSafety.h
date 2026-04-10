#pragma once

// ThreadSafety.h — Ownership-discipline types for Cesium3DTilesSelection.
//
// These types encode threading contracts at the type level so that violations
// are caught in debug builds (assertion failures) rather than silently causing
// data races.
//
// Annotation convention for functions (comment-based; not enforced by tooling):
//   [main-thread]  — must only be called from the main thread.
//   [any-thread]   — may be called from any thread (the type itself or an
//                    atomic/future enforces safety internally).
//   [worker-thread] — must only be called from within a worker-thread dispatch.

#include <CesiumUtility/Assert.h>

#include <thread>
#include <type_traits>
#include <utility>

namespace Cesium3DTilesSelection {

/**
 * @brief Wraps a value that must only be accessed from the main thread.
 *
 * In debug builds, every access through `get()` (mutable or const) asserts
 * that `std::this_thread::get_id()` matches the thread on which this wrapper
 * was constructed. This catches accidental cross-thread access at runtime
 * without requiring a lock.
 *
 * In release builds the wrapper compiles away to zero overhead — `get()`
 * is a direct member reference with no branching or memory barrier.
 *
 * ### Usage
 *
 * ```cpp
 * // In a class declaration:
 * MainThreadOnly<TileContent> _content;
 * MainThreadOnly<TileLoadState> _loadState;
 *
 * // Access (debug: asserts main thread; release: direct access):
 * TileContent& c = _content.get();
 * ```
 *
 * @tparam T The wrapped value type.
 */
template <typename T> class MainThreadOnly {
public:
  /** @brief Default-construct the wrapped value on the calling (main) thread.
   */
  MainThreadOnly()
#ifndef NDEBUG
      : _ownerThread(std::this_thread::get_id())
#endif
  {
  }

  /** @brief Construct with an initial value on the calling (main) thread. */
  template <
      typename... Args,
      typename = std::enable_if_t<std::is_constructible_v<T, Args&&...>>>
  explicit MainThreadOnly(Args&&... args)
      : _value(std::forward<Args>(args)...)
#ifndef NDEBUG
        ,
        _ownerThread(std::this_thread::get_id())
#endif
  {
  }

  // Allow default copy/move so that TileContent and TileLoadState (both
  // trivially moveable) can be moved during tile tree construction, which
  // happens on the main thread.  Thread-id on copy/move keeps the owner as the
  // constructing thread (the main thread).
  /** @brief Copy constructor. Retains the owner thread of the source. */
  MainThreadOnly(const MainThreadOnly&) = default;
  /** @brief Move constructor. Retains the owner thread of the source. */
  MainThreadOnly(MainThreadOnly&&) noexcept = default;
  /** @brief Copy assignment. Retains the owner thread. */
  MainThreadOnly& operator=(const MainThreadOnly&) = default;
  /** @brief Move assignment. Retains the owner thread. */
  MainThreadOnly& operator=(MainThreadOnly&&) noexcept = default;
  ~MainThreadOnly() = default;

  /**
   * @brief Returns a mutable reference to the wrapped value.
   *
   * Debug-asserts that the caller is on the owner (main) thread.
   */
  T& get() noexcept {
    assertMainThread();
    return _value;
  }

  /**
   * @brief Returns a const reference to the wrapped value.
   *
   * Debug-asserts that the caller is on the owner (main) thread.
   *
   * Note: even reads are main-thread-only because the writer is also the main
   * thread and there is no synchronisation — an unsynchronised read from
   * another thread would still be a data race.
   */
  const T& get() const noexcept {
    assertMainThread();
    return _value;
  }

private:
  void assertMainThread() const noexcept {
#ifndef NDEBUG
    CESIUM_ASSERT(
        std::this_thread::get_id() == _ownerThread &&
        "MainThreadOnly<T> accessed from outside the main thread");
#endif
  }

  T _value{};
#ifndef NDEBUG
  std::thread::id _ownerThread;
#endif
};

} // namespace Cesium3DTilesSelection
