#pragma once

/// @file Promise.h
/// Drop-in replacement for CesiumAsync::Promise<T> backed by orkester.
///
/// Holds a shared_ptr<ValueSlot<T>> (C++ value) and a orkester_promise_t (Rust
/// signal).

#include <CesiumAsync/Impl/OrkesterImpl.h>

#include <string>

namespace CesiumAsync {

// ─── Promise<T> ─────────────────────────────────────────────────────────────

/// Drop-in replacement for CesiumAsync::Promise<T>, backed by orkester.
///
/// Copyable (shared internal state). resolve/reject are const-qualified
/// so they can be called from non-mutable lambdas and std::function.
template <typename T> class CESIUMASYNC_API Promise final {
public:
  Promise(const Promise&) = default;
  Promise& operator=(const Promise&) = default;
  Promise(Promise&&) noexcept = default;
  Promise& operator=(Promise&&) noexcept = default;
  ~Promise() noexcept = default;

  /// Resolve with a value. Stores in C++ slot, then signals Rust.
  void resolve(T&& value) const {
    _state->slot->store(std::move(value));
    auto p = _state->promise;
    _state->promise = nullptr;
    orkester_promise_resolve(p);
  }

  void resolve(const T& value) const {
    _state->slot->store(value);
    auto p = _state->promise;
    _state->promise = nullptr;
    orkester_promise_resolve(p);
  }

  /// Reject with an exception_ptr.
  void reject(std::exception_ptr error) const {
    _state->slot->storeError(error);
    auto p = _state->promise;
    _state->promise = nullptr;
    orkester_promise_resolve(p);
  }

  /// Reject with any exception object.
  template <
      typename E,
      std::enable_if_t<
          !std::is_same_v<std::decay_t<E>, std::exception_ptr>,
          int> = 0>
  void reject(E&& exception) const {
    reject(std::make_exception_ptr(std::forward<E>(exception)));
  }

  /// Get the paired Future (can only be called once).
  Future<T> getFuture() {
    auto signal = _state->futureSignal;
    _state->futureSignal = nullptr;
    return Future<T>(_state->system, _state->slot, signal);
  }

private:
  friend class AsyncSystem;
  template <typename R> friend class Future;

  struct State {
    const orkester_async_t* system = nullptr;
    std::shared_ptr<CesiumImpl::ValueSlot<T>> slot;
    orkester_promise_t promise = nullptr;
    orkester_future_t futureSignal = nullptr;

    ~State() noexcept {
      if (promise)
        orkester_promise_drop(promise);
    }
  };

  Promise(
      const orkester_async_t* system,
      std::shared_ptr<CesiumImpl::ValueSlot<T>> slot,
      orkester_promise_t promise,
      orkester_future_t futureSignal)
      : _state(std::make_shared<State>()) {
    _state->system = system;
    _state->slot = std::move(slot);
    _state->promise = promise;
    _state->futureSignal = futureSignal;
  }

  std::shared_ptr<State> _state;
};

// ─── void specialization ────────────────────────────────────────────────────

template <> class CESIUMASYNC_API Promise<void> final {
public:
  Promise(const Promise&) = default;
  Promise& operator=(const Promise&) = default;
  Promise(Promise&&) noexcept = default;
  Promise& operator=(Promise&&) noexcept = default;
  ~Promise() noexcept = default;

  void resolve() const {
    auto p = _state->promise;
    _state->promise = nullptr;
    orkester_promise_resolve(p);
  }

  void reject(std::exception_ptr error) const {
    _state->slot->storeError(error);
    auto p = _state->promise;
    _state->promise = nullptr;
    orkester_promise_resolve(p);
  }

  template <
      typename E,
      std::enable_if_t<
          !std::is_same_v<std::decay_t<E>, std::exception_ptr>,
          int> = 0>
  void reject(E&& exception) const {
    reject(std::make_exception_ptr(std::forward<E>(exception)));
  }

  Future<void> getFuture() {
    auto signal = _state->futureSignal;
    _state->futureSignal = nullptr;
    return Future<void>(_state->system, _state->slot, signal);
  }

private:
  friend class AsyncSystem;

  struct State {
    const orkester_async_t* system = nullptr;
    std::shared_ptr<CesiumImpl::ValueSlot<void>> slot;
    orkester_promise_t promise = nullptr;
    orkester_future_t futureSignal = nullptr;

    ~State() noexcept {
      if (promise)
        orkester_promise_drop(promise);
    }
  };

  Promise(
      const orkester_async_t* system,
      std::shared_ptr<CesiumImpl::ValueSlot<void>> slot,
      orkester_promise_t promise,
      orkester_future_t futureSignal)
      : _state(std::make_shared<State>()) {
    _state->system = system;
    _state->slot = std::move(slot);
    _state->promise = promise;
    _state->futureSignal = futureSignal;
  }

  std::shared_ptr<State> _state;
};

} // namespace CesiumAsync
